#ifndef TAPEINTERFACE_H
#define TAPEINTERFACE_H

#include <string>

class ITapeManager {
   protected:
    virtual void parseConfig() = 0;

    virtual void createTmpTapes() = 0;
    virtual void prepareTapes() = 0;
    virtual void closeTmp() = 0;

    virtual void emulateReadDelay() = 0;
    virtual void emulateWriteDelay() = 0;
    virtual void emulateSequentialReadDelay(int repeats) = 0;
    virtual void emulateSequentialWriteDelay(int repeats) = 0;
    virtual void emulateRewindDelay() = 0;

    virtual std::optional<int32_t> readFromTmpTape(int tape_idx) = 0;
    virtual std::optional<int32_t> readFromInputTape() = 0;
    virtual void writeToOutTape(int32_t value) = 0;
};

#endif  // TAPEINTERFACE_H
