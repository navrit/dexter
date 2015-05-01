#ifndef DATASET_H
#define DATASET_H

#include <QVector>
#include <QByteArray>
#include <QSet>
#include <QPoint>
#include <QRect>

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
  int m_activeFrame = -1;
  int m_nx, m_ny;
  QRect m_boundingBox;
  int m_nFrames =1 , m_tilingMode;
  QVector<QPoint>  m_frameLayouts;
  QVector<int> m_frameOrientation;
  QVector<double>  m_Thresholds;
  QSet<QPoint> m_mask;
  QVector<int> m_correction;
  QVector<int*> m_layers;
  void resizeContainers();
  void computeBoundingBox();
public:
  Dataset(int x, int y, int framesPerLayer = 1, int layers = 0);
  ~Dataset();
  QRect getBoundingBox();
  QByteArray toByteArray();
  void fromByteArray(QByteArray serialized);
  void setLayerCount(int nLayers);
  void setOrientation(QVector<int> orientations){for(int i = 0; i < orientations.length();i++)setOrientation(i, orientations[i]);}
  void setOrientation(int index, int orientation){m_frameOrientation[index] = orientation;}
  void setLayout(int index, QPoint layout){m_frameLayouts[index] = layout;}
  void setTiling(int tilingMode){m_tilingMode = tilingMode;}
  //void setFramesPerGroup(int x, int y){m_nFramesX =x; m_nFramesY = y; m_frameOrientation.resize(x*y);}
  void clear();
  void setActive(int index){m_activeFrame = index;}
  //void addMask(QPoint pixel){m_mask.insert(pixel);}
  //void removeMask(QPoint pixel){m_mask.remove(pixel);}
  void setFramesPerLayer(int nFrames);
  void setLayer(int *data, int layer);
  void addLayer(int* data, int layer);
  void addFrame(int *frame, int index, int layer);
  void addFrame(QVector<int> frame);
  void addFrames(QVector<int*> frames);
  void toJson(); //return JSON object to save.
  QVector<QPoint> getLayoutVector(){return m_frameLayouts;}
  QVector<int> getOrientationVector(){return m_frameOrientation;}
  int getFrameCount(){return m_nFrames;}
  int getLayerCount(){return m_layers.length();}
  int getPixelsPerLayer(){return m_nFrames*m_nx*m_ny;}
  QPoint getSize(){return QPoint(m_nx, m_ny);}
  //int* getActiveFrame(){return m_frames.at(m_activeFrame);}
  int  getActiveIndex(){return m_activeFrame;}
  //int *getFrame(int index);
  int *getFrame(int index, int layer);
  int* getLayer(int layer);
  QVector <int*> getFrames();
  void sumFrame(int * frame, int index, int layer);
  int sample(int x, int y, int layer);
  int x(){return m_nx;}
  int y(){return m_ny;}
};

#endif // DATASET_H
