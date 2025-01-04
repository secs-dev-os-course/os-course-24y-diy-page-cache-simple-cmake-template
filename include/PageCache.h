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
    int req_am;
    int page_idx;
    char* cached_page;
    bool is_modified;
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
    size_t max_cache_pages_am;

    cache_map cache_blocks;  // key = <fd, page num>

    bool (*lfuComparator)(CacheBlock cb1, CacheBlock cb2) = [](CacheBlock cb1, CacheBlock cb2) {
        if (cb1.req_am == cb2.req_am) {
            return cb1.access_time < cb2.access_time;
        }
        return cb1.req_am < cb2.req_am;
    };

    std::set<CacheBlock, decltype(lfuComparator)> priority_queue;

    void syncBlock(int fd, int page_idx);
    void displaceBlock();
    void access(int fd, int page_idx);

   public:
    PageCache(size_t cache_size_bytes);
    void syncBlocks(int fd);
    void cachePage(int fd, int page_idx);
    bool pageExist(int fd, int page_idx);
    CacheBlock getCached(int fd, int page_idx);

    void clearFileCaches(int fd);
};

#endif