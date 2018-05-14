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

thresholdScan::thresholdScan(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::thresholdScan)
{
    ui->setupUi(this);

    QStringList items;
    items << MPX3RX_DAC_TABLE[0].name << MPX3RX_DAC_TABLE[1].name;
    ui->comboBox_thresholdToScan->addItems(items);
    ui->comboBox_thresholdToScan->setCurrentIndex(0);
}

thresholdScan::~thresholdScan()
{
    delete ui;
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

    if( _testPulseEqualisation ) {
        emit testPulseScanComplete();
    }

    resetScan();
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
    thresholdSpacing = ui->spinBox_spacing->value();
    framesPerStep = ui->spinBox_framesPerStep->value();
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
    _testPulseEqualisation = false;
}

void thresholdScan::resetScan()
{
    _stop = false;
    _running = false;
    _testPulseEqualisation = false;
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

        if ( !_testPulseEqualisation ) {
            //! Save the current dataset ------------------------------------------
            _mpx3gui->getDataset()->toTIFF(makePath(), false); //! Save raw TIFF
        } else {
            //! Doing a testPulseEqualisation, so no saving...
        }


        //! Clear the dataset -----------------------------------------------------
        _mpx3gui->getDataset()->zero();


        update_timeGUI();
        //! Increment iteration counter -------------------------------------------
        iteration++;

        startDataTakingThread();

    } else {

        //! DONE
        ui->progressBar->setValue(100);
        finishedScan();

//        qDebug() << "[INFO] Threshold scan finished -----------------";
        return;
    }
}

void thresholdScan::startDataTakingThread()
{
    _mpx3gui->getVisualization()->StartDataTaking("THScan");
}

void thresholdScan::update_timeGUI()
{
    ui->progressBar->setValue( (float(iteration-minTH) / float(maxTH-minTH)) * 100.0);
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
            } else {
                if ( dacCode == MPX3RX_DAC_THRESH_0 ) {
                    SetDAC_propagateInGUI(chipID, dacCode, i);
                }
            }
        }
    }
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
    _mpx3gui->getVisualization()->GetUI()->nTriggersSpinBox->setValue(val);
}

//! ------------------------------------------------------------------------------------------
ThresholdScanThread::ThresholdScanThread(Mpx3GUI * mpx3gui, thresholdScan * thresholdScanA)
{
    _mpx3gui = mpx3gui;
    _thresholdScan = thresholdScanA;
    _ui = _thresholdScan->GetUI();
}

void ThresholdScanThread::SetMpx3GUI(Mpx3GUI *p)
{
    _mpx3gui = p;
}

