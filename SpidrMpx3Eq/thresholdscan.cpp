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

void thresholdScan::initTableView()
{
    myThresholdScanDelegate = new ThresholdScanDelegate(this);

    const int rows = 12;
    const int cols = 3;

    // Create a new model
    // QStandardItemModel(int rows, int columns, QObject * parent = 0)
    _standardItemModel = new QStandardItemModel(rows, cols, this);

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
    ui->tableView_modelBased->setItemDelegate(myThresholdScanDelegate);

    // Set column widths
    ui->tableView_modelBased->setColumnWidth(0, 60);
    ui->tableView_modelBased->setColumnWidth(1, 60);
    ui->tableView_modelBased->setColumnWidth(2, 260);

    // Generate data
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
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
                    value = "Threshold to scan";
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

void thresholdScan::changeAllDACs(int val)
{
    //const int activeDevices = _mpx3gui->getConfig()->getNActiveDevices();
    //! The output of THL0 is very different if the other thresholds TH1-7 are in the noise...

    /*for (int chipID = 0; chipID < activeDevices; chipID++) {
        for (int dacCode = MPX3RX_DAC_THRESH_0; dacCode <= MPX3RX_DAC_THRESH_7; dacCode++ ) {
            SetDAC_propagateInGUI(chipID, thresholdToScan, val);
            }
        }
    }*/
}

void thresholdScan::finishedScan()
{
    // enableSpinBoxes();
    ui->button_startStop->setText(tr("Start"));

    auto stopTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

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

    resetScan();
    emit scanIsDone();
}

void thresholdScan::startScan()
{
    qDebug() << "[INFO]\tStarting a threshold scan";
    //! Use acquisition settings from other view
    resetScan();

    // disableSpinBoxes();

    //! Initialise variables

    //! Get values from GUI
    // minTH = ui->spinBox_minimum->value();
    // maxTH = ui->spinBox_maximum->value();
    // thresholdSpacing = uint(ui->spinBox_spacing->value());
    // framesPerStep = uint(ui->spinBox_framesPerStep->value()); /* Minimum of 0 is enforced in the ui code */
    _activeDevices = _mpx3gui->getConfig()->getNActiveDevices();

    _newPath = ui->lineEdit_path->text();
    if(_newPath.isEmpty() || _newPath == ""){
        _newPath = QDir::homePath();
    }
    setOriginalPath(_newPath);

    _iteration = _minTH;

    //! Integrating frames?
    if (_framesPerStep > 1) {
        _mpx3gui->set_summing(true);
        qDebug() << "[INFO]\tSumming frames: true";
    } else {
        _mpx3gui->set_summing(false);
        qDebug() << "[INFO]\tSumming frames: false";
    }

    //! Print header to log box and start timer
    auto startTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    ui->textEdit_log->append("Starting Threshold scan @ " +
                             startTime);
    _timer->start();

    //! TODO Take Double counter behaviour into account

    _running = true;
    ui->button_startStop->setText(tr("Stop"));

    // enableSpinBoxes();

    //! Work around etc.
    _mpx3gui->GetSpidrController()->stopAutoTrigger();
    Sleep( 100 );

    //! Clear the dataset - not necessary due to driver changes? Check this
    //_mpx3gui->getDataset()->zero();

    setThresholdsToScan();

    startDataTakingThread();
}

void thresholdScan::stopScan()
{
    // enableSpinBoxes();
    ui->button_startStop->setText(tr("Start"));

    //! Print interrupt footer
    ui->textEdit_log->append("GUI Interrupt - stopping Threshold scan at: " +
                            QDateTime::currentDateTimeUtc().toString(Qt::ISODate) +
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
    _mpx3gui->getVisualization()->dataTakingFinished();
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

    //! Main loop through thresholds using all 8 counters (0-7)

    if (_iteration <= _maxTH) {
        //qDebug() << "LOOP " << iteration << maxTH;

        //! Set DACs on all active chips ------------------------------------------
        for (int i = 0 ; i < _activeDevices ; i++) {
            if ( ! _mpx3gui->getConfig()->detectorResponds(i) ) {
                qDebug() << "[ERR ] Device " << i << " not responding.";
            } else {
                //! Check if iteration <= 512
                if  (_iteration <= 512) {
                    changeAllDACs(_iteration);
                }
            }
        }

        //! Save the current dataset ------------------------------------------
        _mpx3gui->getDataset()->toTIFF(makePath(), false); //! Save raw TIFF


        //! Clear the dataset - not necessary due to driver changes? Check this
        // _mpx3gui->getDataset()->zero();


        update_timeGUI();
        //! Increment iteration counter -------------------------------------------
        _iteration += _thresholdSpacing;

        startDataTakingThread();

    } else {

        //! DONE
        ui->progressBar->setValue(100);
        finishedScan();

//        qDebug() << "[INFO] Threshold scan finished -----------------";
        return;
    }
}

void thresholdScan::button_startStop_clicked_remotely()
{
    on_button_startStop_clicked();
}

void thresholdScan::startDataTakingThread()
{
     emit busy(SB_THRESHOLD_SCAN);
    _mpx3gui->getVisualization()->StartDataTaking("THScan");
}

void thresholdScan::update_timeGUI()
{
    /* I know this looks excessive, it's not. */
    ui->progressBar->setValue(int(double(float(_iteration-_minTH) / float(_maxTH-_minTH)) * 100.0));
}

QString thresholdScan::makePath()
{
    QString path = _newPath;
    path.append("/");
    //path.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    path.append("th-");
    path.append(QString::number(_iteration));
    path.append("_raw.tiff");

    return path;
}

QString thresholdScan::getPath(QString msg)
{
    QString path = "";
    path = QFileDialog::getExistingDirectory(
                this,
                msg,
                QDir::currentPath(),
                QFileDialog::ShowDirsOnly);

    // We WILL get a path before exiting this function
    return path;
}

void thresholdScan::SetDAC_propagateInGUI(int chip, int dac_code, int dac_val)
{
    //! Actually set DAC
    _mpx3gui->GetSpidrController()->setDac( chip, dac_code, dac_val );

    //! Important GUI Code -------------------------------------
    //! Adjust the sliders and the SpinBoxes to the new value
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    //! SlideAndSpin works with the DAC index, not the code.
    int dacIndex = _mpx3gui->getDACs()->GetDACIndex( dac_code );
    emit slideAndSpin( dacIndex, dac_val );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    // ---------------------------------------------------------

    //! Set DAC in local config.
    _mpx3gui->getDACs()->SetDACValueLocalConfig( chip, dacIndex, dac_val);
}

void thresholdScan::setThresholdsToScan()
{  
    qDebug() << "[INFO][THSCAN]\tThreshold(s) to scan: NOT IMPLEMENTED";
}

void thresholdScan::on_button_startStop_clicked()
{
    if (_running) {
        ui->button_startStop->setText("Start");
        stopScan();
    } else {
        ui->button_startStop->setText("Stop");
        startScan();
    }
}

void thresholdScan::on_pushButton_setPath_clicked()
{
    ui->lineEdit_path->setText(getPath("Choose a folder to save the files to."));
}

void thresholdScan::slot_colourModeChanged(bool)
{
    // ui->comboBox_thresholdToScan->clear();
    QStringList items;
    int maxThreshold = 0;

    if (_mpx3gui->getConfig()->getColourMode()) {
        maxThreshold = MPX3RX_DAC_THRESH_7;
    } else {
        maxThreshold = MPX3RX_DAC_THRESH_1;
    }

    for (int i = 0; i < maxThreshold; i++) {
        items << MPX3RX_DAC_TABLE[i].name;
    }

    // ui->comboBox_thresholdToScan->addItems(items);
    // ui->comboBox_thresholdToScan->setCurrentIndex(0);
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
}
