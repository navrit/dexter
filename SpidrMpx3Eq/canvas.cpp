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

template <typename INTTYPE> void fill(uint8_t* image, int dim, int gap, Dataset *ds, int* layer) {
    INTTYPE* img = (INTTYPE*) image;
    int imgDim = 2 * dim + gap;

    auto positions = ds->getLayoutVector();
    auto orientations = ds->getOrientationVector();
    int nChips = ds->getFrameCount();
    for (int chip = 0; chip < nChips; chip++) {
        int posx = positions[chip].x();
        int posy = positions[chip].y(); // NB: bottom to top
        int base = (1 - posy) * (dim + gap) * imgDim + posx * (dim + gap);

        imgOrientation io = orientation[orientations[chip]];

        int cs = io.fast.ix - imgDim * io.fast.iy,
            rs = io.slow.ix - imgDim * io.slow.iy;
        if (cs < 0) base += (dim-1)*imgDim;
        if (rs < 0) base += dim-1;

        for (int i = 0; i < dim; i++) {
            int ix = base;
            int j = dim;
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

    const int dim = 256;
    int imgDim = 2 * dim + gap;

    rowStride = imgDim * bytesPerPixel;
    size = imgDim * rowStride;
    image = allocateBuffer(size_t (size));

    if (bytesPerPixel == 2) {
        fill<uint16_t>(image, dim, gap, ds, layer);
    } else {
        fill<uint32_t>(image, dim, gap, ds, layer);
    }
}
