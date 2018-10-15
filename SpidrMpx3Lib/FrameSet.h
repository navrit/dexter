#ifndef FRAMESET_H
#define FRAMESET_H

#include "ChipFrame.h"

const static int number_of_chips = 4;

class FrameSet
{
public:
    FrameSet();
    ~FrameSet();
    void clear();
    void putChipFrame(int chipIndex, ChipFrame * cf);
    bool isComplete();
    bool isEmpty();
    void copyTo32(int chipIndex, uint32_t *dest);
    void copyTo32(uint32_t *dest);
    int pixelsLost();

private:
    ChipFrame* frame[2][number_of_chips];

};

#endif // FRAMESET_H
