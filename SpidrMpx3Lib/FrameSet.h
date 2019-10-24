#ifndef FRAMESET_H
#define FRAMESET_H

#include "ChipFrame.h"
#include "configs.h"

class FrameSet {

public:
    FrameSet();
    ~FrameSet();
    void clear();
    void setBothCounters(bool b);
    bool hasBothCounters() { return counters == 2; }
    ChipFrame* takeChipFrame(int chipIndex, bool counterH);
    void putChipFrame(int chipIndex, ChipFrame * cf);
    bool isComplete(int chipMask);
    void copyTo32(int chipIndex, bool counterH, uint32_t *dest);
    int pixelsLost();

private:
    int counters = 1;
    ChipFrame* frame[2][Config::number_of_chips];

};

#endif // FRAMESET_H
