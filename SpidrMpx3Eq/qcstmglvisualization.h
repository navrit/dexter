#ifndef QCSTMGLVISUALIZATION_H
#define QCSTMGLVISUALIZATION_H

#include <QWidget>
#include <QElapsedTimer>
#include "mpx3gui.h"
#include "gradient.h"
#include "histogram.h"

#define __display_eta_granularity 200 // ms

#include <vector>

using namespace std;

class DataTakingThread;

namespace Ui {
  class QCstmGLVisualization;
}

class QCstmGLVisualization : public QWidget
{
  Q_OBJECT
  Mpx3GUI * _mpx3gui;
  bool _takingData;
  bool _busyDrawing;
  QElapsedTimer * _etatimer;
  QTimer * _timer;
  int _estimatedETA;

  //QMap<int, histogram> histograms;
  QMap<int, QString> layerNames;
public:
  explicit QCstmGLVisualization(QWidget *parent = 0);
  ~QCstmGLVisualization();

  //void SeparateThresholds(int * data, int size, int * th0, int * th2, int * th4, int * th6, int sizeReduced);
  void SeparateThresholds(int * data, int size, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int sizeReduced);

  void SetMpx3GUI(Mpx3GUI * p);
  Mpx3GUI * GetMpx3GUI() { return _mpx3gui; };
  Ui::QCstmGLVisualization * GetUI(){ return ui; };

  pair<int, int> XtoXY(int X, int dimX);
  int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
  void GetAFrame();
  void FlipBusyState();
  void DestroyTimer();
  void ArmAndStartTimer();
  void ETAToZero();

private:
  Ui::QCstmGLVisualization *ui;
  DataTakingThread * _dataTakingThread;
  int getActiveThreshold();
  void addThresholdToSelector(int threshold);
  void setThreshold(int threshold);

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

  void on_summingCheckbox_toggled(bool checked);

  void on_layerSelector_activated(const QString &arg1);
  void UnlockWaitingForFrame();


  void on_obcorrCheckbox_toggled(bool checked);

  void on_pushButton_clicked();

public slots:
  void StartDataTaking();
  void setGradient(int index);
  void on_availible_gradients_changed(QStringList gradients);
  //void on_frame_added(int threshold);
  //void on_frame_updated();
  void on_reload_layer(int);
  void on_reload_all_layers();
  void on_hover_changed(QPoint);
  void on_pixel_selected(QPoint pixel, QPoint position);
  void on_clear();
  void on_range_changed(QCPRange);
  void on_data_taking_finished(int);
  void on_progress_signal(int);
  void changeBinCount(int count);
  void updateETA();

 signals:
  void change_hover_text(QString);
  void stop_data_taking_thread();
  void free_to_draw();
  void busy_drawing();
  void mode_changed(bool);

};

#endif // QCSTMGLVISUALIZATION_H
