#pragma once
#include <vector>
#include <iterator>
#include <iostream>
#include <QDebug>

class Histogram{
public:
enum edgeCaseBehaviourEnum{
    edgesDrop =0,
    edgesClamp = 1,
    edgesResize = 2
  };
private:
  std::vector<unsigned> m_bins;
  long m_min =0, m_max=0, m_binWidth = 1;
  static edgeCaseBehaviourEnum m_defaultEdgeCaseBehaviour;
  edgeCaseBehaviourEnum m_edgeCaseBehaviour;
  inline int valueToBin(int value) const{return (value-m_min)/m_binWidth; }
public:
  Histogram();
  Histogram(int max);
  Histogram(int min, int max);
  Histogram(int min, int max, int binSize);
  Histogram(int *data, size_t size);
  ~Histogram();
  static void setDefaultEdgeCaseBehaviour(edgeCaseBehaviourEnum behaviour){m_defaultEdgeCaseBehaviour  = behaviour;}
  void setEdgeCaseBehaviour(edgeCaseBehaviourEnum behaviour){m_edgeCaseBehaviour = behaviour;}

  inline void addCount(int value, int count = 1){
    if(value <= m_max && value >= m_min){
        int index = valueToBin(value);
        if(index >= m_bins.size() || index < 0)
          qDebug() << "Out of bounds!" << index << "/" << m_bins.size();
       m_bins[valueToBin(value)] += count;
      }
    else{
      switch(m_edgeCaseBehaviour){
        case edgesClamp:
          if(value > m_max)
            m_bins[valueToBin(m_max)] += count;
          else
            m_bins[valueToBin(m_min)] += count;
          break;
        case edgesResize:
          value > m_max? this->setMax(value) : this->setMin(value);
          addCount(value, count);
          break;
        default: //drop
          break;
      }
    }
  }
  template<typename Container>
  void addRange(Container const& container){
    for(auto it = container.begin(); it != container.end(); it++){
        addCount(*it);
    }
  }
  void addRange(int *array, int size){
    for(int i = 0; i < size;i++)
      addCount(array[i]);
  }

  inline void setWidth(int binwidth){m_binWidth = binwidth; m_bins.resize(size());}
  int getWidth() const{return m_binWidth;}
  void setMax(int max);
  int getMax() const{return m_max;}
  void setMin(int min);
  int getMin() const{return m_min;}
  void setRange(int min, int max);
  inline int size(){return (m_max-m_min+1)/m_binWidth+1;}
  inline int at(int value){return (value <= m_max && value >= m_min)? m_bins[valueToBin(value)] : 0;} //bounds checking version of [], non-reference because OOB references.
  inline int atIndex(int index){return /*(index >= 0 && index < size())?*/ m_bins[index];}

//CONTAINER WRAPPERS
  auto begin() -> decltype(m_bins.begin()){return m_bins.begin();}
  auto rbegin() -> decltype(m_bins.rbegin()){return m_bins.rbegin();}
  auto crbegin() -> decltype(m_bins.crbegin()){return m_bins.crbegin();}
  auto cbegin() -> decltype(m_bins.cbegin()) {return m_bins.cbegin();}

  auto end() -> decltype(m_bins.end()){return m_bins.end();}
  auto rend() -> decltype(m_bins.rend()) {return m_bins.rend();}
  auto crend() -> decltype(m_bins.crend()){return m_bins.crend();}
  auto cend() -> decltype(m_bins.cend()){return m_bins.cend();}

//OPERATORS
  unsigned &operator[](int value) { return m_bins[valueToBin(value)];  } //no bounds checking, editale reference
  unsigned operator[](int value) const{ return m_bins[valueToBin(value)];  } //no bounds checking, copied value.
  Histogram& operator+=(const Histogram& rhs);
  Histogram operator+(const Histogram& rhs) const;
  Histogram& operator+=(const int& rhs);
  Histogram operator+(const int& rhs) const;
} ;
