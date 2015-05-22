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
  int index;
  if(m_mapping.contains(threshold) ){
    index =m_mapping[threshold].first;
    delete m_mapping[threshold].second;
  }
  else
    index = generateGraph();

  //QElapsedTimer timer;
  //timer.start();
  //hist->setWidth(m_binSize);
  int min = INT_MAX, max = INT_MIN;
  for(int i = 0; i < size; i++){
    if(data[i] < min)
      min = data[i];
    if(data[i] > max)
      max = data[i];
    }
  Histogram *hist = new Histogram(min, max, m_binSize);
  hist->addRange(data, size);
  //qDebug() << "Histogram creation took" << timer.elapsed() << "milliseconds";

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

void QCstmPlotHistogram::setPlot(int index, Histogram *hist){
  //QElapsedTimer timer;

  //timer.start();
  QCPGraph *graph = this->graph(index);
  graph->clearData();
  int i;
  int sample, oldSample = hist->atIndex(0)-1;
  for( i = 0; i < hist->size(); i++){
      sample = hist->atIndex(i);
      if(sample != oldSample){
          graph->addData(i*m_binSize+hist->getMin(), ((double)sample));
          oldSample = sample;
        }
    }
  graph->addData(i*m_binSize+hist->getMin(), ((double)hist->atIndex(i)));
  graph->rescaleAxes();
  replot();
  //qDebug() << "Histogram plot took " << timer.elapsed() << "milliseconds";
}

void QCstmPlotHistogram::scaleToInterest(){
  double x0 = lowClamp->point1->coords().x();
  double x1 = highClamp->point1->coords().x();
  double delta = x1-x0;
  this->xAxis->setRange(x0-delta*0.1, x1+delta*0.1);
  this->replot();
}

unsigned QCstmPlotHistogram::getTotal(int threshold){
  Histogram *hist = m_mapping[threshold].second;
  unsigned sum = 0;
  for(int i = hist->getMin(); i <= hist->getMax(); i++ )
    sum += hist->at(i);
  return sum;
}

void QCstmPlotHistogram::rebin(){
  /*for(int i = 0; i < m_mapping.size();i++)
    setPlot(m_mapping.values()[i].first, m_mapping.values()[i].second);*/
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
  for(auto it = m_mapping.begin(); it != m_mapping.end(); it++)
    delete it.value().second;
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
  Histogram *hist = m_mapping[threshold].second;
  unsigned total = 0, partialSum  = 0;
  for(int i = hist->getMin(); i <= hist->getMax(); i++)
    total += hist->at(i);
  int minBound = round(lowerPercentile*total), maxBound = round(upperPercentile*total);
  double lowerBound, upperBound;
  int index = hist->getMin();
  do{
      partialSum += hist->at(index++);
    }while(partialSum < minBound);
  lowerBound = index-1;
  while(partialSum < maxBound)
    partialSum += hist->at(index++);
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