void ThresholdScanThread::ConnectToHardware()
{
    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    if( spidrcontrol ) { spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

}

bool ThresholdScanThread::getAbort()
{
    return abort;
}

void ThresholdScanThread::setAbort(bool arg)
{
    abort = arg;
    return;
}

//! TODO Make this work for multiple thresholds
void ThresholdScanThread::run()
{
    // Open a new temporary connection to the SPIDR to avoid collisions to the main one
    // Extract the ip address
    int ipaddr[4] = { 1, 1, 168, 192 };
    if ( _srcAddr != 0 ) {
        ipaddr[3] = (_srcAddr >> 24) & 0xFF;
        ipaddr[2] = (_srcAddr >> 16) & 0xFF;
        ipaddr[1] = (_srcAddr >>  8) & 0xFF;
        ipaddr[0] = (_srcAddr >>  0) & 0xFF;
    }
    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "[ERR]\tDevice not connected !";
        return;
    }

    //! Work around etc.
    spidrcontrol->stopAutoTrigger();
    Sleep( 100 );

    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    int minScan = _ui->spinBox_minimum->value();
    int maxScan = _ui->spinBox_maximum->value();
     // --------------------------------------------------
    const int stepScan = _ui->spinBox_spacing->value();
    bool doReadFrames = true;
    int counter = minScan;
    int lastTH = counter-1;
    const QList<int> thresholds = _mpx3gui->getDataset()->getThresholds();
    const int thresholdToScan = _ui->comboBox_thresholdToScan->currentIndex();
    // ---------------------------------------------------

    const int activeDevices = _mpx3gui->getConfig()->getNActiveDevices();
    const int y = _mpx3gui->getDataset()->y();
    const int x = _mpx3gui->getDataset()->x();
    int framesPerStep = _mpx3gui->getTHScan()->getFramesPerStep();
    if (framesPerStep == 0) {
        _mpx3gui->getTHScan()->setFramesPerStep(1);
        framesPerStep = 1;
    }

    bool summing = false;
    if (framesPerStep > 1){
        summing = true;
        //! TODO Make this work for multiple thresholds
        _summedData = new int [x*y*activeDevices];
        //qDebug() << "[INFO]\tAllocating bytes :" << sizeof(_summedData) << " " << x*y*activeDevices;
        if (_summedData == nullptr) {
            qDebug() << "\n[WARN]\tsummedData could not be allocated memory...\n";
            return;
        }
    }

    //! TODO Make this work for multiple thresholds
    _data = new int [x*y*activeDevices];

    QString newPath = _ui->textEdit_path->toPlainText();

    if(newPath.isEmpty() || newPath == ""){
        newPath = QDir::homePath();
    }
    _mpx3gui->getTHScan()->setOriginalPath(newPath);


    const bool isTestPulseEqualisation = _thresholdScan->getTestPulseEqualisation();
    if (isTestPulseEqualisation) {
        qDebug() << "[INFO]\tStarting a threshold scan for test pulse equalisation.";
        _turnOnThresholds.reserve(x*y*activeDevices);
        std::fill(_turnOnThresholds.begin(), _turnOnThresholds.end(), -1);
    }

    for ( ; scanContinue ; ) {

        // Set DACs on all chips
        for(int i = 0 ; i < activeDevices ; i++) {
            if ( !_thresholdScan->GetMpx3GUI()->getConfig()->detectorResponds( i ) ) {
                qDebug() << "[ERR]\tDevice " << i << " not responding.";
            } else {
                //! Check if counter <= 512
                if  (counter <= 512) {
                    _thresholdScan->changeAllDACs(counter);
                }
            }
        }

        //! Reset all of _summedData to 0 for the next threshold
        //! TODO Make this work for multiple thresholds
        if (summing){
            memset(_summedData, 0, (x*y*activeDevices));
        }

        // Start the trigger as configured
        spidrcontrol->startAutoTrigger();

        doReadFrames = true;

        // Timeout
        int timeOutTime =
                _mpx3gui->getConfig()->getTriggerLength_ms()
                + _mpx3gui->getConfig()->getTriggerDowntime_ms()
                + 10; // ms
        //! The extra 10 ms is a combination of delay in the network plus
        //! system overhead.  This should be predicted and not hard-coded.

        while ( spidrdaq->hasFrame( timeOutTime ) ) {

            // A frame is here
            doReadFrames = true;

            // Check quality, if packets lost don't consider the frame
            if ( spidrdaq->lostCountFrame() != 0 ) {
                doReadFrames = false;
            }

            if ( doReadFrames ) {

                //! TODO fix this for scan using N thresholds
                //! Iterate through existing thresholds

                /*
                for (const int &th : thresholds) {
                    qDebug() << ">>>>>>>>>> Threshold in existance: " << th;
                }

                int tmp;

                int threshold = 0;
                for (int i=0; i < y; i++) {
                    for (int j=0; j < x; j++) {
                        //qDebug() << "(x, y):" << j << "," << i << i*x+j;
//                        _data[i*x + j] = _mpx3gui->getDataset()->sample(j, i, threshold);
                        tmp = _mpx3gui->getDataset()->sample(j, i, threshold);
                        if (tmp > 0)
                            qDebug() << "[DATA]: " << tmp;
                    }
                }
                */

                //qDebug() << "Last: " << lastTH <<  "   Now:" << counter;
                if (summing && lastTH != counter) {
                    sumArrays(x, y);
                }
            }

            // Release
            spidrdaq->releaseFrame();

            if (doReadFrames) {

                //! Replace current frame data with summedData if we're summing
                if (summing) {
                    _data = _summedData;
                }
                // On all active chips
                for(int idx = 0 ; idx < activeDevices ; idx++) {
                    _mpx3gui->getDataset()->setFrame(_data, idx, 0);
                }

                if (!isTestPulseEqualisation) {
                    QString path = newPath;
                    path.append("/");
                    //path.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                    path.append("th-");
                    path.append(QString::number(counter));
                    path.append("_raw.tiff");

                    //qDebug() << "[INFO] Writing to " << path;
                    _mpx3gui->getDataset()->toTIFF(path, false); //! Save raw TIFF
                } else {
                    //! Doing a testPulseEqualisation
                    QVector<int> frame = _mpx3gui->getDataset()->makeFrameForSaving(thresholdToScan, false, false);
                    for (int i=0; i < frame.size(); i++ ){
                        if ( frame[i] >= 10 && _turnOnThresholds[i] > i) {
                            _turnOnThresholds[i] = i;
                            qDebug() << "turnOnThresholds updated:" << _turnOnThresholds[i] << i << frame[i];
                        }
                    }
                }
            }
        }

        lastTH = counter;
        // increment
        counter += stepScan;

        // Check the termination condition
        if ( counter <= maxScan && !abort ) {
            scanContinue = true;
        }
        else {
            scanContinue = false;
        }

    }

    spidrcontrol->stopAutoTrigger();

    delete spidrcontrol;
}

void ThresholdScanThread::sumArrays(int nx, int ny)
{
    //! Bram's special pointer method
//    int * s = _summedData;
//    int * d = _data;
//    for (int i = 0; i < nx*ny; i++)
//        *(s++) += *(d++);

    //! Same as this
    for(unsigned y = 0;  y < (unsigned)ny; y++) {
        for(unsigned x = 0; x < (unsigned)nx; x++) {
             _summedData[y*nx+x] += _data[y*nx+x];
        }
    }
}

void thresholdScan::on_checkBox_incrementOtherThresholds_stateChanged()
{
    if (ui->checkBox_incrementOtherThresholds->isChecked()) {
        set_changeOtherThresholds(true);
    } else {
        set_changeOtherThresholds(false);
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
