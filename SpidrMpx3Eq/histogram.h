#pragma once
#include <vector>
#include <iterator>
#include <iostream>
class Histogram{
public:
enum edgeCaseBehaviourEnum{
    edgesDrop =0,
    edgesClamp = 1,
    edgesResize = 2
  };
private:
  std::vector<unsigned> m_bins;
  int m_min =0, m_max=0, m_binWidth = 1, m_binCount;
  static edgeCaseBehaviourEnum m_defaultEdgeCaseBehaviour;
  edgeCaseBehaviourEnum m_edgeCaseBehaviour;
public:
  Histogram();
  Histogram(int max);
  Histogram(int min, int max);
  Histogram(int *data, size_t size);
  ~Histogram();
  static void setDefaultEdgeCaseBehaviour(edgeCaseBehaviourEnum behaviour){m_defaultEdgeCaseBehaviour  = behaviour;}
  void setEdgeCaseBehaviour(edgeCaseBehaviourEnum behaviour){m_edgeCaseBehaviour = behaviour;}
  inline void addCount(int value, int count = 1){
    if(value <= m_max && value >= m_min)
      m_bins[value-m_min] += count;
    else{
      switch(m_edgeCaseBehaviour){
        case edgesClamp:
          (value > m_max? m_bins[value-m_min] : m_bins[0]) += count;
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
      Histogram::addCount(*it);
    }
  }
  void setBinCount(int binCount){

  }
  void setMax(int max);
  int getMax() const{return m_max;}
  void setMin(int min);
  int getMin() const{return m_min;}
  void setRange(int min, int max);
  inline int size(){return m_max-m_min+1;}
  inline int at(int value){return (value <= m_max && value >= m_min)? m_bins[value-m_min] : 0;} //bounds checking version of [], non-reference because OOB references.

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
  unsigned &operator[](int value) { return m_bins[value-m_min];  } //no bounds checking, editale reference
  unsigned operator[](int value) const{ return m_bins[value-m_min];  } //no bounds checking, copied value.
  Histogram& operator+=(const Histogram& rhs);
  Histogram operator+(const Histogram& rhs) const;
  Histogram& operator+=(const int& rhs);
  Histogram operator+(const int& rhs) const;
} ;
