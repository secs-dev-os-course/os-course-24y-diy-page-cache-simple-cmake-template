#include "TapeManager.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace fs = std::filesystem;

TapeManager::TapeManager(std::string input_tape_name, std::string out_tape_name, size_t ram_bytes) {
    this->input_tape_name = input_tape_name;
    this->out_tape_name = out_tape_name;

    this->ram_bytes = ram_bytes;

    parseConfig();

    std::ifstream in_tape(input_tape_name);
    if (!in_tape.is_open()) {
        closeTmp();
        throw std::runtime_error("Error opening input file!");
    }
    this->in = std::move(in_tape);

    std::ofstream out_tape(out_tape_name);
    if (!out_tape.is_open()) {
        closeTmp();
        throw std::runtime_error( "Error opening output file!");
    }
    this->out = std::move(out_tape);
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::istringstream stream(str);
    std::string token;

    while (std::getline(stream, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}

void TapeManager::parseConfig() {
    std::ifstream config_file(CONFIG_FILE_NAME);
    if (!config_file.is_open()) {
        std::ostringstream oss;
        oss << "Error reading config file: \'" << CONFIG_FILE_NAME << "\'.";
        throw std::runtime_error(oss.str());
    }

    std::string line;
    while (std::getline(config_file, line)) {
        std::vector<std::string> tokens = split(line, ':');
        if (tokens.size() != 2) {
            std::ostringstream oss;
            oss << "Wrong config file: \'" << CONFIG_FILE_NAME
                << "\'\nSyntax: <parameter>:<value> (in nanoseconds, new line for each param).";
            throw std::runtime_error(oss.str());
        }

        if (tokens[0] == "read_delay")
            read_delay = std::stoi(tokens[1]);
        if (tokens[0] == "write_delay")
            write_delay = std::stoi(tokens[1]);
        if (tokens[0] == "move_delay")
            move_delay = std::stoi(tokens[1]);
        if (tokens[0] == "rewind_delay")
            rewind_delay = std::stoi(tokens[1]);
    }
    if (read_delay < 0 || write_delay < 0 || move_delay < 0 || rewind_delay < 0) {
        std::ostringstream oss;
        oss << "Wrong config file: \'" << CONFIG_FILE_NAME
            << "\'\nYou should pass `read_delay`, `write_delay`, `move_delay`, `rewind_delay`"
               " parameters in config file. \nSyntax: <parameter>:<value> (in nanoseconds, new line for each param).";
        throw std::runtime_error(oss.str());
    }
}

void TapeManager::emulateReadDelay() {
    std::this_thread::sleep_for(std::chrono::nanoseconds(move_delay));
    std::this_thread::sleep_for(std::chrono::nanoseconds(read_delay));
}

void TapeManager::emulateWriteDelay() {
    std::this_thread::sleep_for(std::chrono::nanoseconds(move_delay));
    std::this_thread::sleep_for(std::chrono::nanoseconds(write_delay));
}

void TapeManager::emulateSequentialReadDelay(int repeats) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(repeats * move_delay));
    std::this_thread::sleep_for(std::chrono::nanoseconds(repeats * read_delay));
}

void TapeManager::emulateSequentialWriteDelay(int repeats) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(repeats * move_delay));
    std::this_thread::sleep_for(std::chrono::nanoseconds(repeats * write_delay));
}

void TapeManager::emulateRewindDelay() {
    std::this_thread::sleep_for(std::chrono::nanoseconds(rewind_delay));
}

void TapeManager::closeTmp() {
    for (auto& tape : tmp_tapes) {
        tape.close();
    }
}

void TapeManager::prepareTapes() {
    for (const auto& entry : fs::directory_iterator(TMP_DIR_NAME)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            std::ifstream tmp_tape(fs::path(TMP_DIR_NAME) / filename);

            if (!tmp_tape.is_open()) {
                closeTmp();
                throw std::runtime_error("Error opening tmp tape!");
            }

            tmp_tapes.push_back(std::move(tmp_tape));
            emulateRewindDelay();
        }
    }
}

void TapeManager::clearTmpDir(std::string dir_name) {
    try {
        if (fs::exists(dir_name) && fs::is_directory(dir_name)) {
            for (const auto& entry : fs::directory_iterator(dir_name)) {
                const auto& path = entry.path();

                if (fs::is_directory(path))
                    clearTmpDir(path);

                fs::remove(path);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::ostringstream oss;
        oss << "Error while clearing temp directory: " << TMP_DIR_NAME << ".";
        throw std::runtime_error(oss.str());
    }
}

void TapeManager::createTmpDir() {
    std::filesystem::path dir = TMP_DIR_NAME;

    if (!std::filesystem::exists(dir)) {
        if (!std::filesystem::create_directory(dir)) {
            std::ostringstream oss;
            oss << "Failed to create directory \'" << TMP_DIR_NAME << "\'.";
            throw std::runtime_error(oss.str());
        }
    } else {
        clearTmpDir(TMP_DIR_NAME);
    }
}

int TapeManager::getTempTapesAmount() {
    return tmp_tapes.size();
}

void TapeManager::writeChunk(int32_t chunk[], int32_t elems_in_chunk, size_t tape_number) {
    std::sort(chunk, chunk + elems_in_chunk);
    std::ofstream tmp_tape(fs::path(TMP_DIR_NAME) / ("tape_" + std::to_string(tape_number)));

    if (!tmp_tape.is_open()) {
        throw std::runtime_error("Error opening tmp tape!");
    }

    for (size_t j = 0; j < elems_in_chunk; j++) {
        tmp_tape << chunk[j] << std::endl;
    }
    emulateSequentialWriteDelay(elems_in_chunk);
}

void TapeManager::createTmpTapes() {
    std::ifstream input_tape_file(input_tape_name);
    if (!input_tape_file.is_open()) {
        throw std::runtime_error("Error opening input tape!");
    }

    createTmpDir();

    int32_t chunck_size = ram_bytes / sizeof(int32_t);

    size_t idx = 0;
    int32_t chunk[chunck_size];
    int32_t elems_in_chunk = 0;
    int32_t tmp_tapes_count = 0;

    std::optional<int32_t> number = readFromInputTape();
    while (number.has_value()) {
        emulateReadDelay();

        chunk[idx] = number.value();
        elems_in_chunk++;
        idx++;

        if (idx >= chunck_size) {
            writeChunk(chunk, elems_in_chunk, tmp_tapes_count);

            idx = 0;
            elems_in_chunk = 0;
            tmp_tapes_count++;
        }

        number = readFromInputTape();
    }
    if (elems_in_chunk)
        writeChunk(chunk, elems_in_chunk, tmp_tapes_count);
    input_tape_file.close();
}

std::optional<int32_t> TapeManager::readFromTmpTape(int tape_idx) {
    emulateReadDelay();

    int32_t number;
    if (tmp_tapes[tape_idx] >> number) {
        return std::make_optional(number);
    }
    return std::nullopt;
}

void TapeManager::writeToOutTape(int32_t value) {
    emulateWriteDelay();

    out << value << std::endl;
}

std::optional<int32_t> TapeManager::readFromInputTape() {
    emulateReadDelay();

    int32_t value;
    if(in >> value) {
        return std::make_optional(value);
    }
    return std::nullopt;
}

TapeManager::~TapeManager() {
    closeTmp();
    clearTmpDir(TMP_DIR_NAME);
}
