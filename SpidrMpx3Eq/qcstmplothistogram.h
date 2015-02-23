#ifndef QCSTMPLOTHISTOGRAM_H
#define QCSTMPLOTHISTOGRAM_H

#include <QObject>
#include <QWidget>
#include <QResizeEvent>
#include <float.h>
#include "qcustomplot.h"

#include "histogram.h"

class QCstmPlotHistogram : public QCustomPlot
{
 Q_OBJECT
  //QVector<histogram> hist;
  int currentHist = 0;
  bool clicked = false;
  double xClicked = 0, xReleased =0;
  //QVector<QVector<double>> xHist, yHist;
  QCPItemStraightLine *lowClamp, *highClamp;
  void generateGraph(histogram* Histogram);
  void mousePressEvent(QMouseEvent *event){//TODO: check if clicked inside graph.
    QCustomPlot::mousePressEvent(event);
    if(event->button() == Qt::RightButton)
        {
          clicked = true;
          xClicked = xAxis->pixelToCoord(event->x());
          minClampChanged(xClicked);
        }
  }
  void mouseMoveEvent(QMouseEvent *event){ //TODO: don't update actual range, only markers. Update range on release.
    QCustomPlot::mouseMoveEvent(event);
    if(clicked)
        maxClampChanged(xAxis->pixelToCoord(event->x()));
  }

  void mouseReleaseEvent(QMouseEvent *event){ //check if released inside graph.
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
          qDebug() << "dragged: " << xClicked << "to" << xReleased;
          emit rangeChanged(QCPRange(xClicked, xReleased));
        }
  }

public:
  QCstmPlotHistogram(QWidget* &parent);
  virtual ~QCstmPlotHistogram();
  void addHistogram(histogram *hist);
  //void setData(int *data, unsigned nData);
  void clear();
signals:
  void rangeChanged(QCPRange newRange);
public slots:
  void setActive(int index);
  void changeRange(QCPRange newRange);
  void minClampChanged(double min);
  void maxClampChanged(double max);
  void changeBinSize(int binSize);
};

#endif // QCSTMPLOTHISTOGRAM_H
