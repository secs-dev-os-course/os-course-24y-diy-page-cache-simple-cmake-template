#include "PageCache.h"

#include <unistd.h>

#include <memory>
#include <stdexcept>
#include <utility>

CacheBlock::~CacheBlock() {
    delete[] cached_page;
}

template <typename T, typename U>
std::size_t PairHasher::operator()(const std::pair<T, U>& x) const {
    return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
}

PageCache::PageCache(size_t cache_size_bytes = CACHE_SIZE) {
    max_cache_pages_am = cache_size_bytes / PAGE_SIZE;
};

void PageCache::clearFileCaches(int fd) {
    for (auto it = cache_blocks.begin(); it != cache_blocks.end();) {
        if (it->first.first == fd) {
            it = cache_blocks.erase(it);
        } else {
            it++;
        }
    }

    for (auto it = priority_queue.begin(); it != priority_queue.end();) {
        if (it->fd == fd) {
            it = priority_queue.erase(it);
        } else {
            it++;
        }
    }
}

bool PageCache::pageExist(int fd, int page_idx) {
    return cache_blocks.find(std::make_pair(fd, page_idx)) != cache_blocks.end();
}

CacheBlock PageCache::getCached(int fd, int page_idx) {
    access(fd, page_idx);
    return cache_blocks[std::make_pair(fd, page_idx)];
}

void PageCache::access(int fd, int page_idx) {
    std::pair<int, int> data = std::make_pair(fd, page_idx);

    priority_queue.erase(cache_blocks[data]);

    cache_blocks[data].access_time = std::chrono::steady_clock::now();
    cache_blocks[data].req_am++;

    priority_queue.insert(cache_blocks[data]);
}

void PageCache::cachePage(int fd, int page_idx) {
    if (pageExist(fd, page_idx)) {
        access(fd, page_idx);
        return;
    } else if (cache_blocks.size() > max_cache_pages_am) {
        displaceBlock();
    }

    int page_start_pos = page_idx * PAGE_SIZE;

    char* raw_buf = nullptr;
    if (posix_memalign(reinterpret_cast<void**>(&raw_buf), PAGE_SIZE, PAGE_SIZE) != 0) {
        throw std::runtime_error("Failed to allocate aligned memory");
    }
    std::unique_ptr<char[]> buf(raw_buf);

    if (lseek(fd, page_start_pos, SEEK_SET) == -1) {
        throw std::runtime_error("Error seek in file!");
    }

    ssize_t res = read(fd, buf.get(), PAGE_SIZE);
    if (res != PAGE_SIZE) {
        throw std::runtime_error("Error reading file!");
    }

    CacheBlock cache_block = {.fd = fd,
                              .req_am = 0,
                              .page_idx = page_idx,
                              .cached_page = buf.release(),
                              .is_modified = false,
                              .access_time = std::chrono::steady_clock::now()};

    cache_blocks[std::make_pair(fd, page_idx)] = cache_block;
    priority_queue.insert(cache_block);
}

void PageCache::syncBlock(int fd, int page_idx) {
    int page_start_pos = page_idx * PAGE_SIZE;

    if (lseek(fd, page_start_pos, SEEK_SET) == -1) {
        throw std::runtime_error("Sync with external disk error.");
    }
    if (write(fd, cache_blocks[std::make_pair(fd, page_idx)].cached_page, PAGE_SIZE) != PAGE_SIZE) {
        throw std::runtime_error("Sync with external disk error.");
    }
}

void PageCache::syncBlocks(int fd) {
    off_t curr_offset = lseek(fd, 0, SEEK_CUR);
    for (auto it = cache_blocks.begin(); it != cache_blocks.end(); it++) {
        int block_fd = it->first.first;
        if (block_fd != fd) continue;

        int page_idx = it->first.second;
        syncBlock(fd, page_idx);
    }
    if (lseek(fd, curr_offset, SEEK_SET) == -1) {
        throw std::runtime_error("Sync with external disk error.");
    }
    return;
}

void PageCache::displaceBlock() {
    auto to_displace = priority_queue.begin();
    if (to_displace == priority_queue.end()) return;

    cache_blocks.erase(std::make_pair((*to_displace).fd, (*to_displace).page_idx));
    priority_queue.erase(to_displace);

    // Sync with file...
}