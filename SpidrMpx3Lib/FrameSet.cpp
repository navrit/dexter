#include <cassert>
#include "FrameSet.h"
#include <iostream>

FrameSet::FrameSet()
{
    memset(frame, 0, sizeof (frame));
}

FrameSet::~FrameSet() {
    for (int i = 0; i < number_of_chips; i++)
      for (int j = 0; j < 2; j++)
        if (frame[j][i] != nullptr) {
            delete frame[j][i];
        }
}

void FrameSet::setBothCounters(bool b) {
    counters = b ? 2 : 1;
}

/**
 * @brief FrameSet::clear marks the ChipFrames as clear
 */
void FrameSet::clear() {
    for (int i = 0; i < number_of_chips; i++)
      for (int j = 0; j < 2; j++)
        if (frame[j][i] != nullptr) {
            //delete frame[j][i];
            //frame[j][i] = nullptr;
            frame[j][i]->clear();
        }
    counters = 1;
}

/**
 * @brief FrameSet::isComplete checks if all ChipFrames are present
 * @param chipMask the mask indicating which chips are present
 * @return
 */
bool FrameSet::isComplete(int chipMask) {
    for (int j = 0; j < counters; j++)
        for (int i = 0; i < number_of_chips; i++)
            if (((chipMask >> i) & 1) && frame[j][i] == nullptr)
                return false;

    return true;
}

/**
 * @brief FrameSet::takeChipFrame	takes out an existing ChipFrame
 * @param chipIndex the index of the chip
 * @param counterH  whether to take the high counter entry
 * @return the ChipFrame or nullptr
 */
ChipFrame* FrameSet::takeChipFrame(int chipIndex, bool counterH) {
    assert (chipIndex >= 0 && chipIndex < number_of_chips);
    if (counterH) counters = 2;
    ChipFrame **spot = &(frame[counterH ? 1 : 0][chipIndex]);
    ChipFrame *result = *spot;
    if (result != nullptr) {
        *spot = nullptr;
    }
    return result;
}

/**
 * @brief FrameSet::putChipFrame	put in an existing ChipFrame
 * @param chipIndex the index of the chip
 * @param the ChipFrame
 */
void FrameSet::putChipFrame(int chipIndex, ChipFrame *cf) {
    assert (chipIndex >= 0 && chipIndex < number_of_chips);
    assert (cf != nullptr);
    // in 24 bit mode use both counters, not in CRW;
    if (cf->omr.getCountL() == 3) counters = 2;
    int hi = (counters == 2 && cf->omr.getMode() == 4) ? 1 : 0;
    assert (hi == 0 || counters == 2);
    ChipFrame **spot = &(frame[hi][chipIndex]);
    if (*spot != nullptr) delete *spot;
    *spot = cf;
}

/**
 * @brief FrameSet::copyTo32 copy one frame (could be 24 bits) into an 32 bit array
 * @param chipIndex the index of the chip
 * @param counterH whether to use the high counter (not for 24 bits)
 * @param dest where to copy to
 */
void FrameSet::copyTo32(int chipIndex, bool counterH, uint32_t *dest) {
    ChipFrame *f0 = frame[0][chipIndex];
    ChipFrame *f1 = frame[1][chipIndex];
    if (f0 == nullptr)
        return; // sorry, chip is not there
    bool mode24 = f0->omr.getCountL() == 3;
    int n = MPX_PIXELS;
    if (mode24 && f1 != nullptr) {
        uint16_t *src0 = f0->getRow(0);
        uint16_t *src1 = f1->getRow(0);
        while (n--) *(dest++) = (uint32_t(*(src1++)) << 12) | *(src0++) ;
    } else {
        ChipFrame *f = counterH ? f1 : f0;
        if (f == nullptr) {
            std::cerr << " missing ChipFrame" << std::endl;
        } else {
            uint16_t *src = f->getRow(0);
            while (n--) *(dest++) = *(src++);
        }
    }
}

int FrameSet::pixelsLost() {
    int count = 0;
    for (int j = 0; j < counters; j++)
        for (int i = 0; i < number_of_chips; i++)
            if (frame[j][i] != nullptr)
                //count += MPX_PIXELS;		// it's not there
            //else
                count += frame[j][i]->pixelsLost;

    return count;
}

