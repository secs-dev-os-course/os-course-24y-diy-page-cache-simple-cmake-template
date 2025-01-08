#define _GNU_SOURCE

#include "app.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_OPEN_FILES 256
#define BLOCK_SIZE 4096
#define CACHE_SIZE 1024


typedef struct Key {
  dev_t dev; 
  ino_t inode; 
} Key;

typedef struct Meta {
  Key file_key; 
  off_t pointer;
  int fd;
} Meta;

typedef struct Block {
  Key file_key;
  off_t block_start;
  int frequency;
  int is_dirty;
  void* data;
} Block;


// Глобальные переменные
Block cache[CACHE_SIZE];
int cache_count = 0;
Meta file_metadata[MAX_OPEN_FILES] = {0};
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;

// Утилита для выравнивания буфера
void* aligned_alloc(size_t alignment, size_t size) {
  void* ptr;
  if (posix_memalign(&ptr, alignment, size) != 0) {
    return NULL;
  }
  return ptr;
}

// Получение идентификатора файла
int get_file_key(int file_descriptor, Key* file_key) {
    struct stat stat_buf;
    if (fstat(file_descriptor, &stat_buf) < 0) {
        perror("get_file_key: unable to retrieve file status");
        return -1;
    }
    file_key->dev = stat_buf.st_dev;
    file_key->inode = stat_buf.st_ino;
    return 0;
}

// Добавление файлового дескриптора
int register_fd(int file_descriptor) {
    Key file_key;
    if (get_file_key(file_descriptor, &file_key) < 0) {
        return -1;
    }
    for (int index = 0; index < MAX_OPEN_FILES; index++) { //TODO: заменить на бин поиск
        if (file_metadata[index].file_key.inode == 0) {
            file_metadata[index].file_key = file_key;
            file_metadata[index].pointer = 0;
            file_metadata[index].fd = file_descriptor;
            return 0;
        }
    }
    errno = EMFILE;
    return -1;
}

// Дерегистрация дескриптора
int unregister_fd(int file_descriptor) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_metadata[i].fd == file_descriptor) {
            memset(&file_metadata[i], 0, sizeof(Meta));
            return 0;
        }
    }
    errno = EBADF;
    return -1;
}

// Получение индекса блока из кэша
int find_block_in_cache(Key* file_key, off_t block_offset) {
    for (int idx = 0; idx < CACHE_SIZE; idx++) {
        if (cache[idx].file_key.inode == file_key->inode && 
            cache[idx].file_key.dev == file_key->dev &&
            cache[idx].block_start == block_offset) {
            return idx;
        }
    }
    return -1;
}

// Удаление блока из кэша
int evict_block() {
    int least_used_index = -1;
    int lowest_frequency = INT32_MAX;

    for (int idx = 0; idx < CACHE_SIZE; idx++) {
        if (cache[idx].frequency < lowest_frequency) {
            lowest_frequency = cache[idx].frequency;
            least_used_index = idx;
        }
    }

    if (cache[least_used_index].is_dirty) {
        int file_descriptor = -1;
        for (int j = 0; j < MAX_OPEN_FILES; j++) {
            if (file_metadata[j].file_key.inode == cache[least_used_index].file_key.inode &&
                file_metadata[j].file_key.dev == cache[least_used_index].file_key.dev) {
                file_descriptor = file_metadata[j].fd;
                break;
            }
        }

        if (file_descriptor < 0 ||
            pwrite(file_descriptor, cache[least_used_index].data, BLOCK_SIZE, cache[least_used_index].block_start) < 0) {
            perror("evict_block: unable to commit block to file");
            return -1;
        }
    }

    free(cache[least_used_index].data);
    memset(&cache[least_used_index], 0, sizeof(Block));
    return least_used_index;
}



