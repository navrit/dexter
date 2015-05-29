/**
 * \class Histogram
 *
 * \brief A histogram for integer data.
 *
 * This class creates a dense histogram for integer datasets. It features a tunable edge-case behaviour and can easily be accesed and modified using operators.
 *
 */

#pragma once
#include <vector>
#include <iterator>
#include <iostream>
#include <QDebug>

class Histogram{
public:
enum edgeCaseBehaviourEnum{
    edgesDrop =0,//!< Any out-of-bounds data points should be dropped.
    edgesClamp = 1, //!< Any out-of-bounds-data points should be added to the nearest bin.
    edgesResize = 2 //!< If any out-of-bounds data points are found, resize the histogram to fit.
  };//!< Specifies the edge-case behaviour.
private:
  std::vector<unsigned> m_bins;
  long m_min =0, m_max=0, m_binWidth = 1;
  static edgeCaseBehaviourEnum m_defaultEdgeCaseBehaviour;
  edgeCaseBehaviourEnum m_edgeCaseBehaviour;
  inline int valueToBin(int value) const{return (value-m_min)/m_binWidth; }
public:
  Histogram();//!< Create a histogram with a range of [0,0] and binwidth of 1.
  Histogram(int max);//!< Create a histogram with a range of [0, max] and a binwidth of 1.
  Histogram(int min, int max);//!< Create a histogram with a range of [min, max] and a binwidth of 1
  Histogram(int min, int max, int binSize);//! Create a histogram with a range of [min, max] and a binwith of binSize.
  Histogram(int *data, size_t size);//! Create a histogram with a range equal to that of the span of the data and a binSize of 1.
  ~Histogram();
  static void setDefaultEdgeCaseBehaviour(edgeCaseBehaviourEnum behaviour){m_defaultEdgeCaseBehaviour  = behaviour;} //!< Sets the default edgeCaseBehaviour of new histograms.
  void setEdgeCaseBehaviour(edgeCaseBehaviourEnum behaviour){m_edgeCaseBehaviour = behaviour;}//!< Sets the edge case behaviour of this histogram.

  //!Adds count to the bin corresponding to value.
  inline void addCount(int value, int count = 1){
    if(value <= m_max && value >= m_min){
        int index = valueToBin(value);
        if(index >= (int)m_bins.size() || index < 0)
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
  //!Calls addCount for each element in the container.
  template<typename Container>
  void addRange(Container const& container){
    for(auto it = container.begin(); it != container.end(); it++){
        addCount(*it);
    }
  }
  //!Calss addCount for each element in the array.
  void addRange(int *array, int size){
    for(int i = 0; i < size;i++)
      addCount(array[i]);
  }
  //! Changes the binWidth of the histogram and resizes it. Afterwards the data is in an undefined state.
  inline void setWidth(int binwidth){m_binWidth = binwidth; m_bins.resize(size());}
  int getWidth() const{return m_binWidth;}
  void setMax(int max);
  int getMax() const{return m_max;}
  void setMin(int min);
  int getMin() const{return m_min;}
  void setRange(int min, int max);
  //! Returns the amount of bins.
  inline int size(){return (m_max-m_min+1)/m_binWidth+1;}
  //!Returns the size of the bin corresponding to value. If value is out-of-bounds, returns 0.
  inline int at(int value){return (value <= m_max && value >= m_min)? m_bins[valueToBin(value)] : 0;} //bounds checking version of [], non-reference because OOB references.
   //! Returns the size of the bin at the specified index. Does not do bounds checking.
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
  //! Get an mutable reference to the bin corresponding to value.
  unsigned &operator[](int value) { return m_bins[valueToBin(value)];  } //no bounds checking, editale reference
  //! Get an immutable reference to the bin corresponding to value.
  unsigned operator[](int value) const{ return m_bins[valueToBin(value)];  } //no bounds checking, copied value.
  //! Add each element of rhs to this. Follows the normal EdgeCaseBehaviour.
  Histogram& operator+=(const Histogram& rhs);
  //! Add each element of rhs to a copy of this and returns the result. Follows the normal EdgeCaseBehaviour.
  Histogram operator+(const Histogram& rhs) const;
  //! Add one count to the bin corresponding to rhs.
  Histogram& operator+=(const int& rhs);
  //! Add one count to the bin corresponding to rhs.
  Histogram operator+(const int& rhs) const;
} ;
