/**
 * \class Dataset
 *
 * \brief Class for handling detector data.
 *
 * This class stores collected data in their natural orientation, along with detector count, orientation, position and size.
 *
 */

#ifndef DATASET_H
#define DATASET_H

#include <QVector>
#include <QVarLengthArray>
#include <QByteArray>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QMap>
#include <stdint.h>

class Dataset//TODO: specify starting corner?
{
 public:
  enum globals{
    orientationLtRTtB = 0,
    orientationRtLTtB = 1,
    orientationLtRBtT = 2,
    orientationRtLBtT = 3,
    orientationTtBLtR=4,
    orientationTtBRtL=5,
    orientationBtTLtR=6,
    orientationBtTRtL=7
  };  ///Enumerations to define the coordinate system of the chips. (L)eft, (R)ight, (t)o, (T)op, and (B)ottom.
private:
  int m_nx, m_ny; //!Pixel size in the x and y direction
  QRectF m_boundingBox;//!A rectangular box which encompasses all the chips. Hence the name.
  int m_nFrames; //< The amount of detectors, a.k.a. frames here.

  QVector<QPoint>  m_frameLayouts; //<A vector containing the bottom-left corners of the detectors, (0,0) is bottom, left , (1,0) is to the right, (0,1) above.
  QVector<int> m_frameOrientation;//<The orientation of the detectors. see the enum.

  QMap <int, int> m_thresholdsToIndices;
  QVector<int*> m_layers;
  Dataset *correction = nullptr;
  int getLayerIndex(int threshold);
public:
  Dataset(int x, int y, int framesPerLayer = 1);
  Dataset();
  ~Dataset();
  Dataset( const Dataset& other );
  Dataset& operator=( const Dataset& rhs );
  void removeCorrection(){delete correction; correction = nullptr;}
  void zero();
  uint64_t getActivePixels(int threshold);
  int64_t getTotal(int threshold);
  int getContainingFrame(QPoint pixel);
  QPoint getNaturalCoordinates(QPoint pixel, int index);
  QRectF computeBoundingBox();
  int thresholdToIndex(int threshold){return m_thresholdsToIndices.value(threshold, -1);}
  QRect getBoundingBox();
  QByteArray toByteArray();
  void fromByteArray(QByteArray serialized);
  //void setLayerCount(int nLayers);
  void loadCorrection(QByteArray serialized);
  void applyCorrection();
  void applyDeadPixelsInterpolation();
  void setOrientation(QVector<int> orientations){for(int i = 0; i < orientations.length();i++)setOrientation(i, orientations[i]);}
  void setOrientation(int index, int orientation){m_frameOrientation[index] = orientation;}
  void setLayout(int index, QPoint layout){m_frameLayouts[index] = layout;}
  void clear();
  void resize(int nx, int ny){clear(); m_nx = nx; m_ny = ny;}
  void setFramesPerLayer(int newFrameCount);
  void setLayer(int *data, int layer);
  void addLayer(int* data, int layer);
  int newLayer(int layer);
  void setFrame(int *frame, int index, int layer);
  void sumFrame(int *frame, int index, int layer);
  void toJson(); //!<return JSON object to save.
  QVector<QPoint> getLayoutVector(){return m_frameLayouts;}
  QList<int> getThresholds(){return m_thresholdsToIndices.keys();}
  QVector<int> getOrientationVector(){return m_frameOrientation;}
  int getFrameCount() const{return m_nFrames;}
  int getLayerCount() const{return m_layers.count();}
  int getLayerSize() const{return m_nFrames*m_nx*m_ny;}
  uint64_t getPixelsPerLayer() const{return m_nFrames*m_nx*m_ny;}
  QPoint getSize(){return QPoint(m_nx, m_ny);}
  int *getFrame(int index, int threshold);
  int *getFrameAt(int index, int layer);
  int sampleFrameAt(int index, int layer, int x, int y);
  int* getLayer(int threshold);
  int sample(int x, int y, int layer);
  int x() const{return m_nx;}
  int y() const{return m_ny;}
};

#endif // DATASET_H
