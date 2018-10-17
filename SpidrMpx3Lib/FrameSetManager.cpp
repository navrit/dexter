#include "FrameSetManager.h"

#include <assert.h>
#include <iostream>
#include <chrono>

FrameSetManager::FrameSetManager()
{

}

bool FrameSetManager::isFull() {
    return head_ - tail_ == FSM_SIZE-1;
}

bool FrameSetManager::isEmpty() {
    return head_ == tail_;
}

bool FrameSetManager::wait(unsigned long timeout_ms) {
    std::unique_lock<std::mutex> lock(tailMut);
    auto duration = std::chrono::milliseconds(timeout_ms);

    if (isEmpty())
        return _frameAvailableCondition.wait_for( lock, duration ) == std::cv_status::no_timeout;
    else
        return true;
}

FrameSet * FrameSetManager::getFrameSet() {
    if (head_ == tail_) return nullptr;
    std::lock_guard<std::mutex> lock(tailMut);
    tailState = 3;
    return &fs[tail_ & FSM_MASK];
}

void FrameSetManager::releaseFrameSet(FrameSet *fsUsed) {
    std::lock_guard<std::mutex> lock(tailMut);
    if (fsUsed == &fs[tail_ & FSM_MASK]) {
        assert (tailState == 3);
        fsUsed->clear();
        tail_++;
        tailState = 0;
    } else if (fsUsed == nullptr) {
        if (tail_ != head_) {
            fs[tail_ & FSM_MASK].clear();
            tail_++;
        }
        tailState = 0;
    } else {
        std::cerr << " spurious release of FrameSet" << std::endl;
        tailState = 0;
    }
}

void FrameSetManager::putChipFrame(int chipIndex, ChipFrame* cf) {
    std::lock_guard<std::mutex> lock(headMut);
    if (isFull()) {
        std::cerr << " full" << std::endl;
    } else {
        if (headState == 0) {
            frameId = cf->frameId;
            headState = 1;
        }
        assert (headState == 1);
        if (frameId != cf->frameId) {
            // starting a new frame, after publishing this
            head_++;
            _frameAvailableCondition.notify_one();
            if (isFull()) {
                std::cerr << " full" << std::endl;
                headState = 0;
                expectCounterH = false;
                return;
            }
        }
        FrameSet *dest = &fs[head_ & FSM_MASK];
        dest->putChipFrame(chipIndex, cf);
        OMR omr = cf->omr;
        if (omr.getCountL() == 3 && omr.getMode() == 0)
            expectCounterH = true;
        else if (dest->isComplete()) {
            head_++;
            _frameAvailableCondition.notify_one();

            expectCounterH = false;
            headState = 0;
        }
    }
}


ChipFrame *FrameSetManager::newChipFrame(int chipIndex) {
    std::lock_guard<std::mutex> lock(headMut);
    FrameSet *dest = &fs[head_ & FSM_MASK];
    ChipFrame *frame = dest->takeChipFrame(chipIndex, expectCounterH);
    return frame == nullptr ? new ChipFrame() : frame;
    //return  new ChipFrame();
}
