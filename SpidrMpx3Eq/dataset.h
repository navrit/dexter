#ifndef DATASET_H
#define DATASET_H

#include <QVector>
#include <QVarLengthArray>
#include <QByteArray>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QMap>

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
  };
private:
  int m_nx, m_ny;
  QRect m_boundingBox;
  int m_nFrames;

  QVector<QPoint>  m_frameLayouts;
  QVector<int> m_frameOrientation;

  QMap <int, int> m_thresholdsToIndices;
  QVector<int*> m_layers;
  int getLayerIndex(int threshold);
public:
  Dataset(int x, int y, int framesPerLayer = 1, int layers = 0);
  ~Dataset();
  QSize computeBoundingBox();
  int thresholdToIndex(int threshold){return m_thresholdsToIndices.value(threshold, -1);}
  QRect getBoundingBox();
  QByteArray toByteArray();
  void fromByteArray(QByteArray serialized);
  //void setLayerCount(int nLayers);
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
  void toJson(); //return JSON object to save.
  QVector<QPoint> getLayoutVector(){return m_frameLayouts;}
  QList<int> getThresholds(){return m_thresholdsToIndices.keys();}
  QVector<int> getOrientationVector(){return m_frameOrientation;}
  int getFrameCount(){return m_nFrames;}
  int getLayerCount(){return m_layers.count();}
  int getLayerSize(){return m_nFrames*m_nx*m_ny;}
  int getPixelsPerLayer(){return m_nFrames*m_nx*m_ny;}
  QPoint getSize(){return QPoint(m_nx, m_ny);}
  int *getFrame(int index, int threshold);
  int *getFrameAt(int index, int layer);
  int sampleFrameAt(int index, int layer, int x, int y);
  int* getLayer(int layer);
  int sample(int x, int y, int layer);
  int x(){return m_nx;}
  int y(){return m_ny;}
};

#endif // DATASET_H
