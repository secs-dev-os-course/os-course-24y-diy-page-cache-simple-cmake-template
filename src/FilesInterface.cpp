#include <FilesInterface.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "PageCache.h"

FilesManager::FilesManager(int cache_size) : page_cache(cache_size) {
}

int FilesManager::f_open(const char* path) {
    int fd = open(path, O_RDWR | O_DIRECT);  // Bypassing OS page cache
    if (fd == -1) {
        throw std::runtime_error("Error opening file");
    }
    offset_map[fd] = static_cast<off_t>(0);
    return fd;
}

int FilesManager::f_close(int fd) {
    page_cache.clearFileCaches(fd);
    offset_map.erase(fd);
    close(fd);
}

ssize_t FilesManager::f_read(int fd, void* buf, size_t count) {
    char res[count];

    if (offset_map.find(fd) == offset_map.end()) {
        throw std::runtime_error("Error reading file. Such descriptor not exist!");
    }

    int curr_offset = offset_map[fd];
    int first_page_idx = curr_offset / PAGE_SIZE;
    int last_page_idx = (curr_offset + count) / PAGE_SIZE;

    int bytes_left = count;
    int buff_pos = 0;

    for (int i = first_page_idx; i <= last_page_idx; i++) {  // TODO: Align buffer!
        if (!page_cache.pageExist(fd, i)) {
            int page_first_pos = i * PAGE_SIZE;
            f_lseek(fd, page_first_pos, SEEK_SET);
        }
        CacheBlock cache_block = page_cache.getCached(fd, i);

        int start_pos = curr_offset % PAGE_SIZE;
        int end_pos = std::min(PAGE_SIZE - 1, start_pos + bytes_left);
        int bytes_am = end_pos - start_pos + 1;

        std::memcpy(res + buff_pos, cache_block.cached_page + start_pos, bytes_am * sizeof(char));
        buff_pos += bytes_am;
        bytes_left -= bytes_am;
        curr_offset += bytes_am;
    }
}

ssize_t FilesManager::f_write(int fd, const void* buf, size_t count) {
}

off_t FilesManager::f_lseek(int fd, int offset, int whence) {
    off_t curr_offset = lseek(fd, offset, whence);

    if (curr_offset < 0) {
        std::cerr << "Offset error: " << strerror(errno) << std::endl;
        return curr_offset;
    }

    offset_map[fd] = curr_offset;
    int page_idx = curr_offset / PAGE_SIZE;

    page_cache.cachePage(fd, page_idx);

    return curr_offset;
}

int FilesManager::fsync(int fd) {
    cache_map* cached_blocks = page_cache.getAllCached();
    for (auto it = cached_blocks->begin(); it != cached_blocks->end(); it++) {
        int block_fd = it->first.first;
        if (block_fd != fd) continue;

        int page_idx = it->first.second;
        int page_start_pos = page_idx * PAGE_SIZE;

        if (lseek(fd, page_start_pos, SEEK_SET) == -1) {
            return -1;
        }
        if (write(fd, it->second.cached_page, PAGE_SIZE) != PAGE_SIZE) {
            return -1;
        }
    }
    if (lseek(fd, offset_map[fd], SEEK_SET) == -1) {
        return -1;
    }
    return 0;
}
