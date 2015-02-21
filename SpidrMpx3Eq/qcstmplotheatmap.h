#ifndef QCSTMPLOTHEATMAP_H
#define QCSTMPLOTHEATMAP_H

#include <QObject>
#include <QWidget>
#include <QResizeEvent>

#include "qcustomplot.h"

#include <stdio.h>

class QCstmPlotHeatmap : public QCustomPlot
{
  Q_OBJECT
  void resizeEvent(QResizeEvent *event);
  //void mouseMoveEvent(QMouseEvent *event);
  //void toolTipEvent(QToolTip *event);
  ~QCstmPlotHeatmap();
  QCPColorMap *colorMap;//TODO: not dynamically allocated?
  QCPColorScale *colorScale;
  QCPPlotTitle *title;
public:
  void mousePressEvent(QMouseEvent *event);
  void setData(int *data, int nx, int ny);
  void setHeatmap(QCPColorGradient &gradient);
  QCstmPlotHeatmap(QWidget*& parent);
signals:
  void dataRangeChanged(QCPRange newRange);
 public slots:
  //void onXRangeChanged(QCPRange newRange);
  void changeRange(QCPRange newRange);
  void onReplot();

};

#endif // QCSTMPLOTHEATMAP_H
