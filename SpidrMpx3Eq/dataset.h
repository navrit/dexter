#ifndef DATASET_H
#define DATASET_H

#include <QVector>
#include <QSet>
#include <QPoint>

class Dataset
{
private:
  int m_activeFrame = -1;
  int m_nx, m_ny;
  QSet<QPoint> m_mask;
  QVector<int> m_correction;
  QVector<int> m_thresholds;
  QVector<int*> m_frames;
public:
  Dataset(int x, int y);
  ~Dataset();
  void setActive(int index){m_activeFrame = index;}
  //void addMask(QPoint pixel){m_mask.insert(pixel);}
  //void removeMask(QPoint pixel){m_mask.remove(pixel);}
  void addFrame(int *frame);
  void addFrame(QVector<int> frame);
  void addFrames(QVector<int*> frames);
  void toJson(); //return JSON object to save.
  int getFrameCount(){return m_frames.length();}
  QPoint getSize(){return QPoint(m_nx, m_ny);}
  int* getActiveFrame(){return m_frames.at(m_activeFrame);}
  int  getActiveIndex(){return m_activeFrame;}
  int *getFrame(int index);
  QVector <int*> getFrames(){ return m_frames; }
  void sumFrame(int *frame);
  int sample(int x, int y, int layer);
  int x(){return m_nx;}
  int y(){return m_ny;}
};

#endif // DATASET_H
