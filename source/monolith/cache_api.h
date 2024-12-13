#pragma once
#include <cstddef>
#include <sys/types.h>

#ifdef _WIN32
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT
#endif

extern "C" {
    DLL_EXPORT int lab2_open(const char* path);
    DLL_EXPORT int lab2_close(int fd);
    DLL_EXPORT ssize_t lab2_read(int fd, void* buf, size_t count);
    DLL_EXPORT ssize_t lab2_write(int fd, const void* buf, size_t count);
    DLL_EXPORT off_t lab2_lseek(int fd, off_t offset, int whence);
    DLL_EXPORT int lab2_fsync(int fd);
}