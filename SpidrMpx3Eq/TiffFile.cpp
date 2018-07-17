#include "TiffFile.h"
#include <tiffio.h>
#include "QDebug"

TiffFile::TiffFile(QObject *parent) : QObject(parent)
{

}

bool TiffFile::saveToTiff32_512(const char* filePath, const int *pixels)
{
    /*uint width, height;
    width = size;
    height = size;
    */

    const uint16_t size = 512;

    //! FIXME BUG Unresolved, unknown cause
    if (pixels == nullptr) {
        qDebug() << "[ERROR] TiffFile::saveToTiff32_512, pixels == nullptr. pixels ==" << pixels;
        return false;
    }

    TIFF * m_pTiff = nullptr;

    //! Open the TIFF file, write mode
    m_pTiff = TIFFOpen(filePath, "w");

    int row[size];

    if (m_pTiff) {
        //! Write TIFF header tags

        TIFFSetField(m_pTiff, TIFFTAG_SAMPLESPERPIXEL, SAMPLES_PER_PIXEL);      // set number of channels per pixel
        TIFFSetField(m_pTiff, TIFFTAG_BITSPERSAMPLE,   BPP);                    // set the size of the channels
        TIFFSetField(m_pTiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);    // set the origin of the image.

        TIFFSetField(m_pTiff, TIFFTAG_COMPRESSION,     COMPRESSION_NONE);
        TIFFSetField(m_pTiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);    // No idea what this does but it's necessary
        TIFFSetField(m_pTiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_MINISBLACK);

        TIFFSetField(m_pTiff, TIFFTAG_IMAGEWIDTH,      size);                  // set the width of the image
        TIFFSetField(m_pTiff, TIFFTAG_IMAGELENGTH,     size);                 // set the height of the image

        memset(row, 0, size*sizeof(int));
        for (uint y=0; y < size; y++) {
            for (uint x=0; x < size; x++) {
                row[x] = pixels[y*size+ x];
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

bool TiffFile::saveToTiff32_256(const char *filePath, const int *pixels)
{
    /*uint width, height;
    width = size;
    height = size;
    */

    const uint16_t size = 256;

    TIFF * m_pTiff = nullptr;

    //! Open the TIFF file, write mode
    m_pTiff = TIFFOpen(filePath, "w");

    int row[size];

    if (m_pTiff) {
        //! Write TIFF header tags

        TIFFSetField(m_pTiff, TIFFTAG_SAMPLESPERPIXEL, SAMPLES_PER_PIXEL);      // set number of channels per pixel
        TIFFSetField(m_pTiff, TIFFTAG_BITSPERSAMPLE,   BPP);                    // set the size of the channels
        TIFFSetField(m_pTiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);    // set the origin of the image.

        TIFFSetField(m_pTiff, TIFFTAG_COMPRESSION,     COMPRESSION_NONE);
        TIFFSetField(m_pTiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);    // No idea what this does but it's necessary
        TIFFSetField(m_pTiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_MINISBLACK);

        TIFFSetField(m_pTiff, TIFFTAG_IMAGEWIDTH,      size);                  // set the width of the image
        TIFFSetField(m_pTiff, TIFFTAG_IMAGELENGTH,     size);                 // set the height of the image

        memset(row, 0, size*sizeof(int));
        for (uint y=0; y < size; y++) {
            for (uint x=0; x < size; x++) {
                row[x] = pixels[y*size+ x];
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

bool TiffFile::saveToTiff32_516(const char *filePath, const int *pixels)
{
    /*uint width, height;
    width = size;
    height = size;
    */

    const uint16_t size = 516;

    TIFF * m_pTiff = nullptr;

    //! Open the TIFF file, write mode
    m_pTiff = TIFFOpen(filePath, "w");

    int row[size];

    if (m_pTiff) {
        //! Write TIFF header tags

        TIFFSetField(m_pTiff, TIFFTAG_SAMPLESPERPIXEL, SAMPLES_PER_PIXEL);      // set number of channels per pixel
        TIFFSetField(m_pTiff, TIFFTAG_BITSPERSAMPLE,   BPP);                    // set the size of the channels
        TIFFSetField(m_pTiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);    // set the origin of the image.

        TIFFSetField(m_pTiff, TIFFTAG_COMPRESSION,     COMPRESSION_NONE);
        TIFFSetField(m_pTiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);    // No idea what this does but it's necessary
        TIFFSetField(m_pTiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_MINISBLACK);

        TIFFSetField(m_pTiff, TIFFTAG_IMAGEWIDTH,      size);                  // set the width of the image
        TIFFSetField(m_pTiff, TIFFTAG_IMAGELENGTH,     size);                 // set the height of the image

        memset(row, 0, size*sizeof(int));
        for (uint y=0; y < size; y++) {
            for (uint x=0; x < size; x++) {
                row[x] = pixels[y*size+ x];
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

bool TiffFile::saveToTiff32_260(const char *filePath, const int *pixels)
{
    /*uint width, height;
    width = size;
    height = size;
    */

    const uint16_t size = 260;

    TIFF * m_pTiff = nullptr;

    //! Open the TIFF file, write mode
    m_pTiff = TIFFOpen(filePath, "w");

    int row[size];

    if (m_pTiff) {
        //! Write TIFF header tags

        TIFFSetField(m_pTiff, TIFFTAG_SAMPLESPERPIXEL, SAMPLES_PER_PIXEL);      // set number of channels per pixel
        TIFFSetField(m_pTiff, TIFFTAG_BITSPERSAMPLE,   BPP);                    // set the size of the channels
        TIFFSetField(m_pTiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);    // set the origin of the image.

        TIFFSetField(m_pTiff, TIFFTAG_COMPRESSION,     COMPRESSION_NONE);
        TIFFSetField(m_pTiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);    // No idea what this does but it's necessary
        TIFFSetField(m_pTiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_MINISBLACK);

        TIFFSetField(m_pTiff, TIFFTAG_IMAGEWIDTH,      size);                  // set the width of the image
        TIFFSetField(m_pTiff, TIFFTAG_IMAGELENGTH,     size);                 // set the height of the image

        memset(row, 0, size*sizeof(int));
        for (uint y=0; y < size; y++) {
            for (uint x=0; x < size; x++) {
                row[x] = pixels[y*size+ x];
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
