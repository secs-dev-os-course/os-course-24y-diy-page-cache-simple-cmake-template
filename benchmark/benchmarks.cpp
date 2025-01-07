#include <benchmark/benchmark.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>

#include "Sorter.h"

const int BUFFER_SIZE = 1024;

// static void BM_LoaderBench(benchmark::State& state) {
//     std::string input_file = "input.txt";
//     std::string output_file = "output.txt";
//     size_t bytes_amount = 2500;

//     for (auto _ : state) {
//         try {
//             Sorter sorter(input_file, output_file, bytes_amount);
//             sorter.sortTapes();
//         } catch (std::exception& e) {
//             std::cerr << e.what() << std::endl;
//         }
//     }
// }
// BENCHMARK(BM_LoaderBench);

static void BM_SystemCalls(benchmark::State& state) {
    const char* filename = "benchmark_file.txt";
    const char* data = "Hello, World!";
    size_t data_size = strlen(data);

    for (auto _ : state) {
        int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd == -1) {
            std::cerr << "Error opening file for writing!" << std::endl;
            return;
        }

        if (write(fd, data, data_size) == -1) {
            std::cerr << "Error writing to file!" << std::endl;
            close(fd);
            return;
        }

        close(fd);

        fd = open(filename, O_RDONLY);
        if (fd == -1) {
            std::cerr << "Error opening file for reading!" << std::endl;
            return;
        }

        std::vector<char> buffer(data_size);
        if (read(fd, buffer.data(), data_size) == -1) {
            std::cerr << "Error reading from file!" << std::endl;
            close(fd);
            return;
        }

        close(fd);
    }
}
BENCHMARK(BM_SystemCalls);

static void BM_FilesManager(benchmark::State& state) {
    const char* filename = "benchmark_file.txt";
    const char* data = "Hello, World!";
    size_t data_size = strlen(data);
    FilesManager files_manager(1024);

    for (auto _ : state) {
        int fd = files_manager.f_open(filename);
        if (fd == -1) {
            std::cerr << "Error opening file for writing!" << std::endl;
            return;
        }

        if (files_manager.f_write(fd, data, data_size) == -1) {
            std::cerr << "Error writing to file!" << std::endl;
            files_manager.f_close(fd);
            return;
        }

        files_manager.f_close(fd);

        fd = files_manager.f_open(filename);
        if (fd == -1) {
            std::cerr << "Error opening file for reading!" << std::endl;
            return;
        }

        // Not closing files bench
        // files_manager.f_lseek(fd, 0, SEEK_SET);

        std::vector<char> buffer(data_size);
        if (files_manager.f_read(fd, buffer.data(), data_size) == -1) {
            std::cerr << "Error reading from file!" << std::endl;
            files_manager.f_close(fd);
            return;
        }

        files_manager.f_close(fd);
    }
}
BENCHMARK(BM_FilesManager);

static void BM_SystemCalls_O_DIRECT(benchmark::State& state) {
    const char* filename = "benchmark_file.txt";
    const char* data = "Hello, World!";
    size_t data_size = strlen(data);

    char* write_buffer;
    if (posix_memalign(reinterpret_cast<void**>(&write_buffer), 4096, data_size) != 0) {
        std::cerr << "Error allocating aligned memory for writing!" << std::endl;
        return;
    }

    std::memcpy(write_buffer, data, data_size);

    char* read_buffer;
    if (posix_memalign(reinterpret_cast<void**>(&read_buffer), 4096, data_size) != 0) {
        std::cerr << "Error allocating aligned memory for reading!" << std::endl;
        free(write_buffer);
        return;
    }

    for (auto _ : state) {
        int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC | O_DIRECT, 0644);
        if (fd == -1) {
            std::cerr << "Error opening file for writing!" << std::endl;
            perror("open()");
            free(write_buffer);
            free(read_buffer);
            return;
        }

        ssize_t bytes_written = write(fd, write_buffer, 4096);
        if (bytes_written == -1) {
            std::cerr << "Error writing to file!" << std::endl;
            perror("write()");
            close(fd);
            free(write_buffer);
            free(read_buffer);
            return;
        }

        close(fd);

        fd = open(filename, O_RDONLY | O_DIRECT);
        if (fd == -1) {
            std::cerr << "Error opening file for reading!" << std::endl;
            perror("open()");
            free(write_buffer);
            free(read_buffer);
            return;
        }

        ssize_t bytes_read = read(fd, read_buffer, 4096);
        if (bytes_read == -1) {
            std::cerr << "Error reading from file!" << std::endl;
            perror("read()");
            close(fd);
            free(write_buffer);
            free(read_buffer);
            return;
        }

        close(fd);
    }

    free(write_buffer);
    free(read_buffer);
}
BENCHMARK(BM_SystemCalls_O_DIRECT);

BENCHMARK_MAIN();
