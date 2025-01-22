#pragma once

#include <windows.h>

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <list>
#include <vector>
#include <algorithm>

#define CACHE_PAGE_SIZE (256 * 1 << 10) // 128 Kb
#define CACHE_SIZE (256 * 1 << 20) // 128 Mb

class SecondChanceCache {
    struct Block {
        int key;
        std::vector<char> v;
        bool second_chance;

        Block(int key, const std::vector<char> &v, bool second_chance = false);

    };

    std::unordered_map<int, Block *> cache;
    std::list<Block *> cache_list;
    const char *path{};
    bool isOpen{};
    HANDLE hFile{};
    off_t offset = 0;


public:
    explicit SecondChanceCache(const char *path);

    ~SecondChanceCache();

    off_t lseek(off_t offset, int whence);

    size_t read(void *buf, size_t count);

    ssize_t write(const void *buf, size_t count);

    void open();

    void close();

    void fsynchronization();

private:
    void repressionSecondChance();

    ssize_t readPage(int number_page, std::vector<char> &page);

    ssize_t writePage(int number_page, const std::vector<char> &page);

    void putPage(int number_page, const std::vector<char> &new_page);

    std::vector<char>& getPage(int number_page);
};
