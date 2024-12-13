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

namespace {

constexpr int KBlockSize = 16 * 1024;         // 16K
constexpr int KFileSize = 5 * 1024 * 1024;  // 300MB
constexpr double KMegabyte = 1024.0 * 1024.0;

// Генерация уникального имени файла для каждого процесса
[[nodiscard]] auto GenerateUniqueFilename() -> std::string {
  return "test_file_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_" +
         std::to_string(getpid()) + ".bin";
}

// NOLINTNEXTLINE(readability-identifier-naming)
class FileHandler {
public:
  // NOLINTNEXTLINE(readability-identifier-naming)
  explicit FileHandler(const std::string& filename, const char* mode) : file_{nullptr} {
    // NOLINTNEXTLINE(readability-identifier-naming)
    file_ = std::fopen(filename.c_str(), mode);
    // NOLINTNEXTLINE(readability-identifier-naming)
    if (!file_) {
      throw std::runtime_error("Error with file operation");
    }
  }

  ~FileHandler() {
    // NOLINTNEXTLINE(readability-identifier-naming)
    if (file_) {
      // NOLINTNEXTLINE(readability-identifier-naming)
      std::fclose(file_);
      // NOLINTNEXTLINE(readability-identifier-naming)
      file_ = nullptr;
    }
  }

  FileHandler(const FileHandler&) = delete;
  auto operator=(const FileHandler&) -> FileHandler& = delete;
  FileHandler(FileHandler&&) noexcept = default;
  auto operator=(FileHandler&&) noexcept -> FileHandler& = default;

  [[nodiscard]] auto Get() const -> FILE* {
    // NOLINTNEXTLINE(readability-identifier-naming)
    return file_;
  }

private:
  // NOLINTNEXTLINE(readability-identifier-naming)
  FILE* file_{nullptr};
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
    const auto written = std::fwrite(buffer.data(), 1, buffer.size(), file.Get());
    if (written != buffer.size()) {
      throw std::runtime_error("Failed to write to file");
    }
  }
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

        std::rewind(file.Get());
        std::size_t bytes_read{0};
        while ((bytes_read = std::fread(buffer.data(), 1, buffer.size(), file.Get())) > 0) {
          iteration_bytes += static_cast<std::int64_t>(bytes_read);
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

}  // namespace

auto main(int argc, char* argv[]) -> int {
  try {
    if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " <repetitions>\n";
      return 1;
    }
    // NOLINTBEGIN(cppcoreguidelines-init-variables)
    const int repetitions = std::stoi(argv[1]);
    // NOLINTEND(cppcoreguidelines-init-variables)
    MeasureReadThroughput(repetitions);
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}