#include "qcstmplothistogram.h"

QCstmPlotHistogram::QCstmPlotHistogram(QWidget*& parent)
{
  //setMouseTracking(false);
  this->setParent(parent);
  this->addLayer("back");
  this->addLayer("front");
  this->addLayer("overlay");
  //hist = 0;
  //this->addGraph();
  setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
  axisRect()->setupFullAxesBox(true);
  xAxis->setLabel("signal");
  yAxis->setLabel("count");
  //this->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
  //this->graph(0)->setPen(QPen(Qt::black));
  lowClamp = new QCPItemStraightLine(this);    highClamp = new QCPItemStraightLine(this);
  lowClamp->setLayer("overlay"); highClamp->setLayer("overlay");
  lowClamp->setPen(QPen(Qt::blue)); highClamp->setPen(QPen(Qt::blue));
  lowClamp->point1->setCoords(-DBL_MAX,0); lowClamp->point2->setCoords(-DBL_MAX,1);
  highClamp->point1->setCoords(DBL_MAX,0); highClamp->point2->setCoords(DBL_MAX,1);
  this->addItem(lowClamp); this->addItem(highClamp);
}

QCstmPlotHistogram::~QCstmPlotHistogram()
{
  //delete hist;
}

void QCstmPlotHistogram::setHistogram(int threshold, QVector<int> data){
  setHistogram(threshold, data.data(), data.size());
}

void QCstmPlotHistogram::setHistogram(int threshold, int *data, int size){
  int index = m_mapping.contains(threshold) ? m_mapping[threshold].first : generateGraph();
  Histogram hist = Histogram(data, size);
  m_mapping[threshold] = qMakePair(index, hist);
  setPlot(index, hist);
}

/*void QCstmPlotHistogram::addHistogram(int threshold, QVector<int> data){
  addHistogram(threshold, data.data(), data.size());
}

void QCstmPlotHistogram::addHistogram(int threshold, int *data, int size){
  if(!m_mapping.contains(threshold))
    return setHistogram(threshold, data, size);
  m_mapping[threshold].second += Histogram(data, size);
  setPlot(m_mapping[threshold].first, m_mapping[threshold].second);
}*/

int QCstmPlotHistogram::generateGraph(){
  QCPGraph *newGraph = addGraph();
  newGraph->setPen(QPen(Qt::gray));
  newGraph->setLayer("back");
  newGraph->setLineStyle(QCPGraph::lsStepRight);
  return graphCount()-1;
}

void QCstmPlotHistogram::setPlot(int index, Histogram hist){
  QCPGraph *graph = this->graph(index);
  graph->clearData();
  int i;
  int sample = 0;
  for( i = hist.getMin(); i <= hist.getMax(); i+=m_binSize){
      sample = 0;
      for(int j = 0; j < m_binSize; j++)
        sample += hist.at(i+j);
      graph->addData(i, ((double)sample)/m_binSize);
    }
  graph->addData(i, ((double)sample)/m_binSize);
  graph->rescaleAxes();
  replot();
}

unsigned QCstmPlotHistogram::getTotal(int threshold){
  Histogram hist = m_mapping[threshold].second;
  unsigned sum = 0;
  for(int i = hist.getMin(); i <= hist.getMax(); i++ )
    sum += hist[i];
  return sum;
}

void QCstmPlotHistogram::rebin(){
  for(int i = 0; i < m_mapping.size();i++)
    setPlot(m_mapping.values()[i].first, m_mapping.values()[i].second);
}


void QCstmPlotHistogram::setActive(int index){
  if(this->graphCount() == 0)
    return;
  if(-1 == index)
    index = this->graphCount()-1;
  if(m_currentHist >= 0){
      this->graph(m_currentHist)->setPen(QPen(Qt::gray));
      this->graph(m_currentHist)->setLayer("back");
    }
  m_currentHist = index;
  this->graph(m_currentHist)->setPen(QPen(Qt::red));
  this->graph(m_currentHist)->setLayer("front");
  //this->graph(currentHist)->setLayer()
  this->graph(m_currentHist)->rescaleAxes();
  replot();
}

void QCstmPlotHistogram::clear(){
  this->clearGraphs();
  m_mapping.clear();
  m_currentHist = -1;
  this->replot();
}

void QCstmPlotHistogram::changeRange(QCPRange newRange){
  minClampChanged(newRange.lower);
  maxClampChanged(newRange.upper);
  emit rangeChanged(newRange);
}

void QCstmPlotHistogram::minClampChanged(double min){
  lowClamp->point1->setCoords(min,0); lowClamp->point2->setCoords(min,1);
  replot();
}
void QCstmPlotHistogram::maxClampChanged(double max){
  highClamp->point1->setCoords(max,0); highClamp->point2->setCoords(max,1);
  replot();
}

void QCstmPlotHistogram::set_scale_full(int threshold){
  int index = m_mapping[threshold].first;
  if(this->graphCount() == 0)
    return;
  this->changeRange(QCPRange(this->graph(index)->data()->begin().key(), (this->graph(index)->data()->end()-1).key()));
  this->replot();
}

void QCstmPlotHistogram::set_scale_percentile(int threshold, double lowerPercentile, double upperPercentile){

  if(this->graphCount() == 0)
    return;
  Histogram hist = m_mapping[threshold].second;
  unsigned total = 0, partialSum  = 0;
  for(int i = hist.getMin(); i <= hist.getMax(); i++)
    total += hist[i];
  int minBound = round(lowerPercentile*total), maxBound = round(upperPercentile*total);
  double lowerBound, upperBound;
  int index = hist.getMin();
  do{
      partialSum += hist[index++];
    }while(partialSum < minBound);
  lowerBound = index-1;
  while(partialSum < maxBound)
    partialSum += hist[index++];
  upperBound = index;
  this->changeRange(QCPRange(lowerBound, upperBound));
}

void QCstmPlotHistogram::mouseReleaseEvent(QMouseEvent *event){
  QCustomPlot::mouseReleaseEvent(event);
  if((event->button() == Qt::RightButton) && clicked)
    {
      clicked = false;
      xReleased = xAxis->pixelToCoord(event->x());
      maxClampChanged(xReleased);
      if(xReleased < xClicked){
          double tmp = xReleased;
          xReleased = xClicked;
          xClicked = tmp;
        };
      //this->changeRange(QCPRange(xClicked, xReleased));
      emit new_range_dragged(QCPRange(xClicked, xReleased));
    }
}
