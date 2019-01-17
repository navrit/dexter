#include "FrameSetManager.h"

#include <assert.h>
#include <iostream>
#include <chrono>

FrameSetManager::FrameSetManager()
{

}

bool FrameSetManager::isFull() {
    return head_ - tail_ == FSM_SIZE-2;
}

bool FrameSetManager::isEmpty() {
    return head_ == tail_;
}

int FrameSetManager::available() {
    return int(head_ - tail_);
}

bool FrameSetManager::wait(unsigned long timeout_ms) {

    if (isEmpty()) {
        auto duration = std::chrono::milliseconds(timeout_ms);
        std::unique_lock<std::mutex> lock(tailMut);
        return _frameAvailableCondition.wait_for( lock, duration ) == std::cv_status::no_timeout;
    } else
        return true;
}

FrameSet * FrameSetManager::getFrameSet() {
    if (head_ == tail_) return nullptr;
    return &fs[tail_ & FSM_MASK];
}

void FrameSetManager::releaseFrameSet(FrameSet *fsUsed) {
    std::lock_guard<std::mutex> lock(tailMut);
    if (fsUsed == &fs[tail_ & FSM_MASK]) {
        fsUsed->clear();
        tail_++;
    } else if (fsUsed == nullptr) {
        if (tail_ != head_) {
            fs[tail_ & FSM_MASK].clear();
            tail_++;
        }
    } else {
        std::cerr << " spurious release of FrameSet" << std::endl;
    }
}

void FrameSetManager::publish() {
    if (isFull()) {
        _framesLost++;
        fs[head_ & FSM_MASK].clear();
    } else {
        head_++;
        _framesReceived++;
        _frameAvailableCondition.notify_one();
    }
}

void FrameSetManager::putChipFrameB(int chipIndex, ChipFrame* cf) {
    std::lock_guard<std::mutex> lock(headMut);
    //std::cerr << "putB " << int(frameId) << " chip " << chipIndex << "\n";
    if (headState == 0) {
        frameId++;
        //std::cerr << "state 0 found " << int(frameId) << "\n";
        headState = 1;
    }
    cf->frameId = frameId;
    assert (headState == 1);
    FrameSet *dest = &fs[head_ & FSM_MASK];
    dest->putChipFrame(chipIndex, cf);
    if (bothCounters && cf->omr.getMode() == 0)
        expectCounterH = true;
    else if (dest->isComplete()) {
        publish();
        expectCounterH = false;
        headState = 0;
    }
}

void FrameSetManager::putChipFrame(int chipIndex, ChipFrame* cf, uint8_t frameId) {
    std::lock_guard<std::mutex> lock(headMut);
    //std::cerr << "put " << int(frameId) << " chip " << chipIndex << "\n";
    cf->frameId = frameId;
    if (headState == 0) {
        //std::cerr << "state 0 found " << int(frameId) << "\n";
        if (this->frameId == frameId) {
            //std::cerr << "duplicate " << int(frameId) << "\n";
        } else {
            this->frameId = frameId;
        }
        headState = 1;
    }
    assert (headState == 1);
    if (frameId != this->frameId) {
        // starting a new frame, after publishing this
        publish();
        //std::cerr << "jump " << int(this->frameId) << " to " << int(frameId) << "\n";
        this->frameId = frameId;
    }
    FrameSet *dest = &fs[head_ & FSM_MASK];
    dest->putChipFrame(chipIndex, cf);
    OMR omr = cf->omr;
    if (bothCounters && omr.getMode() == 0)
        expectCounterH = true;
    else if (dest->isComplete()) {
        publish();
        expectCounterH = false;
        headState = 0;
    }
}


ChipFrame *FrameSetManager::newChipFrame(int chipIndex, OMR omr) {
    std::lock_guard<std::mutex> lock(headMut);
    uint ahead = head_;
    if (headState == 1 && ! expectCounterH) ahead++;
    FrameSet *scratch = &fs[ahead & FSM_MASK];
    ChipFrame *frame = scratch->takeChipFrame(chipIndex, expectCounterH);
    if (frame == nullptr) frame = new ChipFrame();
    frame->omr = omr;
    return frame;
    //return  new ChipFrame();
}
