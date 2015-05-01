#include "dataset.h"
#include <QDataStream>

Dataset::Dataset(int x, int y, int framesPerLayer, int layers)
{
  m_nx = x; m_ny = y;
  m_nFrames = framesPerLayer;
  resizeContainers();
}

Dataset::~Dataset()
{
  for(int i = 0; i < m_layers.length();i++)
    delete[] m_layers.at(i);
}

QByteArray Dataset::toByteArray(){
  QByteArray ret(0);
  ret += QByteArray::fromRawData((const char*)&m_nx, (int)sizeof(m_nx));
  ret += QByteArray::fromRawData((const char*)&m_ny, (int)sizeof(m_ny));
  ret += QByteArray::fromRawData((const char*)&m_nFrames, (int)sizeof(m_nFrames));
  int layerCount = m_layers.length();
  ret += QByteArray::fromRawData((const char*)&layerCount, (int)sizeof(layerCount));
  ret += QByteArray::fromRawData((const char*)m_frameLayouts.data(),(int)(m_nFrames*sizeof(*m_frameLayouts.data())));
  ret += QByteArray::fromRawData((const char*)m_frameOrientation.data(), (int)(m_nFrames*sizeof(*m_frameOrientation.data())));
  ret += QByteArray::fromRawData((const char*)m_Thresholds.data(),(int)( m_nFrames*sizeof(*m_Thresholds.data())));
  for(int i = 0; i < m_layers.length(); i++)
    for(int j = 0; j < m_nFrames; j++)
      ret += QByteArray::fromRawData((const char*)this->getFrame(j,i), (int)(sizeof(**m_layers.data())*m_nx*m_ny));
  return ret;
}

void Dataset::fromByteArray(QByteArray serialized){
  QDataStream in(&serialized, QIODevice::ReadOnly);
  in.readRawData((char*)&m_nx, (int)sizeof(m_nx));
  in.readRawData((char*)&m_ny, (int)sizeof(m_ny));
  in.readRawData((char*)&m_nFrames, (int)sizeof(m_nFrames));
  resizeContainers();
  int layerCount;
  in.readRawData((char*)&layerCount, (int)sizeof(layerCount));
  setLayerCount(layerCount);
  in.readRawData((char*)m_frameLayouts.data(), m_nFrames*(int)sizeof(*m_frameLayouts.data()));
  in.readRawData((char*)m_frameOrientation.data(), m_nFrames*(int)sizeof(*m_frameOrientation.data()));
  in.readRawData((char*)m_Thresholds.data(), m_nFrames*(int)sizeof(*m_Thresholds.data()));
  for(int i = 0; i < m_layers.length(); i++)
    for(int j = 0; j < m_nFrames; j++)
      in.readRawData((char*)this->getFrame(j,i), (int)sizeof(**m_layers.data())*m_nx*m_ny);
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

void Dataset::addFrame(int *frame, int index, int layer){
  int *newFrame = this->getFrame(index, layer);
  for(int i = 0 ; i < m_nx*m_ny;i++)
    newFrame[i] = frame[i];
}

int* Dataset::getFrame(int index, int layer){
  if(layer == -1)
    layer = m_layers.length();
  return m_layers[layer]+m_nx*m_ny*index;
}

void Dataset::setFramesPerLayer(int nFrames){
  m_nFrames = nFrames;

}

void Dataset::sumFrame(int * frame, int index, int layer){
  int* oldFrame = getFrame(index, layer);
  for(int i = 0; i < m_nx*m_ny; i++)
    oldFrame[i] += frame[i];
}

int Dataset::sample(int x, int y, int layer){
  if(layer >= m_layers.length())
    return 0;
  QPoint layoutSample(x/m_nx, y/m_ny);
  int remainderX = x%m_nx, remainderY= y%m_ny;
  for(int i = 0; i < m_frameLayouts.length();i++){
      if(layoutSample == m_frameLayouts[i])
        return getFrame(i, layer)[remainderY*m_nx+remainderX];
    }
  return 0;
}

void Dataset::resizeContainers(){
  m_frameOrientation.resize(m_nFrames);
  m_frameLayouts.resize(m_nFrames);
  m_Thresholds.resize(m_nFrames);
}

void Dataset::setLayerCount(int nLayers){
  int oldLayerCount = this->getLayerCount();
  for(int i = nLayers; i < oldLayerCount;i++)
    delete[] m_layers.at(i);
  m_layers.resize(nLayers);
  for(int i = oldLayerCount; i < nLayers;i++){
      m_layers[i] = new int[m_nx*m_ny*m_nFrames];
      //m_frameOrientation[i] = Dataset::orientationLtRTtB;
      //m_frameLayouts[i] = QPoint(0,0);
    }
}

void Dataset::clear(){
  m_activeFrame = 0;
  for(int i = 0; i < m_layers.length();i++)
    delete[] m_layers.at(i);
  m_layers.clear();
}

void Dataset::setLayer(int *data, int layer){
  if(layer == -1)
    layer = m_layers.length();
  if(layer >= m_layers.length())
    setLayerCount(layer+1);
  for(int i = 0; i < m_nx*m_ny*m_nFrames;i++)
    m_layers[layer][i] = data[i];
}

void Dataset::addLayer(int *data, int layer){
  if(m_layers.length() <= layer|| m_layers.length() == 0)
    return setLayer(data, layer);
  if(layer == -1)
    layer = m_layers.length()-1;
  for(int i = 0; i < m_nx*m_ny*m_nFrames;i++)
    m_layers[layer][i] += data[i];
}

int* Dataset::getLayer(int layer){
  if(layer == -1)
    layer = m_layers.length()-1;
  return m_layers[layer];
}

QVector <int*> Dataset::getFrames(){
  QVector<int*> ret(0);
  for(int i = 0; i < getLayerCount(); i++){
      for(int j = 0; j < m_nFrames;j++)
        ret.append(getFrame(j,i));
    }
  return ret;
}

