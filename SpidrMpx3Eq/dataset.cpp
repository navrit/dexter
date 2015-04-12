#include "dataset.h"

Dataset::Dataset(int x, int y)
{
  m_nx = x; m_ny = y;
}

Dataset::~Dataset()
{
  for(int i = 0; i < m_frames.length();i++)
    delete[] m_frames.at(i);
}

void Dataset::addFrames(QVector<int *> frames){
  for(int i = 0; i < frames.length(); i++)
    this->addFrame(frames[i]);
}

void Dataset::addFrame(int *frame){
  int *newFrame = new int[m_nx*m_ny];
  for(int i = 0 ; i < m_nx*m_ny;i++)
    newFrame[i] = frame[i];
  m_frames.append(newFrame);
  m_activeFrame = m_frames.length()-1;
}

void Dataset::addFrame(QVector<int> frame){
  addFrame(frame.data());
}

int* Dataset::getFrame(int index){
  if(index == -1)
    return m_frames.at(m_activeFrame);
  return m_frames.at(index);
}

void Dataset::sumFrame(int * frame){
  for(int i = 0; i < m_nx*m_ny; i++)
    m_frames.at(m_activeFrame)[i] += frame[i];
}

int Dataset::sample(int x, int y, int layer){
  if(layer >= m_frames.length() ||  x >= m_nx || y >= m_ny)
    return 0;
  return m_frames.at(layer)[m_nx*y+x];
}

void Dataset::clear(){
  m_activeFrame = 0;
  for(int i = 0; i < m_frames.length();i++)
    delete[] m_frames.at(i);
  m_frames.clear();
}
