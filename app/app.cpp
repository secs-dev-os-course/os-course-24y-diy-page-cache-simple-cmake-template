#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <random>
#include <ctime>
#include <sys/stat.h>
#include <fcntl.h>
#include "lab2.h"

constexpr size_t MEMORY_CHUNK_SIZE = 100000;
constexpr char INPUT_FILE[] = "input_f";

void createFileIfNotExists(const std::string& filename) {
    int f = lab2_open(filename.c_str());
    if (f != -1) {
        lab2_close(f);
        return;
    }
    
    int fd = creat(filename.c_str(), 0666);
    if (fd != -1) {
        close(fd);
    }
}

void generateRandomNumbersFile(const std::string& filename, size_t numElements, int minValue = 0, int maxValue = 1000000) {
    createFileIfNotExists(filename);
    int fd = lab2_open(filename.c_str());
    if (fd == -1) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::mt19937 gen(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dis(minValue, maxValue);

    for (size_t i = 0; i < numElements; ++i) {
        int randomNumber = dis(gen);
        std::string line = std::to_string(randomNumber) + "\n";
        lab2_write(fd, line.c_str(), line.size());
    }

    lab2_close(fd);
}

std::vector<int> readChunk(int fd, size_t chunkSize) {
    std::vector<int> chunk;
    std::string numberStr;
    char ch;
    ssize_t bytesRead;

    while (chunk.size() < chunkSize && (bytesRead = lab2_read(fd, &ch, 1)) > 0) {
        if (ch == 0) break;
        if (ch == '\n') {
            chunk.push_back(std::stoi(numberStr));\
            numberStr.clear();
        } else {
            numberStr += ch;
        }
    }

    if (!numberStr.empty()) {
        chunk.push_back(std::stoi(numberStr));
    }

    return chunk;
}

void writeChunk(const std::vector<int>& chunk, const std::string& filename) {
    createFileIfNotExists(filename);
    int fd = lab2_open(filename.c_str());
    if (fd == -1) {
        std::cerr << "Error creating file: " << filename << std::endl;
        return;
    }

    for (const auto& number : chunk) {
        std::string line = std::to_string(number) + "\n";
        lab2_write(fd, line.c_str(), line.size());
    }

    lab2_close(fd);
}

void mergeFiles(const std::string& file1, const std::string& file2, const std::string& outputFile) {
    
    createFileIfNotExists(file1.c_str());
    createFileIfNotExists(file2.c_str());
    createFileIfNotExists(outputFile.c_str());
    int inFd1 = lab2_open(file1.c_str());
    int inFd2 = lab2_open(file2.c_str());
    int outFd = lab2_open(outputFile.c_str());

    std::string numberStr1, numberStr2;
    char ch1 = 0, ch2 = 0;
    bool hasNum1 = false, hasNum2 = false;

    while (true) {
        if (!hasNum1 && lab2_read(inFd1, &ch1, 1) <= 0) break;
        if (!hasNum2 && lab2_read(inFd2, &ch2, 1) <= 0) break;
        if (ch1 == 0) break;
        if (ch2 == 0) break;

        if (ch1 == '\n') {
            hasNum1 = true;
        } else if (!hasNum1) {
            numberStr1 += ch1;
        } 

    
        if (!hasNum2 && ch2 == '\n') {
            hasNum2 = true;
        } else if (!hasNum2) {
            numberStr2 += ch2;
        }

        if (hasNum1 && hasNum2) {
            int num1 = std::stoi(numberStr1);
            int num2 = std::stoi(numberStr2);

            if (num1 <= num2) {
                lab2_write(outFd, (numberStr1 + "\n").c_str(), numberStr1.size() + 1);
                numberStr1.clear();
                hasNum1 = false;
            } else {
                lab2_write(outFd, (numberStr2 + "\n").c_str(), numberStr2.size() + 1);
                numberStr2.clear();
                hasNum2 = false;
            }
        }
    }


    if (hasNum1) {
       
        lab2_write(outFd, (numberStr1 + "\n").c_str(), numberStr1.size() + 1);
        numberStr1.clear();
        while (lab2_read(inFd1, &ch1, 1) > 0) {
            if (ch1 == '\n' || lab2_read(inFd1, &ch1, 1) <= 0 || ch1 == 0) {
                if (ch1 == 0) break;
               
                lab2_write(outFd, (numberStr1 + "\n").c_str(), numberStr1.size() + 1);
                numberStr1.clear();
            } else {
                numberStr1 += ch1;
            }
        }
    }

    if (hasNum2) {
      
        lab2_write(outFd, (numberStr2 + "\n").c_str(), numberStr2.size() + 1);
        numberStr2.clear();
        while (lab2_read(inFd2, &ch2, 1) > 0) {
            if (ch2 == '\n' || lab2_read(inFd2, &ch2, 1) <= 0|| ch2 == 0) {
                if (ch2 == 0) break;
               
                lab2_write(outFd, (numberStr2 + "\n").c_str(), numberStr2.size() + 1);
                numberStr2.clear();
            } else {
                numberStr2 += ch2;
            }
        }
    }

    lab2_close(inFd1);
    lab2_close(inFd2);
    lab2_close(outFd);
}

void deleteFiles(const std::vector<std::string>& files) {
    for (const auto& file : files) {
        std::remove(file.c_str());
    }
}

void externalMergeSort(size_t numElements, const std::string& outputFile) {
    std::string inputFile = INPUT_FILE + outputFile;
    generateRandomNumbersFile(inputFile, numElements);

    int inFd = lab2_open(inputFile.c_str());
    std::vector<std::string> sortedChunkFiles;

    size_t chunkIndex = 0;
    while (true) {
        auto chunk = readChunk(inFd, MEMORY_CHUNK_SIZE);
        if (chunk.empty()) break;

        std::sort(chunk.begin(), chunk.end());
        std::string chunkFile = "chunk_" + std::to_string(chunkIndex++) + outputFile + ".txt";
        writeChunk(chunk, chunkFile);
        sortedChunkFiles.push_back(chunkFile);
    }

    lab2_close(inFd);
    size_t cnter = 0;
    while (sortedChunkFiles.size() > 1) {
        std::vector<std::string> newSortedChunkFiles;
        std::vector<std::string> deleted;

        for (size_t i = 0; i < sortedChunkFiles.size(); i += 2, cnter+=2) {
            if (i + 1 < sortedChunkFiles.size()) {
                std::string mergedFile = "merged_" + std::to_string(cnter / 2) + outputFile + ".txt";
                mergeFiles(sortedChunkFiles[i], sortedChunkFiles[i + 1], mergedFile);
                newSortedChunkFiles.push_back(mergedFile);
                deleted.push_back(sortedChunkFiles[i]);
                deleted.push_back(sortedChunkFiles[i + 1]);
            } else {
                newSortedChunkFiles.push_back(sortedChunkFiles[i]);
            }
        }
        
        deleteFiles(deleted);
        sortedChunkFiles = newSortedChunkFiles;
    }

    if (!sortedChunkFiles.empty()) {
        std::rename(sortedChunkFiles.front().c_str(), outputFile.c_str());
        std::remove(sortedChunkFiles.front().c_str());
    }
    std::remove(inputFile.c_str());
}

int main() {
    size_t numElements = 1000000;
    std::string outputFile = "output.txt";
    externalMergeSort(numElements, outputFile);
    return 0;
}