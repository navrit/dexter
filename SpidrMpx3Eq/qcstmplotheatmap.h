#ifndef QCSTMPLOTHEATMAP_H
#define QCSTMPLOTHEATMAP_H

#include <QObject>
#include <QWidget>
#include <QResizeEvent>
#include <QDebug>
#include <QContextMenuEvent>

#include "qcustomplot.h"

#include <stdio.h>

class QCstmPlotHeatmap : public QCustomPlot
{
  Q_OBJECT
  void resizeEvent(QResizeEvent *event);
  QPoint toPixel(QPoint screenspace);
  //void mouseMoveEvent(QMouseEvent *event);
  //void toolTipEvent(QToolTip *event);
  ~QCstmPlotHeatmap();
  QList<QCPColorMap*> colorMaps;//TODO: not dynamically allocated?
  QCPColorScale *colorScale;
  //QCPColorGradient currentGradient;
  int active = -1;
public:
  bool event(QEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void contextMenuEvent(QContextMenuEvent *event);
  void addData(int *data, int nx, int ny);
  void setData(int *data, int nx, int ny);
  void clear();
  void setHeatmap(QCPColorGradient gradient);
  void rescaleAxes();
  QCstmPlotHeatmap(QWidget*& parent);
  int GetLastActive(){ return active; };
signals:
  void dataRangeChanged(QCPRange newRange);
  void plotCountChanged(int nPlots);
  void activePlotChanged(int activePlot);
  void mouseOverChanged(QString sample);
  void pixel_selected(QPoint pixel, QPoint pos);
 public slots:
  //void onXRangeChanged(QCPRange newRange);
  void setActive(int);
  void changeRange(QCPRange newRange);
  void onReplot();

};

#endif // QCSTMPLOTHEATMAP_H
