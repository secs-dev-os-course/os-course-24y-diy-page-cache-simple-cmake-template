#include "cache.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

#define ENABLE_PREFETCH 
Cache::Cache(const char* path) {
    file_handle = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
        NULL
    );

    if (file_handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open file");
    }

    update_file_size();
    current_position = 0;
}

void Cache::prefetch_blocks(size_t start_block, size_t block_count) {
    std::cout << "[Prefetch] Starting prefetch from block " << start_block 
              << " for " << block_count << " blocks" << std::endl;

    for (size_t i = 0; i < block_count; ++i) {
        size_t block_number = start_block + i;

        if (cache_map.find(block_number) != cache_map.end()) {
            std::cout << "[Prefetch] Block " << block_number << " already in cache, skipping" << std::endl;
            continue;
        }

        if (lru_list.size() >= MAX_CACHE_SIZE) {
            std::cout << "[Prefetch] Evicting block due to cache size limit" << std::endl;
            evict_block();
        }

        LARGE_INTEGER offset;
        offset.QuadPart = block_number * BLOCK_SIZE;

        lru_list.emplace_front(block_number);
        auto block_it = lru_list.begin();

        SetFilePointerEx(file_handle, offset, NULL, FILE_BEGIN);
        DWORD bytes_read;
        if (!ReadFile(file_handle, block_it->data->data(), BLOCK_SIZE, &bytes_read, NULL)) {
            std::cerr << "[Prefetch] Failed to read block " << block_number << std::endl;
            break;
        }

        cache_map[block_number] = block_it;
        std::cout << "[Prefetch] Block " << block_number << " loaded into cache" << std::endl;
    }
    std::cout << "[Prefetch] Prefetch completed" << std::endl;
}


void Cache::update_file_size() {
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file_handle, &size)) {
        throw std::runtime_error("Failed to get file size");
    }
    file_size = size.QuadPart;
}

Cache::~Cache() {
    sync();
    CloseHandle(file_handle);
}

ssize_t Cache::read(void* buf, size_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    ssize_t result = read_internal(buf, count, current_position);
    if (result > 0) {
        current_position += result;
    }
    return result;
}

ssize_t Cache::write(const void* buf, size_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    ssize_t result = write_internal(buf, count, current_position);
    if (result > 0) {
        current_position += result;
        if (current_position > file_size) {
            file_size = current_position;
        }
    }
    return result;
}

ssize_t Cache::read_internal(void* buf, size_t count, off_t offset) {
    if (offset >= file_size) {
        return 0;
    }

    size_t total_read = 0;
    char* buffer = static_cast<char*>(buf);

    while (total_read < count && (offset + total_read) < file_size) {
        size_t block_number = (offset + total_read) / BLOCK_SIZE;
        size_t block_offset = (offset + total_read) % BLOCK_SIZE;
        auto block_it = cache_map.find(block_number);
        bool block_in_cache = (block_it != cache_map.end());

        #ifdef ENABLE_PREFETCH
            if (!block_in_cache) {
                prefetch_blocks(block_number, 3);
            }
        #endif

    
        auto cache_block = get_block(block_number);
        #ifdef ENABLE_PREFETCH
        std::cout << "[Read] Block " << block_number 
                  << (block_in_cache ? " found in cache" : " loaded from disk") 
                  << std::endl;
        #endif
        size_t remaining = count - total_read;
        size_t can_read = std::min(remaining, BLOCK_SIZE - block_offset);
        can_read = std::min(can_read, static_cast<size_t>(file_size - (offset + total_read)));

        memcpy(buffer + total_read, cache_block->data->data() + block_offset, can_read);
        total_read += can_read;
    }

    return total_read;
}

ssize_t Cache::write_internal(const void* buf, size_t count, off_t offset) {
    size_t total_written = 0;
    const char* buffer = static_cast<const char*>(buf);

    while (total_written < count) {
        size_t block_number = (offset + total_written) / BLOCK_SIZE;
        size_t block_offset = (offset + total_written) % BLOCK_SIZE;
        
        auto block_it = get_block(block_number);
        size_t remaining = count - total_written;
        size_t can_write = std::min(remaining, BLOCK_SIZE - block_offset);
        
        memcpy(block_it->data->data() + block_offset, buffer + total_written, can_write);
        block_it->dirty = true;
        total_written += can_write;
    }

    return total_written;
}

std::list<Cache::CacheBlock>::iterator Cache::get_block(size_t block_number) {
    auto it = cache_map.find(block_number);
    if (it != cache_map.end()) {
        auto block_it = it->second;
        lru_list.splice(lru_list.begin(), lru_list, block_it);
        return lru_list.begin();
    }

    if (lru_list.size() >= MAX_CACHE_SIZE) {
        evict_block();
    }

    LARGE_INTEGER offset;
    offset.QuadPart = block_number * BLOCK_SIZE;
    
    lru_list.emplace_front(block_number);
    auto block_it = lru_list.begin();
    
    SetFilePointerEx(file_handle, offset, NULL, FILE_BEGIN);
    DWORD bytes_read;
    ReadFile(file_handle, block_it->data->data(), BLOCK_SIZE, &bytes_read, NULL);
    
    cache_map[block_number] = block_it;
    return block_it;
}

void Cache::evict_block() {
    auto& block = lru_list.back();
    if (block.dirty) {
        LARGE_INTEGER offset;
        offset.QuadPart = block.block_number * BLOCK_SIZE;
        
        SetFilePointerEx(file_handle, offset, NULL, FILE_BEGIN);
        DWORD bytes_written;
        WriteFile(file_handle, block.data->data(), BLOCK_SIZE, &bytes_written, NULL);
    }
    
    cache_map.erase(block.block_number);
    lru_list.pop_back();
}

int Cache::sync() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    for (auto& block : lru_list) {
        if (block.dirty) {
            LARGE_INTEGER offset;
            offset.QuadPart = block.block_number * BLOCK_SIZE;
            
            SetFilePointerEx(file_handle, offset, NULL, FILE_BEGIN);
            DWORD bytes_written;
            WriteFile(file_handle, block.data->data(), BLOCK_SIZE, &bytes_written, NULL);
            block.dirty = false;
        }
    }
    return 0;
}

off_t Cache::seek(off_t offset, int whence) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    off_t new_position;
    switch (whence) {
        case SEEK_SET:
            new_position = offset;
            break;
        case SEEK_CUR:
            new_position = current_position + offset;
            break;
        case SEEK_END:
            new_position = file_size + offset;
            break;
        default:
            return -1;
    }

    if (new_position < 0) {
        return -1;
    }

    current_position = new_position;
    return current_position;
}