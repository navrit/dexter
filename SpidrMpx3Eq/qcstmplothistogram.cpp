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
  if(this->graphCount() == 0)
    return;
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
  changeBinSize(reduction,this->graphCount()-1);
  graph->rescaleAxes();
}

void QCstmPlotHistogram::clear(){
  this->clearGraphs();
  hists.clear();
  currentHist = -1;
  this->replot();
}

void QCstmPlotHistogram::addHistogram(histogram *hist, int reduction){
  generateGraph(hist, reduction);
  replot();
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
void QCstmPlotHistogram::changeBinSize(int reduction, int histogramToChange){
  int min = hists[histogramToChange]->getMin();
  int binWidth = hists[histogramToChange]->getWidth()*reduction;
  QVector<unsigned> subsampled;
   hists[histogramToChange]->getSubsampled(reduction, &subsampled);
   QCPDataMap *data = graph(histogramToChange)->data();
   data->clear();
   for(int i = 0; i < subsampled.length();i++)
    data->insert((i+0.5)*binWidth+min, QCPData((i+0.5)*binWidth+min, ((double)subsampled[i])/binWidth));
   this->graph(currentHist)->rescaleAxes();
   replot();
}

void QCstmPlotHistogram::rebinHistograms(int binSize){
  if(currentHist == -1)
    return;
  for(int i = 0; i < this->graphCount();i++)
    changeBinSize(binSize, i);
  this->graph(currentHist)->rescaleAxes();
  replot();
}

void QCstmPlotHistogram::swapHistogram(histogram *hist, int binSize){
  hists[currentHist]  = hist;
  changeBinSize(binSize, currentHist);
}

void QCstmPlotHistogram::set_scale_full(){
  //this->rescaleAxes();
  this->changeRange(QCPRange(this->graph(currentHist)->data()->begin().key(), (this->graph(currentHist)->data()->end()-1).key()));
  this->replot();
}

void QCstmPlotHistogram::set_scale_percentile(double lower, double upper){
  double sum = 0;
  auto it = this->graph(currentHist)->data()->begin();
  do{
    sum += it.value().value;
    it++;
  }while(sum < lower);
  double lowerBound = (--it).key();
  do{
    sum += it.value().value;
    it++;
  }while(sum < upper);
  double upperBound = (--it).key();
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
        this->changeRange(QCPRange(xClicked, xReleased));
      }
}
