#include "SecondChanceCache.h"

SecondChanceCache::Block::Block(int key, const std::vector<char> &v, bool second_chance)
        : key(key), v(v), second_chance(second_chance) {}

void SecondChanceCache::repressionSecondChance() {
    if (cache_list.empty()) {
        return;
    }
    while (!cache_list.empty()) {
        Block *block = cache_list.front();
        if (block->second_chance) {
            block->second_chance = false;
            cache_list.pop_front();
            cache_list.push_back(block);
        } else {
            cache.erase(block->key);
            cache_list.pop_front();
            delete block;
            return;
        }
    }

}

void SecondChanceCache::open() {
    hFile = CreateFileA(
            path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_NO_BUFFERING,
            nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(std::to_string(GetLastError()));
    }
    isOpen = true;
}

ssize_t SecondChanceCache::writePage(int number_page, const std::vector<char> &page) {
    LARGE_INTEGER li;
    li.QuadPart = number_page * CACHE_PAGE_SIZE;

    if (!SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN)) {
        throw std::runtime_error("Failed to set file pointer. Error: " + std::to_string(GetLastError()));
    }
    DWORD bytes_written;
    if (!WriteFile(hFile, page.data(), static_cast<DWORD>(page.size()), &bytes_written, nullptr)) {
        throw std::runtime_error("Failed to write file. Error: " + std::to_string(GetLastError()));
    }
    if (bytes_written < page.size()) {
        throw std::runtime_error("Partial write occurred. Expected: " + std::to_string(page.size()) +
                                 ", Written: " + std::to_string(bytes_written));
    }
    return static_cast<ssize_t>(bytes_written);
}


void SecondChanceCache::fsynchronization() {
    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("File handle is invalid. Cannot sync.");
    }
    for (Block *block: cache_list) {
        try {
            writePage(block->key, block->v);
        } catch (const std::exception &e) {
            std::cerr << "Failed to sync page " << block->key << ": " << e.what() << std::endl;
        }
    }
    if (!FlushFileBuffers(hFile)) {
        throw std::runtime_error("Failed to flush file buffers. Error: " + std::to_string(GetLastError()));
    }
}


void SecondChanceCache::close() {
    if (!isOpen || hFile == INVALID_HANDLE_VALUE) {
        return;
    }
    fsync();
    if (!CloseHandle(hFile)) {
        throw std::runtime_error("Failed to close file! Error: " + std::to_string(GetLastError()));
    }
    hFile = INVALID_HANDLE_VALUE;
    isOpen = false;
}


void SecondChanceCache::putPage(int number_page, const std::vector<char> &page) {
    if (cache.size() >= CACHE_SIZE / CACHE_PAGE_SIZE) {
        repressionSecondChance();
    }
    auto *block = new SecondChanceCache::Block(number_page, page, false);
    cache[number_page] = block;
    cache_list.push_back(block);
}

ssize_t SecondChanceCache::readPage(int number_page, std::vector<char> &page) {
    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("File handle is invalid. Cannot read.");
    }
    LARGE_INTEGER li;
    li.QuadPart = number_page * CACHE_PAGE_SIZE;
    if (!SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN)) {
        throw std::runtime_error("Failed to set file pointer. Error: " + std::to_string(GetLastError()));
    }
    DWORD bytes_read = 0;
    if (!ReadFile(hFile, page.data(), static_cast<DWORD>(page.size()), &bytes_read, nullptr)) {
        throw std::runtime_error("Failed to read file. Error: " + std::to_string(GetLastError()));
    }
    return static_cast<ssize_t>(bytes_read);
}


std::vector<char> &SecondChanceCache::getPage(int number_page) {
    if (cache.contains(number_page)) {
        Block *block = cache[number_page];
        block->second_chance = true;
    } else {
        std::vector<char> new_page(CACHE_PAGE_SIZE);
        ssize_t bytes_read = readPage(number_page, new_page);
        if (bytes_read < 0) {
            throw std::runtime_error("Failed to read page from file: " + std::to_string(GetLastError()));
        }
        new_page.resize(bytes_read);
        putPage(number_page, new_page);
        if (!cache.contains(number_page)) {
            throw std::runtime_error("Page insertion into cache failed.");
        }
    }
    return cache.at(number_page)->v;
}


ssize_t SecondChanceCache::write(const void *buf, size_t count) {
    size_t bytes_write = 0;
    int start = offset / CACHE_PAGE_SIZE;
    int end = (offset + count - 1) / CACHE_PAGE_SIZE;
    for (int current = start; current <= end; current++) {
        if (bytes_write >= count) {
            break;
        }
        std::vector<char> &page = getPage(current);
        off_t page_offset;
        if (start == current) {
            page_offset = offset % CACHE_PAGE_SIZE;
        } else {
            page_offset = 0;
        }
        size_t bytes_copy = std::min(page.size() - page_offset, count - bytes_write);
        std::copy_n(static_cast<const char *>(buf) + bytes_write,
                    bytes_copy,
                    page.begin() + page_offset);

        bytes_write += bytes_copy;
        offset += bytes_copy;
    }
    return bytes_write;
}


size_t SecondChanceCache::read(void *buf, size_t count) {
    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("File handle is invalid. Cannot read.");
    }
    size_t bytes_read = 0;
    int start = offset / CACHE_PAGE_SIZE;
    int end = (offset + count - 1) / CACHE_PAGE_SIZE;
    for (int current = start; current <= end; current++) {
        if (bytes_read >= count) {
            break;
        }
        std::vector<char> &page = getPage(current);
        if (page.empty()) {
            break;
        }
        off_t page_offset;
        if (start == current) {
            page_offset = offset % CACHE_PAGE_SIZE;
        } else {
            page_offset = 0;
        }
        size_t bytes_copy = std::min(page.size() - page_offset, count - bytes_read);
        if (bytes_copy <= 0) break;
        std::copy_n(page.begin() + page_offset, bytes_copy, static_cast<char *>(buf) + bytes_read);
        bytes_read += bytes_copy;

    }
    offset += bytes_read;
    return bytes_read;
}


off_t SecondChanceCache::lseek(off_t off_set, int whence) {
    LARGE_INTEGER li;
    LARGE_INTEGER lp;
    if (whence == SEEK_SET) {
        offset = off_set;
    } else if (whence == SEEK_CUR) {
        offset += off_set;
    } else if (whence == SEEK_END) {
        if (GetFileSizeEx(hFile, &li) == 0) {
            throw std::runtime_error("Failed to get file size: " + std::to_string(GetLastError()));
        }
        offset = li.QuadPart + off_set;
    } else {
        throw std::invalid_argument("Invalid 'whence' value: " + std::to_string(whence));
    }
    return offset;
}


SecondChanceCache::SecondChanceCache(const char *path) : path(path) {
    offset = 0;
}

SecondChanceCache::~SecondChanceCache() {
    close();
}

