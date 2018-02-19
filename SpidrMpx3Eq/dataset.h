/**
 * \class Dataset
 *
 * \brief Class for handling detector data.
 *
 * This class stores collected data in their natural orientation, along with detector count, orientation, position and size.
 * Data is stored using a pointer per threshold, containing the data for all chips. One of these pointers is refered to as a layer and a single chip is reffered to as a frame.
 */

#ifndef DATASET_H
#define DATASET_H

#include <QVector>
#include <QVarLengthArray>
#include <QByteArray>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QMap>
#include <QFile>
#include <stdint.h>
#include <vector>
#include "spline.h"
#include <tiffio.h> /* Sam Leffler's libtiff library. */

#include <dlib/optimization.h>

using namespace std;
//using namespace dlib;

namespace Ui {
class QCstmGLVisualization;
}

class Mpx3GUI;
class Color2DRecoGuided;
class CorrectionItem;
class QCstmCorrectionsDialog;

class Dataset//TODO: specify starting corner?
{
public:
    ///Enumerations to define the coordinate system of the chips. (L)eft, (R)ight, (t)o, (T)op, and (B)ottom.
    enum globals {
        orientationLtRTtB = 0,
        orientationRtLTtB = 1,
        orientationLtRBtT = 2,
        orientationRtLBtT = 3,
        orientationTtBLtR=4,
        orientationTtBRtL=5,
        orientationBtTLtR=6,
        orientationBtTRtL=7
    };


    enum cnr_constants {
        stepsize = 2,
        signalpt1 = 2,
        signalpt2 = 3,
        BG_Regions = -1,
        TWO_Regions = 2,
        THREE_Regions = 3
    };

    typedef struct {
        int packetsLost;
        int framesLost;
        bool dataMisaligned;
        int mpx3ClockStops;
    } score_info;


    struct sortPair {
        double thickness;
        double value;
        sortPair(double t, double v){thickness = t; value = v;}
        sortPair() {}
    };

    struct sortingStruct {
        bool operator() (const sortPair &lhs, const sortPair &rhs) { return lhs.thickness < rhs.thickness; }
    } sortByThickness;

    struct bStats {
        QPoint init_pixel;
        QPoint end_pixel;
        std::vector<double>  mean_v;
        std::vector<double> stdev_v;
    } bstats;//!Calculated mean and stdev of the selected region of interest.

private:
    int m_nx, m_ny; //!<Pixel size in the x and y direction, per detector.
    int m_pixelDepthBits;
    int m_pixelDepthCntr;
    QRectF m_boundingBox;//!<A rectangular box which encompasses all the chips. Hence the name.
    int m_nFrames; //!< The amount of detectors, a.k.a. frames here.
                   //!< This is unclear?...
    score_info m_scores; //!< some 'score' info about this frame. A bunch of counters.
    int * m_plainImageBuff = nullptr;

    QVector<QPoint>  m_frameLayouts; //!<A vector containing the bottom-left corners of the detectors, (0,0) is bottom, left , (1,0) is to the right, (0,1) above.
    QVector<int> m_frameOrientation;//!<The orientation of the detectors. see the enum.

    QMap <int, int> m_thresholdsToIndices;//!<Translate threshold values to indices in the vectors.
    QVector<int*>  m_layers;//!<Actual data, one pointer per threshold.
    Dataset * obCorrection = nullptr;//!< A pointer to the Dataset used for the flat-field correction.
    bool corrected; //!indicates whether or not an image has been corrected.
    int getLayerIndex(int threshold);
    void rewindScores();

    QList<int> Profilepoints = QList<int>() << -1 << -1 << -1 << -1 << -1 << -1; //!The points on a profile that are used to calculate the CNR. Initialized to -1 to indicate that no value has been specified (yet).
    QVector<QVector<int> > valuesinRoI;//!A matrix of the values of the pixels contained in the region of interest. Each row corresponds to a row of pixels (LtR), from Bottom to Top.


public:
    Dataset(int x, int y, int framesPerLayer = 1, int pixelDepthBits = 12);
    Dataset();
    ~Dataset();
    Dataset( const Dataset& other );
    Dataset& operator=( const Dataset& rhs );
    void removeCorrection(){ delete obCorrection; obCorrection = nullptr;}
    void zero();//!< Set all layers to zero.
    QPoint getNaturalCoordinates(QPoint pixel, int index); //!< Used by sample to compute coordinates
    int thresholdToIndex(int threshold){return m_thresholdsToIndices.value(threshold, -1);}
    uint64_t getActivePixels(int threshold); //!< Returns the amount of non-zero pixels for a specific threshold.
    int64_t getTotal(int threshold);//!< Returns the sum of all pixels for a specific threshold
    int64_t getOverflow(int threshold);//!< Returns the pixels in overflow for a specific threshold
    int getContainingFrame(QPoint pixel);//!< Returns the frame-index of the frame which contains the specified point. Returns -1 if no frame contains the point.
    QRectF computeBoundingBox(); //!< Computes the minimum bounding box of the set of chips. Returns coordinates and sizes in units of "chips", so e.g. (0,2)x(0,2) instead of (0,512)x(0,512) for a quad.
    int getNChipsX();
    int getNChipsY();

