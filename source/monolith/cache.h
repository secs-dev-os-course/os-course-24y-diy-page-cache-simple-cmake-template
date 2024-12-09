#pragma once
#include <cstddef>
#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>
#include <windows.h>

class Cache {
private:
    static const size_t BLOCK_SIZE = 4096;
    static const size_t MAX_CACHE_SIZE = 1024;
    static_assert((BLOCK_SIZE & (BLOCK_SIZE - 1)) == 0, "BLOCK_SIZE must be power of 2");

    // Класс для выровненного буфера
    class AlignedBlock {
    public:
        AlignedBlock() : data_(nullptr) {
            data_ = static_cast<char*>(_aligned_malloc(BLOCK_SIZE, BLOCK_SIZE));
            if (!data_) throw std::bad_alloc();
        }
        
        ~AlignedBlock() {
            if (data_) _aligned_free(data_);
        }
        
        char* data() { return data_; }
        const char* data() const { return data_; }
        
        AlignedBlock(const AlignedBlock&) = delete;
        AlignedBlock& operator=(const AlignedBlock&) = delete;
        
    private:
        char* data_;
    };

    struct CacheBlock {
        std::unique_ptr<AlignedBlock> data;
        size_t block_number;
        bool dirty;
        
        CacheBlock(size_t block_num) : 
            data(std::make_unique<AlignedBlock>()), 
            block_number(block_num),
            dirty(false) {}
    };

    std::unordered_map<size_t, std::list<CacheBlock>::iterator> cache_map;
    std::list<CacheBlock> lru_list;
    HANDLE file_handle;
    std::mutex cache_mutex;
    off_t file_size;
    off_t current_position;
    
public:
    Cache(const char* path);
    ~Cache();
    
    ssize_t read(void* buf, size_t count);
    ssize_t write(const void* buf, size_t count);
    int sync();
    off_t seek(off_t offset, int whence);
    
private:
    void prefetch_blocks(size_t start_block, size_t block_count);
    void evict_block();
    std::list<CacheBlock>::iterator get_block(size_t block_number);
    ssize_t read_internal(void* buf, size_t count, off_t offset);
    ssize_t write_internal(const void* buf, size_t count, off_t offset);
    void update_file_size();
};