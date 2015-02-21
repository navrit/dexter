#include "qcstmplotheatmap.h"
QCstmPlotHeatmap::QCstmPlotHeatmap(QWidget*& parent){
  this->setParent(parent);
  //aspectRatio = 1;
  colorMap = new QCPColorMap(xAxis, yAxis);
  colorMap->data()->setRange(QCPRange(0, 1), QCPRange(0,1)); //TODO: move this somewhere else, set to proper meter based distance.
  colorMap->clearData();
  setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
  axisRect()->setupFullAxesBox(true);
   xAxis->setLabel("x");
   yAxis->setLabel("y");
  this->addPlottable(colorMap);
   colorScale = new QCPColorScale(this);
   plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
   colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
   colorMap->setColorScale(colorScale); // associate the color map with the color scale
   colorScale->axis()->setLabel("Signal Strength");

   // set the color gradient of the color map to one of the presets:
   colorMap->setGradient(QCPColorGradient::gpThermal);
   colorMap->setInterpolate(false);
   colorMap->setTightBoundary(true);
   connect(colorMap, SIGNAL(dataRangeChanged(QCPRange)), this, SIGNAL(dataRangeChanged(QCPRange)));
}

QCstmPlotHeatmap::~QCstmPlotHeatmap(){

}

void QCstmPlotHeatmap::changeRange(QCPRange newRange){
  colorScale->setDataRange(newRange);
  qDebug() << "Changing range of main plot to " << newRange.lower <<"to"<< newRange.upper;
  replot();
}

void QCstmPlotHeatmap::resizeEvent(QResizeEvent *event){
  QCustomPlot::resizeEvent(event);
  this->xAxis->setScaleRatio(this->yAxis,1);
  this->replot();
}

void QCstmPlotHeatmap::setHeatmap(QCPColorGradient &gradient){
  colorMap->setGradient(gradient);
  replot();
}


void QCstmPlotHeatmap::setData(int *data, int nx, int ny){
  colorMap->data()->setRange(QCPRange(0, nx), QCPRange(0,ny));
  colorMap->clearData();
  colorMap->data()->setSize(nx, ny);
  for(unsigned u = 0;  u < ny; u++)
    for(unsigned w = 0; w < nx;w++){
      colorMap->data()->setCell(w,u, data[u*nx+w]); //TODO: read 0 here. error.
    }
  // rescale the key (x) and value (y) axes so the whole color map is visible:
  //colorMap->rescaleDataRange(true);
  //colorMap->rescaleAxes();
  this->replot();

}

void QCstmPlotHeatmap::onReplot(){//TODO: fix bugs based on fast draggin (see google)
  QCPRange rangeX = this->xAxis->range(), rangeY = this->yAxis->range();
  double xSize = rangeX.size();
  double ySize = rangeY.size();
     double lowerRangeBound = 0;
     double upperRangeBoundX =colorMap->data()->keyRange().maxRange;
     double upperRangeBoundY =colorMap->data()->valueRange().maxRange;
     if(rangeX.upper > upperRangeBoundX){  // restrict max zoom in
         rangeX.upper = upperRangeBoundX;
         rangeX.lower = upperRangeBoundX - xSize;
       }
     if(rangeY.upper > upperRangeBoundY){  // restrict max zoom i
         rangeY.upper = upperRangeBoundY;
         rangeY.lower = upperRangeBoundY - ySize;
       }
     if(rangeX.lower < lowerRangeBound){  // restrict max zoom in
         rangeX.lower = lowerRangeBound;
         rangeX.upper = lowerRangeBound + xSize;
       }
     if(rangeY.lower < lowerRangeBound){  // restrict max zoom in
         rangeY.lower = lowerRangeBound;
         rangeY.upper = lowerRangeBound + ySize;
       }
     this->xAxis->setRange(rangeX);
     this->yAxis->setRange(rangeY);
     this->xAxis->setScaleRatio(this->yAxis,1);
}

void QCstmPlotHeatmap::mousePressEvent(QMouseEvent *event){
  QCustomPlot::mousePressEvent(event);
  if(event->button() == Qt::RightButton){
      double x = this->xAxis->pixelToCoord(event->pos().x());
      double y = this->yAxis->pixelToCoord(event->pos().y());
      double z = colorMap->data()->data(x, y);
      //title->setText(QString("%3 @ (%1 , %2)").arg(x).arg(y).arg(z));
      replot();
    }
}

/*void QCstmPlotHeatmap::mouseMoveEvent(QMouseEvent *event){//TODO: uses a lot of cpu, implement differently or not at all?
    QCustomPlot::mouseMoveEvent(event);
      double x = this->xAxis->pixelToCoord(event->pos().x());
      double y = this->yAxis->pixelToCoord(event->pos().y());
      double z = colorMap->data()->data(x, y);
      title->setText(QString("%3 @ (%1 , %2)").arg(x).arg(y).arg(z));
      replot();
}*/

