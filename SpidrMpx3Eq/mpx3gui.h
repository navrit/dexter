/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef MPX3GUI_H
#define MPX3GUI_H

#include <stdio.h>
#include <iostream>
#include <vector>

#include <QCoreApplication>
#include <QTimer>
#include <QMainWindow>
#include <QVector>
#include <QScrollBar>
#include <QShortcut>

#include <qcustomplot.h>
#include "dataset.h"
#include "gradient.h"
#include "histogram.h"
#include "mpx3eq_common.h"
#include "mpx3config.h"

#include "qcstmsteppermotor.h"
#include "qcstmct.h"
#include "thresholdscan.h"
#include "datacontrollerthread.h"
#include "zmqcontroller.h"
#include "tcpserver.h"
#include "commandhandlerwrapper.h"
#include "GeneralSettings.h"
#include "EnergyCalibrator.h"
#include "EnergyConfiguration.h"

class Mpx3Config;
class QCustomPlot;
class SpidrController;
class SpidrDaq;
class ThlScan;

// Change me when adding extra views
class QCstmEqualization;
class QCstmGLVisualization;
class QCstmConfigMonitoring;
class QCstmDacs;
class QCstmStepperMotor;
class QCstmCT;
class thresholdScan;
class DataControllerThread;
class zmqController;
class TcpServer;
class hdmiConfig;

// Change me when adding extra views
const static int __visualization_page_Id = 0;
const static int __configuration_page_Id = 1;
const static int __dacs_page_Id = 2;
const static int __equalisation_page_Id = 3;
const static int __scans_page_Id = 4;
const static int __energyConfiguration_page_Id = 5;
const static int __hdmi_config_page_Id = 6;
const static int __thresholdScan_page_Id = 7;
const static int __stepperMotor_page_Id = 8;
const static int __ct_page_Id = 9;


#define BIN_FILES "Binary (*.bin)"
#define TIFF_FILES "TIFF (*.tiff)"
#define SPATIAL_TIFF_FILES "Spatial corrected TIFF (*_spatialCorrected.tiff)"
#define RAW_TIFF_FILES "Raw TIFF (*_raw.tiff)"
#define RAW_TIFF16_FILES "Raw TIFF16 (*_raw.tiff)"
#define PGM16_FILES "PGM16 (*_raw.pgm)"
#define BOTH_TIFF_FILES "Corrected and uncorrected TIFFs (*.tiff)"
#define ASCII_FILES "ASCII (*.txt)"
#define JSON_FILES "BH JSON file(*.json)"
#define LONG_PERIOD_US 100000000

const static QString _softwareName = "Dexter";
const static QString _softwareVersion = QString("v2.1.1 BugFix-Raw_TIFF_16 NB - " + QString(GIT_CURRENT_SHA1));

const static int tcpCommandPort = 6351;            //! Diamond - Merlin interface
const static int tcpDataPort = 6352;               //! Diamond - Merlin interface

namespace Ui {
    class Mpx3GUI;
}

class Mpx3GUI : public QMainWindow {

    Q_OBJECT

public:

    explicit Mpx3GUI(QWidget *parent = nullptr);
    ~Mpx3GUI();
    Ui::Mpx3GUI *GetUI() { return _ui; }
    static Mpx3GUI *getInstance();

    Mpx3Config *getConfig();
    Dataset *getDataset() {return workingSet;}
    DataControllerThread *getDataControllerThread() {return dataControllerThread;}
    zmqController *getZmqController() {return m_zmqController;}

    SpidrController *GetSpidrController();
    SpidrDaq *GetSpidrDaq(){ return _spidrdaq; }
    QCstmEqualization *getEqualization();
    QCstmGLVisualization *getVisualization();
    QCstmDacs *getDACs();
    QCstmConfigMonitoring *getConfigMonitoring();
    QCstmStepperMotor *getStepperMotor();
    QCstmCT *getCT();
    thresholdScan *getTHScan();
    hdmiConfig *getHdmiConfig();
    EnergyConfiguration *getEnergyConfiguration();

