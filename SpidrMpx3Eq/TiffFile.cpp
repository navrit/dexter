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

    const bool spatialCorrectionOnly = false;
    const bool crossCorrection = false;

    const static int SAMPLES_PER_PIXEL = 1; // This is greyscale - 3 for RGB, 4 for RGBA
    // Maybe make this the number of thresholds?
    const static int BPP        = 32; // Bits per pixel - gives ~4 billion per pixel upper limit before loss of signal
    const int extraPixels       = 2;

    float edgePixelMagicNumber  = float(2.8);
    float edgePixelMagicNumberSpectro = float(2.3);
    float edgePixelMagicNumberSpectroVertical = float(1.9);
    float edgePixelMagicNumberSpectroHorizontal = float(1.1);

    if (spatialCorrectionOnly){
        edgePixelMagicNumber = 1;
        edgePixelMagicNumberSpectro = 1;
        edgePixelMagicNumberSpectroHorizontal = 1;
        edgePixelMagicNumberSpectroVertical = 1;
    }
    //! Save for all thresholds in separate files
//    for(int i = 0; i < thresholds.length(); i++) {

        int image[height*width];
        int imageCorrected[height*width];
        TIFF * m_pTiff = nullptr;
        QString tmpFilename = filePath;

        //! Default mode - do cross and spatial corrections

        /*
        if (crossCorrection){
            //! Normal Fine Pitch mode - do cross correction
//            qDebug() << getThresholds().count();
            if (thresholds.count() == 1) {
                for (int y=0; y < height; y++) {
                    for (int x=0; x < width; x++) {
                        if (x==(width/2)-1 || x==(width/2) || y==(height/2)-1 || y==(height/2)){
                            //! Hard coded cross correction
                            image[y*width + x] = sample(x, y, thresholds[i]) / edgePixelMagicNumber;
                        } else {
                            //! Default option, sample the pixels directly
                            image[y*width + x] = sample(x, y, thresholds[i]);
                        }
                    }
                }

                //! Spectroscopic mode don't try cross correction - it doesn't work properly
                //! due to a non constant factor between the edge pixels and the main pixels
                //!
                //! Ask Sammi for details. Some manufacturing BS
                //!
                //! Implement later if necessary

            } else {
                for (uint y=0; y < height; y++) {
                    for (uint x=0; x < width; x++) {
                        //! Sample the pixels directly
                        image[y*width + x] =  //sample(x, y, thresholds[i]);
                    }
                }
                tmpFilename.replace(".tiff", "-thl" + QString::number(thresholds[i]) + ".tiff");
            }

            //! Phase 1
            //! Do spatial correction on quadrants, move to respective corners
            for (int y=0; y < height; y++) {
                for (int x=0; x < width; x++) {

                    if (y < (height/2)-extraPixels && x < (width/2)-extraPixels) {      // TL
                        imageCorrected[y*width + x] = sample(x, y, thresholds[i]);
                    } else if (y < (height/2)-extraPixels && x >= (width/2)+extraPixels) {      // TR
                        imageCorrected[y*width + x] = sample(x-2*extraPixels, y, thresholds[i]);
                    } else if (y >= (height/2)+extraPixels && x < (width/2)-extraPixels) {      // BL
                        imageCorrected[y*width + x] = sample(x, y-2*extraPixels, thresholds[i]);
                    } else if (y >= (height/2)+extraPixels && x >= (width/2)+extraPixels) {     // BR
                        imageCorrected[y*width + x] = sample(x-2*extraPixels, y-2*extraPixels, thresholds[i]);
                    }

                }
            }
            //! Phase 2
            //! Loop for cross
            for (int y=0; y < height; y++) {
                for (int x=0; x < width; x++) {
                    if (x >= (width/2)-extraPixels && x <= (width/2)+extraPixels){               // vertical centre
                        if (x == (width/2)-extraPixels ) {                              // L
                            //Set 255, 256, 257 as 1/2.8 of 255
                            // qDebug() << "[L]" << tmp << x << y;
                            if (y <= height/2 - extraPixels || y >= height/2 + extraPixels + 1){

                                int tmp;
                                if (width == 260){ // If spectro mode
                                    tmp = imageCorrected[y*width + x-1] / edgePixelMagicNumberSpectroVertical;
                                } else {
                                    tmp = imageCorrected[y*width + x-1] / edgePixelMagicNumber;
                                }
                                //qDebug() << tmp << x-1 << y << x << y;
                                imageCorrected[y*width + x-1  ] = tmp;
                                imageCorrected[y*width + x    ] = tmp;
                                imageCorrected[y*width + x+1  ] = tmp;
                            }

                        } else if (x == (width/2)+extraPixels ) {                                // R
                            //qDebug() << "[R]" << tmp << x << y;

                            if (y <= height/2 - extraPixels || y >= height/2 + extraPixels + 1) {

                                int tmp;
                                if (width == 260){ // If spectro mode
                                    tmp = imageCorrected[y*width + x] / edgePixelMagicNumberSpectroVertical;
                                } else {
                                    tmp = imageCorrected[y*width + x] / edgePixelMagicNumber;
                                }
                                //                                qDebug() << tmp << x << y;
                                imageCorrected[y*width + x-2  ] = tmp;
                                imageCorrected[y*width + x-1  ] = tmp;
                                imageCorrected[y*width + x    ] = tmp;
                            }

                        }
                    } else if (y >= (height/2)-extraPixels && y <= (height/2)+extraPixels){      // horizontal centre
                        if        (y == (height/2)-extraPixels) {
                            if (x <= width/2 - extraPixels ) {                                   // TL
                                int tmp;
                                if (width == 260){ // If spectro mode
                                    tmp = imageCorrected[(y-1)*width + x] / edgePixelMagicNumberSpectroHorizontal;
                                } else {
                                    tmp = imageCorrected[(y-1)*width + x] / edgePixelMagicNumber;
                                }

                                //qDebug() << "[T]" << tmp << x << y;
                                imageCorrected[(y-1)*width + x  ] = tmp;
                                imageCorrected[(y )*width +  x  ] = tmp;
                                imageCorrected[(y+1)*width + x  ] = tmp;
                            } else if (x >= width/2 + extraPixels + 1) {                         // TR
                                int tmp;
                                if (width == 260){ // If spectro mode
                                    tmp = imageCorrected[(y-1)*width + x] / edgePixelMagicNumberSpectro;
                                } else {
                                    tmp = imageCorrected[(y-1)*width + x] / edgePixelMagicNumber;
                                }

                                //qDebug() << "[T]" << tmp << x << y;
                                imageCorrected[(y-1)*width + x  ] = tmp;
                                imageCorrected[(y )*width +  x  ] = tmp;
                                imageCorrected[(y+1)*width + x  ] = tmp;
                            }
                        } else if (y == (height/2)+extraPixels) {
                            if (x <= width/2 - extraPixels) {                                    // BL
                                int tmp;
                                if (width == 260){ // If spectro mode
                                    tmp = imageCorrected[(y)*width + x] / edgePixelMagicNumberSpectro;
                                } else {
                                    tmp = imageCorrected[(y)*width + x] / edgePixelMagicNumber;
                                }
                                //qDebug() << "[B]" << tmp << x << y;
                                imageCorrected[(y-2)*width + x  ] = tmp;
                                imageCorrected[(y-1)*width + x  ] = tmp;
                                imageCorrected[(y  )*width + x  ] = tmp;
                            } else if (x >= width/2 + extraPixels + 1) {                         // BR
                                int tmp;
                                if (width == 260){ // If spectro mode
                                    tmp = imageCorrected[(y)*width + x] / edgePixelMagicNumberSpectroHorizontal;
                                } else {
                                    tmp = imageCorrected[(y)*width + x] / edgePixelMagicNumber;
                                }
                                //qDebug() << "[B]" << tmp << x << y;
                                imageCorrected[(y-2)*width + x  ] = tmp;
                                imageCorrected[(y-1)*width + x  ] = tmp;
                                imageCorrected[(y  )*width + x  ] = tmp;
                            }

                        }
                    }
                }
            }
            //! Phase 3
            //! Separate loop for central region
            for (int y=0; y < height; y++) {
                for (int x=0; x < width; x++) {
                    if (x >= (width/2)-extraPixels &&
                            x < (width/2)+extraPixels &&
                            y >= (height/2)-extraPixels &&
                            y < (height/2)+extraPixels){                                     // central square
                        int tmp = sample(x,y, thresholds[i]) / edgePixelMagicNumber;
                        //                        qDebug() << "[CENTRAL]" << tmp << x << y;
                        if        (x == (width/2)-extraPixels && y == (height/2)-extraPixels){      //TL
                            //qDebug() << "[TL]" << image[y*width + x] << x << y;
                            //! Following commented code does this:
                            //!     Makes central pixels the average of the adjacent 2 corners
                            int tmp = (imageCorrected[(y-extraPixels)*width + x]+imageCorrected[y*width + x - extraPixels])/2;
                            imageCorrected[ y   *width + x    ] = tmp;
                            imageCorrected[(y-1)*width + x - 1] = tmp;
                            imageCorrected[(y-1)*width + x    ] = tmp;
                            imageCorrected[(y-1)*width + x + 1] = tmp;
                            imageCorrected[(y+1)*width + x - 1] = tmp;
                            imageCorrected[(y+1)*width + x    ] = tmp;
                            imageCorrected[(y+1)*width + x + 1] = tmp;
                            imageCorrected[ y   *width + x + 1] = tmp;
                            imageCorrected[ y   *width + x - 1] = tmp;
                        } else if (x == (width/2)+extraPixels/2 && y == (height/2)-extraPixels) {     //TR
                            //                            qDebug() << "[TR]" << tmp << x << y;
                            int tmp = (imageCorrected[(y-extraPixels)*width + x]+imageCorrected[y*width + x + extraPixels])/2;
                            imageCorrected[ y   *width + x    ] = tmp;
                            imageCorrected[(y-1)*width + x - 1] = tmp;
                            imageCorrected[(y-1)*width + x    ] = tmp;
                            imageCorrected[(y-1)*width + x + 1] = tmp;
                            imageCorrected[(y+1)*width + x - 1] = tmp;
                            imageCorrected[(y+1)*width + x    ] = tmp;
                            imageCorrected[(y+1)*width + x + 1] = tmp;
                            imageCorrected[ y   *width + x + 1] = tmp;
                            imageCorrected[ y   *width + x - 1] = tmp;
                        } else if (x == (width/2)-extraPixels && y == (height/2)+extraPixels/2) {     //BL
                            //                            qDebug() << "[BL]" << tmp << x << y;
                            int tmp = (imageCorrected[(y+extraPixels)*width + x]+imageCorrected[y*width + x - extraPixels])/2;
                            imageCorrected[ y   *width + x    ] = tmp;
                            imageCorrected[(y-1)*width + x - 1] = tmp;
                            imageCorrected[(y-1)*width + x    ] = tmp;
                            imageCorrected[(y-1)*width + x + 1] = tmp;
                            imageCorrected[(y+1)*width + x - 1] = tmp;
                            imageCorrected[(y+1)*width + x    ] = tmp;
                            imageCorrected[(y+1)*width + x + 1] = tmp;
                            imageCorrected[ y   *width + x + 1] = tmp;
                            imageCorrected[ y   *width + x - 1] = tmp;
                        } else if (x == (width/2)+extraPixels/2 && y == (height/2)+extraPixels/2) {     //BR
                            //                            qDebug() << "[BR]" << tmp << x << y;
                            int tmp = (imageCorrected[(y+extraPixels)*width + x]+imageCorrected[y*width + x + extraPixels])/2;
                            imageCorrected[ y   *width + x    ] = tmp;
                            imageCorrected[(y-1)*width + x - 1] = tmp;
                            imageCorrected[(y-1)*width + x    ] = tmp;
                            imageCorrected[(y-1)*width + x + 1] = tmp;
                            imageCorrected[(y+1)*width + x - 1] = tmp;
                            imageCorrected[(y+1)*width + x    ] = tmp;
                            imageCorrected[(y+1)*width + x + 1] = tmp;
                            imageCorrected[ y   *width + x + 1] = tmp;
                            imageCorrected[ y   *width + x - 1] = tmp;
                        }
                    }
                }
            }
        } else {
//            qDebug() << thresholds.count();

            for (uint y=0; y < height; y++) {
                for (uint x=0; x < width; x++) {
                    //! Sample the pixels directly
                    image[y*width + x] = sample(x, y, thresholds[i]);
                }
            }
            if (thresholds.count() > 1){
                tmpFilename.replace(".tiff", "-thl" + QString::number(thresholds[i]) + ".tiff");
            }
        }
        */

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

            /*
            if (crossCorrection){
                for (uint r=0; r < height; r++) {
                    TIFFWriteScanline(m_pTiff, &imageCorrected[r*width], r, 0);
                }
//                qDebug() << "[INFO] Written corrected TIFF";
            } else {*/
//                for (uint r=0; r < height; r++) {
//                    TIFFWriteScanline(m_pTiff, &image[r*width], r, 0);
//                }
//                qDebug() << "[INFO] Written raw TIFF";
//            }


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
//    }
        return true;
}
