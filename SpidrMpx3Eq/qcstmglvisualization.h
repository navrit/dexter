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

#include <QQueue>
#include <QVector>

#define __display_eta_granularity 200 // ms
#define __networkOverhead 0.1

#include <vector>

using namespace std;

class DataTakingThread;
class DataConsumerThread;

class QCstmCorrectionsDialog;
class StatsDialog;
class ImageCalculator;

namespace Ui {
class QCstmGLVisualization;
}

class QCstmGLVisualization : public QWidget
{
    Q_OBJECT
    Mpx3GUI * _mpx3gui = nullptr;
    bool _takingData;
    bool _busyDrawing = false;
    QElapsedTimer * _etatimer = nullptr;
    QTimer * _timer = nullptr;
    int _estimatedETA;

    //QMap<int, histogram> histograms;
    QMap<int, QString> layerNames;

    // Corrections
    QCstmCorrectionsDialog * _corrdialog = nullptr;

    //Statistics
    StatsDialog * _statsdialog = nullptr;

public:
    explicit QCstmGLVisualization(QWidget *parent = 0);
    ~QCstmGLVisualization();

    void timerEvent( QTimerEvent * );
    void refreshScoringInfo();
    void drawFrameImage();
    void rewindScoring();

    //void SeparateThresholds(int * data, int size, int * th0, int * th2, int * th4, int * th6, int sizeReduced);
    void SeparateThresholds(int * data, int size, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int sizeReduced);
    bool isTakingData(){ return _takingData; }

    void SetMpx3GUI(Mpx3GUI * p);
    Mpx3GUI * GetMpx3GUI() { return _mpx3gui; }
    Ui::QCstmGLVisualization * GetUI(){ return ui; }
    DataTakingThread * dataTakingThread(){ return _dataTakingThread; }
    void startupActions();

    pair<int, int> XtoXY(int X, int dimX);
    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
    void SetBusyState();
    void FreeBusyState();
    bool isBusyDrawing();
    void DestroyTimer();
    void ArmAndStartTimer();
    void ETAToZero();
    void setRangeSpinBoxesManual();
    void setRangeSpinBoxesPercentile();

    void clearStatsString();
    void initStatsString();


    void rewindHistoLimits();
    bool getDropFrames(){return _dropFrames;}

    void ConfigureGUIForDataTaking();
    void ConfigureGUIForIdling();

    void FinishDataTakingThread();
    void StopDataTakingThread();
    bool DataTakingThreadIsRunning();
    bool DataTakingThreadIsIdling();
    void CalcETA();

    QString getsaveLineEdit_Text();
    QString getStatsString_deviceId();

    // Used in CT
    void saveImage(QString filename);
    void saveImage(QString filename, QString corrMethod);
    bool runningCT = false;

    // Used in Threshold Scan
    bool runningTHScan = false;

    //!Adds the specified threshold if it didn't exist yet. Then switches to it.
    void setThreshold(int threshold);

    void changeThresholdToNameAndUpdateSelector(int threshold, QString name);
    //!Gets the currently active threshold by looking at the value of the layerselector combobox.
    int getActiveThreshold();
    bool isSaveAllFramesChecked();

    //! Called from Mpx3GUI to disable/enable specific GUI element for developer mode
    void developerMode(bool enabled = false);

private:

    Ui::QCstmGLVisualization * ui = nullptr;
    DataTakingThread * _dataTakingThread = nullptr;
    DataConsumerThread * _dataConsumerThread = nullptr;
    bool _savePNGWithScales = false;
    bool _singleShot = false;
    int _singleShotSaveCurrentNTriggers = 0;
    QCPRange _manualRange;
    QCPRange _manualRangeSave;
    bool _manualRangePicked = false;
    QCPRange _percentileRange;
    QCPRange _percentileRangeNatural;
    bool _logyPlot = false;
    bool _infDataTaking = false;
    unsigned int _nTriggersSave;
    bool _dropFrames = true;

    typedef struct {
        unsigned int nFramesReceived;
        unsigned int nFramesKept;
        unsigned int lostFrames;
        unsigned int lostPackets;
        unsigned int framesCount;
        unsigned int mpx3clock_stops;
        bool dataMisaligned;
    } scoring;

