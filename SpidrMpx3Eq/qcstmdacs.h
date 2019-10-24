#ifndef QCSTMDACS_H
#define QCSTMDACS_H

/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include <QWidget>
#include <QDialog>
#include <QThread>
#include <QJsonObject>

#include "mpx3eq_common.h"
#include "mpx3defs.h"

#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "RemoteThresholdDlg.h"
#include "ServerStatus.h"
//#include "qcstmconfigmonitoring.h"


using namespace std;

#define		__default_DACs_filename "mpx3_defaultDACs.json"
#define		__N_RETRY_ORIGINAL_SETTING	3

class SpidrController;
class SpidrDaq;
class QCustomPlot;
class QCPGraph;
class Mpx3GUI;
class QSpinBox;
class QSlider;
class QLabel;
class QCheckBox;
class SenseDACsThread;
class ScanDACsThread;
class UpdateDACsThread;
class QSignalMapper;
class ModuleConnection;

namespace Ui {
class QCstmDacs;
} // namespace Ui

class QCstmDacs : public QWidget {

    Q_OBJECT

public:

    explicit QCstmDacs(QWidget *parent = nullptr);
    ~QCstmDacs() override;

    void PopulateDACValues();
    UpdateDACsThread * FillDACValues(int devId = -1, bool updateInTheChip = true);

    bool GetDACsFromConfiguration();
    bool WriteDACsFile(std::string);

    static QCstmDacs *getInstance(); //! TODO Get rid of this

    Ui::QCstmDacs  * GetUI() { return ui; }
    QSpinBox ** GetSpinBoxList() { return _dacSpinBoxes; }
    QSlider ** GetSliderList() { return _dacSliders; }
    int GetDACIndex(int dac_code);
    QLabel ** GetLabelsList() { return _dacVLabels ; }
    QCheckBox ** GetCheckBoxList() { return _dacCheckBoxes; }
    int GetDeviceIndex() { return _deviceIndex; }
    int GetNSamples();
    int GetScanStep() { return _scanStep; }
    QCPGraph * GetGraph(int idx);
    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
    Mpx3GUI * GetMpx3GUI() { return _mpx3gui; }

    QCheckBox* getAllDACSimultaneousCheckBox();
    QSpinBox* getDeviceIdSpinBox();

    void setWindowWidgetsStatus(win_status s = win_status::startup );

    // Ask the config !
    int GetDACValueFromConfig(uint chip, int dacIndex);
    void SetDACValueLocalConfig(uint chip, int dacIndex, int val);

    void changeDAC(int threshold, int value); //! For all chips
    void setRemoteRequestForSettingThreshold(bool);

    void setDAC_index(SpidrController *spidrcontrol, int chip, int DAC_index, double DAC_value);
    void setDAC_code(SpidrController *spidrcontrol, int chip, int DAC_code, int DAC_value);

private:
    /* Internal variables and functions */

    Mpx3GUI *_mpx3gui = nullptr; // Connectivity between modules
    SenseDACsThread *_senseThread = nullptr;
    ScanDACsThread *_scanThread = nullptr;
    UpdateDACsThread *_updateDACsThread = nullptr;

    void FillWidgetVectors();
    void SetLimits();
    void enablePossibleThresholds();

    int _scanStep; // Scan
    int _chipIndex; // Current chip
    int _nSamples; // Samples

    bool _dacsSimultaneous; // Simultaneous settings

    /* END - Internal variables and functions */


    /* GUI related */

    Ui::QCstmDacs * ui = nullptr;
    QCPGraph *_graph = nullptr; // Currently active graph

    // Vectors of Widgets
    QSpinBox  * _dacSpinBoxes[MPX3RX_DAC_COUNT];
    QSlider   * _dacSliders[MPX3RX_DAC_COUNT];
    QLabel    * _dacVLabels[MPX3RX_DAC_COUNT];
    QLabel    * _dacLabels[MPX3RX_DAC_COUNT];
    QCheckBox * _dacCheckBoxes[MPX3RX_DAC_COUNT];
    QSignalMapper * _signalMapperSliderSpinBoxConn;
    QSignalMapper * _signalMapperSlider;
    QSignalMapper * _signalMapperSpinBox;

    // In case only a subset of the DACs are selected
    //  to produce the scan, keep track of the id's
    std::map<int, int> _plotIdxMap;

    int _plotIdxCntr;
    uint _deviceIndex; // Current device Id
    bool _remoteRequestForSettingThreshold = false;

    /* END - GUI related */

private slots:
    void onDevNumChanged(int);
    void on_allDACSimultaneousCheckBox_toggled(bool checked);

    void setTextWithIdx(QString,int);
    void UncheckAllDACs();
    void CheckAllDACs();
    void setValueDAC(int);
    void StartDACScan();
    void SetupSignalsAndSlots();
    void FromSpinBoxUpdateSlider(int);
    void FromSliderUpdateSpinBox(int);
    void SenseDACs();
    void ChangeNSamples(int);
    void ChangeScanStep(int);
    void addData(int, int, double);
    void addData(int);
    void scanFinished();
    void slideAndSpin(int DAC_index, int DAC_value);
    void openWriteMenu();
    void ConnectionStatusChanged(bool);
    void sendThresholdToDac();
    void on_remoteThresholdpushButton_clicked();

public slots:
    void ChangeDeviceIndex(int);

    void slot_colourModeChanged(bool);
    void slot_readBothCounters(bool);

signals:
    void busy(SERVER_BUSY_TYPE);
};

class SenseDACsThread : public QThread {

    Q_OBJECT

public:

    explicit SenseDACsThread (int devIndx, QCstmDacs * dacs, SpidrController * sc);

private:

    SpidrController * _spidrcontrol = nullptr;
    QCstmDacs * _dacs = nullptr;
    int _deviceIndex;
    // IP source address (SPIDR network interface)
    int _srcAddr;

    void run() override;

signals:

    // These are used in the parent class as a signal to thread-safe feed
    //  widgets in the ui
    void progress(int);
    void fillText(QString);
};

class ScanDACsThread : public QThread {

    Q_OBJECT

public:

    explicit ScanDACsThread (int devIndx, QCstmDacs * dacs, SpidrController * sc);

private:

    SpidrController * _spidrcontrol = nullptr;
    QCstmDacs * _dacs = nullptr;
    int _deviceIndex;
    // IP source address (SPIDR network interface)
    int _srcAddr;

    void run() override;

signals:

    // These are used in the parent class as a signal to thread-safe feed
    //  widgets in the ui
    void progress(int);
    void fillText(QString);
    void fillTextWithIdx(QString, int);
    void addData(int, int, double);
    void addData(int);
    void scanFinished();
    void slideAndSpin(int, int);
};


class UpdateDACsThread : public QThread {

    Q_OBJECT

public:

    explicit UpdateDACsThread (int devIndx, int nDACConfigsAvailable, QCstmDacs * dacs, SpidrController * sc);
    void SetUpdateInTheChip(bool f) { _updateInTheChip = f; } /* true = send to the chip, false = only update guy */

private:

    SpidrController * _spidrcontrol = nullptr;
    QCstmDacs * _dacs = nullptr;
    int _deviceIndex;
    int _nDACConfigsAvailable;
    int _srcAddr; /* IP source address (SPIDR network interface) */
    bool _updateInTheChip;

    void run() override;

signals:

    // These are used in the parent class as a signal to thread-safe feed widgets in the ui
    void updateFinished();
    void slideAndSpin(int, int);
};


#endif
