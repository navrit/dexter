#include <histogram.h>
#include <limits.h>
Histogram::edgeCaseBehaviourEnum Histogram::m_defaultEdgeCaseBehaviour = Histogram::edgesDrop;

Histogram::Histogram(): Histogram(0){
}

Histogram::Histogram(int max):Histogram(0,max){
}

Histogram::Histogram(int* data, size_t size) : Histogram(){
  int min = INT_MAX, max = INT_MIN;
  for(int i = 0; i < size; i++){
      if(data[i] < min)
        min = data[i];
      if(data[i] > max)
        max = data[i];
  }setRange(min, max);
  for(int i = 0; i < size; i++){
    *this += data[i];
  }
  setEdgeCaseBehaviour(m_defaultEdgeCaseBehaviour);
}

Histogram::Histogram(int min, int max){
  m_min = min;
  m_max = max;
  m_bins.resize(size());
  for(auto it = begin(); it  != end();it++)
    (*it) = 0;
  setEdgeCaseBehaviour(m_defaultEdgeCaseBehaviour);
}

Histogram::~Histogram(){
}

void Histogram::setMax(int max){
  if(max < m_min)
    m_min = max;
  const int offset = size();
  m_max = max;
  m_bins.resize(size());
  for(int i = offset; i < size(); i++)
    m_bins[i] = 0;
}

void Histogram::setMin(int min){
  if(min > m_max)
    setMax(min);
  int old_min = m_min;
  m_min = min;
  std::vector<unsigned> new_bins(size());
  if(m_min < old_min){ //if we need to prepend
      const int offset = old_min-m_min;
      for(int i = 0; i < offset; i++)//set the first bins to 0.
        new_bins[i] = 0;
      for(int i = 0; i < m_bins.size(); i++)//copy the rest.
        new_bins[i+offset] = m_bins[i];
    }
  else{//if we need to drop the first bins.
      const int offset = m_min-old_min;//TODO: respect edgeCaseBehaviour
      new_bins.assign(m_bins.begin()+offset, m_bins.end());
    }
  m_bins.swap(new_bins);
}

void Histogram::setRange(int min, int max){//TODO: test this when less sleepy
  setMin(min);
  setMax(max);
}

Histogram& Histogram::operator+=(const int& rhs){
  addCount(rhs);
  return *this;
}

Histogram& Histogram::operator+=(const Histogram& rhs){
  const int min = rhs.getMin();
  const int max = rhs.getMax();
  if(m_edgeCaseBehaviour == Histogram::edgesResize){
      if(min < getMin())
        setMin(min);
      if(max > getMax())
        setMax(max);
    }
  for(int i = min; i <= max; i++)
    addCount(i,rhs[i]);
  return *this;
}

Histogram Histogram::operator+(const Histogram& rhs) const{//TODO: edge case behaviour inhereted?
  Histogram ret = *this;
  ret += rhs;
  return ret;
}

Histogram Histogram::operator+(const int& rhs) const{
  Histogram ret = *this;
  ret += rhs;
  return ret;
}
