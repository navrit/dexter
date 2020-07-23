#include "thresholdscan.h"
#include "ui_thresholdscan.h"

#include "mpx3gui.h"
#include "ui_mpx3gui.h"

#include "mpx3dacsdescr.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include <QElapsedTimer>
#include <iostream>
#include <fstream>      // std::ofstream

#include "ui_qcstmdacs.h"
#include "ui_qcstmglvisualization.h"


static thresholdScan *thresholdScanInst = nullptr;

thresholdScan::thresholdScan(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::thresholdScan)
{
    ui->setupUi(this);
    thresholdScanInst = this;
    _timer = new QElapsedTimer();

    //! Disable all of the Threshold scan GUI before connection
    this->setEnabled(false);

    initTableView();
}

thresholdScan::~thresholdScan()
{
    delete ui;
}

thresholdScan *thresholdScan::getInstance()
{
    return thresholdScanInst;
}

void thresholdScan::setWindowWidgetsStatus(win_status s)
{
    switch (s) {

    case win_status::startup:
        this->setDisabled( true );
        break;
    case win_status::connected:
        this->setEnabled( true );
        break;
    case win_status::disconnected:
        this->setDisabled( true );
        break;
    default:
        break;
    }
}

void thresholdScan::initTableView()
{
    _myThresholdScanDelegate = new ThresholdScanDelegate(this);

    // Create a new model
    // QStandardItemModel(int tableRows, int columns, QObject * parent = 0)
    _standardItemModel = new QStandardItemModel(_tableRows, _tableCols, this);

    _standardItemModel->setHeaderData(0, Qt::Horizontal, tr("Value"));
    _standardItemModel->setHeaderData(1, Qt::Horizontal, tr("Enabled"));
    _standardItemModel->setHeaderData(2, Qt::Horizontal, tr("Description"));

    const QStringList items = QStringList()
                              << "Threshold 0"
                              << "Threshold 1"
                              << "Threshold 2"
                              << "Threshold 3"
                              << "Threshold 4"
                              << "Threshold 5"
                              << "Threshold 6"
                              << "Threshold 7"
                              << "Step size"
                              << "Frames per step"
                              << "Scan start"
                              << "Scan end";

    //! Print these variables in the Scan Logging?
    //! 1. "Number of steps"
    //! 2. "Estimated time to completion"

    _standardItemModel->setVerticalHeaderLabels(items);

    // Attach the model to the view
    ui->tableView_modelBased->setModel(_standardItemModel);

    // Tie the View with the new ThresholdScanDelegate instance
    // If we do not set this, it will use default delegate - oh no!
    ui->tableView_modelBased->setItemDelegate(_myThresholdScanDelegate);

    //! Set column widths
    ui->tableView_modelBased->setColumnWidth(0, 60);
    ui->tableView_modelBased->setColumnWidth(1, 60);
    ui->tableView_modelBased->setColumnWidth(2, 350);

    //! Generate data
    for (int row = 0; row < _tableRows; row++) {
        for (int col = 0; col < _tableCols; col++) {
            QModelIndex index = _standardItemModel->index(row, col, QModelIndex());

            QVariant value = 99;

            if (col == 0) {
                if (row >= 0 && row <= 7) {
                    value = 0;
                } else if (row == 8) {
                    value = 1;
                } else if (row == 9) {
                    value = 1;
                } else if (row == 10) {
                    value = 256;
                } else if (row == 11) {
                    value = 20;
                } else {
                    value = 101;
                }

                _standardItemModel->setData(index, value);

            } else if (col == 1) {
                if (row >= 0 && row <= 7) {
                    QCheckBox *checkbox = new QCheckBox();
                    checkbox->setCheckable(true);
                    checkbox->setCheckState(Qt::Unchecked);
                    checkbox->setStyleSheet("QCheckBox { \
                                                padding : 5px; \
                                                margin : 3px; \
                                                padding-left : 20px; \
                                            }");
                    ui->tableView_modelBased->setIndexWidget(_standardItemModel->index(row, col), checkbox);
                } else {
                    value = "N/A";
                    _standardItemModel->setData(index, value);
                }

            } else if (col == 2) {
                if (row >= 0 && row <= 7) {
                    value = "Threshold offset and which will be scanned";
                } else if (row == 8) {
                    value = "The scan step size between thresholds";
                } else if (row == 9) {
                    value = "Number of frames to integrate per step";
                } else if (row == 10) {
                    value = "Scan starting threshold";
                } else if (row == 11) {
                    value = "Scan ending threshold";
                } else {
                    value = "...";
                }

                _standardItemModel->setData(index, value);
            }
        }
    }

}

