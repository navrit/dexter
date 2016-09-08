#include "qcstmplotheatmap.h"
#include <iostream>
QCstmPlotHeatmap::QCstmPlotHeatmap(QWidget*& parent){
    this->setParent(parent);
    //aspectRatio = 1;
    //colorMap = new QCPColorMap(xAxis, yAxis);
    //colorMap->data()->setRange(QCPRange(0, 1), QCPRange(0,1)); //TODO: move this somewhere else, set to proper meter based distance.
    //colorMap->clearData();
    setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    axisRect()->setupFullAxesBox(true);
    //xAxis->setLabel("x");
    //yAxis->setLabel("y");
    colorScale = new QCPColorScale(this);
    plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    //colorMap->setColorScale(colorScale); // associate the color map with the color scale
    //colorScale->axis()->setLabel("Signal Strength");

    // set the color gradient of the color map to one of the presets:
    //colorMap->setGradient(QCPColorGradient::gpThermal);

}

QCstmPlotHeatmap::~QCstmPlotHeatmap(){
}

void QCstmPlotHeatmap::changeRange(QCPRange newRange){
    colorScale->setDataRange(newRange);
    replot();
}

void QCstmPlotHeatmap::resizeEvent(QResizeEvent *event){
    QCustomPlot::resizeEvent(event);
    this->xAxis->setScaleRatio(this->yAxis,1);
    this->replot();
}

void QCstmPlotHeatmap::setHeatmap(QCPColorGradient gradient){
    colorScale->setGradient(gradient);
    replot();
}

void QCstmPlotHeatmap::clear(){
    this->clearPlottables();
    colorMaps.clear();
    active = -1;
    emit(plotCountChanged(0));
    replot();
}

void QCstmPlotHeatmap::addData(int *data, int nx, int ny){
    QCPColorMap* newMap = new QCPColorMap(xAxis, yAxis);
    //qDebug() << "Allocated new map, " << newMap;
    colorMaps.append(newMap);
    newMap->setInterpolate(false);
    connect(newMap, SIGNAL(dataRangeChanged(QCPRange)), this, SIGNAL(dataRangeChanged(QCPRange)));//TODO: is this correct?
    newMap->clearData();
    newMap->data()->setRange(QCPRange(0, nx), QCPRange(0,ny));
    newMap->data()->setSize(nx,ny);
    for(unsigned u = 0;  u < (unsigned)ny; u++)
        for(unsigned w = 0; w < (unsigned)nx;w++){
            newMap->data()->setCell(w,ny-1-u, data[u*nx+w]); //TODO: read 0 here. error.
        }
    this->addPlottable(newMap);
    // rescale the key (x) and value (y) axes so the whole color map is visible:
    newMap->rescaleAxes();
    newMap->rescaleDataRange(true);//TODO: this doesn't do anything...?
    this->xAxis->setScaleRatio(this->yAxis,1);
    newMap->setColorScale(colorScale);
    colorScale->rescaleDataRange(true);
    newMap->setVisible(true);
    setActive(colorMaps.count()-1);
    emit(plotCountChanged(colorMaps.count()-1));
}

void QCstmPlotHeatmap::setData(int *data, int nx, int ny){
    if( -1 == active ) {
        return addData(data, nx, ny);
    }
    //qDebug() << "QCstmPlotHeatmap::setData" << active;
    colorMaps[active]->clearData();
    colorMaps[active]->data()->setRange(QCPRange(0, nx), QCPRange(0,ny));
    colorMaps[active]->data()->setSize(nx, ny);

    for(unsigned u = 0;  u < (unsigned)ny; u++)
        for(unsigned w = 0; w < (unsigned)nx;w++){
            //colorMaps[active]->data()->setCell(w,ny-1-u, data[u*nx+w]); //TODO: read 0 here. error.
            colorMaps[active]->data()->setCell(w,u, data[u*nx+w]); //TODO: read 0 here. error.
        }

    colorMaps[active]->rescaleDataRange(true);
    colorScale->rescaleDataRange(true);
    replot();
}

void QCstmPlotHeatmap::setActive(int index){
    if(!colorMaps.count()){
        return;
    }
    if(index >= colorMaps.count() )
        index = 0;
    if(index == active)
        return;
    if(active >= 0){
        //qDebug() << "Masked frame " << active;
        colorMaps[active]->setVisible(false);
    }
    colorMaps[index]->setVisible(true);
    active = index;
    //qDebug() << "now active: " << active;
    emit(activePlotChanged(active));
    colorScale->rescaleDataRange(true);
    this->replot();
}

void QCstmPlotHeatmap::rescaleAxes(void){
    if(0 <= active){
        colorMaps[active]->rescaleAxes();
        colorMaps[active]->rescaleDataRange();
        this->xAxis->setScaleRatio(this->yAxis,1);
    }
}


void QCstmPlotHeatmap::onReplot(){//TODO: fix bugs based on fast draggin (see google)
    QCPRange rangeX = this->xAxis->range(), rangeY = this->yAxis->range();
    double xSize = rangeX.size();
    double ySize = rangeY.size();
    double lowerRangeBound = 0;
    double upperRangeBoundX =colorMaps[active]->data()->keyRange().maxRange;
    double upperRangeBoundY =colorMaps[active]->data()->valueRange().maxRange;
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

void QCstmPlotHeatmap::mouseMoveEvent(QMouseEvent *event){//TODO: uses a lot of cpu, implement differently or not at all?
    QCustomPlot::mouseMoveEvent(event);
    if(-1 != active){
        double x = this->xAxis->pixelToCoord(event->pos().x());
        double y = this->yAxis->pixelToCoord(event->pos().y());
        double z = colorMaps[active]->data()->data(x, colorMaps[active]->data()->valueSize() -y);
        emit(mouseOverChanged(QString("%3 @ (%1 , %2)").arg(round(x)).arg(round(y)).arg(z)));
    }
}

bool QCstmPlotHeatmap::event(QEvent *event){
    if (event->type() == QEvent::ToolTip)
        if(active >= 0){
            QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
            QPoint pixel = toPixel(helpEvent->pos());
            double z = colorMaps[active]->data()->data(pixel.x(), colorMaps[active]->data()->valueSize() -pixel.y());
            QToolTip::showText(helpEvent->globalPos(), QString("%3 @ (%1 , %2)").arg(pixel.x()).arg(pixel.y()).arg(z));
            return true;
        }
    return QWidget::event(event);
}

void QCstmPlotHeatmap::contextMenuEvent(QContextMenuEvent *event){
    emit(pixel_selected(toPixel(event->pos()), event->globalPos()));
    event->accept();
}

QPoint QCstmPlotHeatmap::toPixel(QPoint screenspace){
    return QPoint(round(this->xAxis->pixelToCoord(screenspace.x())),  round(this->xAxis->pixelToCoord(screenspace.y())));
}
