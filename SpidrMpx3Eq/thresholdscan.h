#ifndef THRESHOLDSCAN_H
#define THRESHOLDSCAN_H

#include <QWidget>
#include <QElapsedTimer>
#include <QTableView>
#include <QItemDelegate>
#include <QStandardItemModel>

#include "thresholdscandelegate.h"
#include "ServerStatus.h"

class Mpx3GUI;

namespace Ui {
    class thresholdScan;
}

class thresholdScan : public QWidget
{
    Q_OBJECT

public:
    explicit thresholdScan(QWidget *parent = nullptr);
    virtual ~thresholdScan() override;
    Ui::thresholdScan *GetUI(){ return ui; }
    static thresholdScan *getInstance();

    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
    Mpx3GUI * GetMpx3GUI() { return _mpx3gui; }

    void startScan();

    QString getOriginalPath(){ return _originalPath; }
    void setOriginalPath(QString) { _originalPath = _newPath; }

    uint getFramesPerStep(){ return _framesPerStep; }
    void setFramesPerStep(uint val){ return; } //! Needs attention

    void changeAllDACs(int val);

    void setThresholdsToScan(); //! Needs attention
    int getThresholdsToScan() {return -1;} //! Needs attention

private:
    Ui::thresholdScan *ui = nullptr;
    Mpx3GUI * _mpx3gui = nullptr;

    //! QStandardItemModel provides a classic item-based approach to working with the model.
    QStandardItemModel *_standardItemModel = nullptr;
    //! Make a member pointer to a new MyDelegate instance
    ThresholdScanDelegate *myThresholdScanDelegate = nullptr;
    QElapsedTimer *_timer = nullptr;

    void initTableView();

    void stopScan();
    void resetScan();
    void startDataTakingThread();
    void SetDAC_propagateInGUI(int chip, int dac_code, int dac_val );
    void update_timeGUI();
    bool _stop = false;
    bool _running = false;

    std::vector<std::tuple<int, bool>> thresholdsToScan = { {0, true}, {1,false}, {2,false}, {3,false}, {4,false}, {5,false}, {6,false}, {7,false} };

    uint _thresholdSpacing = 1;
    uint _framesPerStep = 1;
    int _minTH = 0;
    int _maxTH = 511;
    int _iteration = 0;
    int _activeDevices = 0;
    QString _originalPath = "";
    QString _newPath = "";

    QString makePath();
    QString getPath(QString);


public slots:
    void resumeTHScan();
    void button_startStop_clicked_remotely();

private slots:
    void ConnectionStatusChanged(bool connected);
    void finishedScan();
    void on_button_startStop_clicked();
    void on_pushButton_setPath_clicked();
    void slot_colourModeChanged(bool); //! Needs attention
    void on_lineEdit_path_editingFinished();
    void on_lineEdit_path_textEdited(const QString &path);

signals:
    void slideAndSpin(int, int);
    void scanIsDone();
    void busy(SERVER_BUSY_TYPE);
};

#endif // THRESHOLDSCAN_H
