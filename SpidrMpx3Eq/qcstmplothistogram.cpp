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
void QCstmPlotHistogram::setActive(int index){
  this->graph(currentHist)->setPen(QPen(Qt::gray));
  this->graph(currentHist)->setLayer("back");
  currentHist = index;
  this->graph(currentHist)->setPen(QPen(Qt::red));
  this->graph(currentHist)->setLayer("front");
  //this->graph(currentHist)->setLayer()
  this->graph(currentHist)->rescaleAxes();
  replot();
}

void QCstmPlotHistogram::generateGraph(histogram* Histogram){
  QCPGraph* graph = addGraph();
  graph->setPen(QPen(Qt::gray));
  graph->setLineStyle(QCPGraph::lsStepCenter);
  unsigned nBins = Histogram->getNBins();
  int shift = Histogram->getMin();
  double X, Y;//TODO: make a vector to hold these, then reactivate bin rescaling.
  for(unsigned u = 0; u < nBins;u++){
    Y = Histogram->getBin(u);
    X = u+shift;
    graph->addData(X,Y);
   }
  graph->rescaleAxes();
}

void QCstmPlotHistogram::clear(){
  this->clearGraphs();
  currentHist = 0;
}

void QCstmPlotHistogram::addHistogram(histogram *hist){
  generateGraph(hist);
}

void QCstmPlotHistogram::changeRange(QCPRange newRange){
  minClampChanged(newRange.lower);
  maxClampChanged(newRange.upper);
}

void QCstmPlotHistogram::minClampChanged(double min){
  lowClamp->point1->setCoords(min,0); lowClamp->point2->setCoords(min,1);
  replot();
}
void QCstmPlotHistogram::maxClampChanged(double max){
  highClamp->point1->setCoords(max,0); highClamp->point2->setCoords(max,1);
  replot();
}

void QCstmPlotHistogram::changeBinSize(int reduction){ //TODO: fix edge case behaviour due to int truncuation, reduction negative.
  /*unsigned nBins = hist->getNBins(), reducedSize = nBins/reduction;
  int shift = hist->getMin();
  xHist.resize(reducedSize);
  yHist.resize(reducedSize);
  unsigned u;
  for(u = 0; u < reducedSize;u++){
    yHist[u] = 0;
    xHist[u] = (u*reduction+((double)reduction)/2)+shift;
    for(unsigned w = 0; w < reduction; w++)//comparision between signed and unsigned, should never be an issue as reuction should alwyas be positive.
        yHist[u] += hist->getBin(u*reduction+w);
   }
  //TODO: add aditional for here to catch the truncuated tail of the histogram.
  this->graph(currentHist)->setData(xHist, yHist);
  this->graph(currentHist)->rescaleAxes();
  this->replot();*/
}
