#ifndef FILES_INTERFACE
#define FILES_INTERFACE

#include <sys/types.h>

class Lab2File {

public:
	int open(const char *path);
	int close(int fd);
	ssize_t read(int fd, void* buf, size_t count);
	ssize_t write(int fd, const void* buf, size_t count);
	int lseek(int fd, int offset, int whence);
	int fsync(int fd);
};

#endif	