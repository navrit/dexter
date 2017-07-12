#ifndef THRESHOLDSCAN_H
#define THRESHOLDSCAN_H

#include <QWidget>
#include "mpx3gui.h"

namespace Ui {
class thresholdScan;
}

class ThresholdScanThread;
class SpidrController;

class thresholdScan : public QWidget
{
    Q_OBJECT

public:
    explicit thresholdScan(Mpx3GUI *parent = 0);
    ~thresholdScan();
    Ui::thresholdScan *GetUI(){ return ui; }

    void SetMpx3GUI(Mpx3GUI *p) { _mpx3gui = p; }
    Mpx3GUI * GetMpx3GUI() { return _mpx3gui; }

private slots:


    void on_button_startStop_clicked();

private:
    Ui::thresholdScan *ui;
    // Connectivity between modules
    Mpx3GUI * _mpx3gui;

    ThresholdScanThread * _thresholdScanThread;

    void startScan();
    void stopScan();
    void resetScan();
    void resumeScan();
    void startDataTakingThread();
    bool _stop = false;
    bool _running = false;

    uint thresholdSpacing = 1;
    bool saveFrames = true;
    uint minTH = 0;
    uint maxTH = 511;
    uint framesPerStep = 1;

    void enableSpinBoxes();
    void disableSpinBoxes();
};

class ThresholdScanThread : public QThread {

    Q_OBJECT

public:
    explicit ThresholdScanThread(Mpx3GUI *, thresholdScan *);
    void ConnectToHardware();

    bool getAbort();
    void setAbort(bool);

private:

    void run();

    Mpx3GUI * _mpx3gui;
    thresholdScan * _thresholdScan;
    Ui::thresholdScan * _ui;

    SpidrController * _spidrcontrol;

    // IP source address (SPIDR network interface)
    int _srcAddr;
    int * _data;

    bool scanContinue = true;
    bool abort = false; // Not used...

};

#endif // THRESHOLDSCAN_H
