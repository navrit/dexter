#ifndef DATASET_H
#define DATASET_H

#include <QVector>
#include <QSet>
#include <QPoint>

class Dataset//TODO: specify starting corner?
{
 public:
  enum globals{
    tilingClockwise  = 0,
    tilingCounterClockwise  = 1,
    orientationLtRTtB = 0,
    orientationRtLTtB = 1,
    orientationLtRBtT = 2,
    orientationRtLBtT = 3/*,
    orientationTtBLtR,
    orientationTtBRtL,
    orientationBtTLtR,
    orientationBtTRtL*/
  };
private:
  int m_activeFrame = -1;
  int m_nx, m_ny;
  int m_nFrames =1 , m_tilingMode;
  QVector<int> m_frameOrientation;
  QVector<float>  m_Thresholds;
  QVector<QPoint>  m_frameLayouts;
  unsigned tiling = Dataset::tilingClockwise;
  QSet<QPoint> m_mask;
  QVector<int> m_correction;
  QVector<int> m_thresholds;
  QVector<int*> m_layers;
  void resizeContainers();
public:
  Dataset(int x, int y, int framesPerLayer = 1, int layers = 0);
  ~Dataset();
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
  void addFrame(int *frame, int index, int layer);
  void addFrame(QVector<int> frame);
  void addFrames(QVector<int*> frames);
  void toJson(); //return JSON object to save.
  QVector<QPoint> getLayoutVector(){return m_frameLayouts;}
  QVector<int> getOrientationVector(){return m_frameOrientation;}
  int getFrameCount(){return m_nFrames;}
  int getLayerCount(){return m_layers.length();}
  QPoint getSize(){return QPoint(m_nx, m_ny);}
  //int* getActiveFrame(){return m_frames.at(m_activeFrame);}
  int  getActiveIndex(){return m_activeFrame;}
  //int *getFrame(int index);
  int *getFrame(int index, int layer){return m_layers[layer]+m_nx*m_ny*index;}
  QVector <int*> getFrames();
  void sumFrame(int * frame, int index, int layer);
  int sample(int x, int y, int layer);
  int x(){return m_nx;}
  int y(){return m_ny;}
};

#endif // DATASET_H
