#include "dataset.h"
#include "mpx3gui.h"
#include "qcstmBHWindow.h"
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

using namespace std;

Dataset::Dataset(int x, int y, int framesPerLayer, int pixelDepthBits)
{
    m_nx = x; m_ny = y;
    setPixelDepthBits(pixelDepthBits);

    m_nFrames = 0;
    setFramesPerLayer(framesPerLayer);

    obCorrection = nullptr;

    rewindScores();
}

void Dataset::setPixelDepthBits(int pixelDepthBits) {
    m_pixelDepthBits = pixelDepthBits;
    m_pixelDepthCntr = (1 << m_pixelDepthBits) - 1;
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
    setPixelDepthBits(other.getPixelDepthBits());
    // And copy the layers
    m_nFrames = other.getFrameCount();
    m_layers = QVector<int *>( other.getLayerCount() );
    for (int i = 0; i < m_layers.size(); i++) {
        m_layers[i] = new int[getPixelsPerLayer()];
        for(unsigned int j = 0; j < getPixelsPerLayer(); j++)
            m_layers[i][j] = other.m_layers[i][j];
    }
    obCorrection = nullptr;
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
        setPixelDepthBits(copy.getPixelDepthBits());
        //*this->obCorrection = *tocopy.obCorrection;

    }

    return *this;
}

void Dataset::zero() {
    uint64_t pixelsPerLayer = getPixelsPerLayer();
    for (int i = 0; i < m_layers.size(); i++){
        for (unsigned int j = 0; j < pixelsPerLayer; ++j) {
            //! This check may be unnecessary
            if (m_layers[i] != nullptr) {
                m_layers[i][j] = 0;
            }
        }
    }
}

int Dataset::getLayerIndex(int threshold){
    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        layerIndex = newLayer(threshold);
    return layerIndex;
}

QByteArray Dataset::toByteArray() {

    // Keys are Thresholds
    QList<int> keys = m_thresholdsToIndices.keys();
    int layerCount = m_layers.size();

    int headerLength = 4 * sizeof(int)
                     + m_nFrames * (sizeof(*m_frameLayouts.data()) + sizeof(*m_frameOrientation.data()))
                     + keys.size() * sizeof(int);

    int totalLength = headerLength + keys.length() * getLayerSize() * sizeof(int);

    // 68 bit header start offset
    QByteArray ret(totalLength, Qt::Initialization::Uninitialized);
    QDataStream stream(&ret, QIODevice::WriteOnly);
    stream << m_nx << m_ny << m_nFrames << layerCount;
    stream.writeRawData((const char*)m_frameLayouts.data(),(int)(m_nFrames*sizeof(*m_frameLayouts.data())));
    //qDebug()<<"ba 5: "<<ret.length();
    stream.writeRawData((const char*)m_frameOrientation.data(), (int)(m_nFrames*sizeof(*m_frameOrientation.data())));
    //qDebug()<<"ba 6: "<<ret.length();
    stream.writeRawData((const char*)keys.toVector().data(),(int)(keys.size()*sizeof(int))); //thresholds

    //qDebug()<<"ba 7: "<<ret.length();
    // 68 bit header end offset
    for(int i = 0; i < keys.length(); i++)
        stream.writeRawData((const char*)this->getLayer(keys[i]), (int)(sizeof(float)*getLayerSize()));

    return ret;
}

