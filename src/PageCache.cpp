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

void PageCache::clearFileCaches(int fd) {
    for (auto it = cache_blocks.begin(); it != cache_blocks.end();) {
        if (it->first.first == fd) {
            it = cache_blocks.erase(it);
        } else {
            it++;
        }
    }

    for (auto it = freq_pq.begin(); it != freq_pq.end();) {
        if (it->fd == fd) {
            it = freq_pq.erase(it);
        } else {
            it++;
        }
    }

    for (auto it = times_pq.begin(); it != times_pq.end();) {
        if (it->fd == fd) {
            it = times_pq.erase(it);
        } else {
            it++;
        }
    }
}

bool PageCache::pageExist(int fd, int page_num) {
    return cache_blocks.find(std::make_pair(fd, page_num)) != cache_blocks.end();
}

CacheBlock PageCache::getCached(int fd, int page_num) {
    access(fd, page_num);
    return cache_blocks[std::make_pair(fd, page_num)];
}

void PageCache::access(int fd, int page_num) {
    std::pair<int, int> data = std::make_pair(fd, page_num);

    freq_pq.erase(cache_blocks[data]);
    times_pq.erase(cache_blocks[data]);

    cache_blocks[data].access_time = std::chrono::steady_clock::now();
    cache_blocks[data].req_am++;

    freq_pq.insert(cache_blocks[data]);
    times_pq.insert(cache_blocks[data]);
}

void PageCache::cachePage(int fd, int page_idx) {
    if (pageExist(fd, page_idx)) {
        access(fd, page_idx);
        return;
    }

    int page_start_pos = page_idx * PAGE_SIZE;

    std::unique_ptr<char[]> buf = std::make_unique<char[]>(PAGE_SIZE);

    if (lseek(fd, page_start_pos, SEEK_SET) == -1) {
        throw std::runtime_error("Error seek in file!");
    }

    ssize_t res = read(fd, buf.get(), PAGE_SIZE);  // TODO: Align buffer!
    if (res != PAGE_SIZE) {
        throw std::runtime_error("Error reading file!");
    }

    CacheBlock cache_block = {.fd = fd,
                              .req_am = 0,
                              .page_idx = page_idx,
                              .cached_page = buf.release(),
                              .access_time = std::chrono::steady_clock::now()};

    cache_blocks[std::make_pair(fd, page_idx)] = cache_block;
    freq_pq.insert(cache_block);
    times_pq.insert(cache_block);
}

cache_map* PageCache::getAllCached() {
    return &cache_blocks;
}