// Кэширование блока файла
int load_block_into_cache(Key* key, off_t block_offset, size_t* out_bytes_read, bool prefetch) {
    int file_desc = -1;
    for (int index = 0; index < MAX_OPEN_FILES; index++) {
        if (file_metadata[index].file_key.inode == key->inode &&
            file_metadata[index].file_key.dev == key->dev) {
            file_desc = file_metadata[index].fd;
            break;
        }
    }

    if (file_desc < 0) {
        perror("load_block_into_cache: invalid file descriptor");
        return -1;
    }

    void* buffer = aligned_alloc(BLOCK_SIZE, BLOCK_SIZE);
    if (!buffer) {
        perror("load_block_into_cache: unable to allocate memory");
        return -1;
    }

    ssize_t bytes = pread(file_desc, buffer, BLOCK_SIZE, block_offset);
    *out_bytes_read = bytes;

    if (bytes < 0) {
        perror("load_block_into_cache: read error");
        free(buffer);
        return -1;
    }

    if (!prefetch && bytes == 0) {
        free(buffer);
        return -1;
    }

    int index_in_cache;
    if (cache_count < CACHE_SIZE) {
        index_in_cache = cache_count++;
    } else {
        index_in_cache = evict_block();
        if (index_in_cache < 0) {
            free(buffer);
            return -1;
        }
    }

    cache[index_in_cache].data = buffer;
    cache[index_in_cache].file_key = *key;
    cache[index_in_cache].block_start = block_offset;
    cache[index_in_cache].frequency = 1;
    cache[index_in_cache].is_dirty = 0;

    return index_in_cache;
}


// Сохранение измененных блоков
int flush_dirty_blocks(Key* key) {
    for (int idx = 0; idx < CACHE_SIZE; idx++) {
        if (cache[idx].file_key.inode == key->inode && cache[idx].file_key.dev == key->dev &&
            cache[idx].is_dirty) {
            int file_desc = -1;
            for (int jdx = 0; jdx < MAX_OPEN_FILES; jdx++) {
                if (file_metadata[jdx].file_key.inode == key->inode &&
                    file_metadata[jdx].file_key.dev == key->dev) {
                    file_desc = file_metadata[jdx].fd;
                    break;
                }
            }

            if (file_desc < 0 || pwrite(file_desc, cache[idx].data, BLOCK_SIZE, cache[idx].block_start) < 0) {
                perror("flush_dirty_blocks: error writing to file");
                return -1;
            }
            cache[idx].is_dirty = 0;
        }
    }
    return 0;
}


// Функция выполнения операции чтения из файла
ssize_t lab2_read(int fd, void* buffer, size_t byte_count) {
    pthread_mutex_lock(&cache_lock);

    Meta* file_meta = NULL;
    for (int index = 0; index < MAX_OPEN_FILES; index++) {
        if (file_metadata[index].fd == fd) {
            file_meta = &file_metadata[index];
            break;
        }
    }

    if (!file_meta) {
        errno = EBADF;
        pthread_mutex_unlock(&cache_lock);
        return -1;
    }

    Key file_key = file_meta->file_key;
    off_t current_pointer = file_meta->pointer;

    size_t total_bytes_read = 0;
    while (byte_count > 0) {
        off_t block_start_offset = (current_pointer / BLOCK_SIZE) * BLOCK_SIZE;
        size_t offset_within_block = current_pointer % BLOCK_SIZE;
        size_t block_remaining = BLOCK_SIZE - offset_within_block;
        if (block_remaining > byte_count)
            block_remaining = byte_count;

        int cache_pos = find_block_in_cache(&file_key, block_start_offset);
        size_t bytes_fetched = 0;

        if (cache_pos < 0) {
            cache_pos = load_block_into_cache(&file_key, block_start_offset, &bytes_fetched, false);
            if (cache_pos < 0) {
                file_meta->pointer += total_bytes_read;
                pthread_mutex_unlock(&cache_lock);
                return bytes_fetched >= 0 ? total_bytes_read : -1;
            }
        } else {
            bytes_fetched = BLOCK_SIZE;
        }

        block_remaining = block_remaining > bytes_fetched ? bytes_fetched : block_remaining;
        if (block_remaining == 0) {
            break;
        }
        memcpy(buffer + total_bytes_read, cache[cache_pos].data + offset_within_block, block_remaining);
        cache[cache_pos].frequency++;

        total_bytes_read += block_remaining;
        byte_count -= block_remaining;
        current_pointer += block_remaining;
    }

    file_meta->pointer = current_pointer;
    pthread_mutex_unlock(&cache_lock);
    return total_bytes_read;
}


