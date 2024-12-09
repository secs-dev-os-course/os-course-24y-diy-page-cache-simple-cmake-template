#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstddef>
#include <cstdio>

class Cache {
public:
  Cache(size_t cacheSize);
  ~Cache();

  int lab2_open(const char *path);
  ssize_t lab2_read(int fd, void *buf, size_t count);
  ssize_t lab2_write(int fd, const void *buf, size_t count);
  int lab2_close(int fd);
  off_t lab2_lseek(int fd, off_t offset, int whence);
  int lab2_fsync(int fd);

private:
  struct CacheEntry {
    void *data;
    size_t size;
    bool isDirty;
    unsigned int frequency;
  };

  size_t cacheSize;
  size_t nextPageIndex;
  CacheEntry *cache;

  void evictPage();
  void flushPage(int fd, CacheEntry &entry);
  size_t calculatePageIndex(off_t offset);
};

#endif // CACHE_HPP
