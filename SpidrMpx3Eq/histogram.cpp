#include "histogram.h"

histogram::histogram(int *data, unsigned nData, unsigned binWidth)
{
  setData(data, nData, binWidth);
}

void histogram::setData(int *data, unsigned nData, unsigned binWidth){
  this->binWidth = binWidth;
  delete[] bins;
  scanData(data, nData);
  bins = new int[nBins];
  for(unsigned u = 0; u < nBins;u++){
      bins[u] =0;
  }
  addCount(data, nData);
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
  int max = INT_MIN;
  int min = INT_MAX;
  for(unsigned u = 0; u < n;u++){
      if(data[u] < min)
        min = data[u];
      if(data[u] > max)
        max = data[u];
    }
  this->max = max;
  this->min = min;
  this->nBins = (unsigned)(max-min+this->binWidth)/this->binWidth;
}

void histogram::addCount(int * data,  unsigned n){//TODO: add bounds checking, allow for dynamic growing. (Not neccesary for this project atm but is nice).
  for(unsigned u = 0; u < n; u++){
      int location = (data[u]-this->min)/this->binWidth;
      bins[location]++;
    }
}

void histogram::addCount(int data){
      bins[(data-this->min)/this->binWidth]++;
}

