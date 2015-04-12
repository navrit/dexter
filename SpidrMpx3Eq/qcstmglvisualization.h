#ifndef QCSTMGLVISUALIZATION_H
#define QCSTMGLVISUALIZATION_H

#include <QWidget>
#include "mpx3gui.h"
#include "gradient.h"

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
  void Configuration(bool reset);

  ~QCstmGLVisualization();
private:
  Ui::QCstmGLVisualization *ui;
private slots:
  void ConnectionStatusChanged();
public slots:
  void StartDataTaking();
  void setGradient(int index);
  void on_availible_gradients_changed(QStringList gradients);
  void on_hist_added();
  void on_frame_added();
  void on_frame_updated();
  void on_hover_changed(QPoint);
  void on_pixel_selected(QPoint pixel, QPoint position);
  void on_clear();
 signals:
  void change_hover_text(QString);
};

#endif // QCSTMGLVISUALIZATION_H
