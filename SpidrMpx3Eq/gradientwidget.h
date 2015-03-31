#ifndef GRADIENTWIDGET_H
#define GRADIENTWIDGET_H

#include <QWidget>
#include "qcustomplot.h"
#include "qcstmglplot.h"

class GradientWidget : public QWidget
{
  Q_OBJECT
  Gradient*m_gradient = nullptr;
  QImage *m_gradient_image = nullptr;
  QPixmap m_gradient_pixmap;
  float m_max = 100, m_min = -100;
  float m_label_spacing;
  int m_nlabels = 21, m_barWidth = 100;
public:
  explicit GradientWidget(QWidget *parent = 0);
  ~GradientWidget();
  void setGradient(Gradient*gradient);
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent *event);

signals:
  void range_changed(int min, int max);
public slots:
  void set_range(int min, int max);
  void set_range(QCPRange range);
};


#endif // GRADIENTWIDGET_H
