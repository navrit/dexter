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
  QMap<int, QPair<int, Histogram*>> m_mapping;
  int m_binCount = 1;
  int m_currentHist = -1;
  bool clicked = false;
  double xClicked = 0, xReleased =0;
  //QVector<QVector<double>> xHist, yHist;
  QCPItemStraightLine *lowClamp, *highClamp;

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
  void rebin();
  int generateGraph();

public:
  QCstmPlotHistogram(QWidget* &parent);
  virtual ~QCstmPlotHistogram();

  void setHistogram(int threshold, int* data, int size);
  void setHistogram(int threshold, QVector<int> data);

  void addHistogram(int threshold, int* data, int size);
  void addHistogram(int threshold, QVector<int> data);

  void setPlot(int index, Histogram *hist);

  int getMin(int threshold){return m_mapping[threshold].second->getMin();}
  int getMax(int threshold){return m_mapping[threshold].second->getMax();}
  unsigned getBin(int threshold, int level){return m_mapping[threshold].second->at(level);}
  unsigned getTotal(int threshold);

  void clear();
  int getHistogramCount(){return m_mapping.size();}
  Histogram* getHistogram(int threshold){return m_mapping[threshold].second;}
  void scaleToInterest();
signals:
  void rangeChanged(QCPRange newRange);
  void new_range_dragged(QCPRange NewRange);
public slots:
  void set_scale_full(int threshold);
  void set_scale_percentile(int threshold, double lower, double upper);
  void setActive(int index);
  void changeRange(QCPRange newRange);
  void minClampChanged(double min);
  void maxClampChanged(double max);
  void changeBinCount(int binCount){m_binCount = binCount;}
  //void rebinHistograms(int binSize)
};

#endif // QCSTMPLOTHISTOGRAM_H
