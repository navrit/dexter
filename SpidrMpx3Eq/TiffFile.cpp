#include "TiffFile.h"
#include <tiffio.h>
#include "QDebug"

TiffFile::TiffFile(QObject *parent) : QObject(parent)
{

}

bool TiffFile::saveToTiff32(const char* filePath, const uint size, const int *pixels)
{
    uint width, height;
    width = size;
    height = size;

    const static int SAMPLES_PER_PIXEL = 1; // This is greyscale - 3 for RGB, 4 for RGBA
    // Maybe make this the number of thresholds?
    const static int BPP = 32; // Bits per pixel - gives ~4 billion per pixel upper limit before loss of signal

    TIFF * m_pTiff = nullptr;
    QString tmpFilename = filePath;

    //! Open the TIFF file, write mode
    m_pTiff = TIFFOpen(tmpFilename.toLatin1().data(), "w");

    if (m_pTiff) {
        //! Write TIFF header tags

        TIFFSetField(m_pTiff, TIFFTAG_SAMPLESPERPIXEL, SAMPLES_PER_PIXEL);      // set number of channels per pixel
        TIFFSetField(m_pTiff, TIFFTAG_BITSPERSAMPLE,   BPP);                    // set the size of the channels
        TIFFSetField(m_pTiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);    // set the origin of the image.

        TIFFSetField(m_pTiff, TIFFTAG_COMPRESSION,     COMPRESSION_NONE);
        TIFFSetField(m_pTiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);    // No idea what this does but it's necessary
        TIFFSetField(m_pTiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_MINISBLACK);

        TIFFSetField(m_pTiff, TIFFTAG_IMAGEWIDTH,      width);                  // set the width of the image
        TIFFSetField(m_pTiff, TIFFTAG_IMAGELENGTH,     height);                 // set the height of the image

        int row[width];
        for (uint y=0; y < height; y++) {
            for (uint x=0; x < width; x++) {
                row[x] = pixels[y*width + x];
            }
            TIFFWriteScanline(m_pTiff, (void*)row, y, 0);
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
