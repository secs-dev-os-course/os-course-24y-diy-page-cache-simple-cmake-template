#ifndef CACHE_H
#define CACHE_H

#include <sys/types.h>

#define PAGE_SIZE 4096
#define CACHE_PAGES 64

typedef struct {
    void *data;
    off_t offset;
    int reference_bit;
    int dirty;
} page_t;

typedef struct {
    page_t pages[CACHE_PAGES];
    size_t clock_hand;
    int fd;
} cache_t;

extern cache_t cache;

void init_cache();
void flush_page(page_t *page);
void load_page(page_t *page, off_t offset);
page_t *find_or_evict_page(off_t offset);

#endif
