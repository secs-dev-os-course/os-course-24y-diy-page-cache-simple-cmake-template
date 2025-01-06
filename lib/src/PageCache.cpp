#include "PageCache.h"

#include <unistd.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

CacheBlock::~CacheBlock() {
    free(cached_page);
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
        if (it->get()->fd == fd) {
            it = priority_queue.erase(it);
        } else {
            it++;
        }
    }
}

bool PageCache::pageExist(int fd, int page_idx) {
    return cache_blocks.find(std::make_pair(fd, page_idx)) != cache_blocks.end();
}

std::optional<std::shared_ptr<CacheBlock>> PageCache::getCached(int fd, int page_idx) {
    std::pair<int, int> key = std::make_pair(fd, page_idx);
    if (cache_blocks.find(key) != cache_blocks.end()) {
        access(fd, page_idx);
        return std::make_optional(cache_blocks[key]);
    }
    return std::nullopt;
}

void PageCache::access(int fd, int page_idx) {
    std::pair<int, int> key = std::make_pair(fd, page_idx);

    std::shared_ptr<CacheBlock> cache_block = cache_blocks[key];
    cache_block.get()->access_time = std::chrono::steady_clock::now();
    cache_block.get()->req_am++;

    cache_blocks.erase(key);
    priority_queue.erase(cache_block);

    cache_blocks[key] = cache_block;
    priority_queue.insert(cache_block);
}

void PageCache::rmPage(int fd, int page_idx) {
    std::pair<int, int> key = std::make_pair(fd, page_idx);
    std::shared_ptr<CacheBlock> cache_block = cache_blocks[key];

    priority_queue.erase(cache_block);
    cache_blocks.erase(key);
}

void PageCache::cachePage(int fd, int page_idx) {
    if (cache_blocks.size() > max_cache_pages_am) {
        displaceBlock();
    }

    int page_start_pos = page_idx * PAGE_SIZE;

    char* buf;
    if (posix_memalign(reinterpret_cast<void**>(&buf), PAGE_SIZE, PAGE_SIZE) != 0) {
        throw std::runtime_error("Failed to allocate aligned memory");
    }

    if (lseek(fd, page_start_pos, SEEK_SET) == -1) {
        throw std::runtime_error("Error seek in file!");
    }

    ssize_t res = read(fd, buf, PAGE_SIZE);
    if (res != PAGE_SIZE) {
        throw std::runtime_error("Error reading file!");
    }

    auto cache_block = std::make_shared<CacheBlock>(fd, 0, page_idx, buf);

    cache_blocks[std::make_pair(fd, page_idx)] = cache_block;
    priority_queue.insert(cache_block);
}

void PageCache::syncBlock(int fd, int page_idx) {
    int page_start_pos = page_idx * PAGE_SIZE;
    off_t curr_offset = lseek(fd, 0, SEEK_CUR);

    if (lseek(fd, page_start_pos, SEEK_SET) == -1) {
        throw std::runtime_error("Sync with external disk error.");
    }
    if (write(fd, cache_blocks[std::make_pair(fd, page_idx)].get()->cached_page, PAGE_SIZE) != PAGE_SIZE) {
        throw std::runtime_error("Sync with external disk error.");
    }
    if (lseek(fd, curr_offset, SEEK_SET) == -1) {
        throw std::runtime_error("Sync with external disk error.");
    }
}

void PageCache::syncBlocks(int fd) {
    for (auto it = cache_blocks.begin(); it != cache_blocks.end(); it++) {
        int block_fd = it->first.first;
        if (block_fd != fd) continue;

        int page_idx = it->first.second;
        syncBlock(fd, page_idx);
    }
    return;
}

void PageCache::displaceBlock() {
    auto to_displace = priority_queue.begin();
    if (to_displace == priority_queue.end()) return;

    int fd = (*to_displace).get()->fd;
    int page_idx = (*to_displace).get()->page_idx;

    syncBlock(fd, page_idx);
    rmPage(fd, page_idx);
}