#ifndef PAGE_CACHE
#define PAGE_CACHE

#include <chrono>
#include <queue>
#include <unordered_map>

#define CACHE_SIZE 256 * 1024 * 1024  // Bytes
#define PAGE_SIZE 4000

typedef struct CacheBlock {
    int fd;
    int req_am;
    int page_number;
    char* page;
    std::chrono::time_point<std::chrono::steady_clock> access_time;
} CacheBlock;

class CacheBlockFreqComparator {
   public:
    bool operator()(CacheBlock cb1, CacheBlock cb2);
};

class CacheBlockTimeComparator {
   public:
    bool operator()(CacheBlock cb1, CacheBlock cb2);
};

struct PairHasher {
   public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& x) const;
};

class PageCache {
    size_t max_cache_bytes_am;

    std::unordered_map<std::pair<int, int>, CacheBlock, PairHasher> cache_blocks;

    std::priority_queue<CacheBlock, std::vector<CacheBlock>, CacheBlockFreqComparator> freq_pq;
    std::priority_queue<CacheBlock, std::vector<CacheBlock>, CacheBlockTimeComparator> times_pq;

   public:
    PageCache(size_t cache_size_bytes = CACHE_SIZE) : max_cache_bytes_am(cache_size_bytes) {};
    void insertPage(int fd, char* page);
	
};

#endif