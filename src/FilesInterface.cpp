#include <FilesInterface.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

#include "PageCache.h"

FilesManager::FilesManager(int cache_size) : page_cache(cache_size) {
}

int FilesManager::f_open(const char* path) {
    int fd = open(path, O_RDWR | O_DIRECT);  // Bypassing OS page cache
    offset_map.insert(fd, 0);
    return fd;
}

int FilesManager::f_close(int fd) {
    page_cache.clearFileCaches(fd);
    offset_map.erase(fd);
    close(fd);
}

ssize_t FilesManager::f_read(int fd, void* buf, size_t count) {
    char res[count];

    int curr_offset = offset_map[fd];
    int first_page_idx = curr_offset / PAGE_SIZE;
    int last_page_idx = (curr_offset + count) / PAGE_SIZE;

    int bytes_left = count;
    int buff_pos = 0;

    for (int i = first_page_idx; i <= last_page_idx; i++) {
        if (page_cache.pageExist(fd, i)) {
            CacheBlock cache_block = page_cache.getCached(fd, i);

            int bytes_am = (bytes_left >= PAGE_SIZE) ? PAGE_SIZE : bytes_left;
            std::memcpy(res + buff_pos, cache_block.page, bytes_am);
            buff_pos += bytes_am;

        } else {
            
        }
    }
}

ssize_t FilesManager::f_write(int fd, const void* buf, size_t count) {
}

int FilesManager::f_lseek(int fd, int offset, int whence) {
}

int FilesManager::fsync(int fd) {
}
