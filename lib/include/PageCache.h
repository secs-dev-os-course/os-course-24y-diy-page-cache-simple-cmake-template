#ifndef PAGE_CACHE
#define PAGE_CACHE

#include <chrono>
#include <memory>
#include <optional>
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
    std::chrono::time_point<std::chrono::steady_clock> access_time;
    CacheBlock(int fd = 0, int req_am = 0, int page_idx = 0, char* cached_page = nullptr)
        : fd(fd),
          req_am(req_am),
          page_idx(page_idx),
          cached_page(cached_page),
          access_time(std::chrono::steady_clock::now()) {};
    ~CacheBlock();
} CacheBlock;

struct PairHasher {
   public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& x) const;
};

typedef std::unordered_map<std::pair<int, int>, std::shared_ptr<CacheBlock>, PairHasher>
    cache_map;  // key = <fd, page num>

class PageCache {
    size_t max_cache_pages_am;

    cache_map cache_blocks;

    bool (*lfuComparator)(std::shared_ptr<CacheBlock> cb1, std::shared_ptr<CacheBlock> cb2) =
        [](std::shared_ptr<CacheBlock> cb1, std::shared_ptr<CacheBlock> cb2) {
            if (cb1.get()->req_am == cb2.get()->req_am) {
                return cb1.get()->access_time < cb2.get()->access_time;
            }
            return cb1.get()->req_am < cb2.get()->req_am;
        };

    std::set<std::shared_ptr<CacheBlock>, decltype(lfuComparator)> priority_queue;

    void syncBlock(int fd, int page_idx);
    void displaceBlock();
    void rmPage(int fd, int page_idx);
    void access(int fd, int page_idx);

   public:
    PageCache(size_t cache_size_bytes);
    void syncBlocks(int fd);
    void cachePage(int fd, int page_idx);
    bool pageExist(int fd, int page_idx);
    std::optional<std::shared_ptr<CacheBlock>> getCached(int fd, int page_idx);

    void clearFileCaches(int fd);
};

#endif