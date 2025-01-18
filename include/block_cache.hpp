#pragma once
#include <iostream>
#include <unistd.h>
#include <unordered_map>
#include <list>
#include <vector>
#include <set>

using namespace std;

struct CachePage {
    int fd;
    off_t offset;
    vector<char> data;
    bool modified;
    bool referenced;
};

class BlockCache {
public:
  explicit BlockCache(size_t block_size, size_t max_cache_size);

  int open(const char *path);
  int close(int fd);
  ssize_t read(int fd, void *buf, size_t count);
  ssize_t write(int fd, const void *buf, size_t count);
  off_t lseek(int fd, off_t offset);
  int fsync(int fd);

private:
  size_t block_size_;
  size_t max_cache_size_;
  std::list<off_t> lru_list_;
  std::unordered_map<off_t, std::list<off_t>::iterator> lru_map_;
  std::unordered_map<off_t, CachePage> cache_;
  std::unordered_map<int, off_t> fd_offsets_;
  std::set<int> file_descriptors_;

  void evictPage();
  void touchPage(off_t offset);
};
