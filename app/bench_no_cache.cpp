#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>


using namespace std;
using namespace std::chrono;

#define BLOCK_SIZE 2048

double run_readbench(const string& filename) {
  int fd = ::open(filename.c_str(), O_RDWR);
  if (fd == -1){
      return -1;
  }
  // remove default cache on MacOS
  if (fcntl(fd, F_NOCACHE, 1) == -1)
  {
      ::close(fd);
      return -1;
  }
  FILE* file = fdopen(fd, "rb");
  if (!file) {
    cerr << "Error converting file descriptor to FILE*: " << strerror(errno) << endl;
    close(fd);
    return 0;
  }

  vector<char> buffer(BLOCK_SIZE);
  auto start = chrono::high_resolution_clock::now();
  size_t total_bytes_read = 0;

  while (fread(buffer.data(), 1, BLOCK_SIZE, file) == BLOCK_SIZE) {
    total_bytes_read += BLOCK_SIZE;
  }
  total_bytes_read += fread(buffer.data(), 1, BLOCK_SIZE, file);

  auto end = chrono::high_resolution_clock::now();
  chrono::duration<double> elapsed = end - start;
  double throughput = total_bytes_read / elapsed.count() / (1024 * 1024);

  return throughput;
}

void generate_large_file(const string& filename, int64_t size) {
  ofstream largeFile(filename, ios::binary);
  largeFile.seekp(size);
  largeFile.write("", 1);
  largeFile.close();
}

int main(int argc, char *argv[]) {
  size_t size = 512 * 1024 * 1024;
  generate_large_file("large2", size);
  auto result = run_readbench("large");
  cout << "Throughput: " << result << " Mb/s" << endl;
  return 0;
}
