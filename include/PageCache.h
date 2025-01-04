#ifndef PAGE_CACHE
#define PAGE_CACHE

#include <chrono>
#include <queue>
#include <set>
#include <unordered_map>

#define CACHE_SIZE 256 * 1024 * 1024  // Bytes
#define PAGE_SIZE 4096

typedef struct CacheBlock {
    int fd;
    int req_am = 0;
    int page_idx;
    char* cached_page;
    std::chrono::time_point<std::chrono::steady_clock> access_time;

    ~CacheBlock();
} CacheBlock;

struct PairHasher {
   public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& x) const;
};

typedef std::unordered_map<std::pair<int, int>, CacheBlock, PairHasher> cache_map;

class PageCache {
    size_t max_cache_bytes_am;

    cache_map cache_blocks;  // key = <fd, page num>

    bool (*freqComp)(CacheBlock cb1, CacheBlock cb2) = [](CacheBlock cb1, CacheBlock cb2) {
        return cb1.req_am < cb2.req_am;
    };
    bool (*timeComp)(CacheBlock cb1, CacheBlock cb2) = [](CacheBlock cb1, CacheBlock cb2) {
        return cb1.access_time < cb2.access_time;
    };

    std::set<CacheBlock, decltype(freqComp)> freq_pq;
    std::set<CacheBlock, decltype(timeComp)> times_pq;

    void access(int fd, int page_num);

   public:
    PageCache(size_t cache_size_bytes = CACHE_SIZE) : max_cache_bytes_am(cache_size_bytes) {};
    cache_map* getAllCached();
    void cachePage(int fd, int page_num);
    bool pageExist(int fd, int page_num);
    CacheBlock getCached(int fd, int page_num);

    void clearFileCaches(int fd);
};

#endif