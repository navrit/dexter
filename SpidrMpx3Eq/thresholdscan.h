#ifndef THRESHOLDSCAN_H
#define THRESHOLDSCAN_H

#include <QWidget>
#include <QElapsedTimer>
#include <QTableView>
#include <QItemDelegate>
#include <QStandardItemModel>

#include "thresholdscandelegate.h"
#include "ServerStatus.h"
#include "mpx3eq_common.h"

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

    int  getThresholdOffset(uint threshold);
    bool getThresholdScanEnabled(uint threshold);
    QCheckBox *getThresholdScanEnabled_pointer(uint threshold);
    int  getStartTH();
    void setStartTH(int val);
    int  getEndTH();
    void setEndTH(int val);
    uint getStepSize();
    void setStepSize(uint val);
    uint getFramesPerStep();
    void setFramesPerStep(uint val);
    void setThresholdToScan(int threshold, bool scan);

    void setThresholdsOnAllChips(int val);

    QString getCurrentTimeISOms();

    void setWindowWidgetsStatus(win_status s = win_status::startup);

private:
    Ui::thresholdScan *ui = nullptr;
    Mpx3GUI * _mpx3gui = nullptr;

    //! QStandardItemModel provides a classic item-based approach to working with the model.
    QStandardItemModel *_standardItemModel = nullptr;
    //! Make a member pointer to a new MyDelegate instance
    ThresholdScanDelegate *_myThresholdScanDelegate = nullptr;
    QElapsedTimer *_timer = nullptr;

    void initTableView();

    void stopScan();
    void resetScan();
    void startDataTakingThread();
    void SetDAC_propagateInGUI(int chip, int dac_code, int dac_val );
    void update_timeGUI();
    void enableOrDisableGUIItems();
    bool _stop = false;
    bool _running = false;
    bool _isScanDescending = true;

    std::vector<bool> _thresholdsToScan = { true, false, false, false, false, false, false, false };
    std::vector<int> _thresholdOffsets = { 0, 0, 0, 0, 0, 0, 0, 0};

    const int _tableRows = 12;
    const int _tableCols = 3;

    uint _stepSize = 1;
    uint _framesPerStep = 1;
    int _startTH = 0;
    int _endTH = 511;
    int _currentThr = 0;
    uint _iteration = 0;
    uint _activeDevices = 0;
    QString _originalPath = "";
    QString _newPath = "";
    QString _runStartDateTimeWithMs = "";
    QList<int> _thresholds = {};

    QString getPath(QString);


public slots:
    void resumeTHScan();
    void button_startStop_clicked_remotely();

private slots:
    void ConnectionStatusChanged(bool conn);

    void finishedScan();
    void on_button_startStop_clicked();
    void on_pushButton_setPath_clicked();
    void slot_colourModeChanged(bool);
    void slot_doubleCounterModeChanged();
    void on_lineEdit_path_editingFinished();
    void on_lineEdit_path_textEdited(const QString &path);

signals:
    void slideAndSpin(int, int);
    void scanIsDone();
    void busy(SERVER_BUSY_TYPE);
};

#endif // THRESHOLDSCAN_H
