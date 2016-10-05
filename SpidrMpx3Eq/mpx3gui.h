/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#ifndef MPX3GUI_H
#define MPX3GUI_H

//#include <QImage>
#include <QMainWindow>

#include <QVector>
#include <iostream>
#include <vector>
#include <QScrollBar>
#include <QShortcut>

#include <qcustomplot.h>

using namespace std;
class Mpx3Config;

#include "dataset.h"
#include "gradient.h"
#include "histogram.h"
#include "mpx3eq_common.h"
#include "qcstmvoxeltab.h"
#include "mpx3config.h"
#include "qcstmsteppermotor.h"


class Mpx3Config;
class QCustomPlot;
class SpidrController;
class SpidrDaq;
//class DACs;
class QCstmDacs;
class ThlScan;
class BarChart;
class BarChartProperties;
// Change me when adding extra views
class QCstmEqualization;
class QCstmGLVisualization;
class QCstmConfigMonitoring;
//class QcstmDQE;
class QCstmStepperMotor;

// Change me when adding extra views
#define __visualization_page_Id     0
#define __configuration_page_Id     1
#define __dacs_page_Id              2
#define __equalization_page_Id      3
#define __dqe_page_Id               4
#define __scans_page_Id             5
#define __ct_page_Id                6
#define __stepperMotor_page_Id      7

const QString _softwareName = "ASI Dexter???";

namespace Ui {
class Mpx3GUI;
}

class Mpx3GUI : public QMainWindow {

    Q_OBJECT

public:

    explicit Mpx3GUI(QWidget *parent = 0);
    ~Mpx3GUI();
    void SetupSignalsAndSlots();
    Ui::Mpx3GUI * GetUI() { return _ui; }

private:
    // ML605 layout
    //vector<int> _MPX3RX_ORIENTATION = vector< int > {Dataset::orientationTtBRtL, Dataset::orientationBtTLtR, Dataset::orientationBtTLtR, Dataset::orientationTtBRtL};
    //vector<QPoint> _MPX3RX_LAYOUT = vector<QPoint> {QPoint(0, 1), QPoint(1, 1), QPoint(1, 0), QPoint(0, 0)};
    // compactSPIDR layout
    std::vector<int> _MPX3RX_ORIENTATION = std::vector< int > {Dataset::orientationBtTLtR, Dataset::orientationBtTLtR, Dataset::orientationTtBRtL, Dataset::orientationTtBRtL};
    std::vector<QPoint> _MPX3RX_LAYOUT = std::vector<QPoint> {QPoint(1, 1), QPoint(1, 0), QPoint(0, 0), QPoint(0, 1)};
    int mode = 0;
    //QApplication * _coreApp;
    Ui::Mpx3GUI * _ui;
    QVector<QShortcut *> _shortcutsSwitchPages;

    Mpx3Config * config = nullptr;
    Dataset * workingSet;
    Dataset * originalSet;

    SpidrDaq * _spidrdaq = nullptr;
    bool _armedOk = true; // it won't let the application go into the event loop if set to false

    QVector<Gradient*>  gradients;
    //QVector<istogram*> hists;
    void updateHistogram(int layer);

    QLabel m_statusBarMessageLabel;
    QString m_statusBarMessageString;

    bool m_appActiveFirstTime = false;
    //bool m_fileOpen = false;

    void uncheckAllToolbarButtons();

    QString loadButtonFilenamePath = "";

public:

    Mpx3Config* getConfig();
    Dataset* getDataset(){return workingSet;}
    Dataset* getOriginalDataset(){return originalSet;}
    void rebuildCurrentSets(int x, int y, int framesPerLayer);

    bool isArmedOk(){return _armedOk;}
    void startupActions();

    void saveOriginalDataset();
    void rewindToOriginalDataset();
    void setWindowWidgetsStatus(win_status s = win_status::startup);
    pair<int, int> XtoXY(int X, int dimX){
        return make_pair(X % dimX, X/dimX);
    }


