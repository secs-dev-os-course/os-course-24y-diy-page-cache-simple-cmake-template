#include <chrono>
#include <queue>
#include <unordered_map>

#define CACHE_SIZE 256 * 1024 * 1024  // Bytes

typedef struct CacheBlock {
    int fd;
    int req_am;
    int page_number;
    char* page;
    std::chrono::time_point<std::chrono::steady_clock> access_time;
} CacheBlock;

class CacheBlockFreqComparator {
   public:
    bool operator()(CacheBlock cb1, CacheBlock cb2) {
        return cb1.req_am < cb2.req_am;
    }
};

class CacheBlockTimeComparator {
   public:
    bool operator()(CacheBlock cb1, CacheBlock cb2) {
        return cb1.access_time < cb2.access_time;
    }
};

struct PairHasher {
   public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& x) const {
        return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
};

class PageCache {
    size_t max_cache_bytes_am;

    std::unordered_map<std::pair<int, int>, CacheBlock, PairHasher> cache_blocks;

    std::priority_queue<CacheBlock, std::vector<CacheBlock>, CacheBlockFreqComparator> freq_pq;
    std::priority_queue<CacheBlock, std::vector<CacheBlock>, CacheBlockTimeComparator> times_pq;

   public:
    PageCache(size_t cache_size_bytes = CACHE_SIZE) : max_cache_bytes_am(cache_size_bytes) {
    }
};