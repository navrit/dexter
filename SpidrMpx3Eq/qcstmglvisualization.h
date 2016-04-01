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

class QCstmCorrectionsDialog;
class StatsDialog;
class ProfileDialog;

namespace Ui {
class QCstmGLVisualization;
}

class QCstmGLVisualization : public QWidget
{
    Q_OBJECT
    Mpx3GUI * _mpx3gui = nullptr;
    bool _takingData;
    bool _busyDrawing;
    QElapsedTimer * _etatimer = nullptr;
    QTimer * _timer = nullptr;
    int _estimatedETA;

    //QMap<int, histogram> histograms;
    QMap<int, QString> layerNames;

    // Corrections
    QCstmCorrectionsDialog * _corrdialog = nullptr;

    //Statistics
    StatsDialog * _statsdialog = nullptr;
    ProfileDialog * _profiledialog = nullptr;

    // Reco
    Color2DRecoGuided * _reco_Color2DRecoGuided = nullptr;

public:
    explicit QCstmGLVisualization(QWidget *parent = 0);
    ~QCstmGLVisualization();

    //void SeparateThresholds(int * data, int size, int * th0, int * th2, int * th4, int * th6, int sizeReduced);
    void SeparateThresholds(int * data, int size, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int sizeReduced);
    bool isTakingData(){ return _takingData; }

    void SetMpx3GUI(Mpx3GUI * p);
    Mpx3GUI * GetMpx3GUI() { return _mpx3gui; };
    Ui::QCstmGLVisualization * GetUI(){ return ui; };

    pair<int, int> XtoXY(int X, int dimX);
    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
    void GetAFrame();
    void SetBusyState();
    void FreeBusyState();
    void DestroyTimer();
    void ArmAndStartTimer();
    void ETAToZero();
    void setRangeSpinBoxesManual();
    void setRangeSpinBoxesPercentile();

private:
    Ui::QCstmGLVisualization * ui = nullptr;
    DataTakingThread * _dataTakingThread = nullptr;
    bool _savePNGWithScales = false;
    bool _singleShot = false;
    int _singleShotSaveNTriggers = 0;
    QCPRange _manualRange;
    QCPRange _percentileRange;
    bool _logyPlot = false;


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

    //!Temporary save button for images and data.
    void on_saveBitmapPushButton_clicked();

    //!Spinbox for noisyPixelMeanMultiplier parameter
    void on_noisyPixelMeanMultiplier_valueChanged(double arg1);

    void on_correctionsDialogCheckBox_toggled(bool checked);

    void on_recoPushButton_clicked();

    void on_saveWithScaleCheckBox_toggled(bool checked);

    void on_singleshotPushButton_clicked();

    void on_lowerSpin_editingFinished();

    void on_upperSpin_editingFinished();

    void on_logscale(bool);


public slots:
    void StartDataTaking();
    void setGradient(int index);
    //!Used to inform this object of the availible gradients and their names.
    void availible_gradients_changed(QStringList gradients);
    //!Reloads the data for a specific threshold and updates the display, currently reloads all the data for the GLplot
    void reload_layer(int);
    //!Reloads all the data and updates.
    void reload_all_layers(bool corrections = false);
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
    void overflow_update(int);

    //Deleting stats dialog and profile dialog
    void on_user_accepted_stats();
    void on_user_accepted_profile();

signals:
    void change_hover_text(QString);
    void stop_data_taking_thread();
    void free_to_draw();
    void busy_drawing();
    void mode_changed(bool);
    // status bar
    void sig_statusBarAppend(QString mess, QString colorString);
    void sig_statusBarWrite(QString mess, QString colorString);
    void sig_statusBarClean();

};

#endif // QCSTMGLVISUALIZATION_H
