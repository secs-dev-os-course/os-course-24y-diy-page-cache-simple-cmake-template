
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

void evaluate_performance(const char* filepath, int blocksize, int repeat_count) {
    int file_descriptor = open(filepath, O_RDONLY);
    if (file_descriptor < 0) {
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Буфер, выровненный по границе страницы
    void* aligned_memory;
    if (posix_memalign(&aligned_memory, 4096, blocksize) != 0) {
        fprintf(stderr, "Allocation of aligned buffer failed\n");
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }

    size_t total_read_bytes = 0;
    clock_t begin_time = clock();

    for (int repeat = 0; repeat < repeat_count; repeat++) {
        off_t current_offset = lseek(file_descriptor, 0, SEEK_SET);
        if (current_offset == (off_t)-1) {
            fprintf(stderr, "Seeking error: %s\n", strerror(errno));
            free(aligned_memory);
            close(file_descriptor);
            exit(EXIT_FAILURE);
        }

        ssize_t bytes;
        while ((bytes = pread(file_descriptor, aligned_memory, blocksize, current_offset)) > 0) {
            total_read_bytes += bytes;
            current_offset += bytes;
        }

        if (bytes < 0 && errno != EINTR) {
            fprintf(stderr, "Reading error: %s\n", strerror(errno));
            free(aligned_memory);
            close(file_descriptor);
            exit(EXIT_FAILURE);
        }
    }

    clock_t finish_time = clock();
    double elapsed_time = (double)(finish_time - begin_time) / CLOCKS_PER_SEC;

    printf("Total read: %zu bytes\n", total_read_bytes);
    printf("Duration: %.2f seconds\n", elapsed_time);
    printf("Data rate: %.2f MB/s\n", (total_read_bytes / (1024.0 * 1024.0)) / elapsed_time);

    free(aligned_memory);
    close(file_descriptor);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filepath> <repeat_count>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* filepath = argv[1];
    int repeat_count = atoi(argv[2]);

    int blocksize = 2048;

    evaluate_performance(filepath, blocksize, repeat_count);

    return EXIT_SUCCESS;
}
