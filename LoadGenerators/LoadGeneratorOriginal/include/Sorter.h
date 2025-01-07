#ifndef SORTER_H
#define SORTER_H

#include <TapeManager.h>

#include <cstdint>
#include <fstream>
#include <vector>

#include "MinHeap.h"

class Sorter {

private:
	TapeManager tapeManager;

	size_t ram_bytes;

	std::string in_filename;
	std::string out_filename;
	std::string temp_dir_name;

	MinHeap heap;

	void checkEnoughRam();

public:
	Sorter(std::string in_file, std::string out_file, size_t ram_bytes);
	void sortTapes();
};

#endif //SORTER_H
