#ifndef CHIPFRAME_H
#define CHIPFRAME_H

#include <stdint.h>
#include <cstring>
#include "mpx3defs.h"
#include "OMR.h"

class ChipFrame
{
public:
    ChipFrame();
    uint8_t frameId = 0;
    int pixelsLost = 0;
    OMR omr = 0;
    int brokenRows = 0;
    void clear() { memset(data, 0, sizeof(data)); pixelsLost = 0; brokenRows = 0;}
    uint16_t *getRow(int rowNum) { return data + MPX_PIXEL_COLUMNS * rowNum; }

private:
    uint16_t data[MPX_PIXELS] = {0};
};

#endif // CHIPFRAME_H
