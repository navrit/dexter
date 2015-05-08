#include "histogram.h"
#include <iostream>

histogram::histogram(int *data, unsigned nData, unsigned binWidth)
{
  setData(data, nData, binWidth);
}

void histogram::setData(int *data, unsigned nData, unsigned binWidth){
  delete[] bins;
  bins = nullptr;
  max = INT_MIN;
  min = INT_MAX;
  total = 0;
  this->binWidth = binWidth;
  addCount(data, nData);
  std::cout << "added data, bounds are " << min << ", " << max << std::endl;
}

void histogram::getSubsampled(unsigned reduction, QVector<unsigned> *data){
  for(unsigned u = 0; u < nBins;u+=reduction){
      unsigned sum = 0;
      for(unsigned w = 0; (w < reduction) && (w+u < nBins);w++)
        sum += bins[u+w];
        data->append(sum);
    }
}

histogram::histogram()
{
}

histogram::~histogram() //double detor because of copy.
{
  delete[] bins;
}

void histogram::scanData(int *data, unsigned n){
  int oldMin = min, oldMax = max;
  for(unsigned u = 0; u < n;u++){
      if(data[u] < min)
        min = data[u];
      if(data[u] > max)
        max = data[u];
 }
  std::cout << "scanned` data, bounds are " << min << ", " << max << std::endl;
  if((oldMin != min)|| (oldMax != max)){
      int oldnBins = nBins;
      this->nBins = (unsigned)(max-min+this->binWidth)/this->binWidth;
      int *newBins = new int[nBins];
      for(int i = 0; i < nBins;i++)
        newBins[i] = 0;
      if(bins != nullptr)
        for(int i = 0; i < oldnBins; i++)
          newBins[-min+oldMin+i] = bins[i];
      delete[] bins;
      bins = newBins;
    }
}

void histogram::addCount(int * data,  unsigned n){//TODO: add bounds checking, allow for dynamic growing. (Not neccesary for this project atm but is nice).
  scanData(data, n);
  for(unsigned u = 0; u < n; u++){
      int location = (data[u]-this->min)/this->binWidth;
      bins[location]++;
      total += location;
    }
}

void histogram::addCount(int data){
      bins[(data-this->min)/this->binWidth]++;
}

