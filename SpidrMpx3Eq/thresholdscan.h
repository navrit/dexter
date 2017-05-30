#ifndef THRESHOLDSCAN_H
#define THRESHOLDSCAN_H

#include <QWidget>
#include "mpx3gui.h"

namespace Ui {
class thresholdScan;
}

class thresholdScan : public QWidget
{
    Q_OBJECT

public:
    explicit thresholdScan(QWidget *parent = 0);
    ~thresholdScan();
    Ui::thresholdScan *GetUI(){ return ui; }

    void SetMpx3GUI(Mpx3GUI *p);

private slots:


    void on_button_startStop_clicked();

private:
    Ui::thresholdScan *ui = nullptr;
    Mpx3GUI *_mpx3gui = nullptr;

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

#endif // THRESHOLDSCAN_H