void thresholdScan::setThresholdsOnAllChips(int val)
{
    const uint activeDevices = _mpx3gui->getConfig()->getNActiveDevices();

    for (uint chipID = 0; chipID < activeDevices; chipID++) {
        for (int dacCode = MPX3RX_DAC_THRESH_0; dacCode <= MPX3RX_DAC_THRESH_7; dacCode++ ) {
            if ( _thresholdsToScan[dacCode-1] ) {
                const int DAC_value = val + _thresholdOffsets[dacCode-1];
                SetDAC_propagateInGUI(int(chipID), dacCode, DAC_value);
                qDebug() << "[INFO]\tSet Th" << dacCode-1 << " =" << DAC_value;
            } else {
                const int DAC_value = _mpx3gui->getConfig()->getDACValue(chipID, dacCode-1);
                if (DAC_value == 0) {
                    qDebug() << "[WARN]\tTh" << dacCode-1 << "= 0 !!!!!!!!!!!";
                } else {
                    qDebug() << "[INFO]\tLeaving Th" << dacCode-1 << " =" << DAC_value;
                }
            }
        }
    }
}

QString thresholdScan::getCurrentTimeISOms()
{
    return QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
}

void thresholdScan::finishedScan()
{
    ui->button_startStop->setText(tr("Start"));

    auto stopTime = getCurrentTimeISOms();

    ui->textEdit_log->append("Finished Threshold scan @ " +
                            stopTime + "\n");

    //! Print footer to log box and end timer with relevant units

    auto elapsed = _timer->elapsed();
    if (elapsed < 1000){
        ui->textEdit_log->append("Elapsed time: " + QString::number(_timer->elapsed()) + "ms");
    } else {
        ui->textEdit_log->append("Elapsed time: " + QString::number(_timer->elapsed()/1000) + "s");
    }
    ui->textEdit_log->append("\n------------------------------------------");

    ui->progressBar->setValue(100);

    resetScan();
    emit scanIsDone();
}

