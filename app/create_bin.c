#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 2048  // Размер блока, который будет соответствовать размеру кэша

void generate_optimized_file(const char* filename, size_t num_blocks) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Unable to create file");
        exit(EXIT_FAILURE);
    }

    // Каждый блок будет заполнен возрастающими числами для удобства
    unsigned char* block = (unsigned char*)malloc(BLOCK_SIZE);
    if (!block) {
        perror("Memory allocation failed");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < num_blocks; ++i) {
        // Заполняем блок данными (например, возрастающими числами)
        for (size_t j = 0; j < BLOCK_SIZE; ++j) {
            block[j] = (unsigned char)(j % 2048);
        }

        // Пишем блок в файл
        if (fwrite(block, 1, BLOCK_SIZE, file) != BLOCK_SIZE) {
            perror("Write error");
            free(block);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    free(block);
    fclose(file);
    printf("Файл создан успешно: %s\n", filename);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <filename> <num_blocks>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    size_t num_blocks = strtoul(argv[2], NULL, 10);

    generate_optimized_file(filename, num_blocks);

    return EXIT_SUCCESS;
}