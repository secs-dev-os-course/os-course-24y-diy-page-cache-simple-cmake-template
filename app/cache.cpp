#include "cache.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

cache_t cache = { .clock_hand = 0, .fd = -1 };

void init_cache() {
    for (int i = 0; i < CACHE_PAGES; i++) {
        posix_memalign(&cache.pages[i].data, PAGE_SIZE, PAGE_SIZE);
        cache.pages[i].offset = -1;
        cache.pages[i].reference_bit = 0;
        cache.pages[i].dirty = 0;
    }
}

void flush_page(page_t *page) {
    if (page->dirty && page->offset != -1) {
        lseek(cache.fd, page->offset, SEEK_SET);
        write(cache.fd, page->data, PAGE_SIZE);
        page->dirty = 0;
    }
}

void load_page(page_t *page, off_t offset) {
    flush_page(page);
    lseek(cache.fd, offset, SEEK_SET);
    read(cache.fd, page->data, PAGE_SIZE);
    page->offset = offset;
    page->reference_bit = 1;
    page->dirty = 0;
}

page_t *find_or_evict_page(off_t offset) {
    for (int i = 0; i < CACHE_PAGES; i++) {
        if (cache.pages[i].offset == offset) {
            cache.pages[i].reference_bit = 1;
            return &cache.pages[i];
        }
    }

    while (1) {
        page_t *page = &cache.pages[cache.clock_hand];
        if (page->reference_bit == 0) {
            load_page(page, offset);
            cache.clock_hand = (cache.clock_hand + 1) % CACHE_PAGES;
            return page;
        } else {
            page->reference_bit = 0;
            cache.clock_hand = (cache.clock_hand + 1) % CACHE_PAGES;
        }
    }
}