    GeneralSettings *getGeneralSettings() {return _generalSettings;}
    EnergyCalibrator *getEnergyCalibrator() {return _energyCalibrator;}
    void updateEnergyCalibratorParameters();

    int getStepperMotorPageID();
    bool isArmedOk() {return _armedOk;}
    void startupActions();
    void closeRemotely() {on_actionDisconnect_triggered(false);}

    void setWindowWidgetsStatus(win_status s = win_status::startup);

    void addLayer(int *data, int layer);
    Gradient *getGradient(int index);
    void resize(int x, int y);
    void rebuildCurrentSets(int x, int y, int framesPerLayer);

    std::vector<int> getOrientation() { return _MPX3RX_ORIENTATION; }
    std::vector<QPoint> getLayout() { return _MPX3RX_LAYOUT; }

    QPoint getSize();
    void getSize(int *x, int *y);
    int getIntegrate() {return integrate;}
    int getX();
    int getY();
    int getPixelAt(int x, int y, int layer);
    int getFrameCount();

    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
    pair<int, int> XtoXY(int X, int dimX) { return make_pair(X % dimX, X/dimX); }

    QString getLoadButtonFilename();

    bool establish_connection();
    bool equalizationLoaded();

    const QString settingsFile = QApplication::applicationDirPath() + QDir::separator() + "last_configuration.ini";

    //! SPIDR information strings for About dialog
    QString m_SPIDRControllerVersion = "";
    QString m_SPIDRFirmwareVersion = "";
    QString m_SPIDRSoftwareVersion = "";
    QString m_numberOfChipsFound = "";

    QString compileDateTime = QDate::currentDate().toString("yyyy-MM-dd");


    void loadLastConfiguration();
    QString getConfigPath(){ return _configPath;}
    void stopTriggerTimers();
    CommandHandlerWrapper* getCommandHandlerWrapper();

signals:
    void dataChanged();
    void data_cleared();
    void data_zeroed();
    void hist_added(int);
    void hist_changed(int);
    void active_frame_changed(int);
    void availible_gradients_changed(QStringList _gradients);
    void gradient_added(QString gradient);
    void ConnectionStatusChanged(bool); //TODO: emit false when connection is lost for whatever reason.
    void summing_set(bool);
    void reload_layer(int layer);
    void reload_all_layers();
    void sizeChanged(int, int);
    void open_data_failed(); //! Ignore this error: QMetaObject::connectSlotsByName: No matching signal for on_open_data_failed()
    void returnFilename(QString);

    //! Status bar signal functions
    void sig_statusBarAppend(QString mess, QString colorString);
    void sig_statusBarWrite(QString mess, QString colorString);
    void sig_statusBarClean();

    void exitApp(int);

public slots:
    void on_shortcutsSwitchPages();
    void generateFrame();
    void clear_data(bool printToStatusBar = true);
    void zero_data(bool printToStatusBar = true);
    void save_data(bool requestPath, int frameId = 0, QString selectedFileType = "");
    void open_data(bool saveOriginal = true);
    void open_data_with_path(bool saveOriginal = true, bool requestPath = false, QString path = "");
    void set_mode_integral();
    void set_mode_normal();
    void clear_configuration();
    void set_summing(bool);
    void save_config();
    bool load_config();
    bool load_config_remotely(QString path);
    void onConnectionStatusChanged(bool);

    //! Status bar slot functions
    void statusBarAppend(QString mess, QString colorString);
    void statusBarWrite(QString mess, QString colorString);
    void statusBarClean();
    QString removeOneMessage(QString fullMess);

    void on_applicationStateChanged(Qt::ApplicationState);

    void developerMode();

    void loadEqualisationFromPathRemotely(QString path);

