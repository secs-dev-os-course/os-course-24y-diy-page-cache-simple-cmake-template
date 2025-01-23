#include "lab2.h"
#include "cache.h"
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

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
