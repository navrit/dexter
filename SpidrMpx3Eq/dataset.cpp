#include "dataset.h"
#include "spline.h"
#include "qcstmcorrectionsdialog.h"

#include <QDataStream>
#include <QDebug>
#include <QString>
#include <QtGui>
#include <QMessageBox>

#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>      // std::ofstream

#include <tiffio.h> /* Sam Leffler's libtiff library. */

using namespace std;

Dataset::Dataset(int x, int y, int framesPerLayer, int pixelDepthBits)
{
    m_nx = x; m_ny = y;
    m_pixelDepthBits = pixelDepthBits;
    m_pixelDepthCntr = ((int)pow(2, m_pixelDepthBits)) - 1;

    m_nFrames = 0;
    setFramesPerLayer(framesPerLayer);

    obCorrection = 0x0;

    rewindScores();
}

Dataset::Dataset() : Dataset(1,1,1) {
    rewindScores();
}

Dataset::~Dataset()
{
    if(obCorrection) delete obCorrection;
    clear();
}

void Dataset::loadCorrection(QByteArray serialized) {
    delete obCorrection;
    obCorrection  = new Dataset(0,0,0);
    obCorrection->fromByteArray(serialized); //TODO: add error checking on correction to see if it is relevant to the data.
}

void Dataset::rewindScores() {
    m_scores.packetsLost = 0;
    m_scores.framesLost = 0;
    m_scores.dataMisaligned = false;
    m_scores.mpx3ClockStops = 0;
}

int64_t Dataset::getTotal(int threshold) {
    int index = thresholdToIndex(threshold);
    if(index == -1)
        return 0;
    int64_t count = 0;
    for(unsigned int j = 0; j < getPixelsPerLayer(); j++)
        count += m_layers[index][j];
    return count;
}

//! Overflow is checked on a frame per frame basis, not on the integral
int64_t Dataset::getOverflow(int /*threshold*/)
{
    /*
    int index = thresholdToIndex(threshold);
    if(index == -1)
        return 0;
    int64_t count = 0;
    int overflowval = 0;
    for(unsigned int j = 0; j < getPixelsPerLayer(); j++) {
        if ( m_layers[index][j] >= overflowval ) count ++;
    }
    return count;
    */
    return 0;
}

uint64_t Dataset::getActivePixels(int threshold){
    int index = thresholdToIndex(threshold);
    if(index == -1)
        return 0;
    uint64_t count  =0;
    for(unsigned int j = 0; j <getPixelsPerLayer(); j++){
        if(0 != m_layers[index][j])
            count++;
    }
    return count;
}

/*!
 * \brief MyClass::MyClass
 *        The copy constructor
 */
Dataset::Dataset( const Dataset& other ):
    m_boundingBox(other.m_boundingBox),
    m_scores(other.m_scores),
    m_frameLayouts(other.m_frameLayouts),
    m_frameOrientation(other.m_frameOrientation),
    m_thresholdsToIndices(other.m_thresholdsToIndices)
  //m_layers(other.m_layers)
{

    // copy the dimensions
    m_nx = other.x(); m_ny = other.y();
    m_pixelDepthBits = other.getPixelDepthBits();
    // And copy the layers
    m_nFrames = other.getFrameCount();
    m_layers = QVector<int *>( other.getLayerCount() );
    for (int i = 0; i < m_layers.size(); i++) {
        m_layers[i] = new int[getPixelsPerLayer()];
        for(unsigned int j = 0; j < getPixelsPerLayer(); j++)
            m_layers[i][j] = other.m_layers[i][j];
    }
    obCorrection = 0x0;
}

/*!
 * \brief MyClass::operator =
 *        The copy assignment
 */
Dataset& Dataset::operator=( const Dataset& tocopy){

    // Avoid self assignment
    if ( this != &tocopy ) {

        // Make a copy, only used to swap layers, can't use 'tocopy'
        Dataset copy(tocopy);
        // And swap data
        std::swap(this->m_layers, copy.m_layers);

        // Still the rest of the properties need to be copied
        this->m_boundingBox = copy.m_boundingBox;
        this->m_scores = copy.m_scores,
                this->m_frameLayouts = copy.m_frameLayouts,
                this->m_frameOrientation = copy.m_frameOrientation,
                this->m_thresholdsToIndices = copy.m_thresholdsToIndices,
                //this->m_layers = tocopy.m_layers;
                this->m_nx = copy.x();
        this->m_ny = copy.y();
        this->m_nFrames = copy.getFrameCount();
        this->m_pixelDepthBits = copy.getPixelDepthBits();
        //*this->obCorrection = *tocopy.obCorrection;

    }

    return *this;
}

void Dataset::zero(){
    for(int i = 0; i < m_layers.size(); i++){
        for(unsigned int j = 0; j < getPixelsPerLayer(); j++)
            m_layers[i][j] = 0;
    }
}

int Dataset::getLayerIndex(int threshold){
    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        layerIndex = newLayer(threshold);
    return layerIndex;
}

QByteArray Dataset::toByteArray() {

    // 68 bit header start offset
    QByteArray ret(0);
    ret += QByteArray::fromRawData((const char*)&m_nx, (int)sizeof(m_nx));
    ret += QByteArray::fromRawData((const char*)&m_ny, (int)sizeof(m_ny));
    ret += QByteArray::fromRawData((const char*)&m_nFrames, (int)sizeof(m_nFrames));
    int layerCount = m_layers.size();
    ret += QByteArray::fromRawData((const char*)&layerCount, (int)sizeof(layerCount));
    ret += QByteArray::fromRawData((const char*)m_frameLayouts.data(),(int)(m_nFrames*sizeof(*m_frameLayouts.data())));
    ret += QByteArray::fromRawData((const char*)m_frameOrientation.data(), (int)(m_nFrames*sizeof(*m_frameOrientation.data())));
    // Keys are Thresholds
    QList<int> keys = m_thresholdsToIndices.keys();
    ret += QByteArray::fromRawData((const char*)keys.toVector().data(),(int)(keys.size()*sizeof(int))); //thresholds
    // 68 bit header end offset
    for(int i = 0; i < keys.length(); i++)
        ret += QByteArray::fromRawData((const char*)this->getLayer(keys[i]), (int)(sizeof(float)*getLayerSize()));

    return ret;
}

/**
 * This serializes all the layers in a single vector of integers.
 */
QVector<int> Dataset::toQVector() {

    QVector<int> tovec;

    QList<int> keys = m_thresholdsToIndices.keys();
    for(int i = 0; i < keys.length(); i++) {
        int * layer = this->getLayer(keys[i]);
        for ( uint64_t j = 0 ; j < this->getPixelsPerLayer() ; j++) {
            tovec.append( layer[j] );
        }
    }

    return tovec;
}

void Dataset::saveBIN(QString filename)
{
    // And save
    QFile saveFile(filename);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        //        string messg = "Couldn't open: ";
        //        messg += filename.toStdString();
        //        messg += "\nNo output written.";
        //        QMessageBox::warning ( this, QString("Error saving data"), QString(messg.c_str()) );
        return;
    }
    saveFile.write(toByteArray());
    saveFile.close();
}

/*!
 * \brief Dataset::toTIFF
 * \param filename - absolute file path to save to
 * \remark Writes a 32 bit, greyscale, dynamic sized, vertically flipped TIFF image to `filename`,
 *          writes all thresholds (TODO: check this writes all thresholds)
 *
 *          Does cross correction for non spectroscopic mode ONLY - spectroscopic images have a non constant ratio??? #SpecialPixels
 */