void thresholdScan::startScan()
{
    //! Use acquisition settings from other view
    qDebug() << "[INFO]\tStarting a threshold scan";

    resetScan();

    //! Initialise variables
    _runStartDateTimeWithMs = getCurrentTimeISOms();
    _activeDevices = _mpx3gui->getConfig()->getNActiveDevices();

    //! Get values from TableView model (not the logically the same as the GUI)
    _stepSize = getStepSize();
    _framesPerStep = getFramesPerStep();
    _startTH = getStartTH();
    _endTH = getEndTH();

    //! Get threshold starting values from the table model
    for (uint i = 0; i < _thresholdsToScan.size(); ++i) {
        _thresholdsToScan[i] = getThresholdScanEnabled(i);
        qDebug().noquote() << QString("[DEBUG]\t_thresholdsToScan[%1] = %2").arg(i).arg(_thresholdsToScan[i]);
    }

    if ( _startTH < _endTH ) {
        _isScanDescending = false;
    } else {
        _isScanDescending = true;
    }

    //! Set DAC starting values
    if (_isScanDescending) {
        setThresholdsOnAllChips(_startTH);
    } else {
        setThresholdsOnAllChips(_endTH);
    }

    //! Set starting values for thresholds specified
    for (uint i = 0; i < _thresholdOffsets.size(); ++i) {
        _thresholdOffsets[i] = getThresholdOffset(i);

        //! Tell the config about the starting values for the frames
        for (uint chip = 0; chip < uint(_mpx3gui->getConfig()->getNActiveDevices()); ++chip) {
            _mpx3gui->getConfig()->setDACValue(chip, int(i), _thresholdOffsets[i]);
        }
        qDebug().noquote() << QString("[DEBUG]\tSet DAC index = %1 | threshold offset = %2").arg(int(i)).arg(_thresholdOffsets[i]);
    }

    //! At least get something valid in the saving path
    _newPath = ui->lineEdit_path->text();
    if (_newPath.isEmpty() || _newPath == "") {
        _newPath = QDir::homePath();
    }
    setOriginalPath(_newPath);

    //! Make sub-folder
    const QString scanFolder = _newPath + "/scan_" + _runStartDateTimeWithMs;
    QDir().mkdir(scanFolder);
    qDebug() << "[INFO]\tMade scan sub-folder: " << scanFolder;

    //! Integrating frames?
    if ( _framesPerStep > 1 ) {
        _mpx3gui->set_mode_integral();
        qDebug() << "[INFO]\tIntegrating frames: true";
    } else {
        _mpx3gui->set_mode_normal();
        qDebug() << "[INFO]\tIntegrating frames: false";
    }
    //! Tell the config how many frames we want
    _mpx3gui->getConfig()->setNTriggers( int(_framesPerStep) );


    //! Print header to log box and start timer
    auto startTime = getCurrentTimeISOms();
    ui->textEdit_log->append("Starting Threshold scan @ " +
                             startTime);

    //! Update General Settings with the last successfully used threshold path
    _mpx3gui->getGeneralSettings()->setLastThresholdPath(getOriginalPath());

    _currentThr = _startTH;
    _iteration = 0;
    update_timeGUI();
    _timer->start();
    _running = true;
    ui->button_startStop->setText(tr("Stop"));

    //! Work around etc.
    _mpx3gui->GetSpidrController()->stopAutoTrigger();
    Sleep( 100 );

    startDataTakingThread();
}

int thresholdScan::getThresholdOffset(uint threshold)
{
    if (threshold <= 7) {
        bool ok = false;
        auto val = ui->tableView_modelBased->model()->data(_standardItemModel->index(int(threshold), 0, QModelIndex())).toInt(&ok);
        if (ok) return val;
    }
    return 0;
}

bool thresholdScan::getThresholdScanEnabled(uint threshold)
{
    bool val = false;

    if (threshold <= 7) {
        QCheckBox* tmp = qobject_cast<QCheckBox*>(ui->tableView_modelBased->indexWidget(_standardItemModel->index(int(threshold), 1)));
        if (tmp != nullptr) {
            return tmp->isChecked();
        }
    }

    return val;
}

QCheckBox *thresholdScan::getThresholdScanEnabled_pointer(uint threshold)
{
    if (threshold <= 7) {
        QCheckBox* tmp = qobject_cast<QCheckBox*>(ui->tableView_modelBased->indexWidget(_standardItemModel->index(int(threshold), 1)));
        if (tmp != nullptr) {
            return tmp;
        }
    }

    return nullptr;
}

int thresholdScan::getStartTH()
{
    bool ok = false;
    auto val = ui->tableView_modelBased->model()->data(_standardItemModel->index(10, 0, QModelIndex())).toInt(&ok);
    if (ok) {
        return val;
    }
    return 0;
}

void thresholdScan::setStartTH(int val)
{
    if ( val > 0 && val < 512 ) {
        _standardItemModel->setData(_standardItemModel->index(10, 0, QModelIndex()), val);
    }
}

int thresholdScan::getEndTH()
{
    bool ok = false;
    auto val = ui->tableView_modelBased->model()->data(_standardItemModel->index(11, 0, QModelIndex())).toInt(&ok);
    if (ok) return val;
    return 0;
}

void thresholdScan::setEndTH(int val)
{
    if ( val > 0  && val < 512) {
        _standardItemModel->setData(_standardItemModel->index(11, 0, QModelIndex()), val);
    }
}

uint thresholdScan::getStepSize()
{
    bool ok = false;
    auto val = ui->tableView_modelBased->model()->data(_standardItemModel->index(8, 0, QModelIndex())).toUInt(&ok);
    if (ok) return val;
    return 0;
}

void thresholdScan::setStepSize(uint val)
{
    if ( val > 0  && val < 511 ) {
        _standardItemModel->setData(_standardItemModel->index(8, 0, QModelIndex()), val);
    }
}

