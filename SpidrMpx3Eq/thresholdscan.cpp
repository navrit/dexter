#include "thresholdscan.h"
#include "ui_thresholdscan.h"

#include "mpx3gui.h"
#include "ui_mpx3gui.h"

#include "SpidrController.h"

#include <QElapsedTimer>
#include "SpidrDaq.h"

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
//    QElapsedTimer timer;
//    timer.start();

    //! Do the number of frames loop within the threshold scan

    //! TODO Take Double counter behaviour into account

    _running = true;
    ui->button_startStop->setText(tr("Stop"));
    enableSpinBoxes();


    startDataTakingThread();

//    //! Print footer to log box and end timer with relevant units
//    auto stopTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
//    //ui->textEdit_log->append("Finished Threshold scan @ " + stopTime);
//    qDebug() << "Finished Threshold scan @ " + stopTime;

//    auto elapsed = timer.elapsed();
//    if (elapsed < 1000){
//        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()) + "ms");
//    } else {
//        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()/1000) + "s");
//    }

}

void thresholdScan::stopScan()
{
    enableSpinBoxes();

    //! Print interrupt footer
    ui->textEdit_log->append("GUI Interrupt: Stopping Threshold scan at: " +
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

ThresholdScanThread::ThresholdScanThread(Mpx3GUI * mpx3gui, thresholdScan * thresholdScanA) {
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

    //! Work around
    //! If we attempt a connection while the system is already sending data
    //! (this may happen if for instance the program died for whatever reason,
    //!  or when it is close while a very long data taking has been launched and
    //! the system failed to stop the data taking).  If this happens we ought
    //! to stop data taking, and give the system a bit of delay.
    spidrcontrol->stopAutoTrigger();
    Sleep( 100 );

    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    // Send the existing equalization
    //_mpx3gui->getEqualization()->SetAllAdjustmentBits(spidrcontrol);

    connect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
    //connect( this, SIGNAL( addData(int, int, double) ), _thresholdScan, SLOT( addData(int, int, double) ) );
    //connect( this, SIGNAL( addData(int) ), _thresholdScan, SLOT( addData(int) ) );

    int minScan = _ui->spinBox_minimum->value();
    int maxScan = _ui->spinBox_maximum->value();
    int stepScan = _ui->spinBox_spacing->value();
    int dacCodeToScan = MPX3RX_DAC_THRESH_0;
    //int deviceIndex = 0; //Assume quad, so this will be [0-3]
    int nReps = 0;
    bool doReadFrames = true;
    int counter = minScan;

    for ( ; scanContinue ; ) {

        // Set DACs on all chips
        for(int i = 0 ; i < _mpx3gui->getConfig()->getNActiveDevices() ; i++) {
            if ( !_thresholdScan->GetMpx3GUI()->getConfig()->detectorResponds( i ) ) {
                qDebug() << "[ERR ] Device " << i << " not responding.";
            } else {
                qDebug() << "[INFO] Set DACs on :" << i << dacCodeToScan << counter;
                spidrcontrol->setDac( i, dacCodeToScan, counter );
            }
        }

        // Start the trigger as configured
        spidrcontrol->startAutoTrigger();

        doReadFrames = true;
        nReps = 0;

        // Timeout
        int timeOutTime =
                _mpx3gui->getConfig()->getTriggerLength_ms()
                +  _mpx3gui->getConfig()->getTriggerDowntime_ms()
                + 500; // ms
        // TODO ! The extra ms is a combination of delay in the network plus
        // system overhead.  This should be predicted and not hard-coded. TODO !

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
                    qDebug() << "nReps = " << nReps;
                    //_ui->textEdit_log->append("nReps: "+ QString::number((nReps)));
                }
                nReps++;
            }

            // Release
            spidrdaq->releaseFrame();

            // Report to heatmap
            if (doReadFrames){
                UpdateHeatMapSignal(_mpx3gui->getDataset()->x(), _mpx3gui->getDataset()->y());
                qDebug() << "TH:" + QString::number(counter);
            }

        }

        //qDebug() << "nReps = " << nReps;

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
    //disconnect( this, SIGNAL( addData(int, int, double) ), _thresholdScan, SLOT( addData(int, int, double) ) );
    //disconnect( this, SIGNAL( addData(int) ), _thresholdScan, SLOT( addData(int) ) );

    delete spidrcontrol;
}
