#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include "cache_api.h"  // Добавляем наш API

namespace {

constexpr int KBlockSize = 16 * 1024;         // 16K
constexpr int KFileSize = 5 * 1024 * 1024;   // 5MB
constexpr double KMegabyte = 1024.0 * 1024.0;

[[nodiscard]] auto GenerateUniqueFilename() -> std::string {
    return "test_file_" +
           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_" +
           std::to_string(getpid()) + ".bin";
}


class FileHandler {
public:
    explicit FileHandler(const std::string& filename, const char* mode) : fd_(-1) {
        if (mode[0] == 'w') {
            FILE* tmp = std::fopen(filename.c_str(), mode);
            if (tmp) std::fclose(tmp);
        }
        
        fd_ = lab2_open(filename.c_str());
        if (fd_ == -1) {
            throw std::runtime_error("Error with file operation");
        }
    }

    ~FileHandler() {
        if (fd_ != -1) {
            lab2_fsync(fd_);  
            lab2_close(fd_);
            fd_ = -1;
        }
    }

    FileHandler(const FileHandler&) = delete;
    auto operator=(const FileHandler&) -> FileHandler& = delete;
    FileHandler(FileHandler&&) noexcept = default;
    auto operator=(FileHandler&&) noexcept -> FileHandler& = default;

    [[nodiscard]] auto GetFd() const -> int {
        return fd_;
    }

private:
    int fd_;
};

void CreateTestFile(const std::string& filename) {
    FileHandler file(filename, "wb");
    alignas(4096) std::array<char, KBlockSize> buffer{};

    std::random_device random_device;
    std::mt19937 gen(random_device());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < KFileSize / KBlockSize; i++) {
        for (auto& byte : buffer) {
            byte = static_cast<char>(dis(gen));
        }
        const auto written = lab2_write(file.GetFd(), buffer.data(), buffer.size());
        if (written != static_cast<ssize_t>(buffer.size())) {
            throw std::runtime_error("Failed to write to file");
        }
    }
    lab2_fsync(file.GetFd()); 
}

void MeasureReadThroughput(const int repetitions) {
    const std::string filename = GenerateUniqueFilename();
    alignas(4096) std::array<char, KBlockSize> buffer{};
    std::int64_t total_bytes = 0;

    try {
        CreateTestFile(filename);
        {
            FileHandler file(filename, "rb");
            const auto total_start = std::chrono::steady_clock::now();

            for (int i = 0; i < repetitions; i++) {
                const auto iteration_start = std::chrono::steady_clock::now();
                std::int64_t iteration_bytes = 0;

                lab2_lseek(file.GetFd(), 0, SEEK_SET);
                ssize_t bytes_read;
                while ((bytes_read = lab2_read(file.GetFd(), buffer.data(), buffer.size())) > 0) {
                    iteration_bytes += bytes_read;
                }

                const auto iteration_end = std::chrono::steady_clock::now();
                const auto iteration_duration = std::chrono::duration_cast<std::chrono::duration<double>>(
                    iteration_end - iteration_start);

                total_bytes += iteration_bytes;
                const auto iteration_bytes_double = static_cast<double>(iteration_bytes);
                const double iteration_throughput = (iteration_bytes_double / KMegabyte) / iteration_duration.count();

                std::cout << "Iteration " << (i + 1) << " results:\n"
                         << "  Read time: " << iteration_duration.count() << " seconds\n"
                         << "  Data read: " << iteration_bytes / KMegabyte << " MB\n"
                         << "  Throughput: " << iteration_throughput << " MB/s\n\n";
            }

            const auto total_end = std::chrono::steady_clock::now();
            const auto total_duration = std::chrono::duration_cast<std::chrono::duration<double>>(
                total_end - total_start);

            const auto total_bytes_double = static_cast<double>(total_bytes);
            const double avg_throughput = (total_bytes_double / KMegabyte) / total_duration.count();

            std::cout << "Total results for process " << getpid() << ":\n"
                     << "Total read time: " << total_duration.count() << " seconds\n"
                     << "Total data read: " << total_bytes / KMegabyte << " MB\n"
                     << "Average throughput: " << avg_throughput << " MB/s\n";
        }

    
        int retry_count = 0;
        const int max_retries = 5;

        try {
            if (std::filesystem::exists(filename)) {
                FileHandler file(filename, "rb");

                const auto file_size = lab2_lseek(file.GetFd(), 0, SEEK_END);  
                const auto middle_position = file_size / 2;
                lab2_lseek(file.GetFd(), middle_position, SEEK_SET);  

                const auto read_start = std::chrono::steady_clock::now(); 

                ssize_t bytes_read = lab2_read(file.GetFd(), buffer.data(), buffer.size());

                const auto read_end = std::chrono::steady_clock::now();
                const auto read_duration = std::chrono::duration_cast<std::chrono::duration<double>>(read_end - read_start);

                if (bytes_read > 0) {
                    const double read_speed = (static_cast<double>(bytes_read) / KMegabyte) / read_duration.count();

                    std::cout << "Middle data read (" << bytes_read << " bytes): ";
                    for (ssize_t i = 0; i < std::min(static_cast<ssize_t>(10), bytes_read); i++) {
                        std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
                    }
                    std::cout << std::dec << "...\n";  
                    std::cout << "Middle read time: " << read_duration.count() << " seconds\n";
                    std::cout << "Middle read speed: " << read_speed << " MB/s\n";
                } else {
                    std::cerr << "Failed to read middle of the file.\n";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error reading middle of the file: " << e.what() << '\n';
        }

        while (retry_count < max_retries) {
            try {
                if (std::filesystem::exists(filename)) {
                    std::filesystem::remove(filename);
                    break;
                }
                break;
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Failed to remove file on attempt " << retry_count + 1 << ": " << e.what()
                         << '\n';
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                retry_count++;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Process " << getpid() << " error: " << e.what() << '\n';
        if (std::filesystem::exists(filename)) {
            std::filesystem::remove(filename);
        }
    }
}

}

auto main(int argc, char* argv[]) -> int {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <repetitions>\n";
            return 1;
        }
        const int repetitions = std::stoi(argv[1]);
        MeasureReadThroughput(repetitions);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}