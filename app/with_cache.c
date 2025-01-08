
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "app.h"

void perform_test(const char* path, int buffer_size, int num_iterations) {
    int file_desc = lab2_open(path, O_RDONLY, 0644);

    if (file_desc < 0) {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }

    char* read_buffer = (char*)malloc(buffer_size);
    if (!read_buffer) {
        perror("Buffer allocation failed");
        lab2_close(file_desc);
        exit(EXIT_FAILURE);
    }

    size_t bytes_total_read = 0;
    clock_t start_clock = clock();

    int next_access = 0;
    for (int i = 0; i < num_iterations; i++) {
        off_t seek_offset = lab2_lseek(file_desc, 0, SEEK_SET);
        ssize_t read_bytes;


        while ((read_bytes = lab2_read(file_desc, read_buffer, buffer_size)) > 0) {
            bytes_total_read += read_bytes;
        }

        if (read_bytes < 0) {
            perror("File reading error");
            free(read_buffer);
            lab2_close(file_desc);
            exit(EXIT_FAILURE);
        }
    }

    clock_t end_clock = clock();
    double total_seconds = (double)(end_clock - start_clock) / CLOCKS_PER_SEC;

    printf("Bytes read in total: %zu bytes\n", bytes_total_read);
    printf("Duration: %.2f seconds\n", total_seconds);
    printf("Reading speed: %.2f MB/s\n", 
           (bytes_total_read / (1024.0 * 1024.0)) / total_seconds);
    
    free(read_buffer);
    lab2_close(file_desc);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <iterations>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* file_path = argv[1];
    int repeat_count = atoi(argv[2]);

    // Установим размер буфера в 2048 байт
    int buffer_size = 2048;

    perform_test(file_path, buffer_size, repeat_count);

    return EXIT_SUCCESS;
}
