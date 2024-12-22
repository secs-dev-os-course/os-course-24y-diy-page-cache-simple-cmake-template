#define _GNU_SOURCE
#include "lab2.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define PAGE_SIZE 4096
#define CACHE_SIZE 64

// Структура страницы кэша
typedef struct CachePage {
    off_t offset;                 // Смещение страницы в файле
    size_t size;                  // Размер страницы
    void *data;                   // Указатель на данные
    int dirty;                    // Флаг модификации
    int reference;                // Бит использования для алгоритма Clock
    struct CachePage *next;       // Следующая страница в списке
} CachePage;

// Структура кэша
typedef struct Cache {
    CachePage pages[CACHE_SIZE];  // Массив страниц кэша
    CachePage *clock_hand;        // Указатель для алгоритма Clock
    pthread_mutex_t lock;         // Мьютекс для потокобезопасности
} Cache;

// Структура файла
typedef struct File {
    int fd;                       // файловый дескриптор ОС
    off_t pos;                    // Текущая позиция в файле
    Cache cache;                  // Кэш страниц для этого файла
} File;

#define MAX_FILES 256
static File *open_files[MAX_FILES];
static pthread_mutex_t files_lock = PTHREAD_MUTEX_INITIALIZER;

static File *get_file(int fd);
static int register_file(File *file);
static void unregister_file(int fd);

static void cache_init(Cache *cache) {
    memset(cache->pages, 0, sizeof(cache->pages));
    cache->clock_hand = &cache->pages[0];
    pthread_mutex_init(&cache->lock, NULL);
}

static void cache_destroy(Cache *cache) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache->pages[i].data) {
            free(cache->pages[i].data);
            cache->pages[i].data = NULL;
        }
    }
    pthread_mutex_destroy(&cache->lock);
}

static CachePage *cache_lookup(Cache *cache, off_t offset) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache->pages[i].data && cache->pages[i].offset == offset) {
            return &cache->pages[i];
        }
    }
    return NULL;
}

// Выбор страницы для вытеснения (алгоритм Clock)
static CachePage *cache_evict(Cache *cache) {
    CachePage *page = cache->clock_hand;
    while (1) {
        if (page->reference == 0) {
            cache->clock_hand = page + 1;
            if (cache->clock_hand == cache->pages + CACHE_SIZE)
                cache->clock_hand = cache->pages;
            return page;
        } else {
            page->reference = 0;
            page++;
            if (page == cache->pages + CACHE_SIZE)
                page = cache->pages;
            cache->clock_hand = page;
        }
    }
}

static int sync_page(File *file, CachePage *page) {
    if (page->dirty) {
        ssize_t bytes_written = pwrite(file->fd, page->data, PAGE_SIZE, page->offset);
        if (bytes_written != PAGE_SIZE) {
            return -1;
        }
        page->dirty = 0;
    }
    return 0;
}

int lab2_open(const char *path) {
    int os_fd = open(path, O_RDWR | O_DIRECT | O_SYNC);
    if (os_fd == -1) {
        perror("open");
        return -1;
    }

    File *file = malloc(sizeof(File));
    if (!file) {
        close(os_fd);
        return -1;
    }
    file->fd = os_fd;
    file->pos = 0;
    cache_init(&file->cache);

    int fd = register_file(file);
    if (fd == -1) {
        cache_destroy(&file->cache);
        close(os_fd);
        free(file);
        return -1;
    }

    return fd;
}

int lab2_close(int fd) {
    pthread_mutex_lock(&files_lock);
    File *file = get_file(fd);
    if (!file) {
        pthread_mutex_unlock(&files_lock);
        errno = EBADF;
        return -1;
    }
    pthread_mutex_unlock(&files_lock);

    lab2_fsync(fd);

    cache_destroy(&file->cache);
    close(file->fd);
    unregister_file(fd);
    free(file);

    return 0;
}

ssize_t lab2_read(int fd, void *buf, size_t count) {
    pthread_mutex_lock(&files_lock);
    File *file = get_file(fd);
    if (!file) {
        pthread_mutex_unlock(&files_lock);
        errno = EBADF;
        return -1;
    }
    pthread_mutex_unlock(&files_lock);

    pthread_mutex_lock(&file->cache.lock);

    size_t total_read = 0;
    size_t to_read = count;
    char *buffer = buf;

    while (to_read > 0) {
        off_t page_offset = (file->pos / PAGE_SIZE) * PAGE_SIZE;
        size_t page_offset_in_file = file->pos % PAGE_SIZE;
        size_t bytes_from_page = PAGE_SIZE - page_offset_in_file;
        if (bytes_from_page > to_read)
            bytes_from_page = to_read;

        CachePage *page = cache_lookup(&file->cache, page_offset);

        if (!page) {
            page = cache_evict(&file->cache);

            if (page->dirty) {
                sync_page(file, page);
            }

            if (!page->data) {
                posix_memalign(&page->data, PAGE_SIZE, PAGE_SIZE);
                if (!page->data) {
                    pthread_mutex_unlock(&file->cache.lock);
                    return -1;
                }
            }

            ssize_t bytes_read = pread(file->fd, page->data, PAGE_SIZE, page_offset);
            if (bytes_read != PAGE_SIZE) {
                if (bytes_read == -1) {
                    pthread_mutex_unlock(&file->cache.lock);
                    return -1;
                }
                memset(page->data + bytes_read, 0, PAGE_SIZE - bytes_read);
            }

            page->offset = page_offset;
            page->size = PAGE_SIZE;
            page->dirty = 0;
            page->reference = 1;
        } else {
            page->reference = 1;
        }

        memcpy(buffer, page->data + page_offset_in_file, bytes_from_page);

        buffer+= bytes_from_page;
        file->pos += bytes_from_page;
        total_read += bytes_from_page;
        to_read -= bytes_from_page;
    }

    pthread_mutex_unlock(&file->cache.lock);
    return total_read;
}

