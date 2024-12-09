#include "combined.hpp"

#include <cstdio>
#include <pthread.h>

#include "dedup.hpp"
#include "ema_search_int.hpp"

struct ThreadArgs {
  const char *inputFile;
  const char *outputFile;
  int target;
  int repetitions;
};

void *emaSearchThread(void *args) {
  ThreadArgs *threadArgs = (ThreadArgs *)args;
  int result = emaSearchInt(threadArgs->inputFile, threadArgs->target,
                            threadArgs->repetitions);
  pthread_exit((void *)(long)result);
}

void *dedupThread(void *args) {
  ThreadArgs *threadArgs = (ThreadArgs *)args;
  int result = dedup(threadArgs->inputFile, threadArgs->outputFile,
                     threadArgs->repetitions);
  pthread_exit((void *)(long)result);
}

int runCombinedBenchmark(const char *inputFile, const char *outputFile,
                         int target, int repetitions) {
  pthread_t emaThread, dedupThreadHandle;
  ThreadArgs emaArgs = {inputFile, nullptr, target, repetitions};
  ThreadArgs dedupArgs = {inputFile, outputFile, 0, repetitions};

  if (pthread_create(&emaThread, nullptr, emaSearchThread, &emaArgs) != 0) {
    perror("Не получилось создать ema-search-int поток");
    return -1;
  }

  if (pthread_create(&dedupThreadHandle, nullptr, dedupThread, &dedupArgs) !=
      0) {
    perror("Не получилось создать dedup поток");
    return -1;
  }

  void *emaResult;
  void *dedupResult;
  pthread_join(emaThread, &emaResult);
  pthread_join(dedupThreadHandle, &dedupResult);

  if ((long)emaResult != 0 || (long)dedupResult != 0) {
    fprintf(stderr, "Один из потоков сломался\n");
    return -1;
  }

  printf("Объединенный бенчмарк завершен\n");
  return 0;
}
