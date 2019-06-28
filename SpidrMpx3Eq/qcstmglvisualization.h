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
#include "GeneralSettings.h"
#include "commandhandlerwrapper.h"

#include <QQueue>
#include <QVector>

#define __display_eta_granularity 200 // ms
#define __overhead 0.01

#include <vector>

using namespace std;

class DataTakingThread;
class DataConsumerThread;

class QCstmCorrectionsDialog;
class StatsDialog;
class ProfileDialog;

namespace Ui {
class QCstmGLVisualization;
}

class QCstmGLVisualization : public QWidget
{
    enum MASK_OPERATION { MASK,UNMASK,MASK_ALL_OVERFLOW,MASK_ALL_ACTIVE, NULL_MASK };
    Q_OBJECT
    Mpx3GUI * _mpx3gui = nullptr;
    bool _takingData;
    bool _busyDrawing = false;
    QElapsedTimer * _etatimer = nullptr;
    QTimer * _timer = nullptr;
    int _estimatedETA;

    QMap<int, QString> layerNames;
    QCstmCorrectionsDialog * _corrdialog = nullptr;
    StatsDialog * _statsdialog = nullptr;
    ProfileDialog * _profiledialog = nullptr;

public:
    explicit QCstmGLVisualization(QWidget *parent = nullptr);
    ~QCstmGLVisualization();
    static QCstmGLVisualization* getInstance();

    void setThresholdsVector(int chipId,int idx, int value);
    int getThresholdVector(int chipId,int idx);
    void clearThresholdsVector();
    void initialiseThresholdsVector();

    void timerEvent( QTimerEvent * );
    void refreshScoringInfo();
    void drawFrameImage();
    void rewindScoring();

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

    //! Used in CT
    void saveImage(QString filename);
    void saveImage(QString filename, QString corrMethod);
    bool runningCT = false;

    //! Used in Threshold Scan
    bool runningTHScan = false;

    //!Adds the specified threshold if it didn't exist yet. Then switches to it.
    void setThreshold(int threshold);

    void changeThresholdToNameAndUpdateSelector(int threshold, QString name);
    //!Gets the currently active threshold by looking at the value of the layerselector combobox.
    int getActiveThreshold();
    bool isSaveAllFramesChecked();

    //! Called from Mpx3GUI to disable/enable specific GUI element for developer mode
    void developerMode(bool enabled = false);

    DataConsumerThread* getDataConsumerThread(){ return _dataConsumerThread;}

    void onPixelsMasked(int devID, QSet<int> pixelSet);
    bool requestToSetSavePath(QString path);

    void setMaximumContRW_FPS(int FPS);

    typedef struct {
        uint nFramesReceived;
        uint nFramesKept;
        uint lostFrames;
        uint lostPackets;
        uint framesCount;
    } scoring;

    scoring getScoring(){ return _score; }

private:

    Ui::QCstmGLVisualization * ui = nullptr;
    DataTakingThread * _dataTakingThread = nullptr;
    DataConsumerThread * _dataConsumerThread = nullptr;
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
    QString _equalizationPath = "";

    bool _saveCheckBox_isChecked = false;
    bool _saveAllCheckBox_isChecked = false;
    bool _saveLineEdit_isNotEmpty = false;
    QString _saveFileComboBox_text = "";

    MASK_OPERATION _maskOperation = MASK;
    bool _maskingRequestFromServer = false;
    int _thresholdsVector [NUMBER_OF_CHIPS][8] = {{0}}; //! TODO: KIA? Refactor the hardcoded 8 from this?
    void _loadFromThresholdsVector(void);

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

    //! Adds the specified threshold to the layerselector combobox
    void addThresholdToSelector(int threshold);

    QString getPath(QString msg);
    bool zmqRunning = false;

    //! TCP server
    bool autosaveFromServer = false;

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

    void on_correctionsDialogPushButton_clicked();

    void on_singleshotPushButton_clicked();

    void on_lowerSpin_editingFinished();

    void on_upperSpin_editingFinished();

    void on_logscale(bool);

    void on_infDataTakingCheckBox_toggled(bool checked);

    void ntriggers_edit();

    void triggerLength_edit();

    void on_dropFramesCheckBox_clicked(bool checked);

    void on_resetViewPushButton_clicked();

    void on_testBtn_clicked();

    void on_saveLineEdit_editingFinished();
    void on_saveLineEdit_textEdited();

    void onEqualizationPathExported(QString path);

    void on_saveCheckBox_stateChanged();
    void on_saveAllCheckBox_stateChanged();
    void on_saveFileComboBox_currentIndexChanged(const QString &currentText);



    void on_saveLineEdit_textChanged(const QString &arg1);

    void on_saveLineEdit_returnPressed();

public slots:

    void StartDataTaking(QString mode="");
    void StartDataTakingRemotely(bool);
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
    //!Called when the data is zeroed out, the layers remain the same.
    void on_zero();
    //!Called when the display range of the data is changed. (so the scale on the heatmap).
    void range_changed(QCPRange);
    void dataTakingFinished();
    void progress_signal(int);
    void changeBinCount(int count); //! user requested a different bin-count. Recomputes the histograms for each threshold.
    void updateETA();

    void data_misaligned(bool);
    void mpx3clock_stops(int);


    void fps_update(int);
    void overflow_update(int);

    void user_accepted_profile(); //! Deleting profile dialog
    void OperationModeSwitched(int);

    void on_scoring(int, int, int, int, int);

    void bufferOccupancySlot(int);
    void consumerFinishedOneFrame(int frameId);

    void consumerBufferFull(int);

    //! Gets called when the current display needs to be reloaded. Uses the layerselector combo-box to determine what layer to load.
    void active_frame_changed();

    //! Handle GUI scripting shortcuts
    void shortcutStart();
    void shortcutIntegrate();
    void shortcutIntegrateToggle();
    void shortcutFrameLength();
    void shortcutFrameNumber();

    //! Used for ZMQ interface + TCP interface
    void takeImage();
    void takeAndSaveImageSequence();
    void saveImageSlot(QString filePath);
    void setExposure(int microseconds);
    void setNumberOfFrames(int number_of_frames);
    void setThreshold(int threshold, int value);
    void setGainMode(int mode);
    void setCSM(bool active);
    void loadDefaultEqualisation();
    void loadEqualisation(QString path);
    void setReadoutMode(QString mode);
    void setReadoutFrequency(int frequency); //! in Hz
    void loadConfiguration(QString filePath);

    //! Used only for TCP interface
    void onRequestForAutoSaveFromServer(bool);
    void onRequestForSettingPathFromServer(QString);
    void onRequestForSettingFormatFromServer(int);
    void onRequestToMaskPixelRemotely(int,int);
    void onRequestToUnmaskPixelRemotely(int,int);

signals:
    void taking_data_gui();
    void idling_gui();

    void change_hover_text(QString);
    void stop_data_taking_thread();
    void free_to_draw();
    void busy_drawing();
    void mode_changed(bool);

    void sig_statusBarAppend(QString mess, QString colorString);
    void sig_statusBarWrite(QString mess, QString colorString);
    void sig_statusBarClean();

    //! Used to run CT
    void sig_resumeCT();

    //! Used to run TH Scan (threshold scan)
    void sig_resumeTHScan();

    //! Used for ZMQ
    void someCommandHasFinished_Successfully();
    void someCommandHasFailed(QString reply="");
    void busy(SERVER_BUSY_TYPE); //true means it is taking data
};

#endif // QCSTMGLVISUALIZATION_H
