#include "TiffFile.h"
#include <tiffio.h>
#include "QDebug"

TiffFile::TiffFile(QObject *parent) : QObject(parent)
{

}

bool TiffFile::saveToTiff32(const char* filePath, Canvas pixels, int width, int height)
{

    //! Open the TIFF file, write mode
    TIFF * m_pTiff = TIFFOpen(filePath, "w");

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

        uint8_t* img = pixels.image;
        for (uint y=0; y < height; y++) {
            TIFFWriteScanline(m_pTiff, img, y, 0);
            img += pixels.rowStride;
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