    QByteArray toByteArray(); //!< Serializes the dataset for saving.
    QVector<int> toQVector(); //!< Serializes the dataset for saving.
    void saveBIN(QString filename);   //! Puts the dataset into a BIN format and saves.
    void toTIFF(QString filename, bool crossCorrection = true , bool spatialOnly = false);  //! Puts the dataset into a TIFF format and saves.
    void toASCII(QString filename); //! Puts the dataset into ASCII format and saves.

    void fromByteArray(QByteArray serialized); //!< Restores the dataset from a previously serialized set.
    void fromASCIIMatrix(QFile * file, int x, int y, int framesPerLayer);
    void fromASCIIMatrixGetSizeAndLayers(QFile * file, int *x, int *y, int *framesPerLayer);

    void loadCorrection(QByteArray serialized);//!< Loads and sets the correction to a previously serialized set.
    void applyCorrections(QCstmCorrectionsDialog * corrdiag);//<!Handles all corrections.  This function is blocking for the moment !
    void applyOBCorrection();//!< Computes and applies the flat-field correction
    void dumpAllActivePixels(); //!< for testing purposes
    void applyDeadPixelsInterpolation(double meanMultiplier, QMap<int, double> meanvals);
    void applyHighPixelsInterpolation(double meanMultiplier, QMap<int, double> meanvals);
    int applyColor2DRecoGuided(Color2DRecoGuided * );
    void calcBasicStats(QPoint pixel_init, QPoint pixel_end); //!Calculates the Mean and standard deviation of the pixel values in the selected region.
    QMap<int, int> calcProfile(QString axis, int threshold, QPoint pixel_init, QPoint pixel_end); //!Calculates the profile of the pixelvalues in the selected region in the direction of the specified axis.
    QString calcCNR(QMap<int,int> Axismap); //!Calculates the contrast to noise ratio of the region indicated by the Profilepoints.
    double calcRegionMean(int begin, int end, QMap<int, int> Axismap); //!Calculates the mean of a region.
    double calcRegionStdev(int begin, int end, QMap<int,int> AxisMap, double mean);   //!Calculates the standard deviation of a region.
    QVector<QVector<int> > collectPointsROI(int layerIndex, QPoint pixel_init, QPoint pixel_end); //!Collects the data of a region of interest. for a specific layer/threshold.
    QVector<QVector<double> > calcESFdata();
    //QVector<QVector<double> > calcESFfitData(parameter_vector params, double start, int length, double stepsize);
    //QVector<QVector<double> > calcPSFdata(parameter_vector params, double start, int length, double stepsize);
    QPair<double, double> calcMidLine(double bright, double dark, bool BtD);
    //parameter_vector fitESFparams(QVector<QVector<double> > esfdata);
    int calcMaxNroi(int xroi, int yroi);

    QPointF XtoXY(int X, int dimX);
    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
    int countProfilePoints();
    int countProfileRegions();
    QPair<double, double> LinearRegression(QVector<double> x, QVector<double> y);

