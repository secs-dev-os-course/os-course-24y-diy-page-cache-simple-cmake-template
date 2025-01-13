#include <iostream>
#include <cstring>
#include "../include/block_cache.hpp"

using namespace std;

int main() {
    const char* path = "test_file.txt";
    BlockCache cache(2048, 10);

    int fd = cache.open(path);
    if (fd == -1) {
        cerr << "Failed to open file" << endl;
        return 1;
    }

    const char* write_data = "This is successful test!";
    if (cache.write(fd, write_data, strlen(write_data)) == -1) {
        cerr << "Failed to write to cache" << endl;
        return 1;
    }

    if (cache.fsync(fd) == -1) {
        cerr << "Failed to sync cache" << endl;
        return 1;
    }

    if (cache.lseek(fd, 0, SEEK_SET) == -1) {
        cerr << "Failed to seek in file" << endl;
        return 1;
    }

    char read_data[100];
    if (cache.read(fd, read_data, strlen(write_data)) == -1) {
        cerr << "Failed to read from cache" << endl;
        return 1;
    }

    read_data[strlen(write_data)] = '\0';
    cout << "Read from cache: " << read_data << endl;

    if (cache.close(fd) == -1) {
        cerr << "Failed to close file" << endl;
        return 1;
    }

    return 0;
}
