#include "ema_search_int.hpp"
#include "cache.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

int emaSearchInt(const char *filename, int target, int repetitions) {
  int index = -1;
  Cache cache(10);

  for (int i = 0; i < repetitions; ++i) {
    int fd = cache.lab2_open(filename);
    if (fd == -1) {
      perror("Не получилось открыть файл");
      return -1;
    }

    int number, currentIndex = 0;
    char buf[1024];
    ssize_t bytesRead;

    while ((bytesRead = cache.lab2_read(fd, buf, sizeof(buf))) > 0) {

      std::string data(buf, bytesRead);

      std::stringstream ss(data);
      while (ss >> number) {

        if (number == target) {
          index = currentIndex;
          break;
        }
        currentIndex++;
      }

      if (index != -1)
        break;
    }

    cache.lab2_close(fd);

    if (index != -1)
      break;
  }

  if (index != -1) {
    printf("[ema-search-int] Искомое значение %d найдено на %d\n", target,
           index);
  } else {
    printf("[ema-search-int] Искомое значение %d не найдено\n", target);
  }

  return 0;
}
