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
  if(-1 == index)
    index = this->graphCount()-1;
  if(currentHist >= 0){
    this->graph(currentHist)->setPen(QPen(Qt::gray));
    this->graph(currentHist)->setLayer("back");
    }
  currentHist = index;
  this->graph(currentHist)->setPen(QPen(Qt::red));
  this->graph(currentHist)->setLayer("front");
  //this->graph(currentHist)->setLayer()
  this->graph(currentHist)->rescaleAxes();
  replot();
}

void QCstmPlotHistogram::generateGraph(histogram* Histogram, int reduction){
  hists.append(Histogram);
  QCPGraph* graph = addGraph();
  graph->setPen(QPen(Qt::gray));
  graph->setLineStyle(QCPGraph::lsStepCenter);
  this->setActive(-1);
  changeBinSize(reduction);
  /*unsigned nBins = Histogram->getNBins();
  int shift = Histogram->getMin();
  double X, Y;//TODO: make a vector to hold these, then reactivate bin rescaling.
  for(unsigned u = 0; u < nBins;u++){
    Y = Histogram->getBin(u);
    X = u+shift;
    graph->addData(X,Y);
   }*/
  graph->rescaleAxes();
}

void QCstmPlotHistogram::clear(){
  this->clearGraphs();
  currentHist = 0;
  this->replot();
}

void QCstmPlotHistogram::addHistogram(histogram *hist, int reduction){
  generateGraph(hist, reduction);
  replot();
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
void QCstmPlotHistogram::changeBinSize(int reduction){
  if(currentHist < 0)
    return;
  for(int currentHist =0; currentHist < hists.length();currentHist++){
    int min = hists[currentHist]->getMin();
    int binWidth = hists[currentHist]->getWidth()*reduction;
    QVector<unsigned> subsampled;
     hists[currentHist]->getSubsampled(reduction, &subsampled);
     QCPDataMap *data = graph(currentHist)->data();
     data->clear();
     for(int i = 0; i < subsampled.length();i++)
      data->insert((i+0.5)*binWidth+min, QCPData((i+0.5)*binWidth+min, ((double)subsampled[i])/binWidth));
    }
  this->graph(currentHist)->rescaleAxes();
   replot();
}

void QCstmPlotHistogram::swapHistogram(histogram *hist, int binSize){
  hists[currentHist]  = hist;
  changeBinSize(binSize);
}
