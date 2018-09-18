#ifndef THRESHOLDSCAN_H
#define THRESHOLDSCAN_H

#include <QWidget>
#include <mpx3gui.h>

namespace Ui {
class thresholdScan;
}

class ThresholdScanThread;
class SpidrController;

class thresholdScan : public QWidget
{
    Q_OBJECT

public:
    explicit thresholdScan(QWidget *parent = nullptr);
    ~thresholdScan();
    Ui::thresholdScan *GetUI(){ return ui; }
    static thresholdScan *getInstance();

    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
    Mpx3GUI * GetMpx3GUI() { return _mpx3gui; }

    void startScan();

    QString getOriginalPath();
    void setOriginalPath(QString);

    uint getFramesPerStep();
    void setFramesPerStep(uint val);

    void changeAllDACs(int val);

    void setThresholdToScan();
    int getThresholdToScan() {return thresholdToScan;}

private:
    Ui::thresholdScan *ui = nullptr;
    Mpx3GUI * _mpx3gui = nullptr;

    void stopScan();
    void resetScan();
    void startDataTakingThread();
    bool _stop = false;
    bool _running = false;

    void update_timeGUI();
    QElapsedTimer timer;

    int thresholdSpacing = 1;
    int minTH = 0;
    int maxTH = 511;
    int framesPerStep = 1;
    QString originalPath;
    int iteration = 0;
    int activeDevices;
    int dacCodeToScan = 0; //! Need to change this if trying to use more than one threshold to scan with
    int height;
    int width;
    bool changeOtherThresholds = false;
    QString newPath;

    QString makePath();

    bool get_changeOtherThresholds();
    void set_changeOtherThresholds(bool arg);

    void enableSpinBoxes();
    void disableSpinBoxes();

    QString getPath(QString);

    void SetDAC_propagateInGUI(int devId, int dac_code, int dac_val );

    QVector<int> turnOnThresholds;

    int thresholdToScan = 0;

public slots:
    void resumeTHScan();
    void on_button_startStop_clicked_remotely();

private slots:

    void finishedScan();

    void on_button_startStop_clicked();

    void on_pushButton_setPath_clicked();

    void on_spinBox_minimum_valueChanged(int val);

    void on_spinBox_maximum_valueChanged(int val);

    void on_spinBox_framesPerStep_valueChanged(int val);

    void on_checkBox_incrementOtherThresholds_stateChanged();

    void slot_colourModeChanged(bool);

signals:
    void slideAndSpin(int, int);
    void scanIsDone(void);
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

    Mpx3GUI * _mpx3gui = nullptr;
    thresholdScan * _thresholdScan = nullptr;
    Ui::thresholdScan * _ui = nullptr;

    SpidrController * _spidrcontrol = nullptr;

    // IP source address (SPIDR network interface)
    int _srcAddr;
    int * _data = nullptr;
    int * _summedData = nullptr;

    QVector<int> _turnOnThresholds;

    void sumArrays(int, int);

    bool scanContinue = true;
    bool abort = false; // Not used...

};

#endif // THRESHOLDSCAN_H
