#ifndef MINHEAP_H
#define MINHEAP_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <ostream>

struct HeapNode {
    int32_t number;
    uint16_t file_idx;
};

class MinHeap {
private:
    HeapNode* heap;
    size_t heap_size;
    size_t capacity;

    void heapify(size_t i);

public:
    MinHeap(size_t heap_size);

    void insert(HeapNode node);
    std::optional<HeapNode> extractMin();
    std::optional<HeapNode> getMin();

    ~MinHeap();
};

#endif //MINHEAP_H