void Dataset::toTIFF(QString filename, bool crossCorrection, bool spatialOnly)
{
    //! http://research.cs.wisc.edu/graphics/Courses/638-f1999/libtiff_tutorial.htm
    //! http://ridl.cfd.rit.edu/products/manuals/Leach/new/Drivers/ARC_API_SRC_3/3.0/CTiffFile/CTiffFile.cpp
    //! https://schneide.wordpress.com/2015/11/16/multi-page-tiffs-with-cpp/
    //!
    //! These are all terrible references.


    //----------------------------------------------------
    const static int SAMPLES_PER_PIXEL = 1; // This is greyscale - 3 for RGB, 4 for RGBA
    // Maybe make this the number of thresholds?
    const static int BPP        = 32; // Bits per pixel - gives ~4 billion per pixel upper limit before loss of signal
    const int extraPixels       = 2;
    int width                   = getWidth()+2*extraPixels;  // Should always be 512 or 256 for a quad without spatial correction
    int height                  = getHeight()+2*extraPixels; // ""
    float edgePixelMagicNumber  = float(2.8);
    float edgePixelMagicNumberSpectro = float(2.3);
    float edgePixelMagicNumberSpectroVertical = float(1.9);
    float edgePixelMagicNumberSpectroHorizontal = float(1.1);

    if (filename.isEmpty()){
        qDebug() << ">> ERROR empty filename, cancelling.";
        return;
    }

    //! Error checking
    // Should always be an exact multiple of 256x256 (128kB) or 260x260 (~130kB) (256+2*extraPixels)^2 exactly
//    tsize_t tTotalDataSize = width * height * SAMPLES_PER_PIXEL * sizeof( uint32_t );
//    if (!((tTotalDataSize % 131072) == 0 || (tTotalDataSize % 133128) == 0)){
//        qDebug() << "[ERROR] TIFF size check FAILED : " <<  tTotalDataSize;
//    } /*else {
//        qDebug() << "[INFO] TIFF size check passed : " <<  tTotalDataSize;
//    }*/

    if (spatialOnly){
        edgePixelMagicNumber = 1;
        edgePixelMagicNumberSpectro = 1;
        edgePixelMagicNumberSpectroHorizontal = 1;
        edgePixelMagicNumberSpectroVertical = 1;
    }
    //! Save for all thresholds in separate files
    //! Note: Could use TIFF pages but this is a much clearer approach for the user and is more compatible cross systems
    QList<int> thresholds = m_thresholdsToIndices.keys();
    for(int i = 0; i < thresholds.length(); i++) {

        uint32_t image[height*width];
        uint32_t imageCorrected[height*width];
        TIFF * m_pTiff = nullptr;
        QString tmpFilename = filename;

        //! Default mode - do cross and spatial corrections
        if (crossCorrection){
            //! Normal Fine Pitch mode - do cross correction
//            qDebug() << getThresholds().count();
            if (getThresholds().count() == 1) {
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
                for (int y=0; y < height; y++) {
                    for (int x=0; x < width; x++) {
                        //! Sample the pixels directly
                        image[y*width + x] = sample(x, y, thresholds[i]);
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
                        /*if (y > (height/2)-extraPixels-1 && y <= (height/2)+extraPixels ) {
                                //qDebug() << "[CENTRAL] " << x << y;
                                continue;

                            } else*/ if (x == (width/2)-extraPixels ) {                              // L
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
                        //int tmp = sample(x,y, thresholds[i]) / edgePixelMagicNumber;
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
            width  = getWidth();
            height = getHeight();
//            qDebug() << getThresholds().count();

            for (int y=0; y < height; y++) {
                for (int x=0; x < width; x++) {
                    //! Sample the pixels directly
                    image[y*width + x] = sample(x, y, thresholds[i]);
                }
            }
            if (getThresholds().count() > 1){
                tmpFilename.replace(".tiff", "-thl" + QString::number(thresholds[i]) + ".tiff");
            }
        }

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

            if (crossCorrection){
                TIFFSetField(m_pTiff, TIFFTAG_IMAGEWIDTH,      width);                  // set the width of the image
                TIFFSetField(m_pTiff, TIFFTAG_IMAGELENGTH,     height);                 // set the height of the image
                for (int r=0; r < height; r++) {
                    TIFFWriteScanline(m_pTiff, &imageCorrected[r*width], r, 0);
                }
//                qDebug() << "[INFO] Written corrected TIFF";
            } else {
                TIFFSetField(m_pTiff, TIFFTAG_IMAGEWIDTH,      getWidth());                  // set the width of the image
                TIFFSetField(m_pTiff, TIFFTAG_IMAGELENGTH,     getHeight());                 // set the height of the image
                for (int r=0; r < getHeight(); r++) {
                    TIFFWriteScanline(m_pTiff, &image[r*getWidth()], r, 0);
                }
//                qDebug() << "[INFO] Written raw TIFF";
            }


        } else if (m_pTiff == nullptr) {
            qDebug() << "[ERROR] Unable to write TIFF file";
        } else {
            qDebug() << "[ERROR] Unknown TIFF file write failure.";
        }

        //! Cleanup code - close the file when done
        if ( m_pTiff ) {
            TIFFClose( m_pTiff );
        }
    }

}

//! TODO Could do corrections here
QVector<int> Dataset::makeFrameForSaving(int threshold, bool crossCorrection, bool spatialOnly)
{
    //----------------------------------------------------
    const int extraPixels       = 2;
    int width                   = getWidth()+2*extraPixels;  // Should always be 512 or 256 for a quad without spatial correction
    int height                  = getHeight()+2*extraPixels; // ""
    float edgePixelMagicNumber  = float(2.8);
    float edgePixelMagicNumberSpectro = float(2.3);
    float edgePixelMagicNumberSpectroVertical = float(1.9);
    float edgePixelMagicNumberSpectroHorizontal = float(1.1);

    QList<int> thresholds = m_thresholdsToIndices.keys();
    QVector<int> image(height*width);
    QVector<int> imageCorrected(height*width);
    //----------------------------------------------------

    if (spatialOnly){
        edgePixelMagicNumber = 1;
        edgePixelMagicNumberSpectro = 1;
        edgePixelMagicNumberSpectroHorizontal = 1;
        edgePixelMagicNumberSpectroVertical = 1;
    }

    //! Default mode - do cross and spatial corrections
    if (crossCorrection){
        //! Normal Fine Pitch mode - do cross correction
        if (getThresholds().count() == 1) {
            for (int y=0; y < height; y++) {
                for (int x=0; x < width; x++) {
                    if (x==(width/2)-1 || x==(width/2) || y==(height/2)-1 || y==(height/2)){
                        //! Hard coded cross correction
                        image[y*width + x] = int (sample(x, y, thresholds[threshold]) / edgePixelMagicNumber );
                    } else {
                        //! Default option, sample the pixels directly
                        image[y*width + x] = sample(x, y, thresholds[threshold]);
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
            for (int y=0; y < height; y++) {
                for (int x=0; x < width; x++) {
                    //! Sample the pixels directly
                    image[y*width + x] = sample(x, y, thresholds[threshold]);
                }
            }
        }

        //! Phase 1
        //! Do spatial correction on quadrants, move to respective corners
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {

                if (y < (height/2)-extraPixels && x < (width/2)-extraPixels) {      // TL
                    imageCorrected[y*width + x] = sample(x, y, thresholds[threshold]);
                } else if (y < (height/2)-extraPixels && x >= (width/2)+extraPixels) {      // TR
                    imageCorrected[y*width + x] = sample(x-2*extraPixels, y, thresholds[threshold]);
                } else if (y >= (height/2)+extraPixels && x < (width/2)-extraPixels) {      // BL
                    imageCorrected[y*width + x] = sample(x, y-2*extraPixels, thresholds[threshold]);
                } else if (y >= (height/2)+extraPixels && x >= (width/2)+extraPixels) {     // BR
                    imageCorrected[y*width + x] = sample(x-2*extraPixels, y-2*extraPixels, thresholds[threshold]);
                }

            }
        }
        //! Phase 2
        //! Loop for cross
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                if (x >= (width/2)-extraPixels && x <= (width/2)+extraPixels){               // vertical centre
                    /*if (y > (height/2)-extraPixels-1 && y <= (height/2)+extraPixels ) {
                            //qDebug() << "[CENTRAL] " << x << y;
                            continue;

                        } else*/ if (x == (width/2)-extraPixels ) {                              // L
                        //Set 255, 256, 257 as 1/2.8 of 255
                        // qDebug() << "[L]" << tmp << x << y;
                        if (y <= height/2 - extraPixels || y >= height/2 + extraPixels + 1){

                            int tmp;
                            if (width == 260){ // If spectro mode
                                tmp = int (imageCorrected[y*width + x-1] / edgePixelMagicNumberSpectroVertical);
                            } else {
                                tmp = int(imageCorrected[y*width + x-1] / edgePixelMagicNumber);
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
                                tmp = int(imageCorrected[y*width + x] / edgePixelMagicNumberSpectroVertical);
                            } else {
                                tmp = int(imageCorrected[y*width + x] / edgePixelMagicNumber);
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
                                tmp = int(imageCorrected[(y-1)*width + x] / edgePixelMagicNumberSpectroHorizontal);
                            } else {
                                tmp = int(imageCorrected[(y-1)*width + x] / edgePixelMagicNumber);
                            }

                            //qDebug() << "[T]" << tmp << x << y;
                            imageCorrected[(y-1)*width + x  ] = tmp;
                            imageCorrected[(y )*width +  x  ] = tmp;
                            imageCorrected[(y+1)*width + x  ] = tmp;
                        } else if (x >= width/2 + extraPixels + 1) {                         // TR
                            int tmp;
                            if (width == 260){ // If spectro mode
                                tmp = int(imageCorrected[(y-1)*width + x] / edgePixelMagicNumberSpectro);
                            } else {
                                tmp = int(imageCorrected[(y-1)*width + x] / edgePixelMagicNumber);
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
                                tmp = int(imageCorrected[(y)*width + x] / edgePixelMagicNumberSpectro);
                            } else {
                                tmp = int(imageCorrected[(y)*width + x] / edgePixelMagicNumber);
                            }
                            //qDebug() << "[B]" << tmp << x << y;
                            imageCorrected[(y-2)*width + x  ] = tmp;
                            imageCorrected[(y-1)*width + x  ] = tmp;
                            imageCorrected[(y  )*width + x  ] = tmp;
                        } else if (x >= width/2 + extraPixels + 1) {                         // BR
                            int tmp;
                            if (width == 260){ // If spectro mode
                                tmp = int(imageCorrected[(y)*width + x] / edgePixelMagicNumberSpectroHorizontal);
                            } else {
                                tmp = int(imageCorrected[(y)*width + x] / edgePixelMagicNumber);
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
                    //int tmp = int(sample(x,y, thresholds[threshold]) / edgePixelMagicNumber);
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
        width  = getWidth();
        height = getHeight();

        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                //! Sample the pixels directly
                image[y*width + x] = sample(x, y, thresholds[threshold]);
            }
        }
    }

    // -------------------------------------------------------------------
    if (crossCorrection || spatialOnly) {
        return imageCorrected;
    } else {
        return image;
    }
}

void Dataset::toASCII(QString filename)
{
    int sizex = x();
    int sizey = y();
    int nchipsx =  getNChipsX();
    int nchipsy = getNChipsY();
    int len = sizex * sizey * nchipsx * nchipsy;

    QList <int> thresholds = getThresholds();
//    qDebug() << thresholds;
    QList<int>::iterator it = thresholds.begin();
    QList<int>::iterator itE = thresholds.end();

    const int width   = getWidth();  // Should always be 512 or 256 for a quad without spatial correction
    const int height  = getHeight(); // ""
    uint32_t fullFrame[height*width];

    // Do the different thresholds
    for (; it != itE; it++) {
        memset(fullFrame, 0, sizeof(fullFrame));

        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                //! Sample the pixels directly
                fullFrame[y*width + x] = sample(x, height-y-1, *it);
            }
        }

        QString tmp = filename.replace(".txt","");
        tmp.append("_thl-");
        tmp.append(QString::number(*it, 'd', 0));
        tmp.append(".txt");
        string saveLoc = tmp.toStdString();

        //qDebug() << "nchipsx : " << nchipsx << " | nchipsy : " << nchipsy << " --> " << getDataset()->getPixelsPerLayer();

        // Save file
        ofstream of;
        of.open(saveLoc);
        if (of.is_open()) {
            for (int i = 0; i < len; i++) {

                of << fullFrame[i] << " ";

                // new line
                if ((i + 1) % (sizex*nchipsx) == 0) of << "\r\n";

            }
        }

        of.close();

    } // end of for loop that cycles through the layers (threshold)
}

QPointF Dataset::XtoXY(int X, int dimX){
    return QPointF(X % dimX, X/dimX);
}

void Dataset::calcBasicStats(QPoint pixel_init, QPoint pixel_end) {

    //Delete previous stats from other regions
    bstats.mean_v.clear();
    bstats.stdev_v.clear();

    QList<int> keys = m_thresholdsToIndices.keys(); //Layerindices

    //QSize isize = QSize(computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y()); //For what?

    // Region of interest. Find corners.
    int minx = pixel_init.x();
    int maxx = pixel_end.x();
    if ( minx > maxx ) {
        minx = pixel_end.x();
        maxx = pixel_init.x();
    }

    int miny = pixel_init.y();
    int maxy = pixel_end.y();
    if ( miny > maxy ) {
        miny = pixel_end.y();
        maxy = pixel_init.y();
    }

    // Set in bstats to use somewhere else
    bstats.init_pixel = pixel_init;
    bstats.end_pixel = pixel_end;

    double nMean, mean, stdev;

    // Mean
    for(int i = 0; i < keys.length(); i++) {
        mean = 0.;
        nMean = 0.;

        for(int y = miny; y <= maxy; y++){
            for(int x = minx; x <= maxx; x++){
                mean += sample( x, y, keys[i]); //pixel_init is upper left corner.
                nMean++;
            }
        }
        if(nMean != 0) mean /= nMean;
        bstats.mean_v.push_back(mean);
    }

    // Standard Deviation
    for(int i = 0; i < keys.length(); i++) {
        stdev = 0.;
        nMean = 0.;

        for(int y = miny; y <= maxy; y++){
            for(int x = minx; x <= maxx; x++){
                int val = sample( x, y, keys[i] );
                stdev += ( val - bstats.mean_v[i] ) * ( val - bstats.mean_v[i] );
                nMean++;
            }
        }
        if ( nMean != 1 ) stdev /= (nMean - 1);
        stdev = sqrt(stdev);
        bstats.stdev_v.push_back(stdev);

    }
}

bool Dataset::isBorderPixel(int p, QSize isize) {

    if (
            p < isize.width() 	// lower edge: 0 --> width-1
            ||
            p >= ( isize.width()*isize.height() - isize.width() ) // upper edge: width*height-width --> width*height
            ||
            ( ( p % (isize.width()-1) ) == 0 ) // right edge
            ||
            ( ( p %  isize.width()    ) == 0 ) // left edge
            ) return true;

    return false;
}

bool Dataset::isBorderPixel(int x, int y, QSize isize) {

    if ( y <= 0 ) return true;					// lower edge
    if ( y >= isize.height() - 1 ) return true;	// upper edge
    if ( x <= 0 ) return true;					// left edge
    if ( x >= isize.width()  - 1 ) return true;	// right edge

    return false;
}

//! Interhigh interpolation
void Dataset::applyHighPixelsInterpolation(double meanMultiplier, QMap<int, double> meanvals) {

    QList<int> keys = m_thresholdsToIndices.keys();
    // computeBoundingBox().size().width() 	--> gives the number of chips
    // this->x() 							--> gives the number of pixels per chip
    QSize isize = QSize( computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y() );

    for(int i = 0; i < keys.length(); i++) {

        double mean = meanvals[ keys[i] ];


        for(int y = 0 ; y < isize.height() ; y++) {

            for(int x = 0 ; x < isize.width() ; x++) {

                // If a pixel is higher than meanMultiplier times the mean
                if ( sample(x, y, keys[i]) > qRound(meanMultiplier * mean) ) {

                    // Then count how many pixels around this pixel have a value lower than its own value

                    // Check first how many of it's neighbors are active beyond the multiplier * mean
                    map< pair<int, int>, int > activeBellow = activeNeighbors(x, y, keys[i], isize, __less, qRound(meanMultiplier * mean) );

                    // Request at least three active neighbors to consider filling up by averaging.
                    // In the dead-pixel correction we request only 2. We may end up eating corners
                    //   of a distinctive structure which is an actual part of the image.
                    // This means we request a hot pixel almost fully isolated.
                    if ( activeBellow.size() >= 3 ) {
                        setPixel(x, y, keys[i], averageValues( activeBellow ) );
                    }

                }

            } // x

        } // y

    } // layers

}

//! Interlow interpolation
void Dataset::applyDeadPixelsInterpolation(double meanMultiplier, QMap<int, double> meanvals) {

    // The keys are the thresholds
    QList<int> keys = m_thresholdsToIndices.keys();

    QSize isize = QSize(computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y());

    for(int i = 0; i < keys.length(); i++) {

        double mean = meanvals[ keys[i] ];


        for(int y = 0 ; y < isize.height() ; y++) {

            for(int x = 0 ; x < isize.width() ; x++) {

                // is dead pixel
                if ( sample(x, y, keys[i]) == 0 ) {

                    // Check first how many of its neighbors are active
                    map< pair<int, int>, int > actives = activeNeighbors(x, y, keys[i], isize, __bigger, 0);
                    // And check how many of its neighbors are not noisy
                    map< pair<int, int>, int > notNoisy = activeNeighbors(x, y, keys[i], isize, __less, qRound(meanMultiplier * mean) );
                    // The average will be made on the intersection of actives and notNoisy --> the good ones
                    std::vector<int> toAverage = getIntersection(actives, notNoisy);

                    // Request at least two active neighbors to consider filling up by averaging
                    if ( toAverage.size() >= 2 ) {
                        // It is a problem when a dead pixel is close to a noisy pixel.
                        // The noisy pixel should not be taken into account.
                        setPixel(x, y, keys[i], vectorAverage( toAverage ) );
                    }

                }

            } // x
        } // y

    } // layers


}

void Dataset::DumpSmallMap(map< pair<int, int>, int > m1) {

    map< pair<int, int>, int >::iterator i = m1.begin();
    map< pair<int, int>, int >::iterator iE = m1.end();
    for ( ; i != iE ; i++ ) {
        cout << " | " << (*i).first.first << "," << (*i).first.second << "=" << (*i).second;
    }
    //cout << endl;
}

std::vector<int> Dataset::getIntersection(map< pair<int, int>, int > m1, map< pair<int, int>, int > m2) {

    std::vector<int> intersection;

    // loop over m1 and find the matches in m2
    map< pair<int, int>, int >::iterator i = m1.begin();
    map< pair<int, int>, int >::iterator iE = m1.end();
    map< pair<int, int>, int >::iterator f;

    for ( ; i != iE ; i++ ) {

        // If match found
        if ( m2.find( (*i).first ) != m2.end() ) intersection.push_back( (*i).second );

    }

    return intersection;
}

int Dataset::averageValues(map< pair<int, int>, int > m1) {

    double av = 0;

    map< pair<int, int>, int >::iterator i = m1.begin();
    map< pair<int, int>, int >::iterator iE = m1.end();

    for ( ; i != iE ; i++ ) {
        av += (*i).second;
    }
    av /= ( (double)(m1.size()) );

    return qRound(av);
}

int Dataset::vectorAverage(std::vector<int> v) {

    double av = 0;
    std::vector<int>::iterator i  = v.begin();
    std::vector<int>::iterator iE = v.end();
    for ( ; i != iE ; i++ ) {
        av += *i;
    }
    av /= ( (double)(v.size()) );

    return qRound(av);
}


void Dataset::appendSelection(int x, int y, int thl, int compareto, comp c, map< pair<int, int>, int > & activeNeighbors) {

    int val = sample(x, y, thl);

    switch ( c ) {
    case __less:
        if ( val < compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    case __lesseq:
        if ( val <= compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    case __equal:
        if ( val == compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    case __bigger:
        if ( val > compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    case __biggereq:
        if ( val >= compareto ) {
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    default:
        if ( val > compareto ) {		// __bigger
            activeNeighbors[ make_pair(x,y) ] = val;
        }
        break;
    }

}

map< pair<int, int>, int > Dataset::activeNeighbors(int x, int y, int thl, QSize isize, comp c, int activeValue) {

    map< pair<int, int>, int > activeNeighbors;

    // If the pixel is not in the border.  Most probable.
    if ( ! isBorderPixel(x, y, isize) ) {

        appendSelection( x, y+1, thl, activeValue, c, activeNeighbors );
        appendSelection( x, y-1, thl, activeValue, c, activeNeighbors );
        appendSelection( x+1, y, thl, activeValue, c, activeNeighbors );
        appendSelection( x-1, y, thl, activeValue, c, activeNeighbors );
        //if ( sample(x, y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x, y+1, thl) );
        //if ( sample(x, y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x, y-1, thl) );
        //if ( sample(x+1, y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1, y, thl) );
        //if ( sample(x-1, y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1, y, thl) );
        return activeNeighbors;

    }
    // Bottom edge
    if ( y == 0 && x > 0 && x < (isize.width()-1) ) {

        appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
        appendSelection( x  , y+1, thl, activeValue, c, activeNeighbors );
        appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
        //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
        //if ( sample(x  , y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y+1, thl) );
        //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
        return activeNeighbors;
    }
    // Left edge
    if (  y > 0 && y < (isize.height()-1) && x == 0 ) {
        appendSelection( x  , y+1, thl, activeValue, c, activeNeighbors );
        appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
        appendSelection( x  , y-1, thl, activeValue, c, activeNeighbors );

        //if ( sample(x  , y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y+1, thl) );
        //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
        //if ( sample(x  , y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y-1, thl) );
        return activeNeighbors;
    }
    // Top edge
    if (  y == (isize.height()-1) && x > 0 && x < (isize.width()-1) ) {
        appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
        appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
        appendSelection( x  , y-1, thl, activeValue, c, activeNeighbors );
        //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
        //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
        //if ( sample(x  , y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y-1, thl) );
        return activeNeighbors;
    }
    // Right edge
    if (  x == (isize.width()-1) && y > 0 && y < (isize.height()-1) ) {
        appendSelection( x  , y+1, thl, activeValue, c, activeNeighbors );
        appendSelection( x  , y-1, thl, activeValue, c, activeNeighbors );
        appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
        //if ( sample(x  , y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y+1, thl) );
        //if ( sample(x  , y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y-1, thl) );
        //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
        return activeNeighbors;
    }

    // Four corners
    if (  (x % (isize.width()-1) == 0)  &&  (y % (isize.height()-1) == 0)  ) {

        if ( x == 0 && y == 0) {  // left bottom
            appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
            appendSelection( x,   y+1, thl, activeValue, c, activeNeighbors );
            appendSelection( x+1, y+1, thl, activeValue, c, activeNeighbors );
            //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
            //if ( sample(x,   y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x,   y+1, thl) );
            //if ( sample(x+1, y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x+1, y+1, thl) );
        } else if ( x == 0 && y == (isize.height()-1) ) {  // left top
            appendSelection( x+1,   y, thl, activeValue, c, activeNeighbors );
            appendSelection( x,   y-1, thl, activeValue, c, activeNeighbors );
            appendSelection( x+1, y-1, thl, activeValue, c, activeNeighbors );
            //if ( sample(x+1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x+1,   y, thl) );
            //if ( sample(x,   y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x,   y-1, thl) );
            //if ( sample(x+1, y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x+1, y-1, thl) );
        } else if ( x == (isize.width()-1) && y == (isize.height()-1) ) {  // right top
            appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
            appendSelection( x-1, y-1, thl, activeValue, c, activeNeighbors );
            appendSelection( x  , y-1, thl, activeValue, c, activeNeighbors );
            //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
            //if ( sample(x-1, y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x-1, y-1, thl) );
            //if ( sample(x  , y-1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y-1, thl) );
        } else if ( x == (isize.width()-1) && y == 0 ) {  // right bottom
            appendSelection( x-1,   y, thl, activeValue, c, activeNeighbors );
            appendSelection( x-1, y+1, thl, activeValue, c, activeNeighbors );
            appendSelection( x  , y+1, thl, activeValue, c, activeNeighbors );
            //if ( sample(x-1,   y, thl) > activeValue ) activeNeighbors.push_back( sample(x-1,   y, thl) );
            //if ( sample(x-1, y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x-1, y+1, thl) );
            //if ( sample(x  , y+1, thl) > activeValue ) activeNeighbors.push_back( sample(x  , y+1, thl) );
        }
    }

    return activeNeighbors;
}

void Dataset::applyCorrections(QCstmCorrectionsDialog * corrdiag) {

    if ( ! corrdiag ) return;

    //    if ( corrdiag->isCorrectionsActive() ) {  //Always false. previously set by checkbox.
    QMap<int, double> meanvals;// = Dataset::GetPadMean();

    // Corrections
    if ( corrdiag->isSelectedDeadPixelsInter() ) applyDeadPixelsInterpolation( corrdiag->getNoisyPixelMeanMultiplier(), meanvals );
    if ( corrdiag->isSelectedHighPixelsInter() ) applyHighPixelsInterpolation( corrdiag->getNoisyPixelMeanMultiplier(), meanvals );

    if ( corrdiag->isSelectedOBCorr() ) {
        applyOBCorrection();
    }
}

void Dataset::applyOBCorrection() {

    // Check that the OB correction data has been loaded by the user
    if (obCorrection == nullptr)
        return;

    //bool corrected = true;
    bool OBmatch = true;
    bool use_k = false;
    double k = 1;

    QList<int> keys = m_thresholdsToIndices.keys();
    for (int i = 0; i < keys.length(); i++) {
        //double currentTotal = getTotal(keys[i]), correctionTotal = correction->getTotal(keys[i]);
        int * currentLayer = getLayer(keys[i]);
        int * correctionLayer = obCorrection->getLayer(keys[i]);
        if (correctionLayer == nullptr) {
            qDebug() << "[WARN] flat-field correction does not contain a threshold" << keys[i];
            continue;
        }

        // Let the operation happen in float point and then we'll normalize to pass to the int map.
        // Allocate some scratch memory
        double * normFrame = new double[getPixelsPerLayer()];

        // Find the smallest value.  Initialize it for the search.
        double min = 1.;
        if ( currentLayer[0] > 0 ) min = -1.0*log(((double)currentLayer[0]) / ((double)correctionLayer[0]));

        double max = min;
        double low = 0;
        if ( currentLayer[0] > correctionLayer[0] )  min = currentLayer[0];

        // Setting minimum and maximum of the current Data and OB data.
        calcBasicStats(QPoint(0,0), QPoint(x()*getNChipsX(), y()*getNChipsY()));
        // The acceptance for min and max finding would be
        double mean   = bstats.mean_v[i];
        double stdevs = bstats.stdev_v[i]; //
        //qDebug() << "\nDATA --> Mean: " << mean << " | " << "dev: " << bstats.stdev_v[i];
        double Dmin = mean;
        double Dmax = mean;

        // Get the values for the OB correction
        obCorrection->calcBasicStats(QPoint(0,0), QPoint(x()*getNChipsX(), y()*getNChipsY()));
        double meanOB   = obCorrection->bstats.mean_v[i];
        double stdevsOB = obCorrection->bstats.stdev_v[i]; //
        //qDebug() << "\nOB   --> Mean: " << meanOB << " | " << "dev: " << obCorrection->bstats.stdev_v[i];

        double OBmin = meanOB;
        double OBmax = meanOB;

        // This needs to be done avoiding noisy pixels
        for (unsigned int j = 0; j < getPixelsPerLayer(); j++) {

            //Determine minimum and maximum of the current Data and OB Data.
            if( currentLayer[j] > mean - stdevs && currentLayer[j] < mean + stdevs ) {
                if((double)currentLayer[j]<Dmin) Dmin = currentLayer[j];
                if((double)currentLayer[j]>Dmax) Dmax = currentLayer[j];
            }
            if ( correctionLayer[j] > meanOB - stdevsOB && correctionLayer[j] < meanOB + stdevsOB) {
                if((double)correctionLayer[j]<OBmin) OBmin = correctionLayer[j];
                if((double)correctionLayer[j]>OBmax) OBmax = correctionLayer[j];
            }

        }

        qDebug() << "Image(min, max): " << Dmin  << ", " << Dmax;
        qDebug() << "OB   (min, max): " << OBmin << ", " << OBmax;

        //To check wether the OB data is comparable and adjust k-factor accordingly.
        //Only change OBmatch at first layer, next layers should be handled the same.

        if( (OBmax-OBmin) <= 0.5*(Dmax - Dmin) || (OBmax-OBmin) >= 2*(Dmax - Dmin)){
            if(i == 0) OBmatch = false; //Only change influence of OBmatch at first layer
            k = Dmax/OBmax;
        }

        /////////////////////// !!!!!!!!!!!!!!!!!!!!!!!!!!
        OBmatch = true;

        //Give the user the choice to apply the correction, ignore the incomparability or cancel and choose another OBcorrectionfile.
        //Only ask at first layer.
        //        if ( ! OBmatch && i == 0 ) {
        //            qDebug() << "[FAIL] if( (OBmax-OBmin) <= 0.5*(Dmax - Dmin) || (OBmax-OBmin) >= 2*(Dmax - Dmin))";
        //            qDebug() << "(OBmax-OBmin)" << (OBmax-OBmin);
        //            qDebug() << "0.5*(Dmax - Dmin)" << 0.5*(Dmax - Dmin);
        //            qDebug() << "2*(Dmax - Dmin)" << 2*(Dmax - Dmin);

        //            QMessageBox msgBox(QMessageBox::Question, "Warning", "The statistics in the OB data and the current data are such that the OB correction will not yield a good image.\n"
        //                                                                 "- Press 'Cancel' to stop and choose a more compatible OB datafile.\n"
        //                                                                 "- It is also possible to apply a k-factor on the OB correction to make it match the current data.\n"
        //                                                                 "- Press 'Continue' to use the selected OB file as it is.",
        //                               QMessageBox::Yes | QMessageBox::No |QMessageBox::Cancel | QMessageBox::Cancel,0);


        //            msgBox.setButtonText(QMessageBox::Yes, "Apply k-factor");
        //            msgBox.setButtonText(QMessageBox::No, "Continue");
        //            int reply = msgBox.exec();

        //            //Continue with correction:
        //            if(reply == QMessageBox::Yes){
        //                OBmatch = true;
        //                use_k = true;
        //            }
        //            //Continue without correction:
        //            if(reply == QMessageBox::No){
        //                OBmatch = true;
        //                use_k = false;
        //            }

        //        }
        if(OBmatch){
            for (unsigned int j = 0; j < getPixelsPerLayer(); j++) {

                if (currentLayer[j] != 0)
                {
                    if (correctionLayer[j] > 0 && currentLayer[j] > 0) {

                        //Calculation of the correction with and without k-factor
                        if (  use_k ) normFrame[j] = -1.0*log(((double)currentLayer[j]) / (k*(double)correctionLayer[j]));
                        if ( !use_k ) normFrame[j] = -1.0*log(((double)currentLayer[j]) / ((double)correctionLayer[j]));

                        //set Minimum. Value closest to 0 is taken.
                        if (std::abs(normFrame[j]) < min && normFrame[j] != 0)
                            min = std::abs(normFrame[j]);
                        if (std::abs(normFrame[j]) > max && normFrame[j] != 0)
                            max = normFrame[j];
                        if (normFrame[j] < low)
                            low = normFrame[j];
                    }
                    else {
                        currentLayer[j] = 0;
                    }
                }
            }

            // Calculates the amount of decimals before the first digit of the minimum. eg: 0.03 -> 2.
            // this ensures that all values can be converted to integers without losing data.
            int correctionFactor = (int)-floor(log10(min));
            int offset = (int)(std::abs(low)*pow(10.0, correctionFactor));

            //To calculate the range in the corrected values
            double Cmin = offset + round(min*pow(10.0, correctionFactor));
            double Cmax = offset + round(max*pow(10.0, correctionFactor));

            cout << std::setprecision(10) << "low    : " << low << endl;
            cout << std::setprecision(10) << "offset : " << offset << endl;
            cout << std::setprecision(10) << "min    : " << (double)min << endl;
            cout << std::setprecision(10) << "max    : " << (double)max << endl;
            cout << std::setprecision(10) << "correction : " << correctionFactor << endl;
            cout << std::setprecision(10) << "Data Range    : " << (double)Dmax - (double)Dmin << endl;
            cout << std::setprecision(10) << "OB Range   : " << (double)OBmax - (double)OBmin << endl;
            cout << std::setprecision(10) << "Corr. Ln Range   : " << (double)max - (double)low << endl;
            cout << std::setprecision(10) << "Corr. Range   : " << (double)Cmax - (double)Cmin << endl;

            for (unsigned int j = 0; j < getPixelsPerLayer(); j++) {
                if (currentLayer[j] != 0)
                    currentLayer[j] = offset + round(normFrame[j] * pow(10.0, correctionFactor));
                //if (currentLayer[j] < 0)
                //    cout << j << endl;
            }
        }
        delete[] normFrame;

    }

}

void Dataset::dumpAllActivePixels() {


    QList<int> keys = m_thresholdsToIndices.keys();
    std::ofstream ofs ("test.txt", std::ofstream::out);

    for (int i = 0; i < keys.length(); i++) {

        int * currentLayer = getLayer(keys[i]);

        for (unsigned int j = 0; j < getPixelsPerLayer(); j++) {

            if ( currentLayer[j] > 0 ) ofs << j << " ";

        }

    }

}

void Dataset::fromByteArray(QByteArray serialized){
    QDataStream in(&serialized, QIODevice::ReadOnly);
    in.readRawData((char*)&m_nx, (int)sizeof(m_nx));
    in.readRawData((char*)&m_ny, (int)sizeof(m_ny));
    in.readRawData((char*)&m_nFrames, (int)sizeof(m_nFrames)); // nChips
    setFramesPerLayer(m_nFrames);
    int layerCount;
    in.readRawData((char*)&layerCount, (int)sizeof(layerCount));
    //setLayerCount(layerCount);
    in.readRawData((char*)m_frameLayouts.data(), m_nFrames*(int)sizeof(*m_frameLayouts.data()));
    in.readRawData((char*)m_frameOrientation.data(), m_nFrames*(int)sizeof(*m_frameOrientation.data()));
    QVector<int> keys(layerCount);
    in.readRawData((char*)keys.data(), keys.size()*(int)sizeof(int));
    QVector<int> frameBuffer(m_nx*m_ny);
    for(int i = 0; i < keys.size(); i++){
        for(int j = 0; j < m_nFrames; j++){
            in.readRawData((char*)frameBuffer.data(), (int)sizeof(float)*frameBuffer.size());
            this->setFrame(frameBuffer.data(), j, keys[i]);
        }
    }
}

void Dataset::fromASCIIMatrixGetSizeAndLayers(QFile * file, int *x, int *y, int *framesPerLayer)
{

    QTextStream in(file);
    QString line;
    int rowCntr = 0, colCntr = 0;
    int cols = 0;
//    bool ok = false;

    while ( in.readLineInto( &line ) ) {
        QStringList values = line.split('\t',QString::SkipEmptyParts);
        QStringList::const_iterator itr = values.begin();
        for ( ; itr != values.end() ; itr++ ) {
            // values are forced to be integers
            //int val = (int) ( (*itr).toDouble( &ok ) );
            // check the convertion
            //if ( !ok ) return;
            colCntr++;
        }
        cols = colCntr; // save the value
        colCntr = 0;
        rowCntr++;
    }

    *x = cols;
    *y = rowCntr;
    *framesPerLayer = 1; // Not loading chips yet // TODO
}

void Dataset::fromASCIIMatrix(QFile * file, int x, int y, int framesPerLayer)
{

    QTextStream in(file);
    in.seek(0); // go to the beginning, this descriptor has been scanned before
    QString line;
    int rowCntr = 0, colCntr = 0;
    bool ok = false;

    // Fill data structure
    m_nx = x;
    m_ny = y;
    m_nFrames = framesPerLayer;
    int layerCount = 1;         // Not loading layers yet // TODO
    m_frameLayouts[0] = QPoint(0,0);
    m_frameOrientation[0] = orientationLtRTtB;
    QVector<int> keys(layerCount);
    keys[0] = 0;
    QVector<int> frameBuffer(m_nx*m_ny);

    rowCntr = 0; colCntr = 0;
    // read data now
    while ( in.readLineInto( &line ) ) {
        QStringList values = line.split('\t',QString::SkipEmptyParts);
        QStringList::const_iterator itr = values.begin();
        for ( ; itr != values.end() ; itr++ ) {
            // values are forced to be integers
            int val = (int) ( (*itr).toDouble( &ok ) );
            if ( !ok ) return;
            // Pass to 1-dim
            frameBuffer[ XYtoX(colCntr, rowCntr, m_nx ) ] = val;
            // check the convertion
            colCntr++;
        }
        colCntr = 0;
        rowCntr++;
    }

    this->setFrame(frameBuffer.data(), 0, keys[0]); // 1 layer, 1 chip

    qDebug() << "Reading from ASCII file:";
    qDebug() << "   Number of rows    : " << m_nx;
    qDebug() << "   Number of columns : " << m_ny;

}

void Dataset::clear() {

    for(int i =0; i < m_layers.size(); i++){
        delete[] m_layers[i];
    }
    m_layers.clear();
    m_thresholdsToIndices.clear();

    // scores
    rewindScores();

    //setFramesPerLayer(1);
}

int Dataset::getNChipsX() {
    QRectF cb = computeBoundingBox();
    return (int)cb.width();
}

int Dataset::getNChipsY() {
    QRectF cb = computeBoundingBox();
    return (int)cb.height();
}

QRectF Dataset::computeBoundingBox(){
    m_boundingBox.setRect(0,0,0,0);
    int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
    for(int i =0; i < m_frameLayouts.length();i++){
        if(m_frameLayouts[i].x() < min_x)
            min_x = m_frameLayouts[i].x();
        if(m_frameLayouts[i].y() < min_y)
            min_y = m_frameLayouts[i].y();
        if(m_frameLayouts[i].x() > max_x)
            max_x = m_frameLayouts[i].x();
        if(m_frameLayouts[i].y() > max_y)
            max_y = m_frameLayouts[i].y();
    }
    m_boundingBox.setRect(min_x,min_y, max_x+1, max_y+1);

    return m_boundingBox;
}

int Dataset::newLayer(int threshold){
    m_thresholdsToIndices[threshold] = m_layers.size();
    m_layers.append(new int[getPixelsPerLayer()]);
    for(int j = 0; j < getLayerSize(); j++)
        m_layers.last()[j] = 0;
    return m_layers.size()-1;
}

void Dataset::runImageCalculator(QString imgOperator, int index1, int index2, int threshold)
{
    int width = getWidth();
    int height = getHeight();
    QList<int> keys = getThresholds();
    int nThresholds = keys.size();
    uint64_t image[height*width];

    //qDebug() << keys << nThresholds;

    if ( nThresholds == 0 ) {
        QMessageBox::information(0,"Error","No thresholds. Failed operation/");
        return;
    }

    if (imgOperator == "-") {
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                image[y*width + x] = sample(x,y,keys[index1]) - sample(x,y,keys[index2]);
                setPixel(x,y,keys[0],image[y*width + x]);
            }
        }
    } else if (imgOperator == "+") {
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                image[y*width + x] = sample(x,y,keys[index1]) + sample(x,y,keys[index2]);
                setPixel(x,y,keys[0],image[y*width + x]);
            }
        }
    } else {
        qDebug() << "[FAIL] Dataset::runImageCalculator --> selected image operator not implemented. Do it.";
        return;
    }


    qDebug() << keys[0] << "-" << keys[1];


    //threshold = 7;

    //! This shiiiiieeet

    /*
    if(!m_thresholdsToIndices.contains(threshold))
        newLayer(threshold);
    int *newFrame = getFrame(index1, threshold);

    for (int i = 0 ; i < m_nx*m_ny;i++) {

        newFrame[i] = image[i];
    }
    */
    //setPixel(0,0,0,1);


//    //! Loop over different layers in the Dataset.
//    for (int i = 0; i < keys.length(); i++){

//    }

//    _ui->visualizationGL->setThreshold(keys.last()+1);
    //    _ui->visualizationGL->active_frame_changed();
}

void Dataset::calcAllEnergyBins()
{
    qDebug() << "[INFO] Dataset::calcAllEnergyBins() : Calculate all energy bins";
}

void Dataset::debugPrintThesholds(int n)
{
    QList<int> keys = m_thresholdsToIndices.keys();

    for (int i = 0; i < keys.length(); i++) {
        QString filename = "test" + QString::number( keys[i] ) + ".txt";
        std::ofstream ofs ( qPrintable(filename), std::ofstream::out);

        int * currentLayer = getLayer(keys[i]);

        for (unsigned int i = 0; i < n; i++) {

            ofs << i << "\t" << currentLayer[i] << "\n";

        }

    }
}

unsigned int Dataset::setFrame(int *frame, int index, int threshold){

    if(!m_thresholdsToIndices.contains(threshold))
        newLayer(threshold);
    int *newFrame = getFrame(index, threshold);

    // and keep an eye on overflow frames
    unsigned int overflowCntr = 0;

    for (int i = 0 ; i < m_nx*m_ny;i++) {

        //if ( frame[i] > 1 ) continue;

        newFrame[i] = frame[i];
        // overflow check on the current single frame
        if ( frame[i] >= m_pixelDepthCntr ) overflowCntr++;
    }

    return overflowCntr;
}

unsigned int Dataset::sumFrame(int *frame, int index, int threshold){

    if(!m_thresholdsToIndices.contains(threshold))
        newLayer(threshold);
    int * newFrame = getFrame(index, threshold);

    // and keep an eye on overflow frames
    unsigned int overflowCntr = 0;

    for ( int i = 0 ; i < m_nx*m_ny ; i++ ) {

        //if ( frame[i] > 1 ) continue;

        newFrame[i] += frame[i];

        // overflow check on the current single frame
        if ( frame[i] >= m_pixelDepthCntr ) overflowCntr++;

    }

    return overflowCntr;
}

int* Dataset::getFrame(int index, int threshold){
    if(!m_thresholdsToIndices.contains(threshold))
        return nullptr;
    else {
        //int N = m_thresholdsToIndices.size();
        return &m_layers[thresholdToIndex(threshold)][index*m_nx*m_ny];
    }
}

int* Dataset::getFrameAt(int index, int layer){
    return &m_layers[layer][index*m_nx*m_ny];
}

int Dataset::getContainingFrame(QPoint pixel){
    QPoint layoutSample((pixel.x()+m_nx)/m_nx -1, (pixel.y()+m_ny)/m_ny-1);
    for(int i = 0; i < m_frameLayouts.length();i++){
        if(layoutSample == m_frameLayouts[i])//TODO: orientation messes up sampling!
            return i;
    }
    return -1;
}

QPoint Dataset::getNaturalCoordinates(QPoint pixel, int index){
    int x = pixel.x() % m_nx;
    int y = pixel.y() % m_ny;
    int orientation = m_frameOrientation[index];
    if(!(orientation&1))
        x = m_nx -x-1;
    if(orientation&2)
        y = m_ny -y-1;
    if(orientation&4){
        int tmp = x;
        x = y;
        y = tmp;
    }
    return QPoint(x,y);
}

int Dataset::sample(int x, int y, int threshold){
    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        return 0;
    int frameIndex  = getContainingFrame(QPoint(x,y));
    if(frameIndex == -1)
        return 0;
    int* frame = getFrameAt(frameIndex, layerIndex);
    QPoint coordinate = getNaturalCoordinates(QPoint(x,y), frameIndex);
    return frame[coordinate.y()*m_nx+coordinate.x()];
}

int Dataset::getWidth() {
    return ((m_nx)*getNChipsX());
}

int Dataset::getHeight() {
    return ((m_ny)*getNChipsY());
}

void Dataset::setPixel(int x, int y, int threshold, int val) {

    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        return; // couldn't find a layer, no changes applied.
    int frameIndex  = getContainingFrame(QPoint(x,y));
    if(frameIndex == -1)
        return; // couldn't find the frame, no changes applied.
    int * frame = getFrameAt(frameIndex, layerIndex);
    QPoint coordinate = getNaturalCoordinates(QPoint(x,y), frameIndex);
    // set the value
    frame[coordinate.y()*m_nx+coordinate.x()] = val;

}

int Dataset::sampleFrameAt(int index, int layer, int x, int y){
    int* frame = getFrameAt(index, layer);
    int orientation = m_frameOrientation[index];
    if(!(orientation&1))
        x = m_nx -x-1;
    if(orientation&2)
        y = m_ny -y-1;
    if(orientation&4){
        int tmp = x;
        x = y;
        y = tmp;
    }
    return frame[y*m_nx+x];//TODO:check math
}

void Dataset::setFramesPerLayer(int newFrameCount){
    int oldFrameCount =m_nFrames;
    m_nFrames = newFrameCount;
    m_frameOrientation.resize(m_nFrames);
    m_frameLayouts.resize(m_nFrames);
    for(int i = oldFrameCount; i < newFrameCount; i++){
        m_frameOrientation[i] = Dataset::orientationLtRTtB;
        m_frameLayouts[i] = QPoint(0,0);
    }
}

unsigned int Dataset::setLayer(int *data, int threshold){

    unsigned int overflowCntr = 0;

    int layerIndex = getLayerIndex(threshold);

    for(int i = 0; i < m_nFrames*m_nx*m_ny;i++) {

        m_layers[layerIndex][i] = data[i];
        if ( data[i] >= m_pixelDepthCntr ) overflowCntr++;

    }

    return overflowCntr;
}

unsigned int Dataset::addLayer(int *data, int threshold){

    unsigned int overflowCntr = 0;

    int layerIndex = getLayerIndex(threshold);
    for(int i = 0; i < m_nFrames*m_nx*m_ny;i++) {

        m_layers[layerIndex][i] += data[i];
        if ( data[i] >= m_pixelDepthCntr ) overflowCntr++;

    }

    return overflowCntr;
}

int * Dataset::getLayer(int threshold){

    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        return nullptr;
    return m_layers[layerIndex];
}

/*int * Dataset::getFullImageAsArrayWithLayout(int threshold, Mpx3GUI * mpx3gui) {

    // This two members carry all the information about the layout
    // QVector<QPoint>  m_frameLayouts // positions in the pad
    // QVector<int> m_frameOrientation // orientations

    // I want the layout of the whole chip.  I will build it again

    int nChips = getNChipsX() * getNChipsY();
    std::vector<QPoint> frameLayouts = mpx3gui->getLayout(); // Positions in the pad
    std::vector<int> frameOrientation = mpx3gui->getOrientation(); // Orientations

    // - Now work out the offsets
    QVector<QPoint> offsets;
    for ( int i = 0 ; i < nChips ; i++ ) {
        QPoint point = frameLayouts[i];
        offsets.push_back( QPoint( point.x() * x(), point.y() * y() ) );
    }
    // - Work out the orientation
    //   Here we decide where the loop starts and in which direction
    QVector<int> directionx;
    QVector<int> directiony;
    QVector<int> startx;
    QVector<int> starty;
    QVector<int> endx;
    QVector<int> endy;

    for ( int i = 0 ; i < nChips ; i++ ) {
        if ( frameOrientation[i] == orientationTtBRtL ) {
            directionx.push_back(  1 );
            directiony.push_back(  1 );
            startx.push_back( 0 + offsets[i].x() );
            starty.push_back( 0 + offsets[i].y() );
            endx.push_back( x() + offsets[i].x() );
            endy.push_back( y() + offsets[i].y() );

        }
        if ( frameOrientation[i] == orientationBtTLtR ) {
            directionx.push_back( -1 );
            directiony.push_back( -1 );
            startx.push_back( x() + offsets[i].x() - 1 );
            starty.push_back( y() + offsets[i].y() - 1 );
            endx.push_back( 0 + offsets[i].x() );
            endy.push_back( 0 + offsets[i].y() );
        }
    }

    // - Create a buffer for the whole image
    if ( m_plainImageBuff ) delete [] m_plainImageBuff;
    m_plainImageBuff = new int[nChips * x() * y()];

    // - Fill it according to layout
    // - Take one chip
    int pixIdTranslate = 0, pixCntr = 0;
    int sizex_full = getNChipsX() * x();
    int dataIndx = 0;
    int * chipdata = nullptr;

    for ( int i = 0 ; i < nChips ; i++ ) {

        // - Get the layer
        // The data comes organized per chip.
        dataIndx = mpx3gui->getConfig()->getIndexFromID(i);
        if ( dataIndx >= 0 ) chipdata = getFrame( dataIndx , threshold);
        else chipdata = nullptr;

        int x = startx[i];
        int y = starty[i];
        bool gox = true;
        bool goy = true;
        pixCntr = 0;

        for ( ; gox ; ) {

            // rewind
            goy = true;
            y = starty[i];

            for ( ; goy ; ) {

                //////////////////////
                // Data !
                pixIdTranslate = XYtoX(x, y, sizex_full);

                if ( chipdata ) { // There's data for this chip

                    //qDebug() << "[" << i << "]" << x << "," << y << " : " << pixIdTranslate << " | " << pixCntr;

                    m_plainImageBuff[pixIdTranslate] = chipdata[pixCntr++];
                } else {

                    m_plainImageBuff[pixIdTranslate] = 0;

                }

                // direction and stop
                if ( directiony[i] > 0 ) {
                    y++;
                    if ( y >= endy[i] ) goy = false;
                }
                if ( directiony[i] < 0 ) {
                    y--;
                    if ( y < endy[i] ) goy = false;
                }
            }
            // direction and stop
            if ( directionx[i] > 0 ) {
                x++;
                if ( x >= endx[i] ) gox = false;
            }
            if ( directionx[i] < 0 ) {
                x--;
                if ( x < endx[i] ) gox = false;
            }
        }


    }


//    for ( int i = 0 ; i < getPixelsPerLayer() ; i++ ) {
//        if ( i<10 || (i>127&&i<137) ) qDebug() << "[" << i << "] " << layer[i];
//        if ( i>=16384 && i<(16384+10) ) qDebug() << "[" << i << "] " << layer[i];
//    }


    return m_plainImageBuff;
}
*/


/*
////////////////////////////////////////////////
int val = frame[i];
// TO REMOVE !!!!!!!!!!!
// Remove singles quickly
if ( val > 1 ) val = 0;
// Remove singles which are
if ( val != 0
     && i > m_nx
     && i < ((m_nx*m_ny)-1)-m_nx
     && i%m_nx!=0
     && (i%(m_nx-1))!=0 ) {
    if ( frame[i+1] == 0
         &&
         frame[i-1] == 0
         &&
         frame[i+m_nx] == 0
         &&
         frame[i-m_nx] == 0
         ) {
        val = 0;
    }
}
newFrame[i] += val;
////////////////////////////////////////////////
*/
