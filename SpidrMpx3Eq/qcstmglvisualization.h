#ifndef QCSTMGLVISUALIZATION_H
#define QCSTMGLVISUALIZATION_H

#include <QWidget>
#include "mpx3gui.h"

namespace Ui {
  class QCstmGLVisualization;
}

class QCstmGLVisualization : public QWidget
{
  Q_OBJECT
  Mpx3GUI * _mpx3gui;
public:
  explicit QCstmGLVisualization(QWidget *parent = 0);
  void SetMpx3GUI(Mpx3GUI * p);

  ~QCstmGLVisualization();
private:
  Ui::QCstmGLVisualization *ui;
public slots:
  void on_frame_added();
  void on_hover_changed(QPoint);
 signals:
  void change_hover_text(QString);
};

#endif // QCSTMGLVISUALIZATION_H
