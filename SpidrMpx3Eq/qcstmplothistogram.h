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
  QVector<histogram*> hists;
  int currentHist = -1;
  bool clicked = false;
  double xClicked = 0, xReleased =0;
  //QVector<QVector<double>> xHist, yHist;
  QCPItemStraightLine *lowClamp, *highClamp;
  void generateGraph(histogram* Histogram, int reduction);
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

  void mouseReleaseEvent(QMouseEvent *event);
public:
  QCstmPlotHistogram(QWidget* &parent);
  virtual ~QCstmPlotHistogram();
  void changeBinSize(int binSize, int histogramToChange);
  void setHistogram(histogram *hist, int reduction, int index  );
  void addHistogram(histogram *hist, int reduction  );
  //void setData(int *data, unsigned nData);
  void clear();
  void swapHistogram(histogram *hist, int binSize);
  int getHistogramCount(){return hists.length();}
signals:
  void rangeChanged(QCPRange newRange);
  void new_range_dragged(QCPRange NewRange);
public slots:
  void set_scale_full();
  void set_scale_percentile(double lower, double upper);
  void setActive(int index);
  void changeRange(QCPRange newRange);
  void minClampChanged(double min);
  void maxClampChanged(double max);
  void rebinHistograms(int binSize);
};

#endif // QCSTMPLOTHISTOGRAM_H
