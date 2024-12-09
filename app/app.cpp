#include "combined.hpp"
#include "dedup.hpp"
#include "ema_search_int.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>

enum class Loaders { EMA_SEARCH_INT, DEDUP, COMBINED };

void runEmaSearchInt() {
  const char *filename;
  int target, repetitions;

  printf("Введите путь к входному файлу: ");
  char inputFile[256];
  scanf("%s", inputFile);
  filename = inputFile;

  printf("Введите искомое число: ");
  scanf("%d", &target);

  printf("Введите количество повторений: ");
  scanf("%d", &repetitions);

  auto start = std::chrono::high_resolution_clock::now();
  emaSearchInt(filename, target, repetitions);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  printf("[EMA Search Int] Время выполнения: %.5f секунд\n", duration.count());
}

void runDedup() {
  const char *inputFile;
  const char *outputFile;
  int repetitions;

  printf("Введите путь к входному файлу: ");
  char inFile[256], outFile[256];
  scanf("%s", inFile);
  inputFile = inFile;

  printf("Введите путь к выходному файлу: ");
  scanf("%s", outFile);
  outputFile = outFile;

  printf("Введите количество повторений: ");
  scanf("%d", &repetitions);

  auto start = std::chrono::high_resolution_clock::now();
  dedup(inputFile, outputFile, repetitions);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  printf("[Dedup] Время выполнения: %.5f секунд\n", duration.count());
}

void runCombinedBenchmark() {
  const char *inputFile;
  const char *outputFile;
  int target, repetitions;

  printf("Введите путь к входному файлу: ");
  char inFile[256], outFile[256];
  scanf("%s", inFile);
  inputFile = inFile;

  printf("Введите путь к выходному файлу: ");
  scanf("%s", outFile);
  outputFile = outFile;

  printf("Введите искомое число: ");
  scanf("%d", &target);

  printf("Введите количество повторений: ");
  scanf("%d", &repetitions);

  auto start = std::chrono::high_resolution_clock::now();
  int result =
      runCombinedBenchmark(inputFile, outputFile, target,
                           repetitions); // запуск комбинированного бенчмарка
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;

  if (result == 0) {
    printf("[Combined Benchmark] Время выполнения: %.5f секунд\n",
           duration.count());
  } else {
    printf(
        "[Combined Benchmark] Произошла ошибка в комбинированном бенчмарке\n");
  }
}

int main() {
  int choice;

  printf("Выберите нагрузчик:\n");
  printf("1 - EMA Search Int\n");
  printf("2 - Deduplication\n");
  printf("3 - Combined Benchmark\n");
  printf("Введите номер: ");
  scanf("%d", &choice);

  switch (choice) {
  case 1:
    runEmaSearchInt();
    break;
  case 2:
    runDedup();
    break;
  case 3:
    runCombinedBenchmark();
    break;
  default:
    printf("Неверный выбор\n");
  }

  return 0;
}
