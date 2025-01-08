#ifndef APP_OPT_H
#define APP_OPT_H

#include <fcntl.h> 
#include <stddef.h> 
#include <unistd.h>

// Функции API для работы с файлами, обеспечивают базовые операции
int lab2_open(const char* filepath, int access_flags, int permissions);
int lab2_close(int file_descriptor);
ssize_t lab2_read(int file_descriptor, void* buffer, size_t byte_count, int access_time);
ssize_t lab2_write(int file_descriptor, const void* buffer, size_t byte_count, int access_time);
off_t lab2_lseek(int file_descriptor, off_t position, int reference_point);
int lab2_fsync(int file_descriptor);

#endif // APP_OPT_H
