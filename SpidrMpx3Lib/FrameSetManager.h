#ifndef FRAMESETMANAGER_H
#define FRAMESETMANAGER_H

#include <stdint.h>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "FrameSet.h"

#define FSM_SIZE 8192
#define FSM_MASK (FSM_SIZE-1)
class FrameSetManager
{
public:
    FrameSetManager();

    void clear();
    void setBothCounters(bool b) { this->bothCounters = b; }
    void putChipFrame(int chipIndex, ChipFrame* cf, uint8_t frameId);
    void putChipFrameB(int chipIndex, ChipFrame* cf);
    ChipFrame *newChipFrame(int chipIndex, OMR omr);
    bool isFull();
    bool isEmpty();
    int available();
    bool wait(unsigned long timeout_ms);
    FrameSet *getFrameSet();
    void releaseFrameSet(FrameSet *);

    int chipMask = 0;

    // Statistics
    int     _framesReceived = 0;
    int     _framesLost = 0;

private:
    FrameSet fs[FSM_SIZE];
    bool bothCounters = false;
    int dropped = 0;
    uint8_t frameId;
    bool expectCounterH = false;
    // for diagnostics: 0=unused, 1=draft, 2=published, 3=reading
    int headState = 0;
    std::atomic_uint head_{0};
    std::atomic_uint tail_{0};
    std::condition_variable _frameAvailableCondition;
    std::mutex headMut, tailMut;

    void publish();
};

#endif // FRAMESETMANAGER_H
