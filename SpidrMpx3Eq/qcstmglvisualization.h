#ifndef QCSTMGLVISUALIZATION_H
#define QCSTMGLVISUALIZATION_H

#include <QWidget>
#include "mpx3gui.h"
#include "gradient.h"
#include "histogram.h"

namespace Ui {
  class QCstmGLVisualization;
}

class QCstmGLVisualization : public QWidget
{
  Q_OBJECT
  Mpx3GUI * _mpx3gui;
  //QMap<int, histogram> histograms;
  QMap<int, QString> layerNames;
public:
  explicit QCstmGLVisualization(QWidget *parent = 0);
  void SetMpx3GUI(Mpx3GUI * p);
  void Configuration(bool reset);
  void Configuration(int deviceIndex, bool reset);
  ~QCstmGLVisualization();
private:
  Ui::QCstmGLVisualization *ui;
  int getActiveThreshold();
private slots:
  void ConnectionStatusChanged();
  void on_percentileRangeRadio_toggled(bool checked);

  void on_lowerPercentileSpin_editingFinished();

  void on_upperPercentileSpin_editingFinished();

  void on_lowerManualSpin_editingFinished();

  void on_upperManualSpin_editingFinished();

  void on_active_frame_changed();
  void on_new_range_dragged(QCPRange newRange);
  void on_manualRangeRadio_toggled(bool checked);

  void on_fullRangeRadio_toggled(bool checked);

  void on_outOfBoundsCheckbox_toggled(bool checked);

  void on_layerSelector_activated(const QString &arg1);

public slots:
  void StartDataTaking();
  void setGradient(int index);
  void on_availible_gradients_changed(QStringList gradients);
  void on_hist_added(int layer);
  void on_hist_changed(int layer);
  void on_frame_added(int threshold);
  void on_frame_updated();
  void on_hover_changed(QPoint);
  void on_pixel_selected(QPoint pixel, QPoint position);
  void on_clear();
 signals:
  void change_hover_text(QString);
};

#endif // QCSTMGLVISUALIZATION_H
