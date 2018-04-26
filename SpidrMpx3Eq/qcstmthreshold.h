#ifndef QCSTMTHRESHOLD_H
#define QCSTMTHRESHOLD_H

#include <QWidget>
#include "mpx3gui.h"

#include <QPointF>

namespace Ui {
class QCstmThreshold;
}

class CustomScanThread;
class QCstmPlotHeatmap;

class QCstmThreshold : public QWidget
{
    Q_OBJECT

public:

    explicit QCstmThreshold(QWidget *parent = nullptr);
    ~QCstmThreshold();
    Ui::QCstmThreshold * GetUI(){ return ui; }

    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
    Mpx3GUI * GetMpx3GUI() { return _mpx3gui; }


    void SetupSignalsAndSlots();
    int ExtractScanInfo(int * data, int size_in_bytes, int thl);

    void setPoint(QPointF data, int plot);
    void addPoint(QPointF data, int plot);
    double getPoint(int x, int plot);
    int getCurrentPlotIndex(){ return _plotIdxCntr - 1; }

    int getActiveTargetCode();
    QString getActiveTargetName();
    bool isScanDescendant() { return _scanDescendant; }
    bool keepPreviousPlots() { return _keepPlots; }

private:

    Ui::QCstmThreshold *ui;
    // Connectivity between modules
    Mpx3GUI * _mpx3gui;
    CustomScanThread * _scanThread;

    void addFrame(QPoint offset, int layer, int*data);
    // Currently active graph
    QCPGraph * _graph;

    std::vector<double> derivativeFivePointStencil(std::vector<int> x, std::vector<double> f);

    int * _data;
    map<int, int> _plotIdxMap; // keep track of plot's indexes
    int _plotIdxCntr;
    bool _scanDescendant;
    bool _keepPlots;
    bool _logyPlot;

    QString defaultYLabel = tr("Counts");
    QString defaultXLabel = tr("THL (DAC Units)");
    QString altYLabel =     tr("dC/dTHL");

private slots:

    void on_rangeDirectionCheckBox_toggled(bool checked);
    void on_keepCheckbox_toggled(bool checked);
    void on_logyCheckBox_toggled(bool checked);
    void on_thlCalibDifferentiateCheckBox_toggled(bool checked);

    void addData(int dacIdx, int dacVal, double adcVal );
    void addData(int);

    void StartCalibration();
    //void UpdateHeatMap();
    void UpdateChart(int setId, int thlValue);

    void on_pushButtonSave_clicked();

    void on_thlCalibStop_clicked();

signals:
    void slideAndSpin(int, int);
    //void UpdateHeatMapSignal();
    void UpdateChartSignal(int, int); // Not used?


};

class CustomScanThread : public QThread {

    Q_OBJECT

public:
    explicit CustomScanThread(Mpx3GUI *, QCstmThreshold *);
    void ConnectToHardware();
    int PixelsReactive(int * data, int size_in_bytes, int );

    bool getAbort();
    void setAbort(bool);

private:

    void run();

    Mpx3GUI * _mpx3gui;
    QCstmThreshold * _cstmThreshold;
    Ui::QCstmThreshold * _ui;
    QCstmPlotHeatmap * _heatmap;

    // IP source address (SPIDR network interface)
    int _srcAddr;
    int * _data;

    bool scanContinue = true;
    bool abort = false;

    int minimumTurnOnValue = 2;

public slots:
    void UpdateHeatMap(int, int);

signals:
    void addData(int, int, double);
    void addData(int);
    void fillText(QString);
    void UpdateHeatMapSignal(int, int);

};
#endif // QCSTMTHRESHOLD_H
