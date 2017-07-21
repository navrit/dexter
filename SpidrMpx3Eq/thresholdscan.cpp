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
    _thresholdScanThread = nullptr;
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

void thresholdScan::finishedScan()
{
    enableSpinBoxes();
    ui->button_startStop->setText(tr("Start"));
    auto stopTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    ui->textEdit_log->append("Finished Threshold scan @ " +
                            stopTime +
                             "\n------------------------------------------");
    resetScan();

    //! Print footer to log box and end timer with relevant units

    auto elapsed = timer.elapsed();
    if (elapsed < 1000){
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()) + "ms");
    } else {
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()/1000) + "s");
    }
}

void thresholdScan::startScan()
{
    //! Use acquisition settings from other view
    resetScan();

    //! TODO Test this disable/enable spinbox behaviour - offline attempts are not trivial
    disableSpinBoxes();

    //! Initialise variables
    //!
    //! Get values from GUI
    minTH = ui->spinBox_minimum->value();
    maxTH = ui->spinBox_maximum->value();
    thresholdSpacing = ui->spinBox_spacing->value();
    framesPerStep = ui->spinBox_framesPerStep->value();

    //! Print header to log box and start timer
    ui->textEdit_log->append("------------------------------------------");
    auto startTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    ui->textEdit_log->append("Starting Threshold scan @ " +
                             startTime);
    timer.start();

    //! Do the number of frames loop within the threshold scan

    //! TODO Take Double counter behaviour into account

    _running = true;
    ui->button_startStop->setText(tr("Stop"));
    enableSpinBoxes();

    startDataTakingThread();

}

void thresholdScan::stopScan()
{
    enableSpinBoxes();

    //! Print interrupt footer
    ui->textEdit_log->append("GUI Interrupt - stopping Threshold scan at: " +
                            QDateTime::currentDateTimeUtc().toString(Qt::ISODate) +
                             "\n------------------------------------------");
    _stop = true;

    if ( _thresholdScanThread ) {
        if ( _thresholdScanThread->isRunning() ) {
            _thresholdScanThread->setAbort(true);
        }
    }

    _running = false;

}

void thresholdScan::resetScan()
{
    _mpx3gui->getDataset()->clear();
    _stop = false;
    _running = false;
}

void thresholdScan::startDataTakingThread()
{

    if ( _thresholdScanThread ) {
        if ( _thresholdScanThread->isRunning() ) {
            return;
        }
        delete _thresholdScanThread;
        _thresholdScanThread = nullptr;
    }

    // Go on with the scan thread
    _thresholdScanThread = new ThresholdScanThread( _mpx3gui, this );

    connect(_thresholdScanThread, SIGNAL(finished()), this, SLOT(finishedScan()));
    _thresholdScanThread->ConnectToHardware();
    _thresholdScanThread->start();

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
    int ipaddr[4] = { 1, 1, 168, 192 };
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
    int stepScan = _ui->spinBox_spacing->value()+1;
    int dacCodeToScan = MPX3RX_DAC_THRESH_0;
    bool doReadFrames = true;
    int counter = minScan;
    int lastTH = counter-1;

    int activeDevices = _mpx3gui->getConfig()->getNActiveDevices();
    int y = _mpx3gui->getDataset()->y();
    int x = _mpx3gui->getDataset()->x();
    int framesPerStep = _mpx3gui->getTHScan()->getFramesPerStep();

    bool summing = false;
    if (framesPerStep > 1){
        summing = true;
        _summedData = new int [x*y*activeDevices];
        //qDebug() << "[INFO] Allocating bytes :" << sizeof(_summedData) << " " << x*y*activeDevices;
        if (_summedData == nullptr) {
            qDebug() << "\n[WARN] summedData could not be allocated memory...\n";
            return;
        }
    }

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
                //! Check if 0 < counter < 512
                if  (counter < 511) {
                    //qDebug() << "[INFO] 1/2 Set DACs on dev:" << i << "DAC:" << dacCodeToScan << " Val:" << counter;
                    spidrcontrol->setDac( i, dacCodeToScan, counter );
                    //qDebug() << "[INFO] 2/2 Set DACs on dev:" << i << "DAC:" << dacCodeToScan+1 << " Val:" << counter+1;
                    spidrcontrol->setDac( i, dacCodeToScan+1, counter+1 );
                }
            }
        }

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
        //! TODO The extra ms is a combination of delay in the network plus
        //! system overhead.  This should be predicted and not hard-coded.

        while ( spidrdaq->hasFrame( timeOutTime ) ) {

            QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();

            // A frame is here
            doReadFrames = true;

            // Check quality, if packets lost don't consider the frame
            if ( spidrdaq->lostCountFrame() != 0 ) {
                doReadFrames = false;
            }

            if ( doReadFrames ) {
                int size_in_bytes = -1;

                // On all active chips
                for(int i = 0 ; i < activeDevices.size() ; i++) {
                    _data = spidrdaq->frameData(i, &size_in_bytes);
                }

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
                for(int idx = 0 ; idx < activeDevices.size() ; idx++) {
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

        // See the termination condition
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
    int * s = _summedData;
    int * d = _data;
    for (int i = 0; i < nx*ny; i++)
        *(s++) += *(d++);

    //! Same as this
//    for(unsigned y = 0;  y < (unsigned)ny; y++) {
//        for(unsigned x = 0; x < (unsigned)nx; x++) {
//             _summedData[y*nx+x] += _data[y*nx+x];
//        }
//    }
}
