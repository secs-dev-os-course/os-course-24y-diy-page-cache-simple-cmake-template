#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#define PAGE_SIZE 4096  
#define CACHE_PAGES 64  

typedef struct {
    void *data;     
    off_t offset;      
    int reference_bit;     
    int dirty;      
} page_t;

typedef struct {
    page_t pages[CACHE_PAGES];
    size_t clock_hand;
    int fd;                
} cache_t;

static cache_t cache = { .clock_hand = 0, .fd = -1 };

static void init_cache() {
    for (int i = 0; i < CACHE_PAGES; i++) {
        posix_memalign(&cache.pages[i].data, PAGE_SIZE, PAGE_SIZE);
        cache.pages[i].offset = -1;
        cache.pages[i].reference_bit = 0;
        cache.pages[i].dirty = 0;
    }
}

static void flush_page(page_t *page) {
    if (page->dirty && page->offset != -1) {
        lseek(cache.fd, page->offset, SEEK_SET);
        write(cache.fd, page->data, PAGE_SIZE);
        page->dirty = 0;
    }
}

static void load_page(page_t *page, off_t offset) {
    flush_page(page);
    lseek(cache.fd, offset, SEEK_SET);
    read(cache.fd, page->data, PAGE_SIZE);
    page->offset = offset;
    page->reference_bit = 1;
    page->dirty = 0;
}

static page_t *find_or_evict_page(off_t offset) {
    for (int i = 0; i < CACHE_PAGES; i++) {
        if (cache.pages[i].offset == offset) {
            cache.pages[i].reference_bit = 1;
            return &cache.pages[i];
        }
    }

    while (1) {
        page_t *page = &cache.pages[cache.clock_hand];
        if (page->reference_bit == 0) {
            load_page(page, offset);
            cache.clock_hand = (cache.clock_hand + 1) % CACHE_PAGES;
            return page;
        } else {
            page->reference_bit = 0;
            cache.clock_hand = (cache.clock_hand + 1) % CACHE_PAGES;
        }
    }
}

int lab2_open(const char *path) {
    cache.fd = open(path, O_RDWR | O_DIRECT);
    if (cache.fd < 0) return -1;
    init_cache();
    return cache.fd;
}

int lab2_close(int fd) {
    if (fd != cache.fd) return -1;

    for (int i = 0; i < CACHE_PAGES; i++) {
        flush_page(&cache.pages[i]);
        free(cache.pages[i].data);
    }

    cache.fd = -1;
    return close(fd);
}

ssize_t lab2_read(int fd, void *buf, size_t count) {
    if (fd != cache.fd) return -1;

    size_t total_read = 0;
    size_t page_offset = lseek(fd, 0, SEEK_CUR) % PAGE_SIZE;

    while (count > 0) {
        off_t file_offset = lseek(fd, 0, SEEK_CUR) & ~(PAGE_SIZE - 1);
        page_t *page = find_or_evict_page(file_offset);

        size_t to_copy = PAGE_SIZE - page_offset;
        if (to_copy > count) to_copy = count;

        memcpy((char *)buf, (char *)page->data + page_offset, to_copy);

        buf = (char *)buf + to_copy;
        count -= to_copy;
        total_read += to_copy;
        page_offset = 0;
        lseek(fd, to_copy, SEEK_CUR);
    }

    return total_read;
}

ssize_t lab2_write(int fd, const void *buf, size_t count) {
    if (fd != cache.fd) return -1;

    size_t total_written = 0;
    size_t page_offset = lseek(fd, 0, SEEK_CUR) % PAGE_SIZE;

    while (count > 0) {
        off_t file_offset = lseek(fd, 0, SEEK_CUR) & ~(PAGE_SIZE - 1);
        page_t *page = find_or_evict_page(file_offset);

        size_t to_copy = PAGE_SIZE - page_offset;
        if (to_copy > count) to_copy = count;

        memcpy((char *)page->data + page_offset, buf, to_copy);
        page->dirty = 1;

        buf = (const char *)buf + to_copy;
        count -= to_copy;
        total_written += to_copy;
        page_offset = 0;
        lseek(fd, to_copy, SEEK_CUR);
    }

    return total_written;
}

off_t lab2_lseek(int fd, off_t offset, int whence) {
    if (fd != cache.fd) return -1;
    return lseek(fd, offset, whence);
}

int lab2_fsync(int fd) {
    if (fd != cache.fd) return -1;

    for (int i = 0; i < CACHE_PAGES; i++) {
        flush_page(&cache.pages[i]);
    }

    return fsync(fd);
}

#define TEST_FILE_SIZE (PAGE_SIZE * 1024)

void create_test_file(const char *filename) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Failed to create test file");
        exit(1);
    }

    char *buffer = (char*)malloc(PAGE_SIZE);
    memset(buffer, 'A', PAGE_SIZE);

    for (size_t i = 0; i < TEST_FILE_SIZE; i += PAGE_SIZE) {
        if (write(fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
            perror("Failed to write to test file");
            exit(1);
        }
    }

    free(buffer);
    close(fd);
}

void measure_time_without_cache(const char *filename) {
    int fd = open(filename, O_RDONLY | O_DIRECT);
    if (fd < 0) {
        perror("Failed to open file without cache");
        exit(1);
    }

    char *buffer;
    posix_memalign((void **)&buffer, PAGE_SIZE, PAGE_SIZE);

    clock_t start = clock();
    for (size_t i = 0; i < 10000; i++) {
        int j = (i * 435454321) % (TEST_FILE_SIZE / PAGE_SIZE);
        pread(fd, buffer, PAGE_SIZE, j*PAGE_SIZE);
    }
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time with cache: %.6f seconds\n", elapsed);

    free(buffer);
    close(fd);
}

void measure_time_with_cache(const char *filename) {
    int fd = lab2_open(filename);
    if (fd < 0) {
        perror("Failed to open file with cache");
        exit(1);
    }

    char *buffer = (char*)malloc(PAGE_SIZE);

    clock_t start = clock();
    for (size_t i = 0; i < 10000; i++) {
        int j = (i * 435454321) % (TEST_FILE_SIZE / PAGE_SIZE);
        lab2_lseek(fd, j * PAGE_SIZE, SEEK_SET);
        lab2_read(fd, buffer, PAGE_SIZE);
    }
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time without cache: %.6f seconds\n", elapsed);

    free(buffer);
    lab2_close(fd);
}

int main() {
    const char *test_file = "testfile.dat";

    // Create test file
    create_test_file(test_file);

    // Measure performance without cache
    measure_time_without_cache(test_file);

    // Measure performance with cache
    measure_time_with_cache(test_file);

    return 0;
}