uint thresholdScan::getFramesPerStep()
{
    bool ok = false;
    auto val = ui->tableView_modelBased->model()->data(_standardItemModel->index(9, 0, QModelIndex())).toUInt(&ok);
    if (ok) return val;
    return 0;
}

void thresholdScan::setFramesPerStep(uint val)
{
    //! Not aware of the real upper limit of this, I expect it's in SpidrController
    if ( val > 0 ) {
        _standardItemModel->setData(_standardItemModel->index(9, 0, QModelIndex()), val);
    }
}

void thresholdScan::setThresholdToScan(int threshold, bool scan)
{
    if (threshold > 0 && threshold < MPX3RX_DAC_THRESH_7) {
        _thresholdsToScan[threshold] = scan;
    } else {
        qDebug() << "[ERROR]\tThreshold to scan could not be set - requested threshold out of range = " << threshold;
    }
}

void thresholdScan::stopScan()
{
    ui->button_startStop->setText(tr("Start"));
    update_timeGUI();

    //! Print interrupt footer
    ui->textEdit_log->append("GUI Interrupt - stopping Threshold scan at: " +
                             getCurrentTimeISOms() +
                             "\n");

    //! Print footer to log box and end timer with relevant units

    auto elapsed = _timer->elapsed();
    if (elapsed < 1000){
        ui->textEdit_log->append("Elapsed time: " + QString::number(_timer->elapsed()) + "ms");
    } else {
        ui->textEdit_log->append("Elapsed time: " + QString::number(_timer->elapsed()/1000) + "s");
    }
    ui->textEdit_log->append("\n------------------------------------------");

    _stop = true;
    _running = false;

    emit busy(FREE);
}

void thresholdScan::resetScan()
{
    _stop = false;
    _running = false;
    emit busy(FREE);
}

void thresholdScan::resumeTHScan()
{
    if (_stop)
        return;

    if (_thresholds.isEmpty()) {
        _thresholds = _mpx3gui->getDataset()->getThresholds();
    }

    //! Main loop through thresholds using all 8 counters (0-7)

    if ((_currentThr >= _startTH && _currentThr <= _endTH) || (_currentThr <= _startTH && _currentThr >= _endTH)) {
        //qDebug().noquote() << QString("[DEBUG]\tLOOP %1 --> From %2 to %3").arg(_currentThr).arg(_startTH).arg(_endTH);

        //! Set threshold DACs on all chips
        setThresholdsOnAllChips(_currentThr);

        //! Save the data, with the offsets calculated into the filename

        //qDebug() << "[DEBUG]\tth =" << th << " | _thresholdOffsets[th] =" << _thresholdOffsets[th];

        foreach (int thr, _thresholds) {
            //! Calculate the actual threshold used during scan, <0 and >511 are not possible to set anyway
            int actualThr = _currentThr + _thresholdOffsets[thr];
            if (actualThr < 0 ) {
                actualThr = 0;
            } else if (actualThr > 511) {
                actualThr = 511;
            }

            //! Save raw TIFF with the following format:
            //!    “scan_“ + start_run_datetime_second_precision + / + scan_iteration + “-” + “th” + threshold_number + “-” + threshold_value + “.” + file_format
            //!
            //!    [scan_20190722_160023] / [000-511] - th[0-7] - [000-511] . [tiff, pgm etc.]
            _mpx3gui->getDataset()->toTIFF(_newPath, false, false, _runStartDateTimeWithMs, thr, _iteration, actualThr);
        }

        update_timeGUI();
        //! Increment/decrement current threshold counter -------------------------------
        if (_isScanDescending) {
            _currentThr -= _stepSize;
        } else {
            _currentThr += _stepSize;
        }

        _iteration++;

        startDataTakingThread();

    } else {

        //! DONE
        finishedScan();
        qDebug() << "[INFO] Threshold scan finished -----------------";
        return;
    }
}

