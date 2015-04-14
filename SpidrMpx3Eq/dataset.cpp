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

int* Dataset::getFrame(int index){//TODO: fix memory leak!
  if(index == -1)
    index = m_frames.length()-1;
  int* ret  = new int[m_nx*m_ny*m_nFramesX*m_nFramesY];
  int realIndex = m_nFramesX*m_nFramesY*index;
  int offset = 0;
  int offsets[] = {0,
                   m_nx,
                   m_nx*m_nFramesX*m_ny+m_nx,
                   m_nx*m_nFramesX*m_ny
                  }; //hardcoded clockwise, starting top left.
  for(int i = 0; i < m_nFramesX*m_nFramesY; i++){
      offset = offsets[i];
      switch(m_frameOrientation[i]){
        default:
        case(Dataset::orientationLtRTtB)://reading fashion
          for(int y = 0; y < m_ny; y++)
            for(int x = 0; x < m_nx;x++)
              ret[x+y*m_nFramesX*m_nx+offset] = m_frames[realIndex+i][x+y*m_nx];
          break;
          case(Dataset::orientationTtBLtR)://frame 0 and 3
          for(int y = 0; y < m_ny; y++)
            for(int x = 0; x < m_nx;x++)
              ret[x+y*m_nFramesX*m_nx+offset] = m_frames[realIndex][y+x*m_ny];
          break;
        case(Dataset::orientationBtTRtL)://frame 1 and 2
        for(int y = 0; y < m_ny; y++)
          for(int x = 0; x < m_nx;x++)
            ret[x+y*m_nFramesX*m_nx+offset] = m_frames[realIndex][(m_ny-1-y)+(m_nx-1-x)*m_ny];//...I think
        break;
        }
   }
  return ret;
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
