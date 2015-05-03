#include "dataset.h"
#include <QDataStream>

Dataset::Dataset(int x, int y, int framesPerLayer, int layers)
{
  m_nx = x; m_ny = y;
  setFramesPerLayer(framesPerLayer);
}

Dataset::~Dataset()
{
  for(int i =0; i < m_layers.size(); i++)
    delete[] m_layers[i];
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
  for(int i = 0; i < m_layers.size(); i++){
      for(int j = 0; j < m_nFrames; j++){
          in.readRawData((char*)frameBuffer.data(), (int)sizeof(int)*frameBuffer.size());
          this->setFrame(frameBuffer.data(), j, keys[i]);
        }
    }
}

void Dataset::clear(){
  for(int i =0; i < m_layers.size(); i++)
    delete[] m_layers[i];
  m_thresholdsToIndices.clear();
  setFramesPerLayer(0);
}

void Dataset::computeBoundingBox(){
  /*m_boundingBox.setRect(0,0,0,0);
  for(int i =0; i < m_frameLayouts.length();i++){
    if(!m_boundingBox.contains(m_frameLayouts[i])){
        m_boundingBox.united()
      }*/
}

/*void Dataset::addFrames(QVector<int *> frames){
  for(int i = 0; i < frames.length(); i++)
    this->addFrame(frames[i]);
}*/

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

int Dataset::sample(int x, int y, int threshold){
  int layerIndex = thresholdToIndex(threshold);
  if(layerIndex == -1)
    return 0;
  QPoint layoutSample(x/m_nx, y/m_ny);
  int remainderX = x%m_nx, remainderY= y%m_ny;
  for(int i = 0; i < m_frameLayouts.length();i++){
      if(layoutSample == m_frameLayouts[i])//TODO: orientation messes up sampling!
        return 0;// getFrame(i, layer)[remainderY*m_nx+remainderX];
    }
  return 0;
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
