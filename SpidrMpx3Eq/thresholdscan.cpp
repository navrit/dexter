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

    QStringList items;
    items << MPX3RX_DAC_TABLE[0].name << MPX3RX_DAC_TABLE[1].name;
    ui->comboBox_thresholdToScan->addItems(items);
    ui->comboBox_thresholdToScan->setCurrentIndex(0);
    thresholdScanInst = this;
}

thresholdScan::~thresholdScan()
{
    delete ui;
}

thresholdScan *thresholdScan::getInstance()
{
    return thresholdScanInst;
}

QString thresholdScan::getOriginalPath()
{
    return originalPath;
}

void thresholdScan::setOriginalPath(QString newPath)
{
    originalPath = newPath;
}

uint thresholdScan::getFramesPerStep()
{
    return framesPerStep;
}

void thresholdScan::setFramesPerStep(uint val)
{
    ui->spinBox_framesPerStep->setValue(int(val));
}

void thresholdScan::finishedScan()
{
    enableSpinBoxes();
    ui->button_startStop->setText(tr("Start"));
    _mpx3gui->getConfig()->setTriggerDowntime(_shutterDownMem);
    auto stopTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    ui->textEdit_log->append("Finished Threshold scan @ " +
                            stopTime + "\n");

    //! Print footer to log box and end timer with relevant units

    auto elapsed = timer.elapsed();
    if (elapsed < 1000){
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()) + "ms");
    } else {
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()/1000) + "s");
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

    disableSpinBoxes();

    //! Initialise variables

    //! Get values from GUI
    minTH = ui->spinBox_minimum->value();
    maxTH = ui->spinBox_maximum->value();
    thresholdSpacing = uint(ui->spinBox_spacing->value());
    framesPerStep = uint(ui->spinBox_framesPerStep->value()); /* Minimum of 0 is enforced in the ui code */
    activeDevices = _mpx3gui->getConfig()->getNActiveDevices();

    newPath = ui->textEdit_path->toPlainText();
    if(newPath.isEmpty() || newPath == ""){
        newPath = QDir::homePath();
    }
    setOriginalPath(newPath);

    height = _mpx3gui->getDataset()->getHeight();
    width =  _mpx3gui->getDataset()->getWidth();

    iteration = minTH;

    //! Integrating frames?
    if (framesPerStep > 1) {
        _mpx3gui->set_summing(true);
        qDebug() << "[INFO] Set summing: true";
    } else {
        _mpx3gui->set_summing(false);
        qDebug() << "[INFO] Set summing: false";
    }

    //! Print header to log box and start timer
    auto startTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    ui->textEdit_log->append("Starting Threshold scan @ " +
                             startTime);
    timer.start();

    //! TODO Take Double counter behaviour into account

    _running = true;
    ui->button_startStop->setText(tr("Stop"));
    enableSpinBoxes();

    //! Work around etc.
    _mpx3gui->GetSpidrController()->stopAutoTrigger();
    Sleep( 100 );

    //! Clear the dataset -----------------------------------------------------
    _mpx3gui->getDataset()->zero();

    setThresholdToScan();

    startDataTakingThread();
}

void thresholdScan::stopScan()
{
    enableSpinBoxes();
    ui->button_startStop->setText(tr("Start"));

    //! Print interrupt footer
    ui->textEdit_log->append("GUI Interrupt - stopping Threshold scan at: " +
                            QDateTime::currentDateTimeUtc().toString(Qt::ISODate) +
                             "\n");

    //! Print footer to log box and end timer with relevant units

    auto elapsed = timer.elapsed();
    if (elapsed < 1000){
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()) + "ms");
    } else {
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()/1000) + "s");
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

    if (iteration <= maxTH) {
        //qDebug() << "LOOP " << iteration << maxTH;

        //! Set DACs on all active chips ------------------------------------------
        for (int i = 0 ; i < activeDevices ; i++) {
            if ( ! _mpx3gui->getConfig()->detectorResponds(i) ) {
                qDebug() << "[ERR ] Device " << i << " not responding.";
            } else {
                //! Check if iteration <= 512
                if  (iteration <= 512) {
                    changeAllDACs(iteration);
                }
            }
        }

        //! Save the current dataset ------------------------------------------
        _mpx3gui->getDataset()->toTIFF(makePath(), false); //! Save raw TIFF


        //! Clear the dataset -----------------------------------------------------
        _mpx3gui->getDataset()->zero();


        update_timeGUI();
        //! Increment iteration counter -------------------------------------------
        iteration += thresholdSpacing;

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
    ui->progressBar->setValue(int(double(float(iteration-minTH) / float(maxTH-minTH)) * 100.0));
}

QString thresholdScan::makePath()
{
    QString path = newPath;
    path.append("/");
    //path.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    path.append("th-");
    path.append(QString::number(iteration));
    path.append("_raw.tiff");

    return path;
}

bool thresholdScan::get_changeOtherThresholds()
{
    return changeOtherThresholds;
}

void thresholdScan::set_changeOtherThresholds(bool arg)
{
    changeOtherThresholds = arg;
}

void thresholdScan::enableSpinBoxes()
{
    ui->spinBox_minimum->setEnabled(1);
    ui->spinBox_maximum->setEnabled(1);
    ui->spinBox_spacing->setEnabled(1);
    ui->spinBox_framesPerStep->setEnabled(1);

    ui->comboBox_thresholdToScan->setEnabled(1);
}

