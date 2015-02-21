#include "histogram.h"

histogram::histogram(int *data, unsigned nData, unsigned binWidth = 1)
{
  setData(data, nData, binWidth);
}

void histogram::setData(int *data, unsigned nData, unsigned binWidth){
  this->binWidth = binWidth;
  qDebug() << "deleting" << bins;
  delete[] bins;
  scanData(data, nData);
  bins = new unsigned[nBins];
  qDebug() << "allocated" << bins;
  for(unsigned u = 0; u < nBins;u++){
      bins[u] =0;
  }
  addCount(data, nData);
}

histogram::histogram()
{
}

histogram::~histogram() //double detor because of copy.
{
  qDebug() << "detoring" << bins << "from" << this;
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
  qDebug() << min << "to" << max << "->" << nBins;
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

