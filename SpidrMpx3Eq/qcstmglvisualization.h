/**
 * \class QCstmGLVisualization
 *
 * \brief Main visualization UI class.
 *
 * This class acts as a gatekeeper between the main Mpx3GUI class and the various plotting utilities, as well as a few relevant data-taking mechanisms.
 */


#ifndef QCSTMGLVISUALIZATION_H
#define QCSTMGLVISUALIZATION_H

#include <QWidget>
#include <QElapsedTimer>
#include "mpx3gui.h"
#include "gradient.h"
#include "histogram.h"

#define __display_eta_granularity 200 // ms
#define __networkOverhead 0.1

#include <vector>

using namespace std;

class DataTakingThread;
class QCstmBHWindow;

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

  // BH window
  QCstmBHWindow * _bhwindow;

  // Reco
  Color2DRecoGuided * _reco_Color2DRecoGuided = nullptr;

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
  //!Gets the currently active threshold by looking at the value of the layerselector combobox.
  int getActiveThreshold();
  //!Adds the specified threshold to the layerselector combobox
  void addThresholdToSelector(int threshold);
  void changeThresholdToNameAndUpdateSelector(int threshold, QString name);
  //!Adds the specified threshold if it didn't exist yet. Then switches to it.
  void setThreshold(int threshold);

private slots:
  void ConnectionStatusChanged();
  void on_percentileRangeRadio_toggled(bool checked);

  void on_lowerPercentileSpin_editingFinished();

  void on_upperPercentileSpin_editingFinished();

  void on_lowerManualSpin_editingFinished();

  void on_upperManualSpin_editingFinished();

  //! Gets called when the current display needs to be reloaded. Uses the layerselector combo-box to determine what layer to load.
  void active_frame_changed();

  //! Gets called when a new data range was selected in the histogram plot.
  void new_range_dragged(QCPRange newRange);

  void on_manualRangeRadio_toggled(bool checked);

  void on_fullRangeRadio_toggled(bool checked);

  void on_outOfBoundsCheckbox_toggled(bool checked);

  void on_summingCheckbox_toggled(bool checked);

  void on_layerSelector_activated(const QString &arg1);

  void UnlockWaitingForFrame();

  //!Presents the user with a file menu to select a dataset to use for the openbeam correction. If one is selected, it will set it for the current dataset (but not apply it, that happens after data taking).
  void on_obcorrCheckbox_toggled(bool checked);

  //!Load a BH correction
  void on_bhcorrCheckbox_toggled(bool checked);

  //!Temporary save button for images and data.
  void on_pushButton_clicked();

  //!Apply corrections manually
  void on_applyCorr_clicked();

  //!Spinbox for noisyPixelMeanMultiplier parameter
  void on_noisyPixelMeanMultiplier_valueChanged(double arg1);

public slots:
  void StartDataTaking();
  void setGradient(int index);
  //!Used to inform this object of the availible gradients and their names.
  void availible_gradients_changed(QStringList gradients);
  //!Reloads the data for a specific threshold and updates the display, currently reloads all the data for the GLplot
  void reload_layer(int);
  //!Reloads all the data and updates.
  void reload_all_layers();
  //!Called when the pixel hovered by the mouse changes. Takes assembly-coordinates.
  void hover_changed(QPoint);
  //!Called when a pixel has been selected with the right mouse-button. Pixel is assembly-coordinates, position is screenspace (used to determine where to create the context menu).
  //!Position could possibly be removed and simply query the cursor location from here.
  void pixel_selected(QPoint pixel, QPoint position);
  void region_selected(QPoint pixel_begin, QPoint pixel_end, QPoint position);
  //!Called when the data is cleared. Clears all the plots and relevant combo-boxes.
  void on_clear();
  //!Called when the display range of the data is changed. (so the scale on the heatmap).
  void range_changed(QCPRange);
  void data_taking_finished(int);
  void progress_signal(int);
  //!Called when the user request a different bin-count. Recomputes the histograms for each threshold.
  void changeBinCount(int count);
  void updateETA();

  void lost_packets(int);
  void fps_update(int);

 signals:
  void change_hover_text(QString);
  void stop_data_taking_thread();
  void free_to_draw();
  void busy_drawing();
  void mode_changed(bool);


};

#endif // QCSTMGLVISUALIZATION_H