ssize_t lab2_write(int fd, const void *buf, size_t count) {
    pthread_mutex_lock(&files_lock);
    File *file = get_file(fd);
    if (!file) {
        pthread_mutex_unlock(&files_lock);
        errno = EBADF;
        return -1;
    }
    pthread_mutex_unlock(&files_lock);

    pthread_mutex_lock(&file->cache.lock);

    size_t total_written = 0;
    size_t to_write = count;
    const char *buffer = buf;

    while (to_write > 0) {
        off_t page_offset = (file->pos / PAGE_SIZE) * PAGE_SIZE;
        size_t page_offset_in_file = file->pos % PAGE_SIZE;
        size_t bytes_to_page = PAGE_SIZE - page_offset_in_file;
        if (bytes_to_page > to_write)
            bytes_to_page = to_write;

        CachePage *page = cache_lookup(&file->cache, page_offset);

        if (!page) {
            page = cache_evict(&file->cache);

            if (page->dirty) {
                sync_page(file, page);
            }

            if (!page->data) {
                posix_memalign(&page->data, PAGE_SIZE, PAGE_SIZE);
                if (!page->data) {
                    pthread_mutex_unlock(&file->cache.lock);
                    return -1;
                }
            }

            if (bytes_to_page < PAGE_SIZE) {
                ssize_t bytes_read = pread(file->fd, page->data, PAGE_SIZE, page_offset);
                if (bytes_read != PAGE_SIZE) {
                    if (bytes_read == -1) {
                        pthread_mutex_unlock(&file->cache.lock);
                        return -1;
                    }
                    memset(page->data + bytes_read, 0, PAGE_SIZE - bytes_read);
                }
            } else {
                memset(page->data, 0, PAGE_SIZE);
            }

            page->offset = page_offset;
            page->size = PAGE_SIZE;
            page->dirty = 0;
            page->reference = 1;
        } else {
            page->reference = 1;
        }

    
        memcpy(page->data + page_offset_in_file, buffer, bytes_to_page);
        page->dirty = 1;

        buffer += bytes_to_page;
        file->pos += bytes_to_page;
        total_written += bytes_to_page;
        to_write -= bytes_to_page;
    }

    pthread_mutex_unlock(&file->cache.lock);
    return total_written;
}

off_t lab2_lseek(int fd, off_t offset, int whence) {
    pthread_mutex_lock(&files_lock);
    File *file = get_file(fd);
    if (!file) {
        pthread_mutex_unlock(&files_lock);
        errno = EBADF;
        return -1;
    }

    off_t new_pos;
    if (whence == SEEK_SET) {
        new_pos = offset;
    } else if (whence == SEEK_CUR) {
        new_pos = file->pos + offset;
    } else if (whence == SEEK_END) {
        struct stat st;
        if (fstat(file->fd, &st) == -1) {
            pthread_mutex_unlock(&files_lock);
            return -1;
        }
        new_pos = st.st_size + offset;
    } else {
        pthread_mutex_unlock(&files_lock);
        errno = EINVAL;
        return -1;
    }

    if (new_pos < 0) {
        pthread_mutex_unlock(&files_lock);
        errno = EINVAL;
        return -1;
    }

    file->pos = new_pos;
    pthread_mutex_unlock(&files_lock);
    return new_pos;
}

int lab2_fsync(int fd) {
    pthread_mutex_lock(&files_lock);
    File *file = get_file(fd);
    if (!file) {
        pthread_mutex_unlock(&files_lock);
        errno = EBADF;
        return -1;
    }
    pthread_mutex_unlock(&files_lock);

    pthread_mutex_lock(&file->cache.lock);

    for (int i = 0; i < CACHE_SIZE; i++) {
        CachePage *page = &file->cache.pages[i];
        if (page->data && page->dirty) {
            if (sync_page(file, page) == -1) {
                pthread_mutex_unlock(&file->cache.lock);
                return -1;
            }
        }
    }

    pthread_mutex_unlock(&file->cache.lock);
    return 0;
}

static File *get_file(int fd) {
    if (fd < 0 || fd >= MAX_FILES) return NULL;
    return open_files[fd];
}

static int register_file(File *file) {
    pthread_mutex_lock(&files_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (!open_files[i]) {
            open_files[i] = file;
            pthread_mutex_unlock(&files_lock);
            return i;
        }
    }
    pthread_mutex_unlock(&files_lock);
    errno = EMFILE;
    return -1;
}

static void unregister_file(int fd) {
    if (fd < 0 || fd >= MAX_FILES) return;
    pthread_mutex_lock(&files_lock);
    open_files[fd] = NULL;
    pthread_mutex_unlock(&files_lock);
}