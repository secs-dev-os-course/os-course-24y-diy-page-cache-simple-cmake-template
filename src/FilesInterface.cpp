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
    fsync(fd);
    page_cache.clearFileCaches(fd);
    offset_map.erase(fd);
    close(fd);
}

ssize_t FilesManager::f_read(int fd, void* buf, size_t count) {
    if (offset_map.find(fd) == offset_map.end()) {
        throw std::runtime_error("Error reading file. Such descriptor not exist!");
    }

    int curr_offset = offset_map[fd];
    int first_page_idx = curr_offset / PAGE_SIZE;
    int last_page_idx = (curr_offset + count) / PAGE_SIZE;

    int bytes_left = count;
    int buff_pos = 0;

    for (int i = first_page_idx; i <= last_page_idx; i++) {
        std::optional<std::shared_ptr<CacheBlock>> cache_block_opt = page_cache.getCached(fd, i);
        if (cache_block_opt == std::nullopt) {
            page_cache.cachePage(fd, i);
            cache_block_opt = page_cache.getCached(fd, i);
        }

        int start_pos = curr_offset % PAGE_SIZE;
        int end_pos = std::min(PAGE_SIZE - 1, start_pos + bytes_left);
        int bytes_am = end_pos - start_pos + 1;

        std::memcpy(static_cast<char*>(buf) + buff_pos, cache_block_opt.value().get()->cached_page + start_pos,
                    bytes_am * sizeof(char));

        buff_pos += bytes_am;
        bytes_left -= bytes_am;
        curr_offset += bytes_am;

        off_t new_offset = (i == 0) ? PAGE_SIZE : i * PAGE_SIZE;
        f_lseek(fd, new_offset, SEEK_SET);
    }
}

ssize_t FilesManager::f_write(int fd, const void* buf, size_t count) {
    if (posix_memalign((void**)(buf), PAGE_SIZE, count) != 0) {
        throw std::runtime_error("Failed to align buffer memory");
    }

    if (offset_map.find(fd) == offset_map.end()) {
        throw std::runtime_error("Error writing to file. Such descriptor not exist!");
    }

    int curr_offset = offset_map[fd];
    int first_page_idx = curr_offset / PAGE_SIZE;
    int last_page_idx = (curr_offset + count) / PAGE_SIZE;

    int bytes_left = count;
    int buff_pos = 0;

    for (int i = first_page_idx; i <= last_page_idx; i++) {
        std::optional<std::shared_ptr<CacheBlock>> cache_block_opt = page_cache.getCached(fd, i);
        if (cache_block_opt == std::nullopt) {
            page_cache.cachePage(fd, i);
            cache_block_opt = page_cache.getCached(fd, i);
        }

        int start_pos = curr_offset % PAGE_SIZE;
        int end_pos = std::min(PAGE_SIZE - 1, start_pos + bytes_left);
        int bytes_am = end_pos - start_pos + 1;

        std::memcpy(static_cast<char*>(buf) + buff_pos, cache_block_opt.value().get()->cached_page + start_pos,
                    bytes_am * sizeof(char));
        buff_pos += bytes_am;
        bytes_left -= bytes_am;
        curr_offset += bytes_am;

        off_t new_offset = (i == 0) ? PAGE_SIZE : i * PAGE_SIZE;
        f_lseek(fd, new_offset, SEEK_SET);
    }
}

off_t FilesManager::f_lseek(int fd, int offset, int whence) {
    off_t curr_offset = lseek(fd, offset, whence);

    if (curr_offset < 0) {
        std::cerr << "Offset error: " << strerror(errno) << std::endl;
        return curr_offset;
    }

    offset_map[fd] = curr_offset;
    return curr_offset;
}

int FilesManager::fsync(int fd) {
    page_cache.syncBlocks(fd);
}
