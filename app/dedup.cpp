#include "dedup.hpp"
#include "cache.hpp"
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <unordered_set>
#include <vector>

int dedup(const char *inputFile, const char *outputFile, int repetitions) {
  for (int i = 0; i < repetitions; ++i) {
    Cache cache(100);

    int fd = cache.lab2_open(inputFile);
    if (fd == -1) {
      perror("Не получилось открыть файл");
      return -1;
    }

    std::unordered_set<int> uniqueNumbers;
    std::vector<int> numbers;
    char buf[1024];
    ssize_t bytesRead;

    while ((bytesRead = cache.lab2_read(fd, buf, sizeof(buf))) > 0) {

      std::string data(buf, bytesRead);

      std::stringstream ss(data);
      int number;
      while (ss >> number) {
        if (uniqueNumbers.find(number) == uniqueNumbers.end()) {
          uniqueNumbers.insert(number);
          numbers.push_back(number);
        }
      }
    }

    cache.lab2_close(fd);

    FILE *out = fopen(outputFile, "w");
    if (!out) {
      perror("Не получилось открыть файл");
      return -1;
    }

    for (int num : numbers) {
      fprintf(out, "%d\n", num);
    }
    fclose(out);
  }

  printf("[dedup] Бенчмарк завершен и записан в %s\n", outputFile);
  return 0;
}
