#include <fcntl.h>
#include "ema-search-int.h"

#include <SecondChanceCache/API.h>
#include <fstream>
#include <iostream>
#include <random>
#include <share.h>
#include <sys/stat.h>


const size_t FileSize = 256 * (1ULL << 20ULL) / sizeof(int64_t);  // 256 Mb
const size_t RAMSize = FileSize / 8;
const int64_t target = -666;
std::random_device rd;
std::mt19937_64 gen(rd());
std::uniform_int_distribution<int64_t> dis(1, FileSize - 1);

void prepare_array() {
    std::vector<int64_t> output(FileSize);
    for (size_t i = 0; i < FileSize; ++i) {
        output[i] = dis(gen);
    }
    const size_t place = dis(gen);
    output[place] = target;

    std::ofstream out_file("in.bin", std::ios::binary | std::ios::trunc); // NOLINT
    if (!out_file) {
        throw std::runtime_error("Failed to open output file");
    }
    out_file.write(reinterpret_cast<const char *>(output.data()), sizeof(int64_t) * FileSize); // NOLINT
    out_file.close();
}

size_t ema_search_int_without_cache() {
    prepare_array();
    int fd = -1;
    _sopen_s(&fd, "in.bin", _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD);
    if (fd < 0) {
        throw std::runtime_error("Failed to open input file");
    }

    int block = 0;
    int64_t number;

    while (true) {
        block = dis(gen);
        _lseek(fd, block * sizeof(target), SEEK_SET);
        size_t num_read = _read(fd, &number, sizeof(target));
        if (num_read == 0) {
            _close(fd);
            throw std::runtime_error("Error reading from file");
        }
        if (number == target) {
            break;
        }
    }
    _close(fd);
    std::cout << " (block " << block << ")" << '\n';
    return number;
}

size_t ema_search_int_with_cache() {
    prepare_array();
    int fd = lab2_open("in.bin");
    int block = 0;
    int64_t number;

    while (true) {
        block = dis(gen);
        lab2_lseek(fd, block * sizeof(target), SEEK_SET);
        ssize_t num_read = lab2_read(fd, &number, sizeof(target));
        if (num_read == -1) {
            lab2_close(fd);
            throw std::runtime_error("Error reading from file");
        }

        if (number == target) {
            break;
        }
    }
    lab2_close(fd);
    std::cout << "(block " << block << ")" << '\n';
    return number;
}


