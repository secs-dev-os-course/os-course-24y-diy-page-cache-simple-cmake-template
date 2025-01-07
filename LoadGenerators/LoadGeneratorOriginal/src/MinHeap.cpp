#include "MinHeap.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>

MinHeap::MinHeap(size_t capacity) {
    heap = new HeapNode[capacity];
    heap_size = 0;
    this->capacity = capacity;
}

void MinHeap::heapify(size_t i) {
    size_t min = i;
    size_t left = 2 * i + 1;
    size_t right = 2 * i + 2;

    if (left < heap_size && heap[left].number < heap[min].number) {
        min = left;
    }

    if (right < heap_size && heap[right].number < heap[min].number) {
        min = right;
    }

    if (min != i) {
        std::swap(heap[i], heap[min]);
        heapify(min);
    }
}

void MinHeap::insert(HeapNode node) {
    if (heap_size == capacity) {
        throw std::overflow_error("Heap overflow: Cannot insert, heap is full"); // TODO обработать
    }

    heap[heap_size] = node;
    heap_size++;

    size_t i = heap_size - 1;
    while (i != 0 && heap[(i - 1) / 2].number > heap[i].number) {
        std::swap(heap[i], heap[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

std::optional<HeapNode> MinHeap::extractMin() {
    // for (auto i = 0; i < heap_size; i++) {
    //     HeapNode node =  heap[i];
    //     std::cout << node << " ";
    // }
    if (heap_size <= 0) {
        return std::nullopt;
    }

    if (heap_size == 1) {
        heap_size--;
        return heap[0];
    }

    HeapNode min = heap[0];

    heap[0] = heap[heap_size - 1];
    heap_size--;

    heapify(0);

    return min;
}

std::optional<HeapNode> MinHeap::getMin() {
    if (heap_size <= 0) {
        return std::nullopt;
    }
    return heap[0];
}

MinHeap::~MinHeap() {
    delete[] heap;
}
