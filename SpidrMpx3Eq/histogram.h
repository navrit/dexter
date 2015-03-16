#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <QDebug>
class histogram
{
  int max, min;
  int *bins =nullptr;
  unsigned nBins, binWidth =1;
  void scanData(int *data, unsigned nData);
  void setData(int * data,  unsigned nData, unsigned binWidth = 1);
public:
  int* getBins(){return bins;}
  unsigned getBin(unsigned index){return bins[index];}
  unsigned getNBins(){return nBins;}
  unsigned getWidth(){return binWidth;}
  int getMin(){return min;}
  int getMax(){return max;}
  void getSubsampled(unsigned reduction, QVector<unsigned> *data);
  histogram();
  histogram(int *data, unsigned nData, unsigned binWidth = 1);
  void addCount(int * data,  unsigned n);
  void addCount(int data);
  ~histogram();
};

#endif // HISTOGRAM_H
