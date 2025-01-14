#include "../include/block_cache.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


using namespace std;
using namespace std::chrono;

#define BLOCK_SIZE 2048
#define MAX_CACHE_SIZE 16

double run_readbench(const string& filename, size_t file_size) {
  BlockCache cache(BLOCK_SIZE, MAX_CACHE_SIZE);
  int fd = cache.open(filename.c_str());
  if (fd == -1) {
    cerr << "Error opening file: " << strerror(errno) << endl;
    return 0;
  }

  vector<char> buffer(BLOCK_SIZE);
  auto start = chrono::high_resolution_clock::now();
  size_t total_bytes_read = 0;
  ssize_t bytes_read = 0;

  do {
    bytes_read = cache.read(fd, buffer.data(), BLOCK_SIZE);
  } while (bytes_read > 0);

  auto end = chrono::high_resolution_clock::now();
  chrono::duration<double> elapsed = end - start;
  double throughput = total_bytes_read / elapsed.count() / (1024 * 1024);

  cache.close(fd);
  return elapsed.count();
}

void generate_large_file(const string& filename, int64_t size) {
  ofstream largeFile(filename, ios::binary);
  largeFile.seekp(size);
  largeFile.write("", 1);
  largeFile.close();
}


int main(int argc, char *argv[]) {
  size_t size = 512 * 1024 * 1024;
  generate_large_file("large", size);
  auto result = run_readbench("large", size);
  cout << "Throughput: " << result << " Mb/s" << endl;
  return 0;
}