void thresholdScan::disableSpinBoxes()
{
    ui->spinBox_minimum->setDisabled(1);
    ui->spinBox_maximum->setDisabled(1);
    ui->spinBox_spacing->setDisabled(1);
    ui->spinBox_framesPerStep->setDisabled(1);

    ui->comboBox_thresholdToScan->setDisabled(1);
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

void thresholdScan::SetDAC_propagateInGUI(int devId, int dac_code, int dac_val)
{
    //! Actually set DAC
    _mpx3gui->GetSpidrController()->setDac( devId, dac_code, dac_val );

    //! Important GUI Code -------------------------------------
    //! Adjust the sliders and the SpinBoxes to the new value
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    //! SlideAndSpin works with the DAC index, no the code.
    int dacIndex = _mpx3gui->getDACs()->GetDACIndex( dac_code );
    emit slideAndSpin( dacIndex,  dac_val );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    // ---------------------------------------------------------

    //! Set DAC in local config.
    _mpx3gui->getDACs()->SetDACValueLocalConfig( devId, dacIndex, dac_val);
}

void thresholdScan::changeAllDACs(int val)
{
    const int activeDevices = _mpx3gui->getConfig()->getNActiveDevices();
    //! Set all thresholds sequentially within one chip at a time unless the checkbox tells you otherwise
    //!    The output of THL0 is very different if the other thresholds TH1-7 are in the noise...

    for (int chipID = 0; chipID < activeDevices; chipID++) {
        for (int dacCode = MPX3RX_DAC_THRESH_0; dacCode <= MPX3RX_DAC_THRESH_7; dacCode++ ) {
            int i = val;

            if (get_changeOtherThresholds() == true) {
                if ( dacCode == MPX3RX_DAC_THRESH_0 ) {
                    SetDAC_propagateInGUI(chipID, dacCode, i);
                } else if ( dacCode == MPX3RX_DAC_THRESH_1 ) {
                    if ( i == 512 ) {
                        i = 511;
                    }
                    SetDAC_propagateInGUI(chipID, dacCode, i+1);
                } else if ( dacCode == MPX3RX_DAC_THRESH_2 ) {
                    if ( i >= 511 ) {
                        i = 510;
                    }
                    SetDAC_propagateInGUI(chipID, dacCode, i+2);
                } else if ( dacCode == MPX3RX_DAC_THRESH_3 ) {
                    if ( i >= 510 ) {
                        i = 509;
                    }
                    SetDAC_propagateInGUI(chipID, dacCode, i+3);
                } else if ( dacCode == MPX3RX_DAC_THRESH_4 ) {
                    if ( i >= 509 ) {
                        i = 508;
                    }
                    SetDAC_propagateInGUI(chipID, dacCode, i+4);
                } else if ( dacCode == MPX3RX_DAC_THRESH_5 ) {
                    if ( i >= 508 ) {
                        i = 507;
                    }
                    SetDAC_propagateInGUI(chipID, dacCode, i+5);
                } else if ( dacCode == MPX3RX_DAC_THRESH_6 ) {
                    if ( i >= 507 ) {
                        i = 506;
                    }
                    SetDAC_propagateInGUI(chipID, dacCode, i+6);
                } else if ( dacCode == MPX3RX_DAC_THRESH_7 ) {
                    if ( i >= 506 ) {
                        i = 505;
                    }
                    SetDAC_propagateInGUI(chipID, dacCode, i+7);
                }
            } else { //! Use the threshold from the dropdown menu + 1
                SetDAC_propagateInGUI(chipID, thresholdToScan, i);
            }
        }
    }
}

void thresholdScan::setThresholdToScan()
{
    thresholdToScan = ui->comboBox_thresholdToScan->currentIndex()+1; //! +1 because MPX3RX_DAC_THRESH_0 = 1, not 0
    //qDebug() << "[INFO][THSCAN]\tThreshold to scan:" << thresholdToScan;
}

void thresholdScan::on_button_startStop_clicked()
{
    if (_running) {
        ui->button_startStop->setText("Start");
        stopScan();
        _mpx3gui->getConfig()->setTriggerDowntime(_shutterDownMem);
    } else {
        ui->button_startStop->setText("Stop");
        _shutterDownMem = (double)_mpx3gui->getConfig()->getTriggerDowntime_64()/1000.0;
        if(_shutterDownMem < 5.0)
            _mpx3gui->getConfig()->setTriggerDowntime(5.0);
        startScan();
    }
}

void thresholdScan::on_pushButton_setPath_clicked()
{
    ui->textEdit_path->setText(getPath("Choose a folder to save the files to."));
}

void thresholdScan::on_spinBox_minimum_valueChanged(int val)
{
    _mpx3gui->getDACs()->GetUI()->dac0SpinBox->setValue(val);
}

void thresholdScan::on_spinBox_maximum_valueChanged(int val)
{
    _mpx3gui->getDACs()->GetUI()->dac1SpinBox->setValue(val);
}

void thresholdScan::on_spinBox_framesPerStep_valueChanged(int val)
{
    if(!_mpx3gui->getVisualization()->GetUI()->infDataTakingCheckBox->isChecked())
        _mpx3gui->getVisualization()->GetUI()->nTriggersSpinBox->setValue(val);
}

void thresholdScan::on_checkBox_incrementOtherThresholds_stateChanged()
{
    if (ui->checkBox_incrementOtherThresholds->isChecked()) {
        set_changeOtherThresholds(true);
        ui->comboBox_thresholdToScan->setDisabled(true);
    } else {
        set_changeOtherThresholds(false);
        ui->comboBox_thresholdToScan->setDisabled(false);
    }
}

void thresholdScan::slot_colourModeChanged(bool)
{
    ui->comboBox_thresholdToScan->clear();
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

    ui->comboBox_thresholdToScan->addItems(items);
    ui->comboBox_thresholdToScan->setCurrentIndex(0);
}
