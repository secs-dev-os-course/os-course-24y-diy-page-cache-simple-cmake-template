#include <FilesInterface.h>

// int fd = open(path, O_RDWR | O_DIRECT); - open file bypassing OS page cache

int Lab2File::open(const char *path) {

}

int Lab2File::close(int fd) {

}

ssize_t Lab2File::read(int fd, void* buf, size_t count) {

}

ssize_t Lab2File::write(int fd, const void* buf, size_t count) {

}

int Lab2File::lseek(int fd, int offset, int whence) {

}

int Lab2File::fsync(int fd) {

}
