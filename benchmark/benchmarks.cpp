#include <benchmark/benchmark.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>

#include "FilesInterface.h"

// static void BM_SystemCalls(benchmark::State& state) {
//     const char* filename = "benchmark_file.txt";
//     const char* data = "Hello, World!";
//     size_t data_size = strlen(data);
//
//     for (auto _ : state) {
//         int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
//         if (fd == -1) {
//             std::cerr << "Error opening file for writing!" << std::endl;
//             return;
//         }
//
//         if (write(fd, data, data_size) == -1) {
//             std::cerr << "Error writing to file!" << std::endl;
//             close(fd);
//             return;
//         }
//
//         lseek(fd, 0, SEEK_SET);
//
//         std::vector<char> buffer(data_size);
//         if (read(fd, buffer.data(), data_size) == -1) {
//             std::cerr << "Error reading from file!" << std::endl;
//             close(fd);
//             return;
//         }
//
//         close(fd);
//     }
// }
// BENCHMARK(BM_SystemCalls);

static void BM_FilesManager(benchmark::State& state) {
    const char* filename = "benchmark_file.txt";
    const size_t buffer_size = 1024 * 1024; // 1 MB
    std::vector<char> data(buffer_size);

    for (size_t i = 0; i < buffer_size; ++i) {
        data[i] = 'A' + (rand() % 26); // Генерируем символы от 'A' до 'Z'
    }

    FilesManager files_manager(1024);

    for (auto _ : state) {
        // Открываем файл
        int fd = files_manager.f_open(filename);
        if (fd == -1) {
            std::cerr << "Error opening file for writing!" << std::endl;
            return;
        }

        // Записываем данные в файл
        if (files_manager.f_write(fd, data.data(), buffer_size) == -1) {
            std::cerr << "Error writing to file!" << std::endl;
            files_manager.f_close(fd);
            return;
        }

        // Сбрасываем указатель на начало файла для чтения
        files_manager.f_lseek(fd, 0, SEEK_SET);

        // Читаем данные обратно в буфер
        std::vector<char> buffer(buffer_size);
        if (files_manager.f_read(fd, buffer.data(), buffer_size) == -1) {
            std::cerr << "Error reading from file!" << std::endl;
            files_manager.f_close(fd);
            return;
        }

        // Закрываем файл
        files_manager.f_close(fd);
    }
}

// static void BM_SystemCalls_O_DIRECT(benchmark::State& state) {
//     const char* filename = "benchmark_file.txt";
//     const char* data = "Hello, World!";
//     size_t data_size = strlen(data);
//
//     char* write_buffer;
//     if (posix_memalign(reinterpret_cast<void**>(&write_buffer), 4096, data_size) != 0) {
//         std::cerr << "Error allocating aligned memory for writing!" << std::endl;
//         return;
//     }
//
//     std::memcpy(write_buffer, data, data_size);
//
//     char* read_buffer;
//     if (posix_memalign(reinterpret_cast<void**>(&read_buffer), 4096, data_size) != 0) {
//         std::cerr << "Error allocating aligned memory for reading!" << std::endl;
//         free(write_buffer);
//         return;
//     }
//
//     for (auto _ : state) {
//         int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC | O_DIRECT, 0644);
//         if (fd == -1) {
//             std::cerr << "Error opening file for writing!" << std::endl;
//             perror("open()");
//             free(write_buffer);
//             free(read_buffer);
//             return;
//         }
//
//         ssize_t bytes_written = write(fd, write_buffer, 4096);
//         if (bytes_written == -1) {
//             std::cerr << "Error writing to file!" << std::endl;
//             perror("write()");
//             close(fd);
//             free(write_buffer);
//             free(read_buffer);
//             return;
//         }
//
//         lseek(fd, 0, SEEK_SET);
//
//         ssize_t bytes_read = read(fd, read_buffer, 4096);
//         if (bytes_read == -1) {
//             std::cerr << "Error reading from file!" << std::endl;
//             perror("read()");
//             close(fd);
//             free(write_buffer);
//             free(read_buffer);
//             return;
//         }
//
//         close(fd);
//     }
//
//     free(write_buffer);
//     free(read_buffer);
// }
// BENCHMARK(BM_SystemCalls_O_DIRECT);

BENCHMARK_MAIN();
