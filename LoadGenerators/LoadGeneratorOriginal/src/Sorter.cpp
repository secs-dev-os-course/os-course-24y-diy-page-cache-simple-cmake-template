#include "Sorter.h"

#include <TapeManager.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

Sorter::Sorter(std::string in_file, std::string out_file, size_t ram_bytes)
    : tapeManager(in_file, out_file, ram_bytes),
      heap(ram_bytes / sizeof(HeapNode)),
      temp_dir_name(TMP_DIR_NAME) {
    this->ram_bytes = ram_bytes;
}

void Sorter::checkEnoughRam() {
    if (tapeManager.getTempTapesAmount() * sizeof(HeapNode) > ram_bytes) {
        throw std::runtime_error("Too low memory! Sorting is impossible.");
    }
}

void Sorter::sortTapes() {
    tapeManager.createTmpTapes();
    tapeManager.prepareTapes();

    checkEnoughRam();

    for (uint16_t i = 0; i < tapeManager.getTempTapesAmount(); i++) {
        std::optional<int32_t> num = tapeManager.readFromTmpTape(i);
        heap.insert(HeapNode{.number = num.value(), .file_idx = i});
    }

    while (true) {
        std::optional<HeapNode> min = heap.extractMin();
        if (!min.has_value())
            break;

        tapeManager.writeToOutTape(min->number);

        std::optional<int32_t> num = tapeManager.readFromTmpTape(min->file_idx);

        if (num.has_value()) {
            heap.insert(HeapNode{.number = num.value(), .file_idx = min->file_idx});
        }
    }
}
