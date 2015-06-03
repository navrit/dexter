#include "dataset.h"
#include <QDataStream>

Dataset::Dataset(int x, int y, int framesPerLayer)
{
  m_nx = x; m_ny = y;
  m_nFrames = 0;
  setFramesPerLayer(framesPerLayer);
}

Dataset::Dataset() : Dataset(1,1,1){}

Dataset::~Dataset()
{
  clear();
}

int Dataset::getTotal(int threshold){
  int index = thresholdToIndex(threshold);
  if(index == -1)
    return 0;
  int count = 0;
  for(int j = 0; j < m_nx*m_ny*m_nFrames; j++)
    count += m_layers[index][j];
  return count;
}

unsigned Dataset::getActivePixels(int threshold){
  int index = thresholdToIndex(threshold);
  if(index == -1)
    return 0;
  unsigned count  =0;
  for(int j = 0; j < m_nx*m_ny*m_nFrames; j++){
      if(0 != m_layers[index][j])
        count++;
    }
  return count;
}

Dataset::Dataset( const Dataset& other ): m_boundingBox(other.m_boundingBox), m_frameLayouts(other.m_frameLayouts), m_frameOrientation(other.m_frameOrientation), m_thresholdsToIndices(other.m_thresholdsToIndices), m_layers(other.m_layers){
  m_nx = other.x(); m_ny = other.y();
  m_nFrames = other.getFrameCount();
  for(int i = 0; i < m_layers.size(); i++){
      m_layers[i] = new int[m_nx*m_ny*m_nFrames];
      for(int j = 0; j < m_nx*m_ny*m_nFrames; j++)
        m_layers[i][j] = other.m_layers[i][j];
    }
}

Dataset& Dataset::operator =( const Dataset& rhs){
  Dataset copy(rhs);
  std::swap(this->m_layers, copy.m_layers);
  return *this;
}

int Dataset::getLayerIndex(int threshold){
  int layerIndex = thresholdToIndex(threshold);
  if(layerIndex == -1)
    layerIndex = newLayer(threshold);
  return layerIndex;
}

QByteArray Dataset::toByteArray(){
  QByteArray ret(0);
  ret += QByteArray::fromRawData((const char*)&m_nx, (int)sizeof(m_nx));
  ret += QByteArray::fromRawData((const char*)&m_ny, (int)sizeof(m_ny));
  ret += QByteArray::fromRawData((const char*)&m_nFrames, (int)sizeof(m_nFrames));
  int layerCount = m_layers.size();
  ret += QByteArray::fromRawData((const char*)&layerCount, (int)sizeof(layerCount));
  ret += QByteArray::fromRawData((const char*)m_frameLayouts.data(),(int)(m_nFrames*sizeof(*m_frameLayouts.data())));
  ret += QByteArray::fromRawData((const char*)m_frameOrientation.data(), (int)(m_nFrames*sizeof(*m_frameOrientation.data())));
  QList<int> keys = m_thresholdsToIndices.keys();
  ret += QByteArray::fromRawData((const char*)keys.toVector().data(),(int)(keys.size()*sizeof(int))); //thresholds
  for(int i = 0; i < keys.length(); i++)
    ret += QByteArray::fromRawData((const char*)this->getLayer(keys[i]), (int)(sizeof(int)*getLayerSize()));
  return ret;
}

void Dataset::fromByteArray(QByteArray serialized){
  QDataStream in(&serialized, QIODevice::ReadOnly);
  in.readRawData((char*)&m_nx, (int)sizeof(m_nx));
  in.readRawData((char*)&m_ny, (int)sizeof(m_ny));
  in.readRawData((char*)&m_nFrames, (int)sizeof(m_nFrames));
  setFramesPerLayer(m_nFrames);
  int layerCount;
  in.readRawData((char*)&layerCount, (int)sizeof(layerCount));
  //setLayerCount(layerCount);
  in.readRawData((char*)m_frameLayouts.data(), m_nFrames*(int)sizeof(*m_frameLayouts.data()));
  in.readRawData((char*)m_frameOrientation.data(), m_nFrames*(int)sizeof(*m_frameOrientation.data()));
  QVector<int> keys(layerCount);
  in.readRawData((char*)keys.data(), keys.size()*(int)sizeof(int));
  QVector<int> frameBuffer(m_nx*m_ny);
  for(int i = 0; i < keys.size(); i++){
      for(int j = 0; j < m_nFrames; j++){
          in.readRawData((char*)frameBuffer.data(), (int)sizeof(int)*frameBuffer.size());
          this->setFrame(frameBuffer.data(), j, keys[i]);
        }
    }
}

void Dataset::clear(){
  for(int i =0; i < m_layers.size(); i++){
      delete[] m_layers[i];
    }
  m_layers.clear();
  m_thresholdsToIndices.clear();
  //setFramesPerLayer(1);
}

