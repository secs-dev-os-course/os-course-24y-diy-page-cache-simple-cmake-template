#pragma once


#include <cstddef>

#ifdef _MSC_VER
#ifdef BUILD_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif
#else
#define DLL_EXPORT
#endif

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT int lab2_open(const char *path);
DLL_EXPORT int lab2_close(int fd);
DLL_EXPORT ssize_t lab2_read(int fd, void *buf, size_t count);
DLL_EXPORT ssize_t lab2_write(int fd, const void *buf, size_t count);
DLL_EXPORT off_t lab2_lseek(int fd, off_t offset, int whence);
DLL_EXPORT int lab2_fsync(int fd);

#ifdef __cplusplus
}
#endif

