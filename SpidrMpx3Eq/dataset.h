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
    orientationLtRTtB,
    orientationRtLTtB,
    orientationLtRBtT,
    orientationRtLBtT,
    orientationTtBLtR,
    orientationTtBRtL,
    orientationBtTLtR,
    orientationBtTRtL
  };
private:
  int m_activeFrame = -1;
  int m_nx, m_ny;
  int m_nFramesX=1, m_nFramesY =1, m_tilingMode;
  QVector<int> m_frameOrientation;
  unsigned tiling = Dataset::tilingClockwise;
  QSet<QPoint> m_mask;
  QVector<int> m_correction;
  QVector<int> m_thresholds;
  QVector<int*> m_frames;
public:
  Dataset(int x, int y);
  ~Dataset();
  void setOrientation(QVector<int> orientations){for(int i = 0; i < orientations.length();i++)setOrientation(i, orientations[i]);}
  void setOrientation(int index, int orientation){m_frameOrientation[index] = orientation;}
  void setTiling(int tilingMode){m_tilingMode = tilingMode;}
  void setFramesPerGroup(int x, int y){m_nFramesX =x; m_nFramesY = y; m_frameOrientation.resize(x*y);}
  void clear();
  void setActive(int index){m_activeFrame = index;}
  //void addMask(QPoint pixel){m_mask.insert(pixel);}
  //void removeMask(QPoint pixel){m_mask.remove(pixel);}
  void addFrame(int *frame);
  void addFrame(QVector<int> frame);
  void addFrames(QVector<int*> frames);
  void toJson(); //return JSON object to save.
  int getFrameCount(){return m_frames.length()/(m_nFramesX*m_nFramesY);}
  QPoint getSize(){return QPoint(m_nx*m_nFramesX, m_ny*m_nFramesY);}
  int* getActiveFrame(){return m_frames.at(m_activeFrame);}
  int  getActiveIndex(){return m_activeFrame;}
  int *getFrame(int index);
  QVector <int*> getFrames(){
    QVector<int*> ret;
    for(int i = 0; i < getFrameCount(); i++)
      ret.append(getFrame(i));
    return ret;
  }
  void sumFrame(int *frame);
  int sample(int x, int y, int layer);
  int x(){return m_nx;}
  int y(){return m_ny;}
};

#endif // DATASET_H