QSize Dataset::computeBoundingBox(){
  m_boundingBox.setRect(0,0,0,0);
  int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
  for(int i =0; i < m_frameLayouts.length();i++){
      if(m_frameLayouts[i].x() < min_x)
        min_x = m_frameLayouts[i].x();
      if(m_frameLayouts[i].y() < min_y)
        min_y = m_frameLayouts[i].y();
      if(m_frameLayouts[i].x() > max_x)
        max_x = m_frameLayouts[i].x();
      if(m_frameLayouts[i].y() > max_y)
        max_y = m_frameLayouts[i].y();
    }
  m_boundingBox.setRect(0,0, (max_x-min_x+1)*m_nx, (max_y-min_y+1)*m_ny );

  return m_boundingBox.size();
}

int Dataset::newLayer(int threshold){
  m_thresholdsToIndices[threshold] = m_layers.size();
  m_layers.append(new int[m_nx*m_ny*m_nFrames]);
  for(int j = 0; j < getLayerSize(); j++)
    m_layers.last()[j] = 0;
  return m_layers.size()-1;
}

void Dataset::setFrame(int *frame, int index, int threshold){
  if(!m_thresholdsToIndices.contains(threshold))
    newLayer(threshold);
  int *newFrame = getFrame(index, threshold);
  for(int i = 0 ; i < m_nx*m_ny;i++)
    newFrame[i]= frame[i];
}

void Dataset::sumFrame(int *frame, int index, int threshold){
  if(!m_thresholdsToIndices.contains(threshold))
    newLayer(threshold);
  int *newFrame = getFrame(index, threshold);
  for(int i = 0 ; i < m_nx*m_ny;i++)
    newFrame[i] += frame[i];
}

int* Dataset::getFrame(int index, int threshold){
  if(!m_thresholdsToIndices.contains(threshold))
    return nullptr;
  else
    return &m_layers[thresholdToIndex(threshold)][index*m_nx*m_ny];
}

int* Dataset::getFrameAt(int index, int layer){
  return &m_layers[layer][index*m_nx*m_ny];
}

int Dataset::getContainingFrame(QPoint pixel){
  QPoint layoutSample((pixel.x()+m_nx)/m_nx -1, (pixel.y()+m_ny)/m_ny-1);
  for(int i = 0; i < m_frameLayouts.length();i++){
      if(layoutSample == m_frameLayouts[i])//TODO: orientation messes up sampling!
        return i;
    }
  return -1;
}

QPoint Dataset::getNaturalCoordinates(QPoint pixel, int index){
  int x = pixel.x() % m_nx;
  int y = pixel.y() % m_ny;
  int orientation = m_frameOrientation[index];
  if(!(orientation&1))
    x = m_nx -x-1;
  if(orientation&2)
    y = m_ny -y-1;
  if(orientation&4){
      int tmp = x;
      x = y;
      y = tmp;
    }
  return QPoint(x,y);
}

int Dataset::sample(int x, int y, int threshold){
  int layerIndex = thresholdToIndex(threshold);
  if(layerIndex == -1)
    return 0;
  int frameIndex  = getContainingFrame(QPoint(x,y));
  if(frameIndex == -1)
    return 0;
  int* frame = getFrameAt(frameIndex, layerIndex);
  QPoint coordinate = getNaturalCoordinates(QPoint(x,y), frameIndex);
  return frame[coordinate.y()*m_nx+coordinate.x()];
}

int  Dataset::sampleFrameAt(int index, int layer, int x, int y){
  int* frame = getFrameAt(index, layer);
  int orientation = m_frameOrientation[index];
  if(!(orientation&1))
    x = m_nx -x-1;
  if(orientation&2)
    y = m_ny -y-1;
  if(orientation&4){
      int tmp = x;
      x = y;
      y = tmp;
    }
  return frame[y*m_nx+x];//TODO:check math
}

void Dataset::setFramesPerLayer(int newFrameCount){
  int oldFrameCount =m_nFrames;
  m_nFrames = newFrameCount;
  m_frameOrientation.resize(m_nFrames);
  m_frameLayouts.resize(m_nFrames);
  for(int i = oldFrameCount; i < newFrameCount; i++){
      m_frameOrientation[i] = Dataset::orientationLtRTtB;
      m_frameLayouts[i] = QPoint(0,0);
    }
}

void Dataset::setLayer(int *data, int threshold){
  int layerIndex = getLayerIndex(threshold);
  for(int i = 0; i < m_nFrames*m_nx*m_ny;i++)
    m_layers[layerIndex][i] = data[i];
}

void Dataset::addLayer(int *data, int threshold){
  int layerIndex = getLayerIndex(threshold);
  for(int i = 0; i < m_nFrames*m_nx*m_ny;i++)
    m_layers[layerIndex][i] += data[i];
}

int* Dataset::getLayer(int threshold){
  int layerIndex = thresholdToIndex(threshold);
  if(layerIndex == -1)
    return nullptr;
  return m_layers[layerIndex];
}