    void onEqualizationPathExported(QString path);
    void sendingShutter();

private:
    bool _loadingBeforeConnecting = true;
    EnergyCalibrator *_energyCalibrator = nullptr;
    // ML605 layout
    //vector<int> _MPX3RX_ORIENTATION = vector< int > {Dataset::orientationTtBRtL, Dataset::orientationBtTLtR, Dataset::orientationBtTLtR, Dataset::orientationTtBRtL};
    //vector<QPoint> _MPX3RX_LAYOUT = vector<QPoint> {QPoint(0, 1), QPoint(1, 1), QPoint(1, 0), QPoint(0, 0)};
    // compactSPIDR layout
    std::vector<int> _MPX3RX_ORIENTATION = std::vector< int > {Dataset::orientationLtRTtB, Dataset::orientationLtRTtB, Dataset::orientationRtLBtT, Dataset::orientationRtLBtT};
    std::vector<QPoint> _MPX3RX_LAYOUT = std::vector<QPoint> {QPoint(0, 1), QPoint(1, 1), QPoint(1, 0), QPoint(0, 0)};

    //! Kia - TCP Server - Diamond interface
    TcpServer *tcpServer = nullptr;
    TcpServer *dataServer = nullptr;
    CommandHandlerWrapper *commandHandlerWrapper = nullptr;

    int integrate = 0; //! Summing/integral or 'normal' mode
    Ui::Mpx3GUI * _ui = nullptr;
    QVector<QShortcut *> _shortcutsSwitchPages; //! Note: do not initialise this, it is populated on startup

    Mpx3Config * config = nullptr;
    Dataset * workingSet = nullptr;

    SpidrDaq * _spidrdaq = nullptr;
    bool _armedOk = true; //! It won't let the application go into the event loop if set to false

    QVector<Gradient*>  _gradients; //! Initialising this as {nullptr} is apparently bad for some signal connection
    void updateHistogram(int layer);

    QLabel _statusBarMessageLabel;
    QString _statusBarMessageString;

    bool m_appActiveFirstTime = false;

    void _uncheckAllToolbarButtons();

    QString _loadButtonFilenamePath = "";

    void saveMetadataToJSON(QString);

    bool _devMode = false;

    DataControllerThread *dataControllerThread = nullptr;
    zmqController *m_zmqController = nullptr;

    void _initialiseInternalObjects();
    void _initialiseDataset();
    void _initialiseGUITabs();
    void _handleConfigLoading();
    void _connectKeyboardShortcuts();
    void _setupSignalsAndSlots();
    void _intialiseLabels();
    void _initialiseServers();

    bool m_offset = false; //! Used for generating different patterns per test pattern

    bool _loadConfigRemotely = false;
    QString _configPath = "";

    GeneralSettings *_generalSettings;
    void _loadEqualizationFromGeneralSettings();
    bool _loadConfigsFromGeneralSettings();
    QTimer *_shutterOpenTimer; //used for periods longer than 100 seconds
    QTimer *_shutterCloseTimer; //used for periods longer than 100 seconds
    bool _timerStop = false;

    QString _lastTriedIPAddress = "";

private slots:
    void LoadEqualization();
    void loadEqualisationFromPath();
    void on_actionExit_triggered();
    void on_actionConnect_triggered();
    void on_actionVisualization_triggered();
    void on_actionConfiguration_triggered();
    void on_actionDACs_triggered();
    void on_actionEqualization_triggered();
    void on_actionDisconnect_triggered(bool checked);
    void on_actionDefibrillator_triggered(bool checked);
    void on_actionAbout_triggered(bool checked);
    void on_actionStepper_Motor_triggered(bool checked);
    void on_actionThreshold_Scan_triggered();
    void on_actionHDMI_Config_triggered();
    void on_actionEnergy_configuration_triggered();

    void autoConnectToDetector();
    void shutterOpenTimer_timeout();
    void shutterCloseTimer_timeout();
};

#endif // MPX3GUI_H
