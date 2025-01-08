
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include "app_opt.h"



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

    
    size_t file_size = lseek(file_desc, 0, SEEK_END);
    size_t num_blocks = file_size / buffer_size;

    size_t bytes_total_read = 0;
    clock_t start_clock = clock();

    int next_access = num_blocks; 
    int step = -1;  // вместимость 3; 1234 4321 1234 4321
    for (int i = 0; i < num_iterations; i++) {
        // printf("Итерация %d\n", i);
        off_t seek_offset = (step == -1) ? lab2_lseek(file_desc, 0, SEEK_SET) : lab2_lseek(file_desc, -2048, SEEK_END);
        ssize_t read_bytes;

        int offset = 0;
        while ((read_bytes = lab2_read(file_desc, read_buffer, buffer_size, next_access)) > 0) {
            bytes_total_read += read_bytes;
            next_access += step;
            // printf("%c встретится на позиции %d\n", 'A' + offset++, next_access);

            if (step == 1 && lab2_lseek(file_desc, -4096, SEEK_CUR) == -1) {
                break;
            }
        }

        if (read_bytes < 0) {
            perror("File reading error");
            free(read_buffer);
            lab2_close(file_desc);
            exit(EXIT_FAILURE);
        }
        step *= -1;
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


// void perform_test(const char* path, int buffer_size, int num_iterations) {
//     int file_desc = open(path, O_RDONLY);
//     if (file_desc < 0) {
//         perror("Unable to open file");
//         exit(EXIT_FAILURE);
//     }

//     char* read_buffer = (char*)malloc(buffer_size);
//     if (!read_buffer) {
//         perror("Buffer allocation failed");
//         close(file_desc);
//         exit(EXIT_FAILURE);
//     }

//     struct timeval start;
//     struct timeval end;
//     gettimeofday(&start, NULL);

//     size_t file_size = lseek(file_desc, 0, SEEK_END);
//     size_t num_blocks = file_size / buffer_size;
//     printf("Total num blocks: %zu blocks\n", num_blocks);
    
//     size_t total_bytes_read = 0;

//     for (int i = 0; i < num_iterations; i++) {
//         for (int j = 0; j < num_blocks; j++) {
//             off_t offset = (rand() % num_blocks) * buffer_size;
//             lseek(file_desc, offset, SEEK_SET);
            
//             ssize_t read_bytes = read(file_desc, read_buffer, buffer_size);
//             if (read_bytes < 0) {
//                 perror("Read error");
//                 free(read_buffer);
//                 close(file_desc);
//                 exit(EXIT_FAILURE);
//             }
//             total_bytes_read += read_bytes;
//         }
//     }

//     gettimeofday(&end, NULL);
//     double total_seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

//     printf("Total bytes read: %zu bytes\n", total_bytes_read);
//     printf("Duration: %.6f seconds\n", total_seconds);
//     printf("Reading speed: %.2f MB/s\n", (total_bytes_read / (1024.0 * 1024.0)) / total_seconds);

//     free(read_buffer);
//     close(file_desc);
// }


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