void thresholdScan::button_startStop_clicked_remotely()
{
    if (ui->button_startStop->isEnabled()) {
        on_button_startStop_clicked();
    } else {
        //TODO Reply with an error somehow?

        // Meanwhile, respond with a Threshold scan busy error message
        emit busy(SB_THRESHOLD_SCAN);
    }
}

void thresholdScan::ConnectionStatusChanged(bool conn)
{
    if ( conn ) {
        setWindowWidgetsStatus( win_status::connected );
    } else {
        setWindowWidgetsStatus( win_status::disconnected );
    }
}

void thresholdScan::startDataTakingThread()
{
     emit busy(SB_THRESHOLD_SCAN);
    _mpx3gui->getVisualization()->StartDataTaking("THScan");
}

void thresholdScan::update_timeGUI()
{
    /* I know this looks excessive, it's not. */
    if (_startTH < _endTH) {
        ui->progressBar->setValue(int(double(float(_currentThr - std::min(_startTH, _endTH)) / float(std::max(_startTH, _endTH) - std::min(_startTH, _endTH))) * 100.0));
    } else {
        ui->progressBar->setValue(int(double(float(std::max(_startTH, _endTH) - _currentThr ) / float(std::max(_startTH, _endTH) - std::min(_startTH, _endTH))) * 100.0));
    }
}

void thresholdScan::enableOrDisableGUIItems()
{
    auto now = getCurrentTimeISOms();
    bool colourMode = _mpx3gui->getConfig()->getColourMode();
    bool doubleCounterMode = _mpx3gui->getConfig()->getReadBothCounters();
    int thresholds = 1;

    if ( colourMode ) {
        if ( doubleCounterMode ) {
            thresholds = 8;
        } else {
            thresholds = 4;
        }
    } else {
        if ( doubleCounterMode ) {
            thresholds = 2;
        } else {
            thresholds = 1;
        }
    }

    //! Print messages about why thresholds are irrelevant, disabled and greyed out
    QString msg;
    if (thresholds == 1) {
        msg = now + QString(" - Note: Only threshold 0 is relevant because double counter mode = %1 and colour mode = %2").arg(doubleCounterMode).arg(colourMode);
    } else {
        msg = now + QString(" - Note: Thresholds 0-%1 are relevant because double counter mode = %2 and colour mode = %3").arg(thresholds-1).arg(doubleCounterMode).arg(colourMode);
    }

    ui->textEdit_log->append(msg);

    //! Alter the behaviour of various GUI items based on if they are physically relevant or not
    for (int th = 0; th < MPX3RX_DAC_THRESH_7; ++th) {
        if (th >= thresholds) {
            //! Disable the thresholds enable checkboxes that are not connected physically
            setThresholdToScan(th, false);
            auto checkbox = getThresholdScanEnabled_pointer(th);
            if (checkbox != nullptr) {
                checkbox->setChecked(false);
                checkbox->setDisabled(true);
                checkbox->setStyleSheet(" QCheckBox { \
                                                padding-left : 23px; \
                                                background : #AAAAAA; \
                                          }");
                ui->tableView_modelBased->setIndexWidget(_standardItemModel->index(th, 1), checkbox);
            }
        } else {
            //! Enable all physically possible checkboxes
            auto checkbox = getThresholdScanEnabled_pointer(th);
            if (checkbox != nullptr) {
                checkbox->setStyleSheet(" QCheckBox { \
                                                padding : 5px; \
                                                margin : 3px; \
                                                padding-left : 20px; \
                                                background : transparent; \
                                          }");
                checkbox->setDisabled(false);
                ui->tableView_modelBased->setIndexWidget(_standardItemModel->index(th, 1), checkbox);
            }
        }
    }
}

QString thresholdScan::getPath(QString msg)
{
    QString path = "";
    QString startPath = QDir::currentPath();
    const QString lastSavedPath = _mpx3gui->getGeneralSettings()->getLastThresholdPath();

    //! Use the last saved path via General Settings if it is available
    //! It is only saved there if it is valid already so no real error checking is done here
    if (lastSavedPath != "" && QFileInfo(lastSavedPath).isDir() && QFileInfo(lastSavedPath).isReadable()) {
        startPath = lastSavedPath;
    }

    path = QFileDialog::getExistingDirectory(
                this,
                msg,
                startPath,
                QFileDialog::ShowDirsOnly);

    // We WILL get a path before exiting this function
    return path;
}

