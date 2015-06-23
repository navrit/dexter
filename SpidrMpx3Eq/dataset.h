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
#include <stdint.h>

class Dataset//TODO: specify starting corner?
{
 public:
   ///Enumerations to define the coordinate system of the chips. (L)eft, (R)ight, (t)o, (T)op, and (B)ottom.
  enum globals{
    orientationLtRTtB = 0,
    orientationRtLTtB = 1,
    orientationLtRBtT = 2,
    orientationRtLBtT = 3,
    orientationTtBLtR=4,
    orientationTtBRtL=5,
    orientationBtTLtR=6,
    orientationBtTRtL=7
  };
private:
  int m_nx, m_ny; //!<Pixel size in the x and y direction
  QRectF m_boundingBox;//!<A rectangular box which encompasses all the chips. Hence the name.
  int m_nFrames; //!< The amount of detectors, a.k.a. frames here.

  QVector<QPoint>  m_frameLayouts; //!<A vector containing the bottom-left corners of the detectors, (0,0) is bottom, left , (1,0) is to the right, (0,1) above.
  QVector<int> m_frameOrientation;//!<The orientation of the detectors. see the enum.

  QMap <int, int> m_thresholdsToIndices;//!<Translate threshold values to indices in the vectors.
  QVector<int*> m_layers;//!<Actual data, one pointer per threshold.
  Dataset *correction = nullptr;//!< A pointer to tthe Dataset used for the flat-field correction.
  int getLayerIndex(int threshold);
  int newLayer(int layer);//!<Adds a new layer at the specified threshold.
public:
  Dataset(int x, int y, int framesPerLayer = 1);
  Dataset();
  ~Dataset();
  Dataset( const Dataset& other );
  Dataset& operator=( const Dataset& rhs );
  void removeCorrection(){delete correction; correction = nullptr;}
  void zero();//!< Set all layers to zero.
  QPoint getNaturalCoordinates(QPoint pixel, int index); //!< Used by sample to compute coordinates
  int thresholdToIndex(int threshold){return m_thresholdsToIndices.value(threshold, -1);}
  uint64_t getActivePixels(int threshold); //!< Returns the amount of non-zero pixels for a specific threshold.
  int64_t getTotal(int threshold);//!< Returns the sum of all pixels for a  specific threshold
  int getContainingFrame(QPoint pixel);//!< Returns the frame-index of the frame which contains the specified point. Returns -1 if no frame contains the point.
  QRectF computeBoundingBox(); //!< Computes the minimum bounding box of the set of chips. Returns coordinates and sizes in units of "chips", so e.g. (0,2)x(0,2) instead of (0,512)x(0,512) for a quad.
  QByteArray toByteArray(); //!< Serializes the dataset for saving.
  void fromByteArray(QByteArray serialized); //!< Restores the dataset from a previously serialized set.
  void loadCorrection(QByteArray serialized);//!< Loads and sets the correction to a previously serialized set.
  void applyCorrection();//!< Computes and applies the flat-field correction
  void applyDeadPixelsInterpolation();
  void applyHighPixelsInterpolation();
  void calcBasicStats(QPoint pixel_init, QPoint pixel_end);
  QPointF XtoXY(int X, int dimX);
  void setOrientation(QVector<int> orientations){for(int i = 0; i < orientations.length();i++)setOrientation(i, orientations[i]);}
  void setOrientation(int index, int orientation){m_frameOrientation[index] = orientation;}
  void setLayout(int index, QPoint layout){m_frameLayouts[index] = layout;}
  void clear();//!< Removes all data
  void resize(int nx, int ny){clear(); m_nx = nx; m_ny = ny;}//!< Changes the size of each chip. Also calls clear().
  void setFramesPerLayer(int newFrameCount); //!<Sets the amount of chips. New Chips get initialized with location (0,0) and a LtRTtB orientation.
  void setLayer(int *data, int threshold);//!<Overwrites a specific layer with the values pointed to by data.
  void addLayer(int* data, int threshold);//!<Adds the values pointed to by data to the specified layer.
  void setFrame(int *frame, int index, int threshold);//!< Overwrites the data of chip index at the specified threshold with the data pointed to by frame.
  void sumFrame(int *frame, int index, int threshold);//!< Adds the data pointed to by frame to the data of chip index at the specified threshold.
  void toJson(); //!<return JSON object to save.

  QVector<QPoint> getLayoutVector(){return m_frameLayouts;}
  QList<int> getThresholds(){return m_thresholdsToIndices.keys();}
  QVector<int> getOrientationVector(){return m_frameOrientation;}
  int getFrameCount() const{return m_nFrames;}
  int getLayerCount() const{return m_layers.count();}
  int getLayerSize() const{return m_nFrames*m_nx*m_ny;}
  uint64_t getPixelsPerLayer() const{return m_nFrames*m_nx*m_ny;}
  QPoint getSize(){return QPoint(m_nx, m_ny);}
  int *getFrame(int index, int threshold); //!< returns a pointer to the data of chip index at the specified threshold.
  int *getFrameAt(int index, int layer); //!< returns a pointer to the data of chip index at the specified layer-index. (i.e. does not call thresholdToLayer(layer))
  int sampleFrameAt(int index, int layer, int x, int y);//!< Returns the value of the pixel at (x,y), in the coordinate-system of the assembly, of chip index directly at the specified layer. Does take into account the orientation of the chip.
  int* getLayer(int threshold);
  int sample(int x, int y, int threshold);//!<Returns the value of the pixel at (x,y) (assembly coordinates) and the specified threshold.
  int x() const{return m_nx;}
  int y() const{return m_ny;}
};

#endif // DATASET_H
