#include "thresholdscan.h"
#include "ui_thresholdscan.h"

#include "mpx3gui.h"
#include "ui_mpx3gui.h"

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
    ui->spinBox_framesPerStep->setValue(val);
}

void thresholdScan::finishedScan()
{
    enableSpinBoxes();
    ui->button_startStop->setText(tr("Start"));
    auto stopTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    ui->textEdit_log->append("Finished Threshold scan @ " +
                            stopTime + "\n");
    resetScan();

    //! Print footer to log box and end timer with relevant units

    auto elapsed = timer.elapsed();
    if (elapsed < 1000){
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()) + "ms");
    } else {
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()/1000) + "s");
    }
    ui->textEdit_log->append("\n------------------------------------------");
}

void thresholdScan::startScan()
{
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
}

void thresholdScan::resetScan()
{
//    _mpx3gui->getDataset()->clear();
//    _mpx3gui->getVisualization()->reload_all_layers();
//    _mpx3gui->addLayer(0,0);
    _stop = false;
    _running = false;
}

void thresholdScan::resumeTHScan()
{
    if (_stop)
        return;

    //! Main loop through thresholds using all 8 counters (0-7)

    if (iteration <= maxTH) {
        //qDebug() << "LOOP " << iteration << maxTH;

        //! Set DACs on all active chips ------------------------------------------
        for(int i = 0 ; i < activeDevices ; i++) {
            if ( ! _mpx3gui->getConfig()->detectorResponds(i) ) {
                qDebug() << "[ERR ] Device " << i << " not responding.";
            } else {
                //! Check if iteration <= 512
                if  (iteration <= 512) {
                    changeAllDACs(i);
                }
            }
        }

        //! Save the current dataset ----------------------------------------------
        _mpx3gui->getDataset()->toTIFF(makePath(), false); //! Save raw TIFF

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

void thresholdScan::enableSpinBoxes()
{
    ui->spinBox_minimum->setEnabled(1);
    ui->spinBox_maximum->setEnabled(1);
    ui->spinBox_spacing->setEnabled(1);
    ui->spinBox_framesPerStep->setEnabled(1);
}

void thresholdScan::disableSpinBoxes()
{
    ui->spinBox_minimum->setDisabled(1);
    ui->spinBox_maximum->setDisabled(1);
    ui->spinBox_spacing->setDisabled(1);
    ui->spinBox_framesPerStep->setDisabled(1);
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

void thresholdScan::changeAllDACs(int i)
{
    _mpx3gui->GetSpidrController()->setDac(i, MPX3RX_DAC_THRESH_0, i);
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    int dacIndex = _mpx3gui->getDACs()->GetDACIndex( MPX3RX_DAC_THRESH_0 );
    slideAndSpin( dacIndex,  i );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    _mpx3gui->getDACs()->SetDACValueLocalConfig(i, MPX3RX_DAC_THRESH_0, i);


    if( i == 512 )
        i = 511;
    _mpx3gui->GetSpidrController()->setDac(i, MPX3RX_DAC_THRESH_1, i+1);
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    dacIndex = _mpx3gui->getDACs()->GetDACIndex( MPX3RX_DAC_THRESH_1 );
    slideAndSpin( dacIndex,  i+1 );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    _mpx3gui->getDACs()->SetDACValueLocalConfig(i, MPX3RX_DAC_THRESH_1, i+1);


    if( i == 511 )
        i = 510;
    _mpx3gui->GetSpidrController()->setDac(i, MPX3RX_DAC_THRESH_2, i+2);
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    dacIndex = _mpx3gui->getDACs()->GetDACIndex( MPX3RX_DAC_THRESH_2 );
    slideAndSpin( dacIndex,  i+2 );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    _mpx3gui->getDACs()->SetDACValueLocalConfig(i, MPX3RX_DAC_THRESH_2, i+2);


    if( i == 510 )
        i = 509;
    _mpx3gui->GetSpidrController()->setDac(i, MPX3RX_DAC_THRESH_3, i+3);
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    dacIndex = _mpx3gui->getDACs()->GetDACIndex( MPX3RX_DAC_THRESH_3 );
    slideAndSpin( dacIndex,  i+3 );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    _mpx3gui->getDACs()->SetDACValueLocalConfig(i, MPX3RX_DAC_THRESH_3, i+3);


    if( i == 509 )
        i = 508;
    _mpx3gui->GetSpidrController()->setDac(i, MPX3RX_DAC_THRESH_4, i+4);
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    dacIndex = _mpx3gui->getDACs()->GetDACIndex( MPX3RX_DAC_THRESH_4 );
    slideAndSpin( dacIndex,  i+4 );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    _mpx3gui->getDACs()->SetDACValueLocalConfig(i, MPX3RX_DAC_THRESH_4, i+4);


    if( i == 508 )
        i = 507;
    _mpx3gui->GetSpidrController()->setDac(i, MPX3RX_DAC_THRESH_5, i+5);
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    dacIndex = _mpx3gui->getDACs()->GetDACIndex( MPX3RX_DAC_THRESH_5 );
    slideAndSpin( dacIndex,  i+5 );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    _mpx3gui->getDACs()->SetDACValueLocalConfig(i, MPX3RX_DAC_THRESH_5, i+5);


    if( i == 507 )
        i = 506;
    _mpx3gui->GetSpidrController()->setDac(i, MPX3RX_DAC_THRESH_6, i+6);
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    dacIndex = _mpx3gui->getDACs()->GetDACIndex( MPX3RX_DAC_THRESH_6 );
    slideAndSpin( dacIndex,  i+6 );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    _mpx3gui->getDACs()->SetDACValueLocalConfig(i, MPX3RX_DAC_THRESH_6, i+6);


    if( i == 506 )
        i = 505;
    _mpx3gui->GetSpidrController()->setDac(i, MPX3RX_DAC_THRESH_7, i+7);
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    dacIndex = _mpx3gui->getDACs()->GetDACIndex( MPX3RX_DAC_THRESH_7 );
    slideAndSpin( dacIndex,  i+7 );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    _mpx3gui->getDACs()->SetDACValueLocalConfig(i, MPX3RX_DAC_THRESH_7, i+7);
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

    _heatmap = _thresholdScan->GetUI()->framePlot;

}

void ThresholdScanThread::UpdateHeatMap(int sizex, int sizey)
{
    _heatmap->setData( _data, sizex, sizey);
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

void ThresholdScanThread::run()
{
    // Open a new temporary connection to the SPIDR to avoid collisions to the main one
    // Extract the ip address
    const int ipaddr[4] = { 1, 1, 168, 192 };
    if ( _srcAddr != 0 ) {
        ipaddr[3] = (_srcAddr >> 24) & 0xFF;
        ipaddr[2] = (_srcAddr >> 16) & 0xFF;
        ipaddr[1] = (_srcAddr >>  8) & 0xFF;
        ipaddr[0] = (_srcAddr >>  0) & 0xFF;
    }
    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "[ERR ] Device not connected !";
        return;
    }

    //! Work around etc.
    spidrcontrol->stopAutoTrigger();
    Sleep( 100 );

    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    //! Updates heatmap on GUI
    connect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );

    int minScan = _ui->spinBox_minimum->value();
    int maxScan = _ui->spinBox_maximum->value();
     // --------------------------------------------------
    const int stepScan = _ui->spinBox_spacing->value()+1;
    bool doReadFrames = true;
    int counter = minScan;
    int lastTH = counter-1;
    const QList<int> thresholds = _mpx3gui->getDataset()->getThresholds();
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
        //! TODO Check this works for multiple thresholds
        _summedData = new int [x*y*activeDevices];
        //qDebug() << "[INFO] Allocating bytes :" << sizeof(_summedData) << " " << x*y*activeDevices;
        if (_summedData == nullptr) {
            qDebug() << "\n[WARN] summedData could not be allocated memory...\n";
            return;
        }
    }

    //! TODO Check this works for multiple thresholds
    _data = new int [x*y*activeDevices];

    QString newPath = _ui->textEdit_path->toPlainText();

    if(newPath.isEmpty() || newPath == ""){
        newPath = QDir::homePath();
    }
    _mpx3gui->getTHScan()->setOriginalPath(newPath);

    for ( ; scanContinue ; ) {

        // Set DACs on all chips
        for(int i = 0 ; i < activeDevices ; i++) {
            if ( !_thresholdScan->GetMpx3GUI()->getConfig()->detectorResponds( i ) ) {
                qDebug() << "[ERR ] Device " << i << " not responding.";
            } else {
                //! Check if counter <= 512
                if  (counter <= 512) {
//                    //qDebug() << "[INFO] 1/2 Set DACs on dev:" << i << "DAC:" << MPX3RX_DAC_THRESH_0 << " Val:" << counter;
//                    spidrcontrol->setDac( i, MPX3RX_DAC_THRESH_0, counter );
//                    //qDebug() << "[INFO] 2/2 Set DACs on dev:" << i << "DAC:" << MPX3RX_DAC_THRESH_1 << " Val:" << counter+1;
//                    if (counter == 511)
//                        j = 510;
//                    spidrcontrol->setDac( i, MPX3RX_DAC_THRESH_1, counter+1 );
//                    spidrcontrol->setDac( i, MPX3RX_DAC_THRESH_2, counter+2 );
//                    spidrcontrol->setDac( i, MPX3RX_DAC_THRESH_3, counter+3 );
//                    spidrcontrol->setDac( i, MPX3RX_DAC_THRESH_4, counter+4 );
//                    spidrcontrol->setDac( i, MPX3RX_DAC_THRESH_5, counter+5 );
//                    spidrcontrol->setDac( i, MPX3RX_DAC_THRESH_6, counter+6 );
//                    spidrcontrol->setDac( i, MPX3RX_DAC_THRESH_7, counter+7 );
                    _thresholdScan->changeAllDACs(counter);
                }
            }
        }

        //! Reset all of _summedData to 0 for the next threshold
        //! TODO Check this works for multiple thresholds
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
                + 1; // ms
        //! The extra ms is a combination of delay in the network plus
        //! system overhead.  This should be predicted and not hard-coded.

        while ( spidrdaq->hasFrame( timeOutTime ) ) {

            //QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();

            // A frame is here
            doReadFrames = true;

            // Check quality, if packets lost don't consider the frame
            if ( spidrdaq->lostCountFrame() != 0 ) {
                doReadFrames = false;
            }

            if ( doReadFrames ) {

                //! TODO fix this for scan using N thresholds
                //! Iterate through existing thresholds
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

//                int size_in_bytes = -1;
//                // On all active chips
//                for(int i = 0 ; i < activeDevices.size() ; i++) {
//                    _data = spidrdaq->frameData(i, &size_in_bytes);
//                }

                //qDebug() << "Last: " << lastTH <<  "   Now:" << counter;
                if (summing && lastTH != counter) {
                    sumArrays(x, y);
                }

            }

            // Release
            spidrdaq->releaseFrame();

            // Report to heatmap
            if (doReadFrames) {
                emit UpdateHeatMapSignal(x, y);

                //! Replace current frame data with summedData if we're summing
                if (summing) {
                    _data = _summedData;
                }
                // On all active chips
                for(int idx = 0 ; idx < activeDevices ; idx++) {
                    _mpx3gui->getDataset()->setFrame(_data, idx, 0);
                }

                QString path = newPath;
                path.append("/");
                //path.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                path.append("th-");
                path.append(QString::number(counter));
                path.append("_raw.tiff");

                //qDebug() << "[INFO] Writing to " << path;
                _mpx3gui->getDataset()->toTIFF(path, false); //! Save raw TIFF
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

    disconnect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );

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