void thresholdScan::SetDAC_propagateInGUI(int chip, int dac_code, int dac_val)
{
    //qDebug().noquote() << QString("[DEBUG]\tChip = %1 | DAC Code = %2 | DAC value = %3").arg(chip).arg(dac_code).arg(dac_val);

    //! Actually set DAC
    //! If and after it is complete, continue with GUI code
    if ( _mpx3gui->GetSpidrController()->setDac( chip, dac_code, dac_val ) ) {

        auto DACs = _mpx3gui->getDACs();

        //! Adjust the sliders and the SpinBoxes to the new value
        //! Note: SlideAndSpin works with the DAC index, not the code.

        connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
        int dacIndex = DACs->GetDACIndex( dac_code );
        emit slideAndSpin( dacIndex, dac_val );
        disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );

        //! Set DAC in local config.
        DACs->SetDACValueLocalConfig( uint(chip), dacIndex, dac_val);
    }
}

void thresholdScan::on_button_startStop_clicked()
{
    if (_running) {
        ui->button_startStop->setText("Start");
        stopScan();
    } else {
        ui->button_startStop->setText("Stop");
        enableOrDisableGUIItems();
        startScan();
    }
}

void thresholdScan::on_pushButton_setPath_clicked()
{
    enableOrDisableGUIItems();

    ui->lineEdit_path->setText(getPath("Choose a folder to save the files to."));
}

void thresholdScan::slot_colourModeChanged(bool)
{
    enableOrDisableGUIItems();
}

void thresholdScan::slot_doubleCounterModeChanged()
{
    enableOrDisableGUIItems();
}

void thresholdScan::on_lineEdit_path_editingFinished()
{
    bool success = _mpx3gui->getVisualization()->requestToSetSavePath(ui->lineEdit_path->text());
    Q_UNUSED(success);

    if (ui->lineEdit_path->text().isEmpty()) {
        ui->lineEdit_path->setText(QDir::homePath());
    }

    on_lineEdit_path_textEdited(ui->lineEdit_path->text());
}

void thresholdScan::on_lineEdit_path_textEdited(const QString &path)
{
    const QDir dir(path);
    const QFileInfo dir_info(path);


    if (dir.exists()) {

        // Exists and is writable --> white
        if (dir_info.isWritable()) {
            ui->lineEdit_path->setStyleSheet("QLineEdit { background: rgb(255, 255, 255); selection-background-color: rgb(0, 80, 80); }");
            return;

        // Exists and is not writable --> red
        } else {
            const QString msg = "Path exists but is not writable: " + path;
            //emit sig_statusBarAppend(msg, "red");
            qDebug().noquote() << "[INFO]\t" << msg;

            ui->lineEdit_path->setStyleSheet("QLineEdit { background: rgb(255, 128, 128); selection-background-color: rgb(255, 0, 0); }");

            ui->lineEdit_path->clear();
        }

    } else {
        // Does not exist and is writable --> green
        if (!dir_info.isWritable()) {
            const QString msg = "Path does not exist but is writable: " + path;
            //emit sig_statusBarAppend(msg, "black");
            qDebug().noquote() << "[INFO]\t" << msg;

            ui->lineEdit_path->setStyleSheet("QLineEdit { background: rgb(150, 240, 150); selection-background-color: rgb(50, 150, 50); }");

        // Does not exist and is not writable --> red
        } else {
            const QString msg = "Path does not exist and is not writable (yet): " + path;
            //emit sig_statusBarAppend(msg, "red");
            qDebug().noquote() << "[INFO]\t" << msg;

            ui->lineEdit_path->setStyleSheet("QLineEdit { background: rgb(255, 128, 128); selection-background-color: rgb(255, 0, 0); }");

            ui->lineEdit_path->clear();
        }
    }

    if (path.isEmpty() || path == "") {
        ui->lineEdit_path->setStyleSheet("QLineEdit { background: rgb(255, 255, 255); selection-background-color: rgb(0, 80, 80); }");
    }

    enableOrDisableGUIItems();
}
