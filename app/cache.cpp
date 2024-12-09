#include "cache.hpp"
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

Cache::Cache(size_t cacheSize) : cacheSize(cacheSize), nextPageIndex(0) {
  cache = new CacheEntry[cacheSize];
  for (size_t i = 0; i < cacheSize; ++i) {
    cache[i].data = nullptr;
    cache[i].size = 0;
    cache[i].isDirty = false;
    cache[i].frequency = 0;
  }
}

Cache::~Cache() {
  for (size_t i = 0; i < cacheSize; ++i) {
    if (cache[i].data) {
      delete[] static_cast<char *>(cache[i].data);
    }
  }
  delete[] cache;
}

int Cache::lab2_open(const char *path) {
  return open(path, O_RDWR | O_CREAT, 0644);
}

ssize_t Cache::lab2_read(int fd, void *buf, size_t count) {
  ssize_t bytesRead = read(fd, buf, count);
  if (bytesRead > 0) {
    off_t offset = lseek(fd, 0, SEEK_CUR);

    size_t pageIndex = calculatePageIndex(offset);

    cache[pageIndex].frequency++;

    evictPage();
  }
  return bytesRead;
}

ssize_t Cache::lab2_write(int fd, const void *buf, size_t count) {
  ssize_t bytesWritten = write(fd, buf, count);
  if (bytesWritten > 0) {
    off_t offset = lseek(fd, 0, SEEK_CUR);

    size_t pageIndex = calculatePageIndex(offset);

    cache[pageIndex].isDirty = true;
    cache[pageIndex].frequency++;

    evictPage();
  }
  return bytesWritten;
}

int Cache::lab2_close(int fd) {

  for (size_t i = 0; i < cacheSize; ++i) {
    if (cache[i].isDirty) {
      flushPage(fd, cache[i]);
    }
  }
  return close(fd);
}

off_t Cache::lab2_lseek(int fd, off_t offset, int whence) {
  return lseek(fd, offset, whence);
}

int Cache::lab2_fsync(int fd) { return fsync(fd); }

void Cache::evictPage() {

  unsigned int minFrequency = UINT_MAX;
  size_t minIndex = 0;
  for (size_t i = 0; i < cacheSize; ++i) {
    if (cache[i].frequency < minFrequency) {
      minFrequency = cache[i].frequency;
      minIndex = i;
    }
  }

  if (cache[minIndex].isDirty) {
    flushPage(minIndex, cache[minIndex]);
  }

  if (cache[minIndex].data) {
    delete[] static_cast<char *>(cache[minIndex].data);
    cache[minIndex].data = nullptr;
    cache[minIndex].size = 0;
    cache[minIndex].frequency = 0;
  }
}

void Cache::flushPage(int fd, CacheEntry &entry) {
  if (entry.isDirty) {
    write(fd, entry.data, entry.size);
    entry.isDirty = false;
  }
}

size_t Cache::calculatePageIndex(off_t offset) {
  const size_t pageSize = 4096;
  size_t pageIndex = (offset / pageSize) % cacheSize;
  return pageIndex;
}
