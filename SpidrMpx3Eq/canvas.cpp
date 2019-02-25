#include "canvas.h"
#include "dataset.h"
#include <utility>
#include <atomic>

uint8_t* allocateBuffer(size_t size) {
    // OK this is a quick hack, create a nicer version later
    const int poolSize = 32;
    static std::pair<uint8_t*, size_t> pool[poolSize] = {std::pair<uint8_t*,size_t>(nullptr, (size_t) 0)};
    static atomic_int bufferCounter;
    int buffIndex = (bufferCounter++) & (poolSize-1);
    std::pair<uint8_t*, size_t> item = pool[buffIndex];
    if (item.first == nullptr || item.second < size)
        pool[buffIndex] = item = std::pair<uint8_t*,size_t>(new uint8_t[size], size);
    return item.first;
}

Canvas::Canvas() {}

template <typename INTTYPE> void fill(Canvas* canvas, int gap, Dataset *ds, int* layer) {
    INTTYPE* img = (INTTYPE*) canvas->image;
    int imgWidth = canvas->width;

    auto positions = ds->getLayoutVector();
    auto orientations = ds->getOrientationVector();
    int chipW, chipH; // chip width and height on the canvas
    if (orientations[0] >= 4) {
        // image is rotated
        canvas->height = ds->getWidth() + gap;
        canvas->width  = imgWidth = ds->getHeight() + gap;
        chipW = ds->y();
        chipH = ds->x();
    } else {
        chipW = ds->x();
        chipH = ds->y();
    }
    int nChips = ds->getFrameCount();
    int chipRows = ds->y(), chipCols = ds->x(); // chip native rows and columns
    for (int chip = 0; chip < nChips; chip++) {
        int posx = positions[chip].x();
        int posy = positions[chip].y(); // NB: bottom to top
        int base = (1 - posy) * (chipH + gap) * imgWidth + posx * (chipW + gap);

        imgOrientation io = orientation[orientations[chip]];

        int cs = io.fast.ix - imgWidth * io.fast.iy,
            rs = io.slow.ix - imgWidth * io.slow.iy;
        if (cs < 0) base += (chipH-1)*imgWidth;
        if (rs < 0) base += chipW-1;

        for (int i = 0; i < chipRows; i++) {
            int ix = base;
            int j = chipCols;
            switch (cs) {
            case  1: while (j--) img[ix++] = *(layer++); break;
            case -1: while (j--) img[ix--] = *(layer++); break;
            default: while (j--) {
                    img[ix] = *(layer++);
                    ix += cs;
                }
            }
            base += rs;
        }
    }
}

Canvas::Canvas(Dataset *ds, int key, int gap, int bytesPerPixel)
{
    int * layer = ds->getLayer(key);
    if (layer == nullptr) {
        return;
    }

    width = ds->getWidth() + gap;
    height = ds->getHeight() + gap;

    pixelStride = bytesPerPixel;
    rowStride = width * bytesPerPixel;
    size = height * rowStride;
    image = allocateBuffer(size_t (size));

    if (bytesPerPixel == 2) {
        fill<uint16_t>(this, gap, ds, layer);
    } else {
        fill<uint32_t>(this, gap, ds, layer);
    }
}