    QCstmEqualization * getEqualization();
    QCstmGLVisualization * getVisualization();
    QCstmDacs * getDACs();
    QCstmConfigMonitoring * getConfigMonitoring();
//    QCstmDQE * getDQE();
    QCstmStepperMotor * getStepperMotor();

    SpidrController * GetSpidrController();
    SpidrDaq * GetSpidrDaq(){ return _spidrdaq; }
    unsigned int addFrame(int * frame, int index, int layer);
    unsigned int addLayer(int * data, int layer);
    unsigned int addLayer(int * data);

    Gradient* getGradient(int index);
    void resize(int x, int y);
    //histogram* getHist(int index){return hists[index];}

    std::vector<int> getOrientation() { return _MPX3RX_ORIENTATION; }
    std::vector<QPoint> getLayout() { return _MPX3RX_LAYOUT; }

    QPoint  getSize();
    void getSize(int *x, int *y);
    int getMode(){return mode;}
    int getX();
    int getY();
    int getPixelAt(int x, int y, int layer);
    int getFrameCount();

    QString getLoadButtonFilename();

    int XYtoX(int x, int y, int dimX) { return y * dimX + x; };

    bool establish_connection();

    bool equalizationLoaded();

    void setTestPulses();

    // SPIDR information strings for About dialog
    QString m_SPIDRControllerVersion = "";
    QString m_SPIDRFirmwareVersion = "";
    QString m_SPIDRSoftwareVersion = "";
    QString m_numberOfChipsFound = "";

signals:
    void dataChanged();
    void data_cleared();
    void hist_added(int);
    void hist_changed(int);
    /*void reload_layer(int layer);
    void frame_added(int layer);
    void frame_changed(int layer);
    void frames_reload();*/
    void active_frame_changed(int);
    void availible_gradients_changed(QStringList gradients);
    void gradient_added(QString gradient);
    void ConnectionStatusChanged(bool); //TODO: emit false when connection is lost for whatever reason.
    void summing_set(bool);
    void reload_layer(int layer);
    void reload_all_layers();
    void sizeChanged(int, int);
    void open_data_failed(); //! Ignore this error: QMetaObject::connectSlotsByName: No matching signal for on_open_data_failed()
    void returnFilename(QString);

    // status bar
    void sig_statusBarAppend(QString mess, QString colorString);
    void sig_statusBarWrite(QString mess, QString colorString);
    void sig_statusBarClean();

    void exitApp(int);

public slots:
    void on_shortcutsSwithPages();
    void generateFrame(); //Debugging function to generate data when not connected
    void clear_data(bool clearStatusBar = true);
    void save_data();
    void open_data(bool saveOriginal = true);
    void open_data_with_path(bool saveOriginal = true, bool requestPath = false, QString path = "");
    void set_mode_integral();
    void set_mode_normal();
    void clear_configuration();
    void set_summing(bool);
    void save_config();
    void load_config();
    void onConnectionStatusChanged(bool);
    unsigned int dataReady(int layer);

    // status bar
    void statusBarAppend(QString mess, QString colorString);
    void statusBarWrite(QString mess, QString colorString);
    void statusBarClean();
    QString removeOneMessage(QString fullMess);

    void on_applicationStateChanged(Qt::ApplicationState);

private slots:
    void LoadEqualization();
    void on_actionExit_triggered();
    void on_actionConnect_triggered();
    void on_actionVisualization_triggered();
    void on_actionConfiguration_triggered();
    void on_actionDACs_triggered();
    void on_actionEqualization_triggered();
    void on_actionScans_triggered();
    void on_actionDisconnect_triggered(bool checked);
    void on_actionDefibrillator_triggered(bool checked);
    void on_actionRevert_triggered(bool checked);
    void on_actionAbout_triggered(bool checked);
    void on_actionStepper_Motor_triggered(bool checked);
};


#endif // MPX3GUI_H
