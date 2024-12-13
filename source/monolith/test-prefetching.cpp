#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <chrono>
#include "cache_api.h"

void generate_test_file(const char* file_path, size_t file_size) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create file: " << file_path << std::endl;
        return;
    }

    std::vector<char> block(4096, 'A');
    size_t blocks_to_write = file_size / block.size();

    for (size_t i = 0; i < blocks_to_write; ++i) {
        file.write(block.data(), block.size());
        if (!file) {
            std::cerr << "Failed to write to file." << std::endl;
            break;
        }
    }

    file.close();
    if (file) {
        std::cout << "File generated: " << file_path << ", size: " << file_size << " bytes." << std::endl;
    } else {
        std::cerr << "Failed to close the file after writing." << std::endl;
    }
}

void test_prefetching(const char* file_path) {
    int fd = lab2_open(file_path);
    if (fd < 0) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    const size_t block_size = 4096;
    const size_t read_blocks = 10;
    char buffer[block_size * read_blocks];

    std::cout << "Reading blocks sequentially..." << std::endl;
    for (size_t i = 0; i < read_blocks; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now(); 

        ssize_t bytes_read = lab2_read(fd, buffer + i * block_size, block_size);

        auto end_time = std::chrono::high_resolution_clock::now(); 
        std::chrono::duration<double> elapsed_time = end_time - start_time;

        if (bytes_read <= 0) {
            std::cerr << "Failed to read block " << i << std::endl;
            break;
        }

        double speed = bytes_read / elapsed_time.count() / 1024.0;
        std::cout << "Read block " << i 
                  << ", bytes: " << bytes_read 
                  << ", speed: " << speed << " KB/s" << std::endl;
    }

    if (lab2_close(fd) != 0) {
        std::cerr << "Failed to close file." << std::endl;
    } else {
        std::cout << "File closed successfully." << std::endl;
    }
}

int main() {
    const char* file_path = "test_file.dat";
    size_t file_size = 64 * 1024; // 64 КБ (16 блоков по 4096 байт)

    // Генерация тестового файла
    std::cout << "Generating test file..." << std::endl;
    generate_test_file(file_path, file_size);

    // Тестирование предзагрузки
    std::cout << "Testing cache prefetching..." << std::endl;
    test_prefetching(file_path);

    return 0;
}