// Функция записи в файл
ssize_t lab2_write(int fd, const void* buffer, size_t length) {
    pthread_mutex_lock(&cache_lock);

    Meta* file_info = NULL;
    for (int idx = 0; idx < MAX_OPEN_FILES; idx++) {
        if (file_metadata[idx].fd == fd) {
            file_info = &file_metadata[idx];
            break;
        }
    }

    if (!file_info) {
        errno = EBADF;
        pthread_mutex_unlock(&cache_lock);
        return -1;
    }

    Key file_key = file_info->file_key;
    off_t current_position = file_info->pointer;

    size_t total_bytes_written = 0;
    while (length > 0) {
        off_t block_start_offset = (current_position / BLOCK_SIZE) * BLOCK_SIZE;
        size_t offset_within_block = current_position % BLOCK_SIZE;
        size_t block_remaining_space = BLOCK_SIZE - offset_within_block;
        if (block_remaining_space > length)
            block_remaining_space = length;

        int index_within_cache = find_block_in_cache(&file_key, block_start_offset);
        if (index_within_cache < 0) {
            size_t unused_bytes_read = 0;
            index_within_cache = load_block_into_cache(&file_key, block_start_offset, &unused_bytes_read, true);
            if (index_within_cache < 0) {
                file_info->pointer += total_bytes_written;
                pthread_mutex_unlock(&cache_lock);
                return unused_bytes_read >= 0 ? total_bytes_written : -1;
            }
        }

        memcpy(cache[index_within_cache].data + offset_within_block,
               buffer + total_bytes_written, block_remaining_space);
        cache[index_within_cache].frequency++;
        cache[index_within_cache].is_dirty = 1;

        total_bytes_written += block_remaining_space;
        length -= block_remaining_space;
        current_position += block_remaining_space;
    }

    file_info->pointer = current_position;
    pthread_mutex_unlock(&cache_lock);
    return total_bytes_written;
}


// Реализация API
int lab2_open(const char* file_path, int access_flags, int file_mode) {
    int file_descriptor = open(file_path, access_flags | O_DIRECT, file_mode);
    if (file_descriptor < 0) {
        perror("lab2_open: failed to open file");
        return -1;
    }
    if (register_fd(file_descriptor) < 0) {
        close(file_descriptor);
        return -1;
    }
    return file_descriptor;
}

int lab2_close(int fd) {
    pthread_mutex_lock(&cache_lock);

    Key f_key;
    if (get_file_key(fd, &f_key) < 0 || flush_dirty_blocks(&f_key) < 0) {
        pthread_mutex_unlock(&cache_lock);
        return -1;
    }

    for (int idx = 0; idx < CACHE_SIZE; idx++) {
        if (cache[idx].file_key.inode == f_key.inode && cache[idx].file_key.dev == f_key.dev) {
            free(cache[idx].data);
            memset(&cache[idx], 0, sizeof(Block));
        }
    }

    unregister_fd(fd);
    pthread_mutex_unlock(&cache_lock);

    if (close(fd) < 0) {
        perror("lab2_close: failed to close file");
        return -1;
    }
    return 0;
}

// Функция lseek
off_t lab2_lseek(int fd, off_t offset, int direction) {
    pthread_mutex_lock(&cache_lock);

    Key key;
    if (get_file_key(fd, &key) < 0) {
        pthread_mutex_unlock(&cache_lock);
        return -1;
    }

    for (int idx = 0; idx < MAX_OPEN_FILES; idx++) {
        if (file_metadata[idx].fd == fd) {
            off_t new_position;
            switch (direction) {
                case SEEK_SET:
                    new_position = offset;
                    break;
                case SEEK_CUR:
                    new_position = file_metadata[idx].pointer + offset;
                    break;
                case SEEK_END: {
                    struct stat file_stat;
                    if (fstat(fd, &file_stat) < 0) {
                        pthread_mutex_unlock(&cache_lock);
                        return -1;
                    }
                    new_position = file_stat.st_size + offset;
                    break;
                }
                default:
                    errno = EINVAL;
                    pthread_mutex_unlock(&cache_lock);
                    return -1;
            }

            if (new_position < 0) {
                errno = EINVAL;
                pthread_mutex_unlock(&cache_lock);
                return -1;
            }

            file_metadata[idx].pointer = new_position;
            pthread_mutex_unlock(&cache_lock);
            return new_position;
        }
    }

    errno = EBADF;
    pthread_mutex_unlock(&cache_lock);
    return -1;
}

// Функция fsync
int lab2_fsync(int fd) {
    pthread_mutex_lock(&cache_lock);

    Key file_key_instance;
    if (get_file_key(fd, &file_key_instance) < 0) {
        pthread_mutex_unlock(&cache_lock);
        return -1;
    }

    int sync_result = flush_dirty_blocks(&file_key_instance);

    pthread_mutex_unlock(&cache_lock);
    return sync_result;
}