    void setOrientation(QVector<int> orientations){for(int i = 0; i < orientations.length();i++)setOrientation(i, orientations[i]);}
    void setOrientation(int index, int orientation){m_frameOrientation[index] = orientation;}
    void setLayout(int index, QPoint layout){m_frameLayouts[index] = layout;}
    void clear();//!< Removes all data
    void resize(int nx, int ny){clear(); m_nx = nx; m_ny = ny;}//!< Changes the size of each chip. Also calls clear().
    void setFramesPerLayer(int newFrameCount); //!<Sets the amount of chips. New Chips get initialized with location (0,0) and a LtRTtB orientation.
    unsigned int setLayer(int *data, int threshold);//!<Overwrites a specific layer with the values pointed to by data.
    unsigned int addLayer(int* data, int threshold);//!<Adds the values pointed to by data to the specified layer.
    unsigned int setFrame(int *frame, int index, int threshold);//!< Overwrites the data of chip index at the specified threshold with the data pointed to by frame.
    void setPixel(int x, int y, int threshold, int val);//!< Set a pixel value for a given threshold (x,y) (assembly coordinates !)
    unsigned int sumFrame(int *frame, int index, int threshold);//!< Adds the data pointed to by frame to the data of chip index at the specified threshold.
    void toJson(); //!<Saves JSON log file with measurement settings
    void setProfilepoint(int index, QString pos);
    void setProfilepoint(int index, int pos){ if(pos > 0 && pos < 256) setProfilepoint(index, QString("%1").arg(pos));}
    void clearProfilepoints();
    void setCorrected(bool status){corrected = status;}

    QVector<QPoint> getLayoutVector(){return m_frameLayouts;}
    QList<int> getThresholds(){return m_thresholdsToIndices.keys();}
    QVector<int> getOrientationVector(){return m_frameOrientation;}
    QList<int> getProfilepoints(){return Profilepoints;}
    int getFrameCount() const{return m_nFrames;}
    int getLayerCount() const{return m_layers.count();}
    int getLayerSize() const{return m_nFrames*m_nx*m_ny;}
    uint64_t getPixelsPerLayer() const{return m_nFrames*m_nx*m_ny;}
    bool isBorderPixel(int pixel, QSize isize);       //!<Determines if the pixel is at the border (x) (assembly coordinates !)
    bool isBorderPixel(int x, int y, QSize isize);    //!<Determines if the pixel is at the border (x,y) (assembly coordinates !)
    void increasePacketsLost(int val) { m_scores.packetsLost += val; }
    void increaseFramesLost(int val) { m_scores.framesLost += val; }
    void setDataMisaligned(bool val) { m_scores.dataMisaligned = val; }
    void increaseMpx3ClockStops(int val) { m_scores.mpx3ClockStops += val; }

    int getPacketsLost() { return m_scores.packetsLost; }
    int getFramesLost() { return m_scores.framesLost; }
    int getMpx3ClockStops() { return m_scores.mpx3ClockStops; }
    int isDataMisaligned(){ return m_scores.dataMisaligned; }

    QPoint getSize(){return QPoint(m_nx, m_ny);}
    int * getFrame(int index, int threshold); //!< returns a pointer to the data of chip index at the specified threshold.
    int * getFrameAt(int index, int layer); //!< returns a pointer to the data of chip index at the specified layer-index. (i.e. does not call thresholdToLayer(layer))
    int sampleFrameAt(int index, int layer, int x, int y);//!< Returns the value of the pixel at (x,y), in the coordinate-system of the assembly, of chip index directly at the specified layer. Does take into account the orientation of the chip.
    int * getLayer(int threshold);
    //int * getFullImageAsArrayWithLayout(int threshold, Mpx3GUI * mpx3gui);    // Remove probably
    int sample(int x, int y, int threshold); //!<Returns the value of the pixel at (x,y) (assembly coordinates) and the specified threshold.
    int getWidth();
    int getHeight();
    int x() const{return m_nx;} // per chip
    int y() const{return m_ny;} // per chip
    int getPixelDepthBits() const { return m_pixelDepthBits; }


    // Simple tools
    typedef enum {
        __less = 0,
        __lesseq,
        __equal,
        __biggereq,
        __bigger
    } comp;
    map< pair<int, int>, int > activeNeighbors(int x, int y, int thl, QSize isize, comp c = __bigger, int activeValue = 0);//!<Determines if a pixel has active neighbors (assembly coordinates !)
    int vectorAverage(std::vector<int> v);
    int averageValues(map< pair<int, int>, int > m1);
    void appendSelection(int x, int y, int thl, int compareto, comp c, map< pair<int, int>, int > & );
    double calcPadMean(int thlkey, QSize isize);
    std::vector<int> getIntersection(map< pair<int, int>, int > m1, map< pair<int, int>, int > m2);
    void DumpSmallMap(map< pair<int, int>, int > m1);

    QMap<int, double> GetPadMean();

    int newLayer(int layer);//!<Adds a new layer at the specified threshold.

    void runImageCalculator(QString imgOperator, int index1, int index2, int threshold = 0);
    void calcAllEnergyBins();

};



#endif // DATASET_H
