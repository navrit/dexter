#ifndef THRESHOLDSCAN_H
#define THRESHOLDSCAN_H

#include <QWidget>
#include <mpx3gui.h>

namespace Ui {
class thresholdScan;
}

class ThresholdScanThread;
class SpidrController;
class QCstmPlotHeatmap;

class thresholdScan : public QWidget
{
    Q_OBJECT

public:
    explicit thresholdScan(QWidget *parent = 0);
    ~thresholdScan();
    Ui::thresholdScan *GetUI(){ return ui; }

    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
    Mpx3GUI * GetMpx3GUI() { return _mpx3gui; }

    QString getOriginalPath();
    void setOriginalPath(QString);

    uint getFramesPerStep();
    void setFramesPerStep(uint val);

private:
    Ui::thresholdScan *ui;
    Mpx3GUI * _mpx3gui;

    //ThresholdScanThread * _thresholdScanThread;

    void startScan();
    void stopScan();
    void resetScan();
    void startDataTakingThread();
    bool _stop = false;
    bool _running = false;

    void update_timeGUI();
    QElapsedTimer timer;

    int thresholdSpacing = 1;
    bool saveFrames = true;
    int minTH = 0;
    int maxTH = 511;
    int framesPerStep = 1;
    QString originalPath;
    int iteration = 0;
    int activeDevices;
    int dacCodeToScan = 0; //! Need to change this if trying to use more than one threshold to scan with
    int height;
    int width;
    QString newPath;

    QString makePath();

    void enableSpinBoxes();
    void disableSpinBoxes();

    QString getPath(QString);

    void SetDAC_propagateInGUI(int devId, int dac_code, int dac_val );

    void changeAllDACs(int i);

public slots:
    void resumeTHScan();

private slots:

    void finishedScan();

    void on_button_startStop_clicked();

    void on_pushButton_setPath_clicked();

    void on_spinBox_minimum_valueChanged(int val);

    void on_spinBox_maximum_valueChanged(int val);

    void on_spinBox_framesPerStep_valueChanged(int val);

signals:
    void slideAndSpin(int, int);
};

class ThresholdScanThread : public QThread {

    Q_OBJECT

public:
    explicit ThresholdScanThread(Mpx3GUI *, thresholdScan *);
    void SetMpx3GUI(Mpx3GUI *p);

    void ConnectToHardware();

    bool getAbort();
    void setAbort(bool);

private:

    void run();

    Mpx3GUI * _mpx3gui;
    thresholdScan * _thresholdScan;
    Ui::thresholdScan * _ui;
    QCstmPlotHeatmap * _heatmap;

    SpidrController * _spidrcontrol;

    // IP source address (SPIDR network interface)
    int _srcAddr;
    int * _data;
    int * _summedData;

    void sumArrays(int, int);

    bool scanContinue = true;
    bool abort = false; // Not used...

public slots:
    void UpdateHeatMap(int, int);

signals:
    //void addData(int, int, double);
    //void addData(int);
    void UpdateHeatMapSignal(int, int);

};

#endif // THRESHOLDSCAN_H
