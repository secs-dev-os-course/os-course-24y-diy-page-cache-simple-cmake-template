#include "cache_api.h"
#include "cache.h"
#include <unordered_map>
#include <memory>
#include <mutex>

static std::unordered_map<int, std::unique_ptr<Cache>> cache_map;
static std::mutex api_mutex;
static int next_fd = 1;

DLL_EXPORT int lab2_open(const char* path) {
    std::lock_guard<std::mutex> lock(api_mutex);
    try {
        int fd = next_fd++;
        cache_map[fd] = std::make_unique<Cache>(path);
        return fd;
    } catch (...) {
        return -1;
    }
}

DLL_EXPORT int lab2_close(int fd) {
    std::lock_guard<std::mutex> lock(api_mutex);
    return cache_map.erase(fd) ? 0 : -1;
}

DLL_EXPORT ssize_t lab2_read(int fd, void* buf, size_t count) {
    std::lock_guard<std::mutex> lock(api_mutex);
    auto it = cache_map.find(fd);
    if (it == cache_map.end()) return -1;
    return it->second->read(buf, count);
}

DLL_EXPORT ssize_t lab2_write(int fd, const void* buf, size_t count) {
    std::lock_guard<std::mutex> lock(api_mutex);
    auto it = cache_map.find(fd);
    if (it == cache_map.end()) return -1;
    return it->second->write(buf, count);
}

DLL_EXPORT off_t lab2_lseek(int fd, off_t offset, int whence) {
    std::lock_guard<std::mutex> lock(api_mutex);
    auto it = cache_map.find(fd);
    if (it == cache_map.end()) return -1;
    return it->second->seek(offset, whence);
}

DLL_EXPORT int lab2_fsync(int fd) {
    std::lock_guard<std::mutex> lock(api_mutex);
    auto it = cache_map.find(fd);
    if (it == cache_map.end()) return -1;
    return it->second->sync();
}