Canvas Dataset::toCanvas(int threshold) {
    return (m_pixelDepthBits == 24)
            ? Canvas(this, threshold, 0, 4)
            : Canvas(this, threshold, 0, 2);
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

Canvas Dataset::createCorrectedImage(int threshold, bool spatialOnly) {
    //----------------------------------------------------
    const int extraPixels       = 2;
    int width                   = getWidth()+2*extraPixels;  // Should always be 512 or 256 for a quad without spatial correction
    int height                  = getHeight()+2*extraPixels; // ""
    float edgePixelMagicNumber  = float(2.8);
    float edgePixelMagicNumberSpectro = float(2.3);
    float edgePixelMagicNumberSpectroVertical = float(1.9);
    float edgePixelMagicNumberSpectroHorizontal = float(1.1);

    QList<int> thresholds = m_thresholdsToIndices.keys();
    //----------------------------------------------------

    if (spatialOnly){
        edgePixelMagicNumber = 1;
        edgePixelMagicNumberSpectro = 1;
        edgePixelMagicNumberSpectroHorizontal = 1;
        edgePixelMagicNumberSpectroVertical = 1;
    }

    //! Phase 1
    //! Do spatial correction on quadrants, move to respective corners

    int gap = 2 * extraPixels;
    Canvas canvas(this, thresholds[threshold], gap, 4);
    int * imageCorrected = (int*) (canvas.image);

    //! Phase 2
    //! Loop for cross

    if (spatialOnly) {
        int ix = m_nx;
        // gap columns of height
        int n = height;
        while (n-- > 0) {
            for (int i2 = 0; i2 < gap; i2++)
                imageCorrected[ix+i2] = 0;
            ix += width;
        }
        // gap rows of width
        ix = m_ny * width;
        n = gap * width;
        while (n--) imageCorrected[ix++] = 0;
    } else {
        int edgeLo = m_nx - 1, edgeHi = m_nx + gap;
        auto at = [imageCorrected, width](int i, int j) -> int & { return imageCorrected[i * width + j]; };
        // remember the corners
        int tl = at(edgeLo, edgeLo),
            tr = at(edgeLo, edgeHi),
            bl = at(edgeHi, edgeLo),
            br = at(edgeHi, edgeHi);

        float scale = 1.0 / (width == 260? edgePixelMagicNumberSpectroVertical : edgePixelMagicNumber);
        for (int i = 0; i < height; i++) {
            int tmp = round(at(i, edgeLo) * scale);
            for (int i2 = 0; i2 <= extraPixels; i2++)
                at(i, edgeLo + i2) = tmp;
        }
        for (int i = 0; i < height; i++) {
            int tmp = round(at(i, edgeHi) * scale);
            for (int i2 = 0; i2 <= extraPixels; i2++)
                at(i, edgeHi - i2) = tmp;
        }

        // restore the corners
        at(edgeLo, edgeLo) = tl;
        at(edgeLo, edgeHi) = tr;
        at(edgeHi, edgeLo) = bl;
        at(edgeHi, edgeHi) = br;

        scale = 1.0 / (width == 260 ? edgePixelMagicNumberSpectroHorizontal : edgePixelMagicNumber);

        for (int j = 0; j < width; j++) {
            int tmp = round(at(edgeLo, j) * scale);
            for (int i2 = 0; i2 <= extraPixels; i2++)
                at(edgeLo + i2, j) = tmp;
        }
        for (int j = 0; j < width; j++) {
            int tmp = round(at(edgeHi, j) * scale);
            for (int i2 = 0; i2 <= extraPixels; i2++)
                at(edgeHi - i2, j) = tmp;
        }

        //! Phase 3
        //! Separate loop for central region
        int val = round(0.5 * (at(edgeLo, edgeLo + 1) + at(edgeLo + 1, edgeLo)));
        for (int i = edgeLo; i <= edgeLo + extraPixels; i++) {
            for (int j = edgeLo; j <= edgeLo + extraPixels; j++)
                at(i, j) = val;
        }
        val = round(0.5 * (at(edgeLo, edgeHi - 1) + at(edgeLo + 1, edgeHi)));
        for (int i = edgeLo; i <= edgeLo + extraPixels; i++) {
            for (int j = edgeHi - extraPixels; j <= edgeHi; j++)
                at(i, j) = val;
        }
        val = round(0.5 * (at(edgeHi, edgeLo + 1) + at(edgeHi - 1, edgeLo)));
        for (int i = edgeHi - extraPixels; i <= edgeHi; i++) {
            for (int j = edgeLo; j <= edgeLo + extraPixels; j++)
                imageCorrected[i * width + j] = val;
        }
        val = round(0.5 * (at(edgeHi, edgeHi - 1) + at(edgeHi - 1, edgeHi)));
        for (int i = edgeHi - extraPixels; i <= edgeHi; i++) {
            for (int j = edgeHi - extraPixels; j <= edgeHi; j++)
                at(i, j) = val;
        }
    }
    return canvas;
}

/*!
 * \brief Dataset::toTIFF
 * \param filename - absolute file path to save to
 * \remark Writes a 32 bit, greyscale, dynamic sized, vertically flipped TIFF image to `filename`,
 *          writes all thresholds
 *
 *          Does cross correction for non spectroscopic mode ONLY - spectroscopic images have a non constant ratio??? #SpecialPixels
 */
void Dataset::toTIFF(QString filename, bool crossCorrection, bool spatialOnly) {

    //----------------------------------------------------

    if (filename.isEmpty()){
        qDebug() << ">> ERROR empty filename, cancelling.";
        return;
    }

    //! Save for all thresholds in separate files
    //! Note: Could use TIFF pages but this is a much clearer approach for the user and is more compatible cross systems
    QList<int> thresholds = m_thresholdsToIndices.keys();
    for(int i = 0; i < thresholds.length(); i++) {

        QString tmpFilename = filename;
        if (thresholds.count() > 1){
            tmpFilename.replace(".tiff", "-thl" + QString::number(thresholds[i]) + ".tiff");
        }

        //! Default mode - do cross and spatial corrections
        if (crossCorrection){
            Canvas imageCorrected = createCorrectedImage(i, spatialOnly);
            imageCorrected.saveToTiff(tmpFilename.toUtf8().data());
        } else {
            Canvas image = getFullImageAsArrayWithLayout(i);
            image.saveToTiff(tmpFilename.toUtf8().data());
        }
    }
}

//! Cheaper than expected
Canvas Dataset::makeFrameForSaving(int threshold, bool crossCorrection, bool spatialOnly) {
    //----------------------------------------------------

    if (crossCorrection) {
        return createCorrectedImage(threshold, spatialOnly);
    } else {
        return getFullImageAsArrayWithLayout(threshold);
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

QVector<QVector<int> > Dataset::collectPointsROI(int threshold, QPoint pixel_init, QPoint pixel_end)
{
    //The data is made so that:
    //      - Each row represents an horizontal row of pixels, starting from the bottom of the selected RoI.
    //      - The elements in each row represent the pixels in the row, starting from the left.
    //The data can thus be seen as a normal cartesian system, where the left side of each pixel is the index.
    //To get datapoints in the middle of each pixel, a correction of +0.5 pixel has to be added in both the x and y direction, when working with the data..

    int Ny = abs( pixel_end.y() - pixel_init.y() ) + 1;//Number of pixels in the y-direction.
    int Nx = abs( pixel_end.x() - pixel_init.x() ) + 1;

    //Setup data matrix of size x*y.
    if(!valuesinRoI.empty()){
        valuesinRoI.clear();
        //valuesinRoI.squeeze();//Release all memory previously reserved (??)
    }
    valuesinRoI.resize(Ny);
    for(int i = 0; i < Ny; i++){
        valuesinRoI[i].resize(Nx);
    }

    //Fill data matrix with values within the RoI.
    for(int y = 0; y < Ny; y++){
        for(int x = 0; x < Nx; x++){
            //Let y go from bottom to top and let x go from left to right in valuesinRoI.
            //y = 0 is bottom row and is filled last in this loop.
            valuesinRoI[Ny - 1 - y][x] = sample( pixel_init.x() + x, pixel_init.y() - y, threshold);
        }
    }

    return valuesinRoI;
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

void Dataset::setProfilepoint(int index, QString pos){
    int p;
    if(pos.isEmpty()) p = -1;
    else p = pos.toInt();

    Profilepoints[index] = p;
}

void Dataset::clearProfilepoints()
{
    for(int i = 0; i < Profilepoints.length(); i++)
        Profilepoints[i] = -1;
}

QMap<int, int> Dataset::calcProfile(QString axis, int threshold, QPoint pixel_init, QPoint pixel_end){

    int height = abs( pixel_end.y() - pixel_init.y() );
    int width = abs( pixel_end.x() - pixel_init.x() );

    //For each X or Y value (resp.), add up the pixel values.
    QMap<int, int> profilevals;
    if(axis == "X")
        for(int y = pixel_init.y(); y >= pixel_init.y() - height; y--){     //pixel_init is upper left corner.
            for(int x = pixel_init.x(); x <= pixel_init.x() + width; x++){
                if(profilevals.contains( x ))
                    profilevals[ x ] += sample( x, y, threshold);
                else profilevals[ x ] = sample( x, y, threshold);
            }
        }
    if(axis == "Y")
        for(int y = pixel_init.y(); y >= pixel_init.y() - height; y--){     //pixel_init is upper left corner.
            for(int x = pixel_init.x(); x <= pixel_init.x() + width; x++){
                if(profilevals.contains( y ))
                    profilevals[ y ] += sample( x, y, threshold);
                else profilevals[ y ] = sample( x, y, threshold);
            }
        }
    return profilevals;
}

//!Calculates the Contrast to Noise Ratio of a regionprofile.
QString Dataset::calcCNR(QMap<int, int> Axismap){
    QString data = "";
    double mean;
    double stdev;
    int Npoints = countProfilePoints();
    int Nregions = countProfileRegions();

    //Show message and stats of the profile when only 1 or 2 numbers are filled in.
    if(Npoints < 4){//Nregions != TWO_Regions && Nregions != THREE_Regions && Nregions != BG_Regions){
        mean = calcRegionMean(Axismap.firstKey(), Axismap.lastKey(), Axismap);
        stdev = calcRegionStdev(Axismap.firstKey(), Axismap.lastKey(), Axismap, mean);
        return QString("Please choose four or six points, indicating two or three regions, for the calculation of the CNR.\nMean: %1 \nStdev: %2").arg(mean).arg(stdev);
    }

    else {
        QVector<double> mean_v;
        QVector<double> stdev_v;
        double cnr;
        double snr; //Signal to Noise Ratio in dB.
        bool left = false;
        //bool right = false;
        int signal;
        int bg;
        double meanfactor = 0.5; //can be adjusted to the number of regions later on if wanted

        for(int i = 0; i < Profilepoints.size(); i += stepsize){
            if(Profilepoints[i] != -1){
                if(Profilepoints[i+1] == -1) return "Please choose boundary points for two or three areas.";
                else {
                    double mean = calcRegionMean(Profilepoints[i], Profilepoints[i+1], Axismap);
                    mean_v.push_back(mean);
                    stdev_v.push_back(calcRegionStdev(Profilepoints[i], Profilepoints[i+1], Axismap, mean));
                    if(i < stepsize) left = true;
                    if(i >= 2*stepsize ) left = false; //right = true;
                }
            }
        }

        if (Nregions == BG_Regions){
            data += "Two background signals have been selected. No CNR calculation has been done\n\n";
            data += "\tBackground 1\tBackground 2\n";
            data += QString("Mean:\t%1\t%2\n").arg(mean_v[0]).arg(mean_v[1]);
            data += QString("Stdev:\t%1\t%2\n").arg(stdev_v[0]).arg(stdev_v[1]);
        }

        else if (Nregions == THREE_Regions){
            signal = 1;
            cnr = mean_v[signal];
            snr = mean_v[signal];
            double cnrdiv = 0;
            double snrdiv = 0 ;

            for(int i = 0; i < mean_v.length(); i++)
                if(i!= signal) cnr -= meanfactor*mean_v[i];

            if(!corrected) cnr *= -1;

            //Dividing by the mean of the background sigma and mean respectively for cnr and snr.
            for(int i = 0; i < mean_v.length(); i++){
                if(i!=signal){
                    cnrdiv += meanfactor*stdev_v[i];
                    snrdiv += meanfactor*mean_v[i];
                }
            }
            if(snrdiv != 0) snr /= snrdiv;
            if(cnrdiv != 0) cnr /= cnrdiv;

            cnr *= sqrt(Profilepoints[signal * stepsize + 1] - Profilepoints[signal * stepsize]);

            if(!corrected && snr !=0) snr = 1/snr;
            //else snr /= 0.5*(mean_v[0] + mean_v[2]);
            snr = 10*log10(snr);

            data += "\tBackground\tSignal\tBackground\n";
            data += QString("Mean:\t%1\t%2\t%3\n").arg(mean_v[0]).arg(mean_v[1]).arg(mean_v[2]);
            data += QString("Stdev:\t%1\t%2\t%3\n").arg(stdev_v[0]).arg(stdev_v[1]).arg(stdev_v[2]);
        }

        else if (Nregions == TWO_Regions){

            if(mean_v[0] >= mean_v[1]){
                signal = 0;
                bg = 1;
                data+="\tSignal\tBackground\n";
            }
            else {
                signal = 1;
                bg = 0;
                data += "\tBackground\tSignal\n";
            }

            cnr = mean_v[signal] - mean_v[bg];
            if(!corrected){ //signal and bg are reversed
                cnr *= -1;
                if(stdev_v[signal] != 0) cnr/= stdev_v[signal];
            }
            else if (stdev_v[bg] != 0) cnr /= stdev_v[bg];
            else return "Attempting to divide by 0!";

            if(left) cnr *= sqrt(Profilepoints[signal * stepsize + 1] - Profilepoints[signal * stepsize]);
            else cnr *= sqrt(Profilepoints[signal * stepsize + stepsize + 1] - Profilepoints[signal * stepsize + stepsize]);

            if(mean_v[bg] != 0) snr = mean_v[signal] / mean_v[bg];
            if(!corrected) snr = 1/snr;
            snr = 10*log10(snr);

            data += QString("Mean:\t%1\t%2\n").arg(mean_v[0]).arg(mean_v[1]);
            data += QString("Stdev:\t%1\t%2\n").arg(stdev_v[0]).arg(stdev_v[1]);
        }

        else return QString("Something went wrong, please choose boundary points for two or three areas.");

        data += QString("\nCNR:\t%1\nSNR:\t%2 dB").arg(cnr).arg(snr);

    }
    return data;
}

int Dataset::countProfilePoints(){
    int n=0;
    for(int i=0; i < Profilepoints.size(); i++){
        if(Profilepoints[i]!= -1) n++;
    }
    return n;
}

int Dataset::countProfileRegions(){
    int n=0;

    if(Profilepoints[signalpt1] == -1 || Profilepoints[signalpt2] == -1)
        return -1; //Only backgrounds are selected (or less, this gets checked in CNR).

    for(int i=0; i < Profilepoints.size(); i += stepsize)
        if(Profilepoints[i] != -1 && Profilepoints[i+1] != -1) n++;
    return n;
}

QPair<double, double> Dataset::LinearRegression(QVector<double> x, QVector<double> y)
{   double a, b; //The slope (a) and point of intersection (b) of a straight line y= ax+b.
    double Sx = 0, Sy = 0, Sxx = 0, Sxy = 0, Syy = 0, xm = 0, ym = 0;
    int n = x.length();

    if(n != 0){
        //Normal least squares regression:
        //    for(int i = 0; i < n; i++)
        //    {
        //        Sx  += x[i];
        //        Sxx += x[i]*x[i];
        //        Sy  += y[i];
        //        Sxy += x[i]*y[i];
        //    }
        //Look at the horizontal distances for least squares.
        //Consider y as x-coordinate and x as y-coordinate.
        //Since y goes in the other direction, y = - y in that follows i.
        //    for(int i = 0; i < n; i++)
        //    {
        //        Sx  += - y[i];
        //        Sxx += y[i]*y[i];
        //        Sy  += x[i];
        //        Sxy += - x[i]*y[i];
        //    }

        //Use Deming Regression, looking at both the horizontal and vertical distances.
        for(int i = 0; i < n; i++){
            xm += x[i];
            ym += y[i];
        }
        xm /= n;
        ym /= n;

        for(int i = 0; i < n; i++)
        {
            Sxx += (x[i] - xm) * (x[i] - xm);
            Sxy += (x[i] - xm) * (y[i] - ym);
            Syy += (y[i] - ym) * (y[i] - ym);
        }
        Sxx /= n-1;
        Sxy /= n-1;
        Syy /= n-1;

        a = sqrt( (Syy - Sxx) * (Syy - Sxx) + 4*Sxy*Sxy );
        a += Syy - Sxx;
        a /= 2 * Sxy;

        b = ym - a * xm;

        //        a  = n * Sxy - Sx*Sy;
        //        a /= n*Sxx - Sx*Sx;
        //        b  = (Sy - a * Sx );
        //        b /= n;
    }
    return qMakePair(a, b);
}

double Dataset::calcRegionMean(int begin, int end, QMap<int, int> Axismap){

    double mean =0.;
    double nMean = 0.;

    QMap<int,int>::const_iterator i = Axismap.constBegin();
    while(i != Axismap.constEnd())
    {
        if(i.key() >= begin && i.key() <= end){
            mean +=   i.value();
            nMean++;
        }
        i++;
    }
    if(nMean != 0) mean /= nMean;


    return mean;
}

double Dataset::calcRegionStdev(int begin, int end, QMap<int, int> Axismap, double mean){

    double stdev = 0.;
    double N = 0.;

    QMap<int,int>::const_iterator i = Axismap.constBegin();
    while(i != Axismap.constEnd())
    {
        if(i.key() >= begin && i.key() <= end){
            stdev += (i.value() - mean)*(i.value() - mean);
            N++;
        }
        i++;
    }
    if ( N != 0 ) stdev /= N;
    stdev = sqrt(stdev);

    return stdev;
}

//!Calculates the Edge Spread Function to be plotted in the DQEview, to calculate the MTF.
QVector<QVector<double> > Dataset::calcESFdata()
{   //Setup data vectors of proper size.
    QVector<QVector<double> > esfData(2);
    //We need a vector for the distance  one for the pixel values.

    int Nx = valuesinRoI[0].length();   //The number of columns i.e. number of pixels in the x-direction.
    int Ny = valuesinRoI.length();      //The number of rows i.e. number of pixels in the y-direction.
    int i;
    for(i = 0; i < esfData.length(); i++){
        esfData[i].resize( Ny * Nx );   //Assumes all rows of valuesinRoI have equal length, which they do for a rectangular RoI.
    }

    //Calculate mean values for the bright and dark part.
    int nx = Nx/6;  //Take some part of the brightest and darkest area's.
    int Nmean = 0;
    double mleft = 0, mright = 0;
    double bright = 0., dark = 0.;
    bool BtD = false, DtB = false;  //Bright to Dark and vice versa.

    for(int y=0; y < Ny; y++)
        for (i=0; i < nx; i++){
            mleft += valuesinRoI[y][i];
            mright   += valuesinRoI[y][Nx - nx];
            Nmean++;
        }
    if(mleft > mright){
        bright = mleft;
        dark = mright;
        BtD = true;
    }
    else if(mleft <= mright){
        bright = mright;
        dark = mleft;
        DtB = true;
    }

    if(Nmean != 0){
        bright /= Nmean;
        dark   /= Nmean;
    }

    QPair<double, double> ab = calcMidLine(bright, dark, BtD);

    double a = ab.first;
    double b = ab.second;
    //if ( a==0 && b==0 ||  isnan(a) || isnan(b) ) {
    if ( (a==0 && b==0) ||  (a!=a || b!=b) ) {
        esfData.clear();   //Return empty data...Midline doesn't make sense.

        QMessageBox msgbox;
        msgbox.setWindowTitle("Edge calculation error");
        msgbox.setText("An error has occurred in the calculation of the position of the edge. This happens when the selected "
                       "region is not large enough to distinguish 'bright area' and 'dark area' values to determine the edge position."
                       "\n \n Try making the selected region larger.");
        msgbox.setIcon(QMessageBox::Warning);
        msgbox.exec();

        return esfData;
    }

    else{
        double ap = -1.0, bp;   //a and b of a line perpendicular to the edge line, through a certain point.
        if(a!=0) ap /= a;
        double xi, yi;          //Point of intersection of perpendicular line and edge line.
        double d;               //Distance of a point to the line, in #pixels.
        i =0;

        //For each point in the RoI, calculate the distance to the edge mid line.
        for(double y = 0; y < valuesinRoI.length(); y++)
            for(double x = 0; x < valuesinRoI[y].length(); x++){
                //Take the middle of each pixel.
                x += 0.5;
                y += 0.5;
                bp  = y - ap * x;
                xi  = bp - b;
                xi /= a - ap; //a - ap is never zero.
                yi  = a * xi + b;

                d = sqrt((xi - x) * (xi - x) + (yi - y) * (yi - y));
                if(DtB)
                    if(x < xi)
                        d = -d;
                if(BtD)
                    if(x > xi)
                        d = -d;

                x -= 0.5;
                y -= 0.5; //To get the appropriate values.
                esfData[0][i] = d;
                esfData[1][i] += (valuesinRoI[y][x] - dark) / (bright - dark) ;
                i++;
            }
    }
    return esfData;
}


QPair<double, double> Dataset::calcMidLine(double bright, double dark, bool BtD)
{
    int Nx = valuesinRoI[0].length(); //The number of columns i.e. number of pixels in the x-direction.
    int Ny = valuesinRoI.length();    //The number of rows i.e. number of pixels in the y-direction (BtT).

    //We want an x- and y-value for the middle of each row.
    QVector<double> xm(Ny);
    QVector<double> ym(Ny);
    double diff = bright - dark;
    double borderval = dark + 0.1 * diff; //??     Not 0.5*(bright + dark): gives a bordervalue that is too high.

    //Look for the first pixel that is dark, take middle of row as y and x between light and dark pixel.
    if(BtD) //Bright to Dark
        for(int y = 0; y < Ny; y++)
            for(int x = 0; x < Nx; x++){
                if(valuesinRoI[y][x] < borderval)
                    if(x > 0)
                        if(valuesinRoI[y][x - 1] > borderval)
                            if(x < Nx - 1)
                                if(valuesinRoI[y][x + 1] < borderval){  //To check it's not just a randomly darker pixel.
                                    xm[y] = x;                          //Distance to the middle of pixel x-1 (centered at x-1+0.5 = x-0.5) and x (centered at x+0.5).
                                    ym[y] = y + 0.5;                    //Middle of the pixel.
                                }
            }
    else //Dark to Bright
        for(int y = 0; y < Ny; y++)
            for(int x = 0; x < Nx; x++){
                if(valuesinRoI[y][x] > borderval)
                    if(x > 0)
                        if(valuesinRoI[y][x - 1] < borderval)
                            if(x < Nx - 1)
                                if(valuesinRoI[y][x + 1] > borderval){  //To check it's not just a randomly brighter pixel.
                                    xm[y] = x;                          //Distance to the middle of pixel x-1 (centered at x-1+0.5 = x-0.5) and x (centered at x+0.5).
                                    ym[y] = y + 0.5;                    //Middle of the pixel.
                                }
            }


    return LinearRegression(xm, ym);
}

int Dataset::calcMaxNroi(int xroi, int yroi)
{
    int Nx = m_nx / xroi;
    int Ny = m_ny / yroi;

    return Nx * Ny * m_nFrames;
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

double Dataset::calcPadMean(int thlkey, QSize isize) {

    double mean = 0.0;
    int meanCntr = 0;

    for(int y = 0 ; y < isize.height() ; y++) {

        for(int x = 0 ; x < isize.width() ; x++) {

            // Around pixel (29,30)(dead) for info
            /*
            if ( x == 29 && y == 30 ) {
                cout
                        << sample(x, y+1, thlkey)
                        << ", " << sample(x+1, y, thlkey)
                        << ", " << sample(x, y-1, thlkey)
                        << ", " << sample(x-1, y, thlkey)
                        << endl;
            }
            */

            // sample and skip if it's a dead pixel
            int sampling = sample(x, y, thlkey);
            if ( sampling <= 0 ) continue;

            // skip borders
            if ( isBorderPixel(x,y, isize) ) continue;

            // calculate the mean
            mean += sampling;
            meanCntr++;


        }

    }
    // no borders !
    mean /= (double)meanCntr;

    return mean;
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

QMap<int, double> Dataset::GetPadMean() {

    QMap<int, double> meanvals;

    QList<int> keys = m_thresholdsToIndices.keys();
    QSize isize = QSize(computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y());
    for ( int i = 0; i < keys.length(); i++ ) {
        meanvals[keys[i]] = calcPadMean(keys[i], isize); // this function is CPU consuming and this scope is blocking !
    }

    // Print
    cout << '\t';
    for(int i = 0; i < keys.length(); i++) {
        cout <<  "[" << keys[i] << "]" << '\t';
    }
    cout << endl;

    cout << fixed;
    cout.precision(0);
    cout << "mean:" << '\t';
    for(int i = 0; i < keys.length(); i++) {
        cout << meanvals[ keys[i] ] << '\t';
    }
    cout << endl;



    return meanvals;
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
    if ( corrdiag->isSelectedBHCorr()) {
        corrdiag->callBHCorrection();
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

/*
void Dataset::applyBHCorrection(QVector<double> thickness, Dataset* originalSet, QMap<double, Dataset> map)
//Makes signal to thickness conversion
{
    QList<int> keys = m_thresholdsToIndices.keys();
    if(m_spline==nullptr) m_spline = new tk::spline;  // instantiate spline if not defined

    //Loop over layers
    for (int i = 0; i < keys.length(); i++)
    {
        //Create data structure
        QVector<QVector<double>> bhData(getPixelsPerLayer());
        sort(thickness.begin(), thickness.end());
        for(int j = 0; j<thickness.size(); j++)
        {
                int * layer = map[thickness[j]].getLayer(keys[i]);
                for(unsigned int k = 0; k<getPixelsPerLayer(); k++){ bhData[k].push_back(layer[k]); }
        }
        //Apply correction
        int * currentLayer = originalSet->getLayer(keys[i]);
        for(unsigned int j = 0; j< getPixelsPerLayer(); j++)
        {
            QVector<double> temp = bhData[j];
            m_spline->set_points(thickness.toStdVector(), temp.toStdVector(), false);
            currentLayer[j] = (*m_spline)(currentLayer[j]); //Do the interpolation

            if(j % (getPixelsPerLayer() / 100) == 0)
            {
                emit Dataset::updateProgressBar( (100 / keys.size()) * (i+1) * j / getPixelsPerLayer() );
            }
        }
    }
    }

}
*/

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

    // NB: you cannot delete the layers themselves here as they are often
    // just a slice of the buffer borrowed from the DataConsumerThread
    // ..   it's a design flaw
    m_layers.clear();
    m_thresholdsToIndices.clear();

    // scores
    rewindScores();

    //setFramesPerLayer(1);
}

void Dataset::resize(int nx, int ny, bool connected){
    if (connected) {
        clear();
    }
    m_nx = nx;
    m_ny = ny;
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

void Dataset::calcAllEnergyBins()
{
    qDebug() << "[INFO] Dataset::calcAllEnergyBins() : Calculate all energy bins";
}

void Dataset::setFrame(FrameSet *frame, int index, int threshold){

    if(!m_thresholdsToIndices.contains(threshold))
        newLayer(threshold);
    uint32_t *newFrame = reinterpret_cast<uint32_t*>(getFrame(index, threshold));

    frame->copyTo32(index, false, newFrame);
}

void Dataset::setFrame(int *frame, int index, int threshold){

    if(!m_thresholdsToIndices.contains(threshold))
        newLayer(threshold);
    int *newFrame = getFrame(index, threshold);

    for (int i = 0 ; i < m_nx*m_ny;i++) {

        //if ( frame[i] > 1 ) continue;

        newFrame[i] = frame[i];
    }
}

int* Dataset::getFrame(int index, int threshold){
    if(!m_thresholdsToIndices.contains(threshold))
        return nullptr;
    else {
        //int N = m_thresholdsToIndices.size();
        return &m_layers[thresholdToIndex(threshold)][index*m_nx*m_ny];
    }
}

//! Relatively cheap
int* Dataset::getFrameAt(int index, int layer){
    return &m_layers[layer][index*m_nx*m_ny];
}

//! Can get EXPENSIVE
int Dataset::getContainingFrame(QPoint pixel){
    QPoint layoutSample((pixel.x()+m_nx)/m_nx -1, (pixel.y()+m_ny)/m_ny-1);
    for(int i = 0; i < m_frameLayouts.length();i++){
        if(layoutSample == m_frameLayouts[i])//TODO: orientation messes up sampling!
            return i;
    }
    return -1;
}

//! Can get EXPENSIVE
QPoint Dataset::getNaturalCoordinates(QPoint pixel, int index){
    int x = pixel.x() % m_nx;
    int y = pixel.y() % m_ny;
    int orientation = m_frameOrientation[index];
    if(orientation&1)
        x = m_nx -x-1;
    if(!(orientation&2))
        y = m_ny -y-1;
    if(orientation&4){
        int tmp = x;
        x = y;
        y = tmp;
    }
    return QPoint(x,y);
}

//! Can get very EXPENSIVE
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
    if(orientation&1)
        x = m_nx -x-1;
    if(!(orientation&2))
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

void Dataset::setLayer(int *data, int threshold) {

    int layerIndex = getLayerIndex(threshold);
    m_layers[layerIndex] = data;
}

void Dataset::addLayer(int *data, int threshold) {

    int layerIndex = getLayerIndex(threshold);
    int *dest = m_layers[layerIndex];
    int n = m_nFrames*m_nx*m_ny;
    while (n--) {
        *(dest++) += *(data++);
    }
}

int * Dataset::getLayer(int threshold){

    int layerIndex = thresholdToIndex(threshold);
    if(layerIndex == -1)
        return nullptr;
    return m_layers[layerIndex];
}


Canvas Dataset::getFullImageAsArrayWithLayout(int threshold) {
    return Canvas(this, m_thresholdsToIndices[threshold], 0, 4);
}


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
