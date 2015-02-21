#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <QDebug>
class histogram
{
  int max, min;
  unsigned *bins =0, nBins, binWidth;
  void scanData(int *data, unsigned nData);
  void setData(int * data,  unsigned nData, unsigned binWidth = 1);
public:
  unsigned* getBins(){return bins;}
  unsigned getBin(unsigned index){return bins[index];}
  unsigned getNBins(){return nBins;}
  int getMin(){return min;}
  int getMax(){return max;}
  histogram();
  histogram(int *data, unsigned nData, unsigned binWidth);
  void addCount(int * data,  unsigned n);
  void addCount(int data);
  ~histogram();
};

#endif // HISTOGRAM_H
