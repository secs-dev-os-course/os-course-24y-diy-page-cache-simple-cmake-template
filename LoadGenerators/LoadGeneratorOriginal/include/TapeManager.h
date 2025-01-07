#ifndef TAPEMANAGER_H
#define TAPEMANAGER_H

#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "ITapeManager.h"

#define TMP_DIR_NAME "tmp"
#define CONFIG_FILE_NAME "config"

class TapeManager : public ITapeManager {
   private:
    std::string input_tape_name;
    std::string out_tape_name;

    size_t ram_bytes;

    std::ofstream out;
    std::ifstream in;

    std::vector<std::ifstream> tmp_tapes;

    void clearTmpDir(std::string dir_name);
    void createTmpDir();

    void writeChunk(int32_t chunk[], int32_t elems_in_chunk, size_t tape_number);

    int read_delay = -1;
    int write_delay = -1;
    int move_delay = -1;
    int rewind_delay = -1;

    void parseConfig() override;

   public:
    void createTmpTapes() override;
    void prepareTapes() override;
    void closeTmp() override;

    int getTempTapesAmount();

    TapeManager(std::string input_tape_name, std::string out_tape_name, size_t ram_bytes);

    void emulateReadDelay() override;
    void emulateWriteDelay() override;
    void emulateRewindDelay() override;
    void emulateSequentialReadDelay(int repeats) override;
    void emulateSequentialWriteDelay(int repeats) override;

    std::optional<int32_t> readFromTmpTape(int tape_idx) override;
    std::optional<int32_t> readFromInputTape() override;
    void writeToOutTape(int32_t value) override;

    ~TapeManager();
};

#endif  // TAPEMANAGER_H
