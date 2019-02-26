#include "canvas.h"
#include "dataset.h"
#include <tiffio.h>
#include "QDebug"
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
        canvas->width  = ds->getWidth() + gap;
        canvas->height = imgWidth = ds->getHeight() + gap;
        chipW = ds->x();
        chipH = ds->y();
    }
    canvas->rowStride = imgWidth * canvas->pixelStride;
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

    pixelStride = bytesPerPixel;
    size = (ds->getWidth() + gap) * (ds->getHeight() + gap) * bytesPerPixel;
    image = allocateBuffer(size_t (size));

    if (bytesPerPixel == 2) {
        fill<uint16_t>(this, gap, ds, layer);
    } else {
        fill<uint32_t>(this, gap, ds, layer);
    }
}

bool Canvas::saveToTiff(const char* filePath)
{

    //! Open the TIFF file, write mode
    TIFF * m_pTiff = TIFFOpen(filePath, "w");

    if (m_pTiff) {
        //! Write TIFF header tags

        TIFFSetField(m_pTiff, TIFFTAG_SAMPLESPERPIXEL, 1);				        // set number of channels per pixel
        TIFFSetField(m_pTiff, TIFFTAG_BITSPERSAMPLE,   8 * pixelStride);	    // set the size of the channels
        TIFFSetField(m_pTiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);    // set the origin of the image.

        TIFFSetField(m_pTiff, TIFFTAG_COMPRESSION,     COMPRESSION_NONE);
        TIFFSetField(m_pTiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);    // No idea what this does but it's necessary
        TIFFSetField(m_pTiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_MINISBLACK);

        TIFFSetField(m_pTiff, TIFFTAG_IMAGEWIDTH,      width);                  // set the width of the image
        TIFFSetField(m_pTiff, TIFFTAG_IMAGELENGTH,     height);                 // set the height of the image

        uint8_t* img = image;
        for (uint y=0; y < height; y++) {
            TIFFWriteScanline(m_pTiff, img, y, 0);
            img += rowStride;
        }

    } else if (m_pTiff == nullptr) {
        qDebug() << "[ERROR] Unable to write TIFF file";
        return false;
    } else {
        qDebug() << "[ERROR] Unknown TIFF file write failure.";
        return false;
    }

    //! Cleanup code - close the file when done
    if ( m_pTiff ) {
        TIFFClose( m_pTiff );
    }
    return true;
}

bool Canvas::saveToPGM16(const char* filePath) {
    FILE * f = fopen(filePath, "w");
    fprintf(f, "P5\n%d %d\n%d\n", width, height, 4095);
    const int bufsiz = 16384;
    uint16_t buf[bufsiz];
    uint16_t* img = (uint16_t*) image;
    int n = size/2;
    while (n > 0) {
        int n2 = n > bufsiz ? bufsiz : n;
        uint16_t* bufp = buf;
        for (int i = 0; i < n2; i++) {
            uint16_t l = *(img++);
            *(bufp++) = ((l >> 8) & 0x00ff) | ((l << 8) & 0xff00);
        }
        fwrite (buf, n2 * 2, 1, f);
        n -= n2;
    }
    fclose(f);
}
