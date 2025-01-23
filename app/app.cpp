#include "lab2.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#define PAGE_SIZE 4096
#define TEST_FILE_SIZE (PAGE_SIZE * 1024)

void create_test_file(const char *filename) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Failed to create test file");
        exit(1);
    }

    char *buffer = (char*)malloc(PAGE_SIZE);
    memset(buffer, 'A', PAGE_SIZE);

    for (size_t i = 0; i < TEST_FILE_SIZE; i += PAGE_SIZE) {
        if (write(fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
            perror("Failed to write to test file");
            exit(1);
        }
    }

    free(buffer);
    close(fd);
}

void measure_time_without_cache(const char *filename) {
    int fd = open(filename, O_RDONLY | O_DIRECT);
    if (fd < 0) {
        perror("Failed to open file without cache");
        exit(1);
    }

    char *buffer;
    posix_memalign((void **)&buffer, PAGE_SIZE, PAGE_SIZE);

    clock_t start = clock();
    for (size_t i = 0; i < 10000; i++) {
        int j = (i * 435454321) % (TEST_FILE_SIZE / PAGE_SIZE);
        pread(fd, buffer, PAGE_SIZE, j * PAGE_SIZE);
    }
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time without cache: %.6f seconds\n", elapsed);

    free(buffer);
    close(fd);
}

void measure_time_with_cache(const char *filename) {
    int fd = lab2_open(filename);
    if (fd < 0) {
        perror("Failed to open file with cache");
        exit(1);
    }

    char *buffer = (char*)malloc(PAGE_SIZE);

    clock_t start = clock();
    for (size_t i = 0; i < 10000; i++) {
        int j = (i * 435454321) % (TEST_FILE_SIZE / PAGE_SIZE);
        lab2_lseek(fd, j * PAGE_SIZE, SEEK_SET);
        lab2_read(fd, buffer, PAGE_SIZE);
    }
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Time with cache: %.6f seconds\n", elapsed);

    free(buffer);
    lab2_close(fd);
}

int main() {
    const char *test_file = "testfile.dat";

    create_test_file(test_file);
    measure_time_without_cache(test_file);
    measure_time_with_cache(test_file);

    return 0;
}