    typedef struct {
        QString counts;
        QString lostPackets;
        QString lostFrames;
        QString mpx3ClockStops;
        QString overflow;
        bool overflowFlg;
        QString displayString; // this is the composition actually being displayed
        QString devicesIdString;
    } stats_str;

    typedef struct {
        QLabel * devicesNamesLabel = nullptr;
        QPushButton * correctionsDialogueButton = nullptr;
    } extra_widgets;

    stats_str _statsString;
    scoring _score;
    extra_widgets _extraWidgets;
    int _timerId;

    void BuildStatsString();
    void BuildStatsStringCounts(uint64_t counts);
    void BuildStatsStringLostPackets(uint64_t lostPackets);
    void BuildStatsStringLostFrames(uint64_t lostFrames);
    void BuildStatsStringMpx3ClockStops(uint64_t stops);
    void BuildStatsStringOverflow(bool overflow);

    //!Adds the specified threshold to the layerselector combobox
    void addThresholdToSelector(int threshold);

    QString getPath(QString msg);

private slots:
    void ConnectionStatusChanged(bool connecting);
    void on_percentileRangeRadio_toggled(bool checked);


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

    void on_correctionsDialogPushButton_clicked();

    void on_saveWithScaleCheckBox_toggled(bool checked);

    void on_singleshotPushButton_clicked();

    void on_lowerSpin_editingFinished();

    void on_upperSpin_editingFinished();

    void on_logscale(bool);

    void on_infDataTakingCheckBox_toggled(bool checked);

    void ntriggers_edit();

    void triggerLength_edit();

    void on_dropFramesCheckBox_clicked(bool checked);

    void on_resetViewPushButton_clicked();

    void on_saveCheckBox_clicked();

    void on_saveAllCheckBox_toggled(bool checked);

public slots:

    void StartDataTaking(QString mode = "");
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
    //!Called when the data is cleared. Clears all the plots and relevant combo-boxes.
    void on_clear();
    //!Called when the data is zeroed out, the layers remain the same.
    void on_zero();
    //!Called when the display range of the data is changed. (so the scale on the heatmap).
    void range_changed(QCPRange);
    void data_taking_finished(int);
    void progress_signal(int);
    //!Called when the user request a different bin-count. Recomputes the histograms for each threshold.
    void changeBinCount(int count);
    void updateETA();

    void data_misaligned(bool);
    void mpx3clock_stops(int);


    void fps_update(int);
    void overflow_update(int);

    //Deleting stats dialog
//    void on_user_accepted_stats();
    void OperationModeSwitched(int);

    void on_scoring(int, int, int, int, int, int, bool);

    void bufferOccupancySlot(int);
    void consumerFinishedOneFrame(int frameId);

    void consumerBufferFull(int);

    //! Gets called when the current display needs to be reloaded. Uses the layerselector combo-box to determine what layer to load.
    void active_frame_changed();

    void shortcutStart();
    void shortcutIntegrate();
    void shortcutIntegrateToggle();
    void shortcutFrameLength();
    void shortcutFrameNumber();

    //! Used for ZMQ
    void takeImage();
    void takeAndSaveImageSequence();
    void saveImageSlot(QString filePath);
    void setExposure(uint64_t microseconds);
    void setNumberOfFrames(uint64_t number_of_frames);
    void setThreshold(uint16_t threshold, uint16_t value);
    void setGainMode(QString mode);
    void setCSM(bool active);
    void loadEqualisation(QString filePath);
    void setReadoutMode(QString mode);
    void setReadoutFrequency(uint16_t frequency); //! in Hz
    void loadConfiguration(QString filePath);
    void setNumberOfAverages(uint64_t number_of_averages); //! Should I really do this?

signals:
    void taking_data_gui();
    void idling_gui();

    void change_hover_text(QString);
    void stop_data_taking_thread();
    void free_to_draw();
    void busy_drawing();
    void mode_changed(bool);
    // status bar
    void sig_statusBarAppend(QString mess, QString colorString);
    void sig_statusBarWrite(QString mess, QString colorString);
    void sig_statusBarClean();

    //! Used to run CT
    void sig_resumeCT();

    //! Used to run TH Scan (threshold scan)
    void sig_resumeTHScan();

    //! Used for ZMQ
    void dataTakingFinishedAndSaved();

};

#endif // QCSTMGLVISUALIZATION_H
