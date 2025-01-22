#include "API.h"
#include "SecondChanceCache.h"

std::vector<SecondChanceCache *> cache_readers;

int lab2_open(const char *path) {
    try {
        auto cache_reader = new SecondChanceCache(path);
        cache_readers.push_back(cache_reader);
        cache_reader->open();
        return cache_readers.size() - 1;
    } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}

int lab2_close(int fd) {
    try {
        cache_readers[fd]->close();
        return 0;
    } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}

ssize_t lab2_read(int fd, void *buf, size_t count) {
    try {
        ssize_t bytes_read = cache_readers[fd]->read(buf, count);
        return bytes_read;
    } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
}

ssize_t lab2_write(int fd, const void *buf, size_t count) {
    try {
        return cache_readers[fd]->write(buf, count);
    } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
}

off_t lab2_lseek(int fd, off_t offset, int whence) {
    try {
        return cache_readers[fd]->lseek(offset, whence);
    } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
}

int lab2_fsync(int fd) {
    try {
        cache_readers[fd]->fsynchronization();
        return 0;
    } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
}
