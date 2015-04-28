#ifndef QCSTMRULER_H
#define QCSTMRULER_H

#include <QWidget>
#include <QImage>
class QCstmRuler : public QWidget
{
public:
  enum orientations{
    orientationTop,
    orientationBottom,
    orientationLeft,
    orientationRight,
  };
  Q_OBJECT
  QImage *m_image = nullptr;
  QPointF m_display_min, m_display_max;
  QRectF m_cutoff;
  int m_requiredWidth = 0, m_nSteps = 25, m_dashLength = 10, m_subDashCount = 4, m_subDashLength = 5;
  int m_orientation = orientationLeft;
  void recomputeDisplayRange();
  void paintLeft();
  void paintTop();
public:
  explicit QCstmRuler(QWidget *parent = 0);
  //void setRange(QPoint range){m_min = range.x(); m_max = range.y();}
  void setNSteps(int nSteps){m_nSteps = nSteps;}
  void setOrientation(int orientation){m_orientation = orientation;update();}
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent *event);
signals:
public slots:
  void set_cutoff(QRectF newCutoff){m_cutoff = newCutoff;}
  void set_cutoff(QPoint newCutoff){m_cutoff = QRectF(0,0,newCutoff.x(), newCutoff.y());}
  void on_bounds_changed(QRectF newBounds){
    m_display_min = QPointF(newBounds.x(), newBounds.y());
    m_display_max = QPointF(newBounds.width(), newBounds.height());
    update();
  }
};

#endif // QCSTMRULER_H
