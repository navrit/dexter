#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"
#include "qcstmequalization.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "DataTakingThread.h"
#include "dataconsumerthread.h"

#include "qcstmcorrectionsdialog.h"
#include "statsdialog.h"
#include "profiledialog.h"

#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"

#include "thresholdscan.h"
#include "ui_thresholdscan.h"
#include "ui_mpx3gui.h"

#include <stdio.h>
#include <QDialog>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include "mpx3dacsdescr.h"


static QCstmGLVisualization* qCstmGLVisualizationInst;

QCstmGLVisualization::QCstmGLVisualization(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmGLVisualization)
{



    ui->setupUi(this);
    _dataTakingThread = nullptr;
    _dataConsumerThread = nullptr;

    FreeBusyState();
    _takingData = false;

    _busyDrawing = false;
    _etatimer = nullptr;
    _timer = nullptr;
    _estimatedETA = 0;

    // Defaults from GUI
    ui->dropFramesCheckBox->setChecked( true );
    ui->summingCheckbox->setChecked( true );
    _dropFrames = true;

    // Range selection on histogram. Init values
    rewindHistoLimits();

    // Log Scale for histogram
    connect(ui->logCheckBox, SIGNAL(clicked(bool)), this, SLOT(on_logscale(bool)));

    _extraWidgets.devicesNamesLabel = nullptr;
    _extraWidgets.correctionsDialogueButton = nullptr;

    // Stats string
    clearStatsString();

    // Initial stretch of the splitter.  Give more space to visualization Matrix
    ui->splitter->setStretchFactor(0, 3);
    ui->splitter->setStretchFactor(1, 1);

    developerMode(false);
    qCstmGLVisualizationInst = this;
   // connect(this,SIGNAL(infDataTakingToggeled(bool)),this->ui->infDataTakingCheckBox,SLOT(setChecked(bool)));
    //_initializeThresholdsVector();
    QTimer::singleShot(0, this, SLOT(_initializeThresholdsVector()));

}

QCstmGLVisualization::~QCstmGLVisualization() {
    delete ui;
}

QCstmGLVisualization *QCstmGLVisualization::getInstance()
{
    return qCstmGLVisualizationInst;
}

void QCstmGLVisualization::setThresholdsVector(int chipId, int idx, int value)
{
    if(chipId >=0 && chipId < NUMBER_OF_CHIPS && idx >=0 && idx < 8)
        _thresholdsVector[chipId][idx] = value;
    //_loadFromThresholdsVector();
}



int QCstmGLVisualization::getThresholdVector(int chipId, int idx)
{
    if(chipId >=0 && chipId < NUMBER_OF_CHIPS && idx >=0 && idx < 8)
        return _thresholdsVector[chipId][idx];
    return -1;
}

void QCstmGLVisualization::clearThresholdsVector()
{
    for (int chip = 0; chip < NUMBER_OF_CHIPS; ++chip) {
        for (int idx = 0; idx < 8; ++idx) {
            _thresholdsVector[chip][idx] = 0;
        }
    }
}

void QCstmGLVisualization::timerEvent(QTimerEvent *)
{
    refreshScoringInfo();

    drawFrameImage();
}

void QCstmGLVisualization::refreshScoringInfo()
{

    updateETA();

    // Progress
    QString prog = "";

    int nTriggers = _mpx3gui->getConfig()->getNTriggers();
    uint nFramesKept = _score.nFramesKept;
    /* Not necessary to do this, nFramesKept is not modified in DataTakingThread for colour mode
     * if ( _mpx3gui->getConfig()->getColourMode() ) {
     *    nFramesKept /= 4;
    }*/

    if ( nTriggers > 0 ) {
        prog = QString("%1").arg( nFramesKept );
        if ( _score.lostFrames != 0 ) prog += QString("<font color=\"red\">(%1)</font>").arg( _score.lostFrames );
        prog += QString("/%1").arg( nTriggers );
    } else {
        prog = QString("%1").arg( _score.nFramesKept ); // nTriggers=0 is keep taking data forever
        if ( _score.lostFrames != 0 ) prog += QString("<font color=\"red\">(%1)</font>").arg( _score.lostFrames );
    }
    ui->frameCntr->setText( prog );

    // FPS
    fps_update(int(_score.nFramesReceived));
    BuildStatsStringLostFrames( _score.lostFrames );

    //
    BuildStatsStringLostPackets( _score.lostPackets );

    //
    BuildStatsString();

    // Network consumption?
}

void QCstmGLVisualization::drawFrameImage()
{
    // On data cleared the active threshold is -1
    // Force it here to be at least the first threshold
    //int actTHL = getActiveThreshold();
    //if ( actTHL < 0 ) actTHL = 0;
    //reload_layer( actTHL );

    reload_all_layers();

}

void QCstmGLVisualization::rewindScoring()
{
    _score.nFramesReceived = 0;
    _score.nFramesKept = 0;
    _score.lostFrames = 0;
    _score.lostPackets = 0;
    _score.framesCount = 0;
    _score.mpx3clock_stops = 0;
    _score.dataMisaligned = false;

}

void QCstmGLVisualization::rewindHistoLimits() {
    _manualRange = QCPRange( 0, 0 ); // No image loaded yet.
    _manualRangeSave = _manualRange;
    _manualRangePicked = false;
    _percentileRange = QCPRange( 0.025, 0.975 ); // These instead are reasonable percentile cuts
    _percentileRangeNatural =  QCPRange( 0, 0 );
    setRangeSpinBoxesPercentile();
}

void QCstmGLVisualization::SetBusyState() {

    _busyDrawing = true;
    emit busy_drawing();

}

void QCstmGLVisualization::FreeBusyState() {

    _busyDrawing = false;
    emit free_to_draw();

}

bool QCstmGLVisualization::isBusyDrawing() {
    return  _busyDrawing;
}

void QCstmGLVisualization::UnlockWaitingForFrame() {
    qDebug() << "QCstmGLVisualization::UnlockWaitingForFrame ...";
}

void QCstmGLVisualization::updateETA() {

    // Recalculate ETA first
    //_estimatedETA = _mpx3gui->getConfig()->getTriggerPeriodMS() *  _mpx3gui->getConfig()->getNTriggers(); // ETA in ms
    //_estimatedETA += _estimatedETA * __networkOverhead; // add ~10% network overhead.  FIXME  to be calculated at startup

    if ( _etatimer && ! _infDataTaking ) {
        // show eta in display
        // h must be in the range 0 to 23, m and s must be in the range 0 to 59, and ms must be in the range 0 to 999.
        QTime n(0, 0, 0);                // n == 00:00:00
        QTime t(0, 0, 0);
        int diff = _estimatedETA - _etatimer->elapsed();
        if (diff > 0) t = n.addMSecs( _estimatedETA - _etatimer->elapsed() );
        QString textT = t.toString("hh:mm:ss");
        ui->etaCntr->setText( textT );
    } else {
        ui->etaCntr->setText( "--:--:--" );
    }

}

void QCstmGLVisualization::FinishDataTakingThread() {

    if ( _dataTakingThread ) {
        delete _dataTakingThread;
        _dataTakingThread = nullptr;
    }

    if( _dataConsumerThread ) {
        delete _dataConsumerThread;
        _dataConsumerThread = nullptr;
    }

}

void QCstmGLVisualization::StopDataTakingThread()
{
    if ( _dataTakingThread ) _dataTakingThread->stop();
}

bool QCstmGLVisualization::DataTakingThreadIsRunning()
{
    if ( _dataTakingThread ) return _dataTakingThread->isRunning();
    return false;
}

// Not idling means that is actively taking data or that there is
//  no DataTakingThread at all. This has to be used in combination
//  with DataTakingThreadIsRunning
bool QCstmGLVisualization::DataTakingThreadIsIdling()
{
    if ( _dataTakingThread ) return _dataTakingThread->isIdling();
    return false;
}

void QCstmGLVisualization::ConfigureGUIForDataTaking() {

    _mpx3gui->getDataset()->setPixelDepthBits(_mpx3gui->getConfig()->getPixelDepth());

    emit taking_data_gui();

    if (_mpx3gui->getConfig()->getNTriggers() == 1) {
        ui->singleshotPushButton->setVisible(true);
        ui->singleshotPushButton->setText("Stop");
        ui->startButton->setVisible(false);
    } else {
        ui->startButton->setVisible(true);
        ui->startButton->setText("Stop");
        ui->singleshotPushButton->setVisible(false);
    }

    emit sig_statusBarAppend("Start","blue");

    // Config stats
    ui->groupBoxConfigAndStats->setEnabled( false );
    ui->statsLabel->setEnabled( true ); // keep the stats label alive

    // Don't let the user change DACs while taking data
    _mpx3gui->getDACs()->setEnabled( false );
}

void QCstmGLVisualization::ConfigureGUIForIdling() {

    emit idling_gui();

    ui->startButton->setText("Start");
    ui->singleshotPushButton->setText("Single");

    ui->startButton->setVisible(true);
    ui->singleshotPushButton->setVisible(true);

    emit sig_statusBarAppend("Done","blue");

    // Config stats
    ui->groupBoxConfigAndStats->setEnabled( true );

    _mpx3gui->getDACs()->setEnabled( true );
}

void QCstmGLVisualization::CalcETA() {

    if ( _mpx3gui->getConfig()->getOperationMode() == Mpx3Config::__operationMode_SequentialRW ) {
        _estimatedETA = _mpx3gui->getConfig()->getTriggerPeriodMS() *  _mpx3gui->getConfig()->getNTriggers(); // ETA in ms.
        _estimatedETA += _estimatedETA * __networkOverhead; // add ~10% network overhead.
    } else {
        double period_ms =  (1. / (double)(_mpx3gui->getConfig()->getContRWFreq())) * 1000;
        _estimatedETA = period_ms * _mpx3gui->getConfig()->getNTriggers(); // ETA in ms.
        _estimatedETA += _estimatedETA * __networkOverhead; // add ~10% network overhead.
    }

}

//! Used in Mpx3GUI::save_data to send the save file path from the UI to save_data.
//! This is done after
QString QCstmGLVisualization::getsaveLineEdit_Text() {
    return ui->saveLineEdit->text();
}

QString QCstmGLVisualization::getStatsString_deviceId()
{
    return _statsString.devicesIdString;
}

void QCstmGLVisualization::saveImage(QString filename)
{
    if (filename == "") {
        filename = QDir::homePath();
    }
    _mpx3gui->getVisualization()->GetUI()->saveLineEdit->setText(filename);
    _mpx3gui->save_data(true, 0, "TIFF");
}

void QCstmGLVisualization::saveImage(QString filename, QString corrMethod)
{
    if (corrMethod == "Beam Hardening"){
        //! Only do spatial correction in this mode
        _mpx3gui->getDataset()->toTIFF(filename, true, true);
    }
}

void QCstmGLVisualization::StartDataTaking(QString mode) {



    if (mode == "CT") {
        runningCT = true;
    } else if (mode == "THScan") {
        runningTHScan = true;
    }
    else
    {
        runningCT = false;
        runningTHScan = false;
    }

    if ( !_dataTakingThread ) {

        _dataConsumerThread = new DataConsumerThread(_mpx3gui, this);
        _dataConsumerThread->setObjectName("Consumer thread");

        _dataTakingThread = new DataTakingThread(_mpx3gui, _dataConsumerThread, this);


        _dataTakingThread->setObjectName("Producer thread");
        _dataTakingThread->ConnectToHardware();

    }
    if(_mpx3gui->getConfig()->getTriggerMode() == SHUTTERMODE_AUTO){ //if is Auto, it is internal otherwise is external
        _dataTakingThread->setExternalTrigger(false);
    //todo : disable/enable  ui->triggerLengthSpinBox->setReadOnly(false);
    }
    else
    {
        _dataTakingThread->setExternalTrigger(true);

    }

    if ( ! _takingData ) { // new data
         //_loadFromThresholdsVector();
        //qDebug() << "start taking data .. ";
        _takingData = true;
        if(!runningTHScan)
            emit busy(SB_DATA_TAKING);

        // ETA
        CalcETA();

        // Producer and Consumer threads
        _dataTakingThread->setFramesRequested( _mpx3gui->getConfig()->getNTriggers() );
        _dataTakingThread->takedata();
        // The data taking thread will awake the consumer
        //_dataConsumerThread->consume();

        // Timers
        ArmAndStartTimer();

        // GUI
        ConfigureGUIForDataTaking();

        // info refresh
        _timerId = this->startTimer( 100 ); // 42 (answer to life) // 100 ms is a good compromise to refresh the scoring info
        //qDebug() << "Start : " << _timerId;

    } else { // stop

        // By premature user signal !
        _dataTakingThread->stop(); // this calls by SIGNAL/SLOT the data_taking_finished
    }

    //! TODO Why is this all commented out?
    /*
    // The Start button becomes the Stop button
    if ( ! _takingData ) {

        // In the situation where data has been lost and
        //  the system is trying to complete the requested
        //  number of frames, the data shouldn't be cleared.
        bool clearpreviousdata = true;
        if ( _dataTakingThread ) {
            if ( _dataTakingThread->isACompleteJob() ) {
                qDebug() << "[INFO] continue with "
                         << _dataTakingThread->getMissingFramesToCompleteJob()
                         << "missing frames";
                clearpreviousdata = false;
            }
        }
        if ( _infDataTaking ) clearpreviousdata = false;

        if ( clearpreviousdata ) {
            _mpx3gui->clear_data( false );
            rewindHistoLimits();
        }

        // Threads
        if ( _dataTakingThread ) {
            if ( _dataTakingThread->isRunning() ) {
                return;
            }
            //disconnect(_senseThread, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );

            // If this is a complete job, keep the same thread
            if ( ! _dataTakingThread->isACompleteJob() ) {
                delete _dataTakingThread;
                _dataTakingThread = nullptr;
            }

        }

        // Create the thread
        if ( ! _dataTakingThread ) {
            _dataTakingThread = new DataTakingThread(_mpx3gui, this);
            _dataTakingThread->ConnectToHardware();
        }

        // Change the Start button to Stop
        ui->startButton->setText( "Stop" );
        ui->singleshotPushButton->setText( "Stop" );

        // Start data taking
        // FIXME, depends on the mode !
        _estimatedETA = _mpx3gui->getConfig()->getTriggerPeriodMS() *  _mpx3gui->getConfig()->getNTriggers(); // ETA in ms
        _estimatedETA += _estimatedETA * __networkOverhead; // add ~10% network overhead.  FIXME  to be calculated at startup

        // Free to draw now
        FreeBusyState();
        _dataTakingThread->setFramesRequested (
                    _mpx3gui->getConfig()->getNTriggers()
                    );

        _takingData = true;
        _dataTakingThread->start();

        // Start the timer to display eta
        ArmAndStartTimer();

        emit sig_statusBarAppend("start","blue");

    } else {

        // Attempt to stop the thread
        if ( _dataTakingThread ) emit stop_data_taking_thread();

        // The thread will try to finish itself.
        // Now let's block this thread until that
        // happens. It will be fast
        _dataTakingThread->wait();

        // Change the Stop button to Start
        ui->startButton->setText( "Start" );
        ui->singleshotPushButton->setText( "single" );

        _takingData = false;

        // force the GUI to update
        update();

        // Finish
        DestroyTimer();
        ETAToZero();

        emit sig_statusBarAppend("stop","orange");
    }

    //Set corrected status of the newly taken data to false.
    _mpx3gui->getDataset()->setCorrected(false);
    */


}

void QCstmGLVisualization::StartDataTakingRemotely(bool flag)
{
    _takingData = flag;
    StartDataTaking();
}

void QCstmGLVisualization::ETAToZero() {

    QString textT = QTime(0,0,0).toString("hh:mm:ss");
    ui->etaCntr->setText( textT );

}

void QCstmGLVisualization::setRangeSpinBoxesManual()
{

    // Geometry
    // I will force the geometry so it looks
    //  the same in both Manual and Percentile.
    ui->lowerSpin->setFixedWidth( 80 );
    ui->upperSpin->setFixedWidth( 80 );

    // Values
    ui->lowerSpin->setMinimum( -4E16 );
    ui->lowerSpin->setMaximum(  4E16 );
    ui->lowerSpin->setSingleStep( 1 );
    ui->lowerSpin->setDecimals( 0 );
    ui->lowerSpin->setValue( _manualRange.lower );

    ui->upperSpin->setMinimum( -4E16 );
    ui->upperSpin->setMaximum(  4E16 );
    ui->upperSpin->setSingleStep( 1 );
    ui->upperSpin->setDecimals( 0 );
    ui->upperSpin->setValue( _manualRange.upper );

}

void QCstmGLVisualization::setRangeSpinBoxesPercentile()
{

    // Geometry
    ui->lowerSpin->setFixedWidth( 80 );
    ui->upperSpin->setFixedWidth( 80 );

    // Values
    ui->lowerSpin->setMinimum( 0 );
    ui->lowerSpin->setMaximum( 1 );
    ui->lowerSpin->setSingleStep( 0.01 );
    ui->lowerSpin->setDecimals( 3 );
    ui->lowerSpin->setValue( _percentileRange.lower );

    ui->upperSpin->setMinimum( 0 );
    ui->upperSpin->setMaximum( 1 );
    ui->upperSpin->setSingleStep( 0.01 );
    ui->upperSpin->setDecimals( 3 );
    ui->upperSpin->setValue( _percentileRange.upper );

}

void QCstmGLVisualization::clearStatsString()
{

    _statsString.counts.clear();
    _statsString.lostPackets.clear();
    _statsString.lostFrames.clear();
    _statsString.mpx3ClockStops.clear();
    _statsString.overflow.clear();
    _statsString.overflowFlg = false;

    _statsString.displayString.clear();

}

void QCstmGLVisualization::initStatsString()
{
    // when offline or upon startup

    _statsString.displayString = "Offline";
}

void QCstmGLVisualization::data_taking_finished(int /*nFramesTaken*/) { //when acquisition is done,

    // Recover from single shot if it was requested
    if ( _singleShot ) {
        _singleShot = false;
        _mpx3gui->getConfig()->setNTriggers( _singleShotSaveCurrentNTriggers );
    }

    reload_all_layers(); //! Only update the 0th threshold for speed?
//    _mpx3gui->saveOriginalDataset();

    refreshScoringInfo();
    DestroyTimer();
    ETAToZero();

    ConfigureGUIForIdling();

    rewindScoring();

    //! If Save checkbox is checked and Save line edit is not empty,
    //!    Save the data to .bin file with path obtained from UI
    if(
            ui->saveCheckBox->isChecked()
            &&
            !(ui->saveLineEdit->text().isEmpty())
            &&
            !isSaveAllFramesChecked() // if it's checked the last was already saved
            ) {
        QString selectedFileType = ui->saveFileComboBox->currentText();
        _mpx3gui->save_data(true, 0, selectedFileType);
    }

    _takingData = false;
    if(!runningTHScan)
        emit busy(FREE);

    if (runningCT) {
        emit sig_resumeCT();

    }
    if (runningTHScan) {
        emit sig_resumeTHScan();

    }

    // inform the consumer that the data taking is finished
    if ( _dataConsumerThread->isRunning() ) {
        _dataConsumerThread->dataTakingSaysIFinished();
    }

    emit someCommandHasFinished_Successfully();

    /*
    if ( _takingData ) {

        _takingData = false;
        DestroyTimer();
        ETAToZero();

        _dataTakingThread->rewindScoring();

        // When finished taking data save the original data
        //_mpx3gui->saveOriginalDataset();

        // And replot, this also attempts to apply selected corrections
        //reload_all_layers( true );

        // Change the Stop button to Start
        ui->startButton->setText( "Start" );
        ui->singleshotPushButton->setText( "single" );

        // At this point I need to decide if the data taking is really finished.
        // If the user is requesting that all frames are needed we look at
        // Inf data taking takes priority here

        if ( _infDataTaking ) {

            _dataTakingThread->rewindScoring();
            StartDataTaking();

        } else {

            if ( ui->completeFramesCheckBox->isChecked() ) {

                int missing = _dataTakingThread->calcScoreDifference();

                if ( missing != 0 ) {
                    //qDebug() << "[INFO] missing : " << missing;
                    StartDataTaking();
                } else {
                    _dataTakingThread->rewindScoring();
                }

            }
        }


        // If single shot, recover previous NTriggers
        if ( _singleShot ) {
            ui->nTriggersSpinBox->setValue( _singleShotSaveCurrentNTriggers );
            _singleShot = false;
            _singleShotSaveCurrentNTriggers = 0;
        }

        emit sig_statusBarAppend("done","blue");

        /////////////////////////////
        // TEST
        //_mpx3gui->getDataset()->dumpAllActivePixels();

    }
*/
    // Also we will inform the visualization to go straight to the very last frame to be drawn
    //  in case the data taking thread was too fast compared to drawing

}

void QCstmGLVisualization::ArmAndStartTimer(){

    // See for previous instances
    if( _timer ) delete _timer;
    if( _etatimer ) delete _etatimer;

    _etatimer = new QElapsedTimer;
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(updateETA()));
    _timer->start( __display_eta_granularity );
    // and start the elapsed timer
    _etatimer->start();

}

void QCstmGLVisualization::DestroyTimer() {

    // refresh scoring timer (local)
    this->killTimer( _timerId );

    // timer
    disconnect(_timer, SIGNAL(timeout()), this, SLOT(updateETA()));
    if( _timer ) delete _timer;
    _timer = nullptr;
    if( _etatimer ) delete _etatimer;
    _etatimer = nullptr;

}

void QCstmGLVisualization::SeparateThresholds(int * data, int /*size*/, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int /*sizeReduced*/) {

    // Layout of 110um pixel
    //  -------------
    //  | P3  |  P1 |
    //	-------------
    //  | P4  |  P2 |
    //  -------------
    //  Where:
    //  	P1 --> TH0, TH1
    //		P2 --> TH2, TH3
    //		P3 --> TH4, TH5
    //		P4 --> TH6, TH7

    int indx = 0, indxRed = 0, redi = 0, redj = 0;

    for (int j = 0 ; j < __matrix_size_y ; j++) {

        redi = 0;
        for (int i = 0 ; i < __matrix_size_x  ; i++) {

            indx = XYtoX( i, j, __matrix_size_x);
            indxRed = XYtoX( redi, redj, __matrix_size_x / 2); // This index should go up to 128*128

            //if(indxRed > 16380 ) cout << "indx " << indx << ", indxRed = " << indxRed << endl;

            if( i % 2 == 0 && j % 2 == 0) {
                (*th6)[indxRed] = data[indx]; // P4
            }
            if( i % 2 == 0 && j % 2 == 1) {
                (*th4)[indxRed] = data[indx]; // P3
            }
            if( i % 2 == 1 && j % 2 == 0) {
                (*th2)[indxRed] = data[indx]; // P2
            }
            if( i % 2 == 1 && j % 2 == 1) {
                (*th0)[indxRed] = data[indx]; // P1
            }

            if (i % 2 == 1) redi++;

        }

        if (j % 2 == 1) redj++;

    }

}

pair<int, int> QCstmGLVisualization::XtoXY(int X, int dimX){
    return make_pair(X % dimX, X/dimX);
}

void QCstmGLVisualization::ConnectionStatusChanged(bool connecting) {

    if ( connecting ) {

        ui->startButton->setEnabled( true ); // Enable or disable the button depending on the connection status.
        ui->singleshotPushButton->setEnabled( true );

        // Report the chip ID's
        // Make space in the dataTakingGridLayout
        QVector<int> devs = _mpx3gui->getConfig()->getActiveDevices();
        QVector<int>::const_iterator i  = devs.begin();
        QVector<int>::const_iterator iE = devs.end();
        _statsString.devicesIdString.clear();

        for ( ; i != iE ; i++ ) {
            int indx = _mpx3gui->getConfig()->getIndexFromID( *i );
            QString devID = QString("[") + QString::number(*i) + QString("] ");
            _statsString.devicesIdString.append( devID +_mpx3gui->getConfig()->getDeviceWaferId( indx ) );
            if ( (i+1) != iE ) _statsString.devicesIdString.append( " | " );
        }

        if ( _extraWidgets.devicesNamesLabel == nullptr ) {
            _extraWidgets.devicesNamesLabel = new QLabel(this);
            _extraWidgets.devicesNamesLabel->setAlignment( Qt::AlignRight );
        }

        _extraWidgets.devicesNamesLabel->setText( _statsString.devicesIdString );
        _extraWidgets.devicesNamesLabel->setToolTip(tr("Device ID in [] followed by chip ID code"));
        int colCount = ui->dataTakingGridLayout->columnCount();
        ui->dataTakingGridLayout->addWidget( _extraWidgets.devicesNamesLabel, 1, 0, 1, colCount );

        sig_statusBarAppend(_statsString.devicesIdString,"green");

    } else {
        FinishDataTakingThread();

        ui->startButton->setEnabled( false );
        ui->singleshotPushButton->setEnabled( false );
    }

}

void QCstmGLVisualization::setGradient(int index){
    ui->glPlot->setGradient(_mpx3gui->getGradient(index));
}

void QCstmGLVisualization::SetMpx3GUI(Mpx3GUI *p){

    _mpx3gui = p;
    setGradient(0);
    changeBinCount(ui->binCountSpinner->value());

    connect(_mpx3gui, SIGNAL(sizeChanged(int, int)), ui->glPlot, SLOT(setSize(int, int)));
    connect(ui->startButton, SIGNAL(clicked(bool)), this, SLOT(StartDataTaking()));
    connect(this, SIGNAL(mode_changed(bool)), _mpx3gui, SLOT(set_summing(bool)));
    connect(_mpx3gui, SIGNAL(summing_set(bool)), ui->summingCheckbox, SLOT(setChecked(bool)));
    connect(ui->gradientSelector, SIGNAL(activated(int)), this, SLOT(setGradient(int)));
    connect(ui->generateDataButton, SIGNAL(clicked()), _mpx3gui, SLOT(generateFrame()));
    connect(_mpx3gui, SIGNAL(data_cleared()), this, SLOT(on_clear()));
    connect(_mpx3gui, SIGNAL(data_zeroed()), this, SLOT(on_zero()));
    connect(_mpx3gui, SIGNAL(reload_layer(int)), this, SLOT( reload_layer(int)));
    connect(_mpx3gui, SIGNAL(reload_all_layers()), this, SLOT(reload_all_layers()));
    connect(_mpx3gui, SIGNAL(availible_gradients_changed(QStringList)), this, SLOT(availible_gradients_changed(QStringList)));
    connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), this, SLOT(range_changed(QCPRange)));

    connect(ui->binCountSpinner, SIGNAL(valueChanged(int)), this, SLOT(changeBinCount(int)));
    connect(ui->glPlot->getPlot(), SIGNAL(hovered_pixel_changed(QPoint)),this, SLOT(hover_changed(QPoint)));
    connect(ui->glPlot->getPlot(), SIGNAL(pixel_selected(QPoint,QPoint)), this, SLOT(pixel_selected(QPoint,QPoint)));
    connect(ui->glPlot->getPlot(), SIGNAL(region_selected(QPoint,QPoint,QPoint)), this, SLOT(region_selected(QPoint,QPoint,QPoint)));

    connect(this, SIGNAL(change_hover_text(QString)), ui->mouseOverLabel, SLOT(setText(QString)));
    connect(ui->histPlot, SIGNAL(new_range_dragged(QCPRange)), this, SLOT(new_range_dragged(QCPRange)));

    connect( this, &QCstmGLVisualization::sig_statusBarAppend, _mpx3gui, &Mpx3GUI::statusBarAppend );
    connect( this, &QCstmGLVisualization::sig_statusBarWrite, _mpx3gui, &Mpx3GUI::statusBarWrite );
    connect( this, &QCstmGLVisualization::sig_statusBarClean, _mpx3gui, &Mpx3GUI::statusBarClean );

    // Connection to configuration
    connect( ui->nTriggersSpinBox, SIGNAL(valueChanged(int)),
             this, SLOT(ntriggers_edit()) );

    connect( ui->triggerLengthSpinBox, SIGNAL(valueChanged(int)),
             this, SLOT(triggerLength_edit()) );

    // This one need both the connection to the mirror combo box and the signal to the Configuration to take place
    connect( ui->operationModeComboBox_Vis, SIGNAL(activated(int)),
             _mpx3gui->getConfigMonitoring()->getUI()->operationModeComboBox,
             SLOT( setCurrentIndex(int) ) );
    connect( ui->operationModeComboBox_Vis, SIGNAL(activated(int)),
             _mpx3gui->getConfig(),
             SLOT( setOperationMode(int) ) );

    // What to do over a switch of Operation Mode
    connect( ui->operationModeComboBox_Vis, SIGNAL(activated(int)),
             this,
             SLOT( OperationModeSwitched(int) ));
    connect( ui->operationModeComboBox_Vis, SIGNAL(activated(int)),
             _mpx3gui->getConfigMonitoring(),
             SLOT( OperationModeSwitched(int) ));


    // Double click to reset view
    // Linked to on_resetViewPushButton_clicked()
    connect(ui->glPlot->getPlot(), &QCstmGLPlot::double_click,
            this, &QCstmGLVisualization::reload_all_layers);
    connect(ui->resetViewPushButton, SIGNAL(pressed()),
            this, SLOT(on_resetViewPushButton_clicked()));

    // DataTaking / idling actions
    connect(this, SIGNAL(taking_data_gui()), _mpx3gui->getConfigMonitoring(), SLOT(when_taking_data_gui()));
    connect(this, SIGNAL(idling_gui()), _mpx3gui->getConfigMonitoring(), SLOT(when_idling_gui()));


    // Defaults
    emit mode_changed( ui->summingCheckbox->isChecked() );

    // CT stuff
    connect( this, SIGNAL(sig_resumeCT()), _mpx3gui->getCT(), SLOT(resumeCT()));

    // TH Scan
    connect( this, SIGNAL(sig_resumeTHScan()), _mpx3gui->getTHScan(), SLOT(resumeTHScan()));

    // Getting equalization path
    connect(_mpx3gui->getEqualization(),SIGNAL(equalizationPathExported(QString)),this,SLOT(onEqualizationPathExported(QString)));
}

void QCstmGLVisualization::ntriggers_edit() {

    // Modify the spinner on the config side
    _mpx3gui->getConfigMonitoring()->getUI()->nTriggersSpinner->setValue(
                ui->nTriggersSpinBox->value()
                );
    if(ui->nTriggersSpinBox->value() == 0)
        _mpx3gui->getTHScan()->GetUI()->spinBox_framesPerStep->setValue(1);
    else
        _mpx3gui->getTHScan()->GetUI()->spinBox_framesPerStep->setValue(
                    ui->nTriggersSpinBox->value()
                    );

    // And try to send the new config
    _mpx3gui->getConfig()->setNTriggers(
                ui->nTriggersSpinBox->value()
                );

}

void QCstmGLVisualization::triggerLength_edit() {

    // Depends on the mode, the same box is used for
    // triggerLength --> SequentialRW
    // Freq          --> ContinuousRW

    if ( _mpx3gui->getConfig()->getOperationMode()
         == Mpx3Config::__operationMode_SequentialRW ) {
        // triggerLength
        _mpx3gui->getConfigMonitoring()->getUI()->triggerLengthSpinner->setValue(
                    ui->triggerLengthSpinBox->value()
                    );
        // Send the new config
        _mpx3gui->getConfig()->setTriggerLength(
                    ui->triggerLengthSpinBox->value()
                    );

    } else if ( _mpx3gui->getConfig()->getOperationMode()
                == Mpx3Config::__operationMode_ContinuousRW )  {
        // contRWFreq
        _mpx3gui->getConfigMonitoring()->getUI()->contRWFreq->setValue(
                    ui->triggerLengthSpinBox->value()
                    );
        // send the new config --> contRWFreq
        _mpx3gui->getConfig()->setContRWFreq(
                    ui->triggerLengthSpinBox->value()
                    );
    }

}

void QCstmGLVisualization::startupActions()
{
    _mpx3gui->open_data_with_path(true, true, "://icons/startupimage.bin" );
}

void QCstmGLVisualization::changeBinCount(int count) {
    ui->histPlot->changeBinCount(count);
    QList<int> thresholds = _mpx3gui->getDataset()->getThresholds();
    for ( int i = 0; i < thresholds.size(); i++ ) {
        addThresholdToSelector(thresholds[i]);
        ui->histPlot->setHistogram(thresholds[i], _mpx3gui->getDataset()->getLayer(thresholds[i]), _mpx3gui->getDataset()->getPixelsPerLayer());
    }
}

void QCstmGLVisualization::fps_update(int nframes_done) {

    // check if there is a datataking thread is running
    if ( ! _dataTakingThread->isRunning() ) return;
    if (! _etatimer) return;

    double fpsVal = ((double)nframes_done) / ((double)_etatimer->elapsed() / 1000.); // elapsed() comes in milliseconds

    QString fpsS = QString::number( round( fpsVal ) , 'd', 0 );
    fpsS += " fps";

    ui->fpsLabel->setText( fpsS );

}

void QCstmGLVisualization::overflow_update(int ovf_cntr) {

    if ( ovf_cntr > 0 ) {

        BuildStatsStringOverflow( true );

        // Bring the user to full range so the hot spots can be seen
        //ui->fullRangeRadio->setChecked( true );
        //on_fullRangeRadio_toggled( true );

    } else {

        BuildStatsStringOverflow( false );

    }

}

void QCstmGLVisualization::BuildStatsString() {

    // Build the string to show

    //! Renable this if it turns out that the calculating the number of active pixels can be made faster
    //! See "BuildStatsStringCounts( _mpx3gui->getDataset()->getActivePixels(getActiveThreshold()) );"
    // _statsString.displayString  = "fired: ";
    //_statsString.displayString += _statsString.counts;

    _statsString.displayString  = "Online";

    if ( ! _statsString.lostFrames.isEmpty() ) {
        _statsString.displayString += " | lf: ";
        _statsString.displayString += _statsString.lostFrames;
    }

    if ( ! _statsString.lostPackets.isEmpty() ) {
        _statsString.displayString += " | lp: ";
        _statsString.displayString += _statsString.lostPackets;
    }

    if ( ! _statsString.mpx3ClockStops.isEmpty() ) {
        _statsString.displayString += " | clk: ";
        _statsString.displayString += _statsString.mpx3ClockStops;
    }

    if ( _statsString.overflowFlg ) {
        _statsString.displayString += " | ";
        _statsString.displayString += _statsString.overflow;
    }

    if ( _mpx3gui->getDataset()->isDataMisaligned() ) {
        _statsString.displayString += " | MISALIGNED !";
    }


    // Use the string to set the label
    ui->statsLabel->setText( _statsString.displayString );

}

void QCstmGLVisualization::BuildStatsStringCounts(uint64_t counts)
{

    QString plS = "<font color=\"black\">";
    if ( _mpx3gui->getDataset()->isDataMisaligned() ) plS = "<font color=\"red\">";
    plS += QString::number( counts, 'd', 0 );
    plS += "</font>";

    _statsString.counts = plS;

    BuildStatsString();
}

void QCstmGLVisualization::BuildStatsStringLostPackets(uint64_t lostPackets)
{

    if ( lostPackets == 0 ) {
        _statsString.lostPackets.clear();
        return;
    }

    // Retrieve the counter and display
    QString plS = "<font color=\"black\">";
    if ( _mpx3gui->getDataset()->isDataMisaligned() ) plS = "<font color=\"red\">";
    plS += QString::number( lostPackets, 'd', 0 );
    plS += "</font>";

    _statsString.lostPackets = plS;

}

void QCstmGLVisualization::BuildStatsStringLostFrames(uint64_t lostFrames)
{

    if ( lostFrames == 0 ) {
        _statsString.lostFrames.clear();
        return;
    }

    // Retrieve the counter and display
    QString plS = "<font color=\"black\">";
    if ( _mpx3gui->getDataset()->isDataMisaligned() ) plS = "<font color=\"red\">";
    plS += QString::number( lostFrames, 'd', 0 );
    plS += "</font>";

    _statsString.lostFrames = plS;

}

void QCstmGLVisualization::BuildStatsStringMpx3ClockStops(uint64_t stops)
{

    if ( stops == 0 ) {
        _statsString.mpx3ClockStops.clear();
        return;
    }

    // Retrieve the counter and display
    QString plS = "<font color=\"black\">";
    if ( _mpx3gui->getDataset()->isDataMisaligned() ) plS = "<font color=\"red\">";
    plS += QString::number( stops, 'd', 0 );
    plS += "</font>";

    _statsString.mpx3ClockStops = plS;

}

void QCstmGLVisualization::BuildStatsStringOverflow(bool overflow)
{

    if ( overflow ) {
        QString ovfS = "<font color=\"red\">";
        ovfS += "overflow</font>";
        _statsString.overflow = ovfS;
        _statsString.overflowFlg = true;
    } else {
        _statsString.overflow = "";
        _statsString.overflowFlg = false;
    }

    BuildStatsString();

}

QString QCstmGLVisualization::getPath(QString msg)
{
    QString path = "";
    if (autosaveFromServer) {
        path = ui-> saveLineEdit->text();
    } else {
        path = QFileDialog::getExistingDirectory(
                    this,
                    msg,
                    QDir::currentPath(),
                    QFileDialog::ShowDirsOnly);
    }

    return path;
}

QPoint QCstmGLVisualization::previewIndexToChipIndex(QPoint previewPixel, int *chipId)
{
    const int COL_SIZE = 512 , ROW_SIZE = 512;
    QPoint chipIndex;
    if(previewPixel.x() >= COL_SIZE/2 && previewPixel.y() >= ROW_SIZE/2){
        *chipId = 0;
        chipIndex.setX(ROW_SIZE - 1 - previewPixel.y());
        chipIndex.setY(COL_SIZE - 1 - previewPixel.x());
    }
    else if(previewPixel.x() >= COL_SIZE/2 && previewPixel.y() < ROW_SIZE/2){
        *chipId = 1;
        chipIndex.setX((ROW_SIZE/2) - 1 - previewPixel.y());
        chipIndex.setY(COL_SIZE - 1 - previewPixel.x());
    }
    else if(previewPixel.x() < COL_SIZE/2 && previewPixel.y() < ROW_SIZE/2){
        *chipId = 2;
        chipIndex.setX(previewPixel.y());
        chipIndex.setY(previewPixel.x());
    }
    else if(previewPixel.x() < COL_SIZE/2 && previewPixel.y() >= ROW_SIZE/2){
        *chipId = 3;
        chipIndex.setX(previewPixel.y() - (ROW_SIZE/2));
        chipIndex.setY(previewPixel.x());
    }
    return chipIndex;
}

void QCstmGLVisualization::developerMode(bool enabled)
{
    if (enabled){
        //! Enable a bunch of 'advanced' buttons
        ui->binCountSpinner->show();
        ui->binWidthLabel->show();
        ui->outOfBoundsCheckbox->show();
        ui->generateDataButton->show();
        ui->dropFramesCheckBox->show();
        ui->bufferOccupancy->show();
        ui->label->show();
        ui->progressBar->show();
        ui->label_2->show();
        ui->completeFramesCheckBox->show();
        ui->testBtn->show();

    } else {
        //! Disable a bunch of 'advanced' buttons
        ui->binCountSpinner->hide();
        ui->binWidthLabel->hide();
        ui->outOfBoundsCheckbox->hide();
        ui->generateDataButton->hide();
        ui->dropFramesCheckBox->hide();
        ui->bufferOccupancy->hide();
        ui->label->hide();
        ui->progressBar->hide();
        ui->label_2->hide();
        ui->completeFramesCheckBox->hide();
        ui->testBtn->hide();
    }
}

void QCstmGLVisualization::user_accepted_profile()
{
    //Delete the corresponding window
    if ( _profiledialog ) {
        delete _profiledialog;
        _profiledialog = nullptr;

        //Re-initialize the Profilepoints list for new CNR calculation in new dialog
        _mpx3gui->getDataset()->clearProfilepoints();
    }

}

void QCstmGLVisualization::OperationModeSwitched(int indx)
{
    // Swith the triggerLengthSpinBox into ContRWFreq if in ContinuousRW mode
    if ( indx == Mpx3Config::__operationMode_SequentialRW ) {

        ui->triggerLengthSpinBoxLabel->setText( "Length (µs)" );
        ui->triggerLengthSpinBox->setValue( _mpx3gui->getConfig()->getTriggerLength() );

        ui->triggerLengthSpinBoxLabel->setToolTip( tr("Trigger length") );
        ui->triggerLengthSpinBox->setToolTip( tr("Trigger length") );

    } else if ( indx == Mpx3Config::__operationMode_ContinuousRW ) {

        ui->triggerLengthSpinBoxLabel->setText( "CRW (Hz)" );
        ui->triggerLengthSpinBox->setValue( _mpx3gui->getConfig()->getContRWFreq() );

        ui->triggerLengthSpinBoxLabel->setToolTip( tr("ContinuousRW Mode. Enter frequency in Hz.") );
        ui->triggerLengthSpinBox->setToolTip( tr("ContinuousRW Mode. Enter frequency in Hz.") );
    }
}

void QCstmGLVisualization::data_misaligned(bool misaligned) {

    // Increase the current packet loss
    _mpx3gui->getDataset()->setDataMisaligned( misaligned );

}

void QCstmGLVisualization::mpx3clock_stops(int stops) {

    // Increase the current packet loss
    _mpx3gui->getDataset()->increaseMpx3ClockStops( stops );

    BuildStatsStringMpx3ClockStops( _mpx3gui->getDataset()->getMpx3ClockStops() );

}

void QCstmGLVisualization::range_changed(QCPRange newRange){
    //ui->lowerManualSpin->setValue(newRange.lower);
    //ui->upperManualSpin->setValue(newRange.upper);
    //ui->lowerSpin->setValue(newRange.lower);
    //ui->upperSpin->setValue(newRange.upper);
    ui->glPlot->set_range(newRange);
}

void QCstmGLVisualization::new_range_dragged(QCPRange newRange){

    _manualRange = newRange;
    range_changed(newRange);

    // force it to go manual if needed
    if(!ui->manualRangeRadio->isChecked()) {
        ui->manualRangeRadio->setChecked(true);
        return;
    }

    setRangeSpinBoxesManual();

}

void QCstmGLVisualization::on_clear(){
    layerNames.clear();
    ui->histPlot->clear();
    ui->layerSelector->clear();
    ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset());
}

void QCstmGLVisualization::on_zero()
{
    ui->histPlot->clear();
    ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset());
}

void QCstmGLVisualization::availible_gradients_changed(QStringList gradients){
    ui->gradientSelector->clear();
    ui->gradientSelector->addItems(gradients);
}

void QCstmGLVisualization::hover_changed(QPoint pixel){
    emit(change_hover_text(QString("%1 @ (%2, %3)").arg(_mpx3gui->getPixelAt(pixel.x(), pixel.y(),getActiveThreshold())).arg(pixel.x()).arg(pixel.y())));
}

void QCstmGLVisualization::reload_layer(int threshold){

    // Get busy
    SetBusyState();

    //_mpx3gui->saveOriginalDataset();
    // Corrections
    //if( _corrdialog ) _mpx3gui->getDataset()->applyCorrections( _corrdialog );

    //int layer = _mpx3gui->getDataset()->thresholdToIndex(threshold);
    ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset()); //TODO: only read specific layer.

    ui->histPlot->setHistogram(threshold,
                               _mpx3gui->getDataset()->getLayer(threshold),
                               _mpx3gui->getDataset()->getPixelsPerLayer(),
                               _manualRange.lower,
                               _manualRange.upper);


    setThreshold(threshold);

    // done
    active_frame_changed();

    // Get free
    FreeBusyState();

}

// All the bits needed to process the progress of the data taking
void QCstmGLVisualization::on_scoring(int nFramesReceived,
                                      int nFramesKept,
                                      int lostFrames,
                                      int lostPackets,
                                      int framesCount,
                                      int mpx3clock_stops,
                                      bool dataMisaligned) {

    _score.nFramesReceived = nFramesReceived;
    _score.nFramesKept = nFramesKept;
    _score.lostFrames = lostFrames;
    _score.lostPackets = lostPackets;
    _score.framesCount = framesCount;
    _score.mpx3clock_stops = mpx3clock_stops;
    _score.dataMisaligned = dataMisaligned;

}

void QCstmGLVisualization::progress_signal(int framecntr) {

    _score.nFramesReceived = framecntr;

    /*
    // framecntr: frames kept
    int argcntr = framecntr;
    if ( _dataTakingThread ) {
        if ( _dataTakingThread->isACompleteJob() ) {
            argcntr += _dataTakingThread->getFramesReceived();
        }
    }

    int nTriggers = _mpx3gui->getConfig()->getNTriggers();
    QString prog;
    if ( nTriggers > 0 ) prog = QString("%1/%2").arg( argcntr ).arg( nTriggers );
    else prog = QString("%1").arg( argcntr ); // nTriggers=0 is keep taking data forever

    ui->frameCntr->setText( prog );
    */

}


void QCstmGLVisualization::reload_all_layers(bool corrections) {

    // Get busy
    SetBusyState();

    //_mpx3gui->saveOriginalDataset();

    // Corrections
    if ( corrections && _corrdialog ) {
        _mpx3gui->getDataset()->applyCorrections( _corrdialog );
    }


    ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset()); //TODO: only read specific layer.
    QList<int> thresholds = _mpx3gui->getDataset()->getThresholds();
    for ( int i = 0 ; i < thresholds.size() ; i++ ) {

        addThresholdToSelector(thresholds[i]);

        ui->histPlot->setHistogram(thresholds[i],
                                   _mpx3gui->getDataset()->getLayer(thresholds[i]),
                                   _mpx3gui->getDataset()->getPixelsPerLayer(),
                                   _manualRange.lower,
                                   _manualRange.upper);

    }

    // done
    active_frame_changed();

    //ui->glPlot->update();

    // Get free
    FreeBusyState();

}

void QCstmGLVisualization::addThresholdToSelector(int threshold){
    QString label = QString("Threshold %1").arg(threshold);
    if(!layerNames.contains(threshold)){
        layerNames[threshold] =label ;
        ui->layerSelector->clear();
        ui->layerSelector->addItems(QStringList(layerNames.values()));
    }
}

void QCstmGLVisualization::changeThresholdToNameAndUpdateSelector(int threshold, QString name) {

    if ( name == "Ca_Z20" ) name = "Unknown";
    if ( name == "Al_Z13" ) name = "Unknown";

    QString label = QString("Material %1 | %2").arg( name ).arg( threshold ); // The threshold number is needed here !  When reloaded the program finds it by searching for it.  Change this !  TODO

    if ( layerNames.contains(threshold) ) {
        layerNames[threshold] = label ;
        ui->layerSelector->setItemText( _mpx3gui->getDataset()->thresholdToIndex(threshold), label );
    } else {
        qDebug() << "[QCstmGLVisualization::changeThresholdToNameAndUpdateSelector] The given threshold didn't exist !";
    }

}

void QCstmGLVisualization::setThreshold(int threshold){
    addThresholdToSelector(threshold);
    //cout << "[INDEX] " << threshold << " --> " << _mpx3gui->getDataset()->thresholdToIndex(threshold) << endl;
    ui->layerSelector->setCurrentIndex(_mpx3gui->getDataset()->thresholdToIndex(threshold));
}

int QCstmGLVisualization::getActiveThreshold(){
    QStringList list = ui->layerSelector->currentText().split(" ");
    if(list.size() < 2)
        return -1;
    // The index will be found in the last position.  TODO ! Let's don't pick the right threshold like this. Have a map !
    int valIdx = list.size() - 1;
    return list[valIdx].toInt();
}

bool QCstmGLVisualization::isSaveAllFramesChecked()
{
    return ui->saveAllCheckBox->isChecked();
}

void QCstmGLVisualization::active_frame_changed(){

    //ui->layerSelector->addItem(QString("%1").arg(threshold));
    int layer = _mpx3gui->getDataset()->thresholdToIndex(this->getActiveThreshold());
    ui->glPlot->getPlot()->setActive(layer);
    ui->histPlot->setActive(layer);

    //! EXPENSIVE function. Maybe reimplement this later
    //! BuildStatsStringCounts( _mpx3gui->getDataset()->getActivePixels(getActiveThreshold()) );

    //ui->countsLabel->setText(QString("%1").arg(_mpx3gui->getDataset()->getActivePixels(getActiveThreshold())));

    if(ui->percentileRangeRadio->isChecked())
        on_percentileRangeRadio_toggled(true);
    else if(ui->fullRangeRadio->isChecked())
        on_fullRangeRadio_toggled(true);

}

void QCstmGLVisualization::shortcutStart()
{
    ui->startButton->animateClick();
}

void QCstmGLVisualization::shortcutIntegrate()
// You always want integrate checked having pressed this key
{
    if (ui->summingCheckbox->isChecked()){
        qDebug() << "[INFO] Integrate checkbox already checked, doing nothing";
    } else {
        ui->summingCheckbox->setChecked(true);
    }
}

void QCstmGLVisualization::shortcutIntegrateToggle()
{
    if (ui->summingCheckbox->isChecked()){
        ui->summingCheckbox->setChecked(false);
    } else {
        ui->summingCheckbox->setChecked(true);
    }
}

void QCstmGLVisualization::shortcutFrameLength()
{
    ui->triggerLengthSpinBox->setFocus();
}

void QCstmGLVisualization::shortcutFrameNumber()
{
    ui->nTriggersSpinBox->setFocus();
}

void QCstmGLVisualization::takeImage()
{
    //! Delete current image
    //! Turn off autosave
    //! Trigger start data taking
#ifdef QT_DEBUG
    qDebug() << ("[INFO]\tZMQ \n\t + Delete current image \n\t + Trigger start data taking \n\t + Turn off autosave");
#endif

    zmqRunning = true;

    _mpx3gui->zero_data();
    ui->saveCheckBox->setChecked(false);
    StartDataTaking();

    //! Emit someCommandHasFinished_Successfully() in dataTakingFinished
}

void QCstmGLVisualization::takeAndSaveImageSequence()
{
    //! Activate autosave to home directory or whatever
    //! Trigger data taking
#ifdef QT_DEBUG
    qDebug() << ("[INFO]\tZMQ Activate autosave to home directory or whatever \n\t + Trigger data taking");
#endif

    zmqRunning = true;

    ui->saveCheckBox->setChecked(true);
    ui->saveLineEdit->setText(QDir::homePath() + QDir::separator());
    ui->saveAllCheckBox->setChecked(true);
    _mpx3gui->zero_data();
    StartDataTaking();

    //! Emit someCommandHasFinished_Successfully() in dataTakingFinished
}

void QCstmGLVisualization::saveImageSlot(QString filePath)
{
    //! TODO More error checking here?
    saveImage(filePath);
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ \n\tSaved raw image as tiff to :" << filePath;
#endif
    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::setExposure(int microseconds)
{
    //! Switch to Sequential RW, then set open and closed time
    //!    Always 2ms for down time
    int index = -1;
    index = ui->operationModeComboBox_Vis->findText("Sequential R/W");
    if (index >= 0) {
        ui->operationModeComboBox_Vis->setCurrentIndex(index);
        _mpx3gui->getConfig()->setTriggerDowntime(2000);

        ui->triggerLengthSpinBox->setValue(microseconds);
        triggerLength_edit();

        emit someCommandHasFinished_Successfully();
    } else {
        emit someCommandHasFailed(QString("DEXTER --> ACQUILA ZMQ : Could not set exposure"));
    }

}

void QCstmGLVisualization::setNumberOfFrames(int number_of_frames)
{
    ui->nTriggersSpinBox->setValue(number_of_frames);
    ntriggers_edit();
    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::setThreshold(int threshold, int value)
{
    //! Set specified threshold to value
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Set threshold " << threshold << "to value " << value;
#endif

    //! TODO this operates on all chips simultaneously (check this), make is operate on a specified chip
    _mpx3gui->getDACs()->changeDAC(threshold, value);

    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::setGainMode(int mode)
{
    //! Set by value
    _mpx3gui->getConfig()->setGainMode(mode);
    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::setCSM(bool active)
{
    //! True --> CSM ON
    //! False -> CSM OFF
    int index = -1;
    if ( active ) {
        index = _mpx3gui->getConfigMonitoring()->getUI()->csmSpmCombo->findText("ON");
    } else {
        index = _mpx3gui->getConfigMonitoring()->getUI()->csmSpmCombo->findText("OFF");
    }
    _mpx3gui->getConfigMonitoring()->getUI()->csmSpmCombo->setCurrentIndex(index);
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Set CSM:" << active;
#endif

    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::loadDefaultEqualisation()
{
    _mpx3gui->getEqualization()->LoadEqualization(false,false);

    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::loadEqualisation(QString path)
{
    if (path[path.length()-1] != QDir::separator()) {
        path = path.append(QDir::separator());
    }

    _mpx3gui->getEqualization()->LoadEqualization(false,false ,path);

    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::setReadoutMode(QString mode)
{
    //! Sequential or CRW
    int index = -1;
    mode = mode.toLower();
    if ( ( mode.contains( "srw" )) || ( mode.contains("seq") ) ) {
        index = _mpx3gui->getConfigMonitoring()->getUI()->operationModeComboBox->findText("Sequential R/W");
    } else if ( ( mode.contains("crw") ) || ( mode.contains("cont")) ) {
        index = _mpx3gui->getConfigMonitoring()->getUI()->operationModeComboBox->findText("Continuous R/W");
    } else {
        qDebug() << "[ERROR]\tZMQ could not set readout mode : " << mode;
        emit someCommandHasFailed(QString("DEXTER --> ACQUILA ZMQ : Could not set readout mode"));
        return;
    }

    _mpx3gui->getConfigMonitoring()->getUI()->operationModeComboBox->setCurrentIndex(index);
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Set Readout mode:" << mode;
#endif

    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::setReadoutFrequency(int frequency)
{
    //! Will switch to CRW, then set the frequency
    setReadoutMode("Continuous R/W");
    _mpx3gui->getConfig()->setContRWFreq(frequency);
    _mpx3gui->getConfigMonitoring()->setReadoutFrequency(frequency);
    ui->triggerLengthSpinBox->setValue(frequency);

#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Set CRW frequency:" << frequency;
#endif

    //! Don't need a command has finished here since it calls setReadoutMode()
//    emit someCommandHasFinished_Successfully();
}

void QCstmGLVisualization::loadConfiguration(QString filePath)
{
    //! From default location unless otherwise specfied
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Load configuration from:" << filePath;
#endif

    if (_mpx3gui->getConfig()->fromJsonFile(filePath, true)) {
        emit someCommandHasFinished_Successfully();
    } else {
        emit someCommandHasFailed(QString("DEXTER --> ACQUILA ZMQ : Could not load configuration JSON file from" + filePath));
    }
}

void QCstmGLVisualization::onRequestForAutoSaveFromServer(bool val)
{
    autosaveFromServer = true; //! This is so I don't have to modify on_saveCheckBox_clicked()
    ui->saveCheckBox->setChecked(val);
    ui->saveAllCheckBox->setChecked(val);
    autosaveFromServer = false; //! So it only skips the GUI call to get the
                                //! path if it's being called by the TCP server

#ifdef QT_DEBUG
    qDebug() << "[INFO] QCstmGLVisualization::onRequestForAutoSaveFromServer =" << val;
#endif
}

void QCstmGLVisualization::onRequestForSettingPathFromServer(QString path)
{
    bool success = requestToSetSavePath(path);
    Q_UNUSED(success);
}

void QCstmGLVisualization::onRequestForSettingFormatFromServer(int idx)
{
    ui->saveFileComboBox->setCurrentIndex(idx);
}

void QCstmGLVisualization::region_selected(QPoint pixel_begin, QPoint pixel_end, QPoint position){

    int threshold = getActiveThreshold();

    QMenu contextMenu;
    QPoint pixel_begin_checked = pixel_begin;
    QPoint pixel_end_checked = pixel_end;

    //##################################
    // Do basic out-of-bounds type check
    if (pixel_begin.x() < 0) {
        pixel_begin_checked.setX(0);
    }
    if (pixel_begin.y() < 0) {
        pixel_begin_checked.setY(0);
    }
    // User is for sure trying to break the program...
    if (pixel_end.x() < 0 || pixel_end.y() < 0) {
        // Negative end pixel input - try again;
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle("Input error");
        msgBox->setInformativeText("You selected a pixel range with at least one negative end coordinate. \n\n Try again by ending your selection within the image.");
        msgBox->setDefaultButton(QMessageBox::Cancel);

        QSpacerItem* horizontalSpacer = new QSpacerItem(400, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        QGridLayout* layout = (QGridLayout*)msgBox->layout();
        layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());

        msgBox->exec();
        return;
    }

    int xImageLength = _mpx3gui->getDataset()->x()*_mpx3gui->getDataset()->getNChipsX();
    int yImageLength = _mpx3gui->getDataset()->y()*_mpx3gui->getDataset()->getNChipsY();
    if (pixel_begin.x() > xImageLength || pixel_begin.y() > yImageLength) {
        // Stop being an idiot
        return;
    }
    if (pixel_begin.x() > xImageLength) {
        pixel_begin_checked.setX(xImageLength);
    }
    if (pixel_begin.y() > yImageLength) {
        pixel_begin_checked.setY(yImageLength);
    }
    if (pixel_end.x() > xImageLength) {
        pixel_end_checked.setX(xImageLength);
    }
    if (pixel_end.y() > yImageLength) {
        pixel_end_checked.setY(yImageLength);
    }
    //##################################

    //Have the region only in the header:
    QLabel* label = new QLabel(QString("\n    For region (%1, %2) --> (%3, %4) \n").arg(pixel_begin_checked.x()).arg(pixel_begin_checked.y()).arg(pixel_end_checked.x()).arg(pixel_end_checked.y())
                               , this);
    QWidgetAction wid(&contextMenu);
    wid.setDefaultWidget(label);
    contextMenu.addAction(&wid);

    QAction calcStats(QString("Calculate statistics"), &contextMenu);
    //QAction calcStats(QString("Calculate statistics (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y()), &contextMenu);
    contextMenu.addAction(&calcStats);

    QAction calcProX(QString("ProfileX"), &contextMenu);
    //QAction calcProX(QString("ProfileX (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y()), &contextMenu);
    contextMenu.addAction(&calcProX);

    QAction calcProY(QString("ProfileY"), &contextMenu);
    //QAction calcProY(QString("ProfileY (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y()), &contextMenu);
    contextMenu.addAction(&calcProY);

    contextMenu.setMinimumWidth(300);

    // Show the menu
    QAction * selectedItem = contextMenu.exec(position);

    // Selected item
    if (selectedItem == &calcStats) {

        // Calc basic stats
        _mpx3gui->getDataset()->calcBasicStats(pixel_begin_checked, pixel_end_checked);

        // Now display it

        _statsdialog = new StatsDialog(this);
        _statsdialog->SetMpx3GUI(_mpx3gui);
        _statsdialog->changeText();
        _statsdialog->show();

    }

    else if(selectedItem == &calcProX || selectedItem == &calcProY) {
        //else if(selectedItem != nullptr) {

        QString axis;
        if(selectedItem == &calcProX) axis = "X";
        if(selectedItem == &calcProY) axis = "Y";

        //Display
        _profiledialog = new ProfileDialog(this);
        _profiledialog->SetMpx3GUI(_mpx3gui);
        _profiledialog->setRegion(pixel_begin_checked, pixel_end_checked);
        _profiledialog->setAxis(axis);
        _profiledialog->changeTitle();

        //QList<int> thresholdlist = _mpx3gui->getDataset()->getThresholds();
        //QStringList combolist;
        //for(int i = 0; i < thresholdlist.length(); i++)
        //    combolist.append(QString("Threshold %1").arg(thresholdlist[i]));

        //Calculate the profile of the selected region of the selected layer

        //_profiledialog->setSelectedThreshold(threshold);
        //_profiledialog->setAxisMap(_mpx3gui->getDataset()->calcProfile(axis, threshold, pixel_begin_checked, pixel_end_checked));

        int layerIndex = getActiveThreshold();
        _profiledialog->setLayer(layerIndex);
        _profiledialog->setAxisMap(_mpx3gui->getDataset()->calcProfile(axis, layerIndex, pixel_begin_checked, pixel_end_checked));
        _profiledialog->plotProfile();

        _profiledialog->show();

    }

    //    delete label;
    //    delete a;
}

//! TODO BUG The maths on this is incorrect, apparently opposing chips have pixels masked when they should not be...
void QCstmGLVisualization::pixel_selected(QPoint pixel, QPoint position){

    int chipID;
    QPoint chipIndex = previewIndexToChipIndex(pixel,&chipID);

    if(!_mpx3gui->getConfig()->isConnected())
        return;
    int frameIndex = _mpx3gui->getDataset()->getContainingFrame(pixel);
    if(frameIndex == -1)
        return;
    QPoint naturalCoords = _mpx3gui->getDataset()->getNaturalCoordinates(pixel, frameIndex);
    int naturalFlatCoord = naturalCoords.y()*_mpx3gui->getDataset()->x()+naturalCoords.x();
    if(_mpx3gui->getConfig()->getColourMode()) {
        naturalFlatCoord = 4*naturalCoords.y()*_mpx3gui->getDataset()->x() + 2*naturalCoords.x();
    }
    int deviceID = _mpx3gui->getConfig()->getActiveDevices()[frameIndex];

    if(_maskingRequestFromServer)
    {
        _maskingRequestFromServer = false;
    }

    else{//for interactive use
        QMenu contextMenu;
        QAction mask(QString("Mask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu),
                unmask(QString("Unmask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu),
                maskAllOverflow(QString("Mask all pixels in overflow (per chip)"), &contextMenu),
                maskAllActive(QString("Mask all active pixels (per chip)"), &contextMenu);

        contextMenu.addAction(&mask);
        contextMenu.addAction(&unmask);
        contextMenu.addAction(&maskAllOverflow);
        contextMenu.addAction(&maskAllActive);


        QAction* selectedItem = contextMenu.exec(position);
        if(!_mpx3gui->getConfig()->isConnected())
            return;
        if (!_mpx3gui->equalizationLoaded()) {
            QMessageBox::information(nullptr, "Error", "An equalisation must be loaded in order to mask a pixel. Failed operation/");
            return;
        }


        if(selectedItem == &mask) _maskOpration = MASK;
        else if(selectedItem == &unmask) _maskOpration = UNMASK;
        else if(selectedItem == &maskAllOverflow) _maskOpration = MASK_ALL_OVERFLOW;
        else if(selectedItem == &maskAllActive) _maskOpration = MASK_ALL_ACTIVE;
        else _maskOpration = NULL_MASK;
    }


    if(_maskOpration == MASK) {
        if ( _mpx3gui->getConfig()->getColourMode() ) {

            //qDebug() << "Nat[" << deviceID << "]:"
            //       << naturalFlatCoord
            //       << naturalFlatCoord+1
            //       << naturalFlatCoord+_mpx3gui->getX()*2
            //       << naturalFlatCoord+1+_mpx3gui->getX()*2;

            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord);

            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord+1);

            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord+_mpx3gui->getX()*2);

            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord+1+_mpx3gui->getX()*2);

        }
        else{
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord);
        //_mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel2D(QPair<int,int>(chipIndex.x(),chipIndex.y()));

        }
    }
    else if(_maskOpration == UNMASK) {
        if(_mpx3gui->getConfig()->getColourMode()){
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord);

            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord+1);

            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord+_mpx3gui->getX()*2);

            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord+1+_mpx3gui->getX()*2);


        }
        else{
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord);
      //  _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel2D(QPair<int,int>(chipIndex.x(),chipIndex.y()));

        }
    }
    else if((_maskOpration == MASK_ALL_OVERFLOW) || (_maskOpration == MASK_ALL_ACTIVE)){

        //! Find all pixels in overflow or active depending on what is selected
        int nx = _mpx3gui->getDataset()->x();
        int ny = _mpx3gui->getDataset()->y();
        int nChipsX = _mpx3gui->getDataset()->getNChipsX();
        int nChipsY = _mpx3gui->getDataset()->getNChipsY();
        int activeThreshold = getActiveThreshold();
        int overflowval = _mpx3gui->getDataset()->getPixelDepthBits();
        overflowval = (1<<overflowval) - 1;

        QVector<QPoint> toMask;
        for ( int xi = 0 ; xi < nx*nChipsX ; xi++ ) {
            for ( int yi = 0 ; yi < ny*nChipsY ; yi++ ) {
                if (_maskOpration == MASK_ALL_OVERFLOW) {
                    if ( _mpx3gui->getDataset()->sample(xi, yi, activeThreshold) == overflowval ) {
                        toMask.push_back( QPoint(xi, yi) );
                    }
                } else if (_maskOpration == MASK_ALL_ACTIVE) {
                    if ( _mpx3gui->getDataset()->sample(xi, yi, activeThreshold) > 0 ) {
                        toMask.push_back( QPoint(xi, yi) );
                    }
                }
            }
        }

        // Now find out the natural coords for all of them
        // Well have up to 4 groups, one per chip
        // QMap < containingframe, naturalcoordsVector >
        QMap<int, QVector<int>> naturalFlatCoords;

        QVector<QPoint>::const_iterator itr  = toMask.begin();
        QVector<QPoint>::const_iterator itrE = toMask.end();
        bool colourMode = _mpx3gui->getConfig()->getColourMode();

        for ( ; itr != itrE ; itr++ ) {
            int chipId = _mpx3gui->getDataset()->getContainingFrame( *itr );
            QPoint naturalCoords = _mpx3gui->getDataset()->getNaturalCoordinates( *itr, chipId );
            if (colourMode) {
                naturalFlatCoords[ chipId ].push_back( 4*naturalCoords.y()*nx + 2*naturalCoords.x() );
            } else {
                naturalFlatCoords[ chipId ].push_back( naturalCoords.y()*nx+naturalCoords.x() );
            }
        }

        // done, now mask them all
        QMap<int, QVector<int>>::const_iterator itrM  = naturalFlatCoords.begin();
        QMap<int, QVector<int>>::const_iterator itrME = naturalFlatCoords.end();

        for ( ; itrM != itrME ; itrM++ ) {

            int chip = (itrM).key();
            int vsize = (*itrM).size();
            qDebug() << "[DEBUG]\tChip" << chip << "--> Mask" << vsize;

            for ( int iv = 0 ; iv < vsize ; iv++ ) {
                if ( colourMode ) {
                    _mpx3gui->getEqualization()->GetEqualizationResults( chip )->maskPixel( (*itrM).value(iv) );
                    _mpx3gui->getEqualization()->GetEqualizationResults( chip )->maskPixel( (*itrM).value(iv)+1 );
                    _mpx3gui->getEqualization()->GetEqualizationResults( chip )->maskPixel( (*itrM).value(iv)+_mpx3gui->getX()*2);
                    _mpx3gui->getEqualization()->GetEqualizationResults( chip )->maskPixel( (*itrM).value(iv)+1+_mpx3gui->getX()*2);
                } else {
                    _mpx3gui->getEqualization()->GetEqualizationResults( chip )->maskPixel( (*itrM).value(iv) );
                }
            }

        }

    }

    if (_maskOpration == NULL_MASK) {
        return;
    }

    _mpx3gui->getEqualization()->SetAllAdjustmentBits( _mpx3gui->getConfig()->getController(), deviceID, true, false);
}

void QCstmGLVisualization::on_manualRangeRadio_toggled(bool checked)
{

    // Before toogle save the current range to percentile
    // If unselecting save the info
    if ( ! checked ) {
        if ( _manualRangePicked ) {
            _manualRangeSave = _manualRange;
        }
    }

    if ( checked ) {

        // When toggling here recompute the min and max
        // if the range is still the initial (0,1)
        //if ( _manualRange == QCPRange( 0,0 ) ) {
        // current threshold
        int activeTHL = getActiveThreshold();
        if ( activeTHL >= 0 ) {

            // Throw a recomendation here to the user.  If manual has never been set, then use
            //  the values from percentile.
            if ( ! _manualRangePicked ) {
                _manualRange = _percentileRangeNatural;
                _manualRangeSave = _manualRange;
            } else {
                _manualRange = _manualRangeSave;
            }

        } else {
            _manualRange = QCPRange( 0, 0 );
        }
        //}

        setRangeSpinBoxesManual();
        ui->histPlot->changeRange( _manualRange );
        //ui->histPlot->changeRange(QCPRange(ui->lowerManualSpin->value(), ui->upperManualSpin->value()));
        //i+
        ui->histPlot->scaleToInterest();
    }

}

void QCstmGLVisualization::on_fullRangeRadio_toggled(bool checked)
{

    if ( checked ) {

        int activeTHL = getActiveThreshold();

        if ( activeTHL >= 0 ) {

            // When toggling here recompute the min and max
            int * data = _mpx3gui->getDataset()->getLayer( activeTHL );
//            if (data == nullptr)
//                qDebug() << "WTF IS HAPPENING??"; //! TODO FIX THIS SHIT RIGHT NOW
//                return;
            int size = _mpx3gui->getDataset()->getPixelsPerLayer();
            int min = INT_MAX, max = INT_MIN;
            for(int i = 0; i < size; i++) {
//                if (data[i] == 0x0) {
//                    min = 0;
//                    max = 1;
//                }
                if(data[i] < min)
                    min = data[i];
                if(data[i] > max)
                    max = data[i];
            }
            _manualRange = QCPRange( min, max );

        } else {
            _manualRange = QCPRange( 0,1 );
        }

        setRangeSpinBoxesManual();
        ui->histPlot->set_scale_full(getActiveThreshold());
    }

    ui->histPlot->scaleToInterest();

}

void QCstmGLVisualization::on_percentileRangeRadio_toggled(bool checked)
{

    // If unselecting save the info
    if ( !checked ) _percentileRange = QCPRange( ui->lowerSpin->value(), ui->upperSpin->value() );

    if ( checked ) {

        setRangeSpinBoxesPercentile();
        ui->histPlot->changeRange( _percentileRange );

        //ui->histPlot->set_scale_percentile(getActiveThreshold(),
        //                                   ui->lowerPercentileSpin->value(),
        //                                   ui->upperPercentileSpin->value());
        _percentileRangeNatural = ui->histPlot->set_scale_percentile(getActiveThreshold(),
                                                                     ui->lowerSpin->value(),
                                                                     ui->upperSpin->value());
        ui->histPlot->scaleToInterest();
    }

}

void QCstmGLVisualization::on_outOfBoundsCheckbox_toggled(bool checked)
{
    ui->glPlot->getPlot()->setAlphaBlending(checked);
}

void QCstmGLVisualization::on_layerSelector_activated(const QString &arg1)
{
    QStringList split = arg1.split(' ');
    int threshold = split.last().toInt();
    int layer = _mpx3gui->getDataset()->thresholdToIndex(threshold);
#ifdef QT_DEBUG
    qDebug() << "[DEBUG] INDEX" << threshold << "-->" << layer << "from list" << _mpx3gui->getDataset()->getThresholds() << "\n";
#endif

    ui->glPlot->getPlot()->setActive(layer);
    ui->histPlot->setActive(layer);
    ui->layerSelector->setCurrentIndex(layer);
    this->active_frame_changed();
}

void QCstmGLVisualization::on_summingCheckbox_toggled(bool checked){
    emit mode_changed(checked);
}

void QCstmGLVisualization::on_correctionsDialogPushButton_clicked(){
    if (!_corrdialog) {
        // Setup corrections and be ready to apply them
        _corrdialog = new QCstmCorrectionsDialog(this);
        _corrdialog->SetMpx3GUI( _mpx3gui );

    } else {
        // _corrDialog already made
    }

    //! Continue to make the user use the existing dialog or dismiss it with a "non-modal dialog"
    _corrdialog->show();
    _corrdialog->raise();
    _corrdialog->activateWindow();

}

void QCstmGLVisualization::on_singleshotPushButton_clicked()
{
    // Temporarily change the configuration to a single shot
    _singleShot = true;
    _singleShotSaveCurrentNTriggers = _mpx3gui->getConfig()->getNTriggers();

    // Select only one trigger
    _mpx3gui->getConfig()->setNTriggers( 1 );

    // And just start taking data
    StartDataTaking();

}

void QCstmGLVisualization::on_lowerSpin_editingFinished()
{

    // See that the order is right
    if(ui->upperSpin->value() < ui->lowerSpin->value())
        ui->upperSpin->setValue(ui->lowerSpin->value());

    // Then decide what to do
    if ( ui->fullRangeRadio->isChecked() ) { // go to manual
        _manualRangeSave.lower = ui->lowerSpin->value();
        if ( ! _manualRangePicked ) {
            _manualRangeSave.upper = ui->upperSpin->value();
            _manualRangePicked = true;
        }
        ui->manualRangeRadio->setChecked( true );
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
        return;
    }

    if( ui->manualRangeRadio->isChecked() ) {
        _manualRangePicked = true;
        _manualRangeSave.lower = ui->lowerSpin->value();
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
    }

    if( ui->percentileRangeRadio->isChecked() ) {
        _percentileRange = QCPRange( ui->lowerSpin->value(), ui->upperSpin->value() );
        on_percentileRangeRadio_toggled(ui->percentileRangeRadio->isChecked());
    }

}

void QCstmGLVisualization::on_upperSpin_editingFinished()
{
    if(ui->upperSpin->value() < ui->lowerSpin->value())
        ui->lowerSpin->setValue(ui->upperSpin->value());

    // Then decide what to do
    if ( ui->fullRangeRadio->isChecked() ) { // go to manual
        _manualRangeSave.upper = ui->upperSpin->value();
        if ( ! _manualRangePicked ) {
            _manualRangeSave.lower = ui->lowerSpin->value();
            _manualRangePicked = true;
        }
        ui->manualRangeRadio->setChecked( true );
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
        return;
    }

    if(ui->manualRangeRadio->isChecked()) {
        _manualRangePicked = true;
        _manualRangeSave.upper = ui->upperSpin->value();
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
    }

    if(ui->percentileRangeRadio->isChecked()) {
        _percentileRange = QCPRange( ui->lowerSpin->value(), ui->upperSpin->value() );
        on_percentileRangeRadio_toggled(ui->percentileRangeRadio->isChecked());
    }

}

void QCstmGLVisualization::on_logscale(bool checked)
{

    _logyPlot = checked;

    if ( _logyPlot ) {
        ui->histPlot->yAxis->setScaleType( QCPAxis::stLogarithmic );
        ui->histPlot->yAxis->setRangeLower( __range_min_whenLog );
    } else {
        ui->histPlot->yAxis->setScaleType( QCPAxis::stLinear );
        ui->histPlot->yAxis->setRangeLower( 0 );
    }

    ui->histPlot->replot(QCustomPlot::rpQueued);

}

void QCstmGLVisualization::on_infDataTakingCheckBox_toggled(bool checked)
{

    _infDataTaking = checked;
    ui->infDataTakingCheckBox->setChecked(checked);

    if ( checked ) {
        _nTriggersSave = _mpx3gui->getConfig()->getNTriggers();
        // In this case configure the system for 0 triggers.  It will do for both operation modes
        _mpx3gui->getConfig()->setNTriggers( 0 );
        //
        ui->nTriggersSpinBox->setEnabled( false );
    } else {
        _mpx3gui->getConfig()->setNTriggers( _nTriggersSave );
        ui->nTriggersSpinBox->setEnabled( true );
    }

}

void QCstmGLVisualization::on_dropFramesCheckBox_clicked(bool checked){
    _dropFrames = checked;
}

void QCstmGLVisualization::bufferOccupancySlot(int occ){
    ui->bufferOccupancy->setValue( occ );
}

void QCstmGLVisualization::consumerFinishedOneFrame(int frameId){

    //! If Save checkbox is checked and Save line edit is not empty,
    //! AND the used requested every frame to be saved.
    //! Save the data to .tiff file with path obtained from UI
    if(
            ui->saveCheckBox->isChecked() &&
            !(ui->saveLineEdit->text().isEmpty()) &&
            ui->saveAllCheckBox->isChecked()
            ){
        QString selectedFileType = ui->saveFileComboBox->currentText();
        _mpx3gui->save_data(true, frameId, selectedFileType);
    }
}

//! Resets the view - uses same function as double clicking
void QCstmGLVisualization::on_resetViewPushButton_clicked(){
    reload_all_layers();
}

//! Clear saveLineEdit on_saveCheckBox_clicked by a user, every time
void QCstmGLVisualization::on_saveCheckBox_clicked(){
    if ( zmqRunning ) {
        return;
    } else {

        //! Open file dialog, get path and set saveLineEdit to given path and continue
        if(ui->saveCheckBox->isChecked()){

            //! Get the Absolute folder path
            QString path = getPath("Select a directory to auto-saved files to");

            //! GUI update - save LineEdit set to path from dialog
            ui->saveLineEdit->setText(path);

            //! If user selected nothing, path comes back empty (or "/" ?)
            if(path.isEmpty()) {
                ui->saveCheckBox->setChecked(0);
                ui->saveAllCheckBox->setChecked(0);
                ui->saveLineEdit->clear();
            }

            //qDebug() << "[INFO]\tSelected path:" << path;

            //! When finished, see data_taking_finished() where the data is saved
        } else {
            ui->saveLineEdit->clear();
        }
    }
}

void QCstmGLVisualization::consumerBufferFull(int)
{

    QMessageBox::critical(this, tr("System buffer"),
                          tr("The system can't keep up. Please review your settings. Increasing UDP kernel buffer size or getting a faster CPU may help."));

}

void QCstmGLVisualization::on_saveAllCheckBox_toggled(bool checked)
{
    if ( zmqRunning ) {
        ui->saveCheckBox->setChecked( true );
        return;
    }
    if ( checked ) {
        ui->saveCheckBox->setChecked( true );
        if ( getsaveLineEdit_Text() == "" ) {
            on_saveCheckBox_clicked();
        }
    }
}


uint32 assembleData(uint8 a,uint8 b,uint8 c,uint8 d){
    uint32 res = 0;
    res = a | (b << 8)| (c << 16)  | (d << 24);
    return res;
}

void QCstmGLVisualization::on_testBtn_clicked()
{

//    for(int i = 0; i< 4; i++)
//    {
//    int val22 = 0;
//    Mpx3GUI::getInstance()->GetSpidrController()->getDac(i,MPX3RX_DAC_TABLE[ 0 ].code,&val22);
//    qDebug() << "value : " << val22;
//    }

//    Dataset *ds = _mpx3gui->getDataset();
//    for (int key = 0; key < 8; ++key) {
//        int * layer = ds->getLayer(key);
//        if (layer == nullptr){
//            qDebug() << "Layer [" << key <<"] is null.";
//        }
//        else
//            qDebug() << "Layer [" << key <<"] has data.";

//    }



//    qDebug () << "SlOPE 0 :" << Mpx3GUI::getInstance()->getGeneralSettings()->getSlope(0);
//    qDebug () << "SlOPE 1 :" << Mpx3GUI::getInstance()->getGeneralSettings()->getSlope(1);
//    qDebug () << "SlOPE 2 :" << Mpx3GUI::getInstance()->getGeneralSettings()->getSlope(2);
//    qDebug () << "SlOPE 3 :" << Mpx3GUI::getInstance()->getGeneralSettings()->getSlope(3);
//    qDebug () << "Trigger Mode: " << Mpx3GUI::getInstance()->getConfig()->getTriggerMode();
    //Mpx3GUI::getInstance()->GetSpidrController()->setDac(1,0+1,16);
    for (int chip = 0; chip < 4; chip++) {
        for (int idx = 0; idx < 8; idx++) {
            int val = 0;
            _mpx3gui->GetSpidrController()->getDac(chip,idx+1,&val);
            qDebug() << "Chip ["<<chip<<"] ... Threshold ["<<idx<<"] : "<< val;
        }
    }

    for (int chip = 0; chip < 4; chip++) {
        for (int idx = 0; idx < 8; idx++) {
            _thresholdsVector[chip][idx];
            qDebug() << "[Matrix] : Chip ["<<chip<<"] ... Threshold ["<<idx<<"] : "<< _thresholdsVector[chip][idx];;
        }
    }

    //Mpx3GUI::getInstance()->getConfig()->setInhibitShutter(true);
   // Mpx3GUI::getInstance()->getConfig()->setInhibitShutter(false);

//    GeneralSettings *settings = new GeneralSettings;
//    //settings->setConfigPath("Kiavash");
//    //settings->setEqualizationPath("Matin");
//    //settings->writeSetting();
//    settings->readSetting();
//    qDebug() << "Equalization path : " << settings->getEqualizationPath();
//    qDebug() << "Config path : " << settings->getConfigPath();


//    std::pair<const char*,int> image = Mpx3GUI::getInstance()->getDataset()->toSocketData();
//    qDebug() << "Image size is : " << image.second;

//    QString filename = "Pixel.txt";
//    QFile file(filename);
//    file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
//    QTextStream stream(&file);

//    QString filename2 = "Pixel2.txt";
//    QFile file2(filename2);
//    file2.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
//    QTextStream stream2(&file2);




//    int idx = 0;
//    uint32_t* pp = (uint32_t*) image.first;
//    while(idx < image.second){
//        uint32_t pixel = *pp;
//        stream << pixel << endl;
//        pp++;
//        idx = idx + 4;
//    }
//    file.close();
//    qDebug() << "Save is done.";

//    QByteArray testBa(100,'0');
//    qDebug() << "Size of testba : " << testBa.length();
//    QByteArray rep;
//    rep.append((255 & 0x000000FF));
//    rep.append((155 & 0x000000FF));
//    rep.append((25 & 0x000000FF));
//    rep.append((205 & 0x000000FF));
//    testBa.replace(10,4,rep);
//    qDebug() << "Size of testba : " << testBa.length();
//    qDebug() << "testba : " << (uint8_t) testBa.at(9);
//    qDebug() << "testba : " << (uint8_t) testBa.at(10);
//    qDebug() << "testba : " << (uint8_t) testBa.at(11);
//    qDebug() << "testba : " << (uint8_t) testBa.at(12);
//    qDebug() << "testba : " << (uint8_t) testBa.at(13);
//    qDebug() << "testba : " << (uint8_t) testBa.at(14);


//    int big = 0xFFFFFFF0;
//    uint16_t small = big;
//    qDebug()<< " big == " << big;
//    qDebug()<< " small == " << small;
}

void QCstmGLVisualization::on_saveLineEdit_editingFinished()
{
    bool success = requestToSetSavePath(ui->saveLineEdit->text());
    Q_UNUSED(success);
}

void QCstmGLVisualization::onPixelsMasked(int devID, QSet<int> pixelSet)
{
    /* So that the equalisation won't save the mask file to the wrong folder during equalisation */
    if ( _equalizationPath != "") {

        qDebug() << "[INFO]\tPixels masked, making a file in " << _equalizationPath ;

        QFile file(_equalizationPath + QString("mask_") + QString::number(devID));

        if (file.open(QIODevice::ReadWrite | QFile::Truncate)) {
            QTextStream stream(&file);
            QSetIterator<int> i(pixelSet);
            while (i.hasNext()) {
                stream << QString::number(i.next()) << "\n";
            }
            file.close();
        } else {
            qDebug() << "[ERROR]\tCannot open the mask file.";
        }
    } else {
        return;
    }
}

bool QCstmGLVisualization::requestToSetSavePath(QString path)
{
    /* Check the path exists, try to create it, otherwise set it to the home directory. */

    QDir dir(path);

    if (!dir.exists()) {
        if (dir.mkpath(".")) {
            const QString msg = "Folder did not exist, successfully created the requested folder";
            emit sig_statusBarAppend(msg, "black");
            qDebug().noquote() << "[INFO]\t" << msg;
        }
    }

    path = ui->saveLineEdit->text();
    QFileInfo dir_info(path);

    if (!dir_info.isWritable()) {
        ui->saveLineEdit->setText(QDir::homePath());

        const QString msg = "Path was not writable, set autosave path to your home directory";
        emit sig_statusBarAppend(msg, "black");
        qDebug().noquote() << "[INFO]\t" << msg;
    }

    return true;
}

void QCstmGLVisualization::setMaximumContRW_FPS(int FPS)
{
    if (FPS == -1) {
        ui->triggerLengthSpinBox->setMaximum(int(0x7fffffff));
        return;
    } else if (_mpx3gui->getConfig()->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW) {
        ui->triggerLengthSpinBox->setMaximum(FPS);
        if (ui->triggerLengthSpinBox->value() > FPS) {
            ui->triggerLengthSpinBox->setValue(FPS);
        }
    }
}

void QCstmGLVisualization::_loadFromThresholdsVector()
{
    for (int chipId = 0; chipId < NUMBER_OF_CHIPS; ++chipId) {
        for (int idx = 0; idx < 8; ++idx) {
            _mpx3gui->GetSpidrController()->setDac(chipId,idx+1,_thresholdsVector[chipId][idx]);

        }
    }
}

void QCstmGLVisualization::_initializeThresholdsVector()
{
    for (int chip = 0; chip < NUMBER_OF_CHIPS; ++chip) {
        for (int idx = 0; idx < 8; ++idx) {
            _thresholdsVector[chip][idx] = _mpx3gui->getConfig()->getDACValue(chip,idx);
        }
    }
}

void QCstmGLVisualization::onEqualizationPathExported(QString path)
{
    _equalizationPath = path;
}

void QCstmGLVisualization::onRequestToMaskPixelRemotely(int x , int y)
{
    _maskingRequestFromServer = true;
    _maskOpration = MASK;

    pixel_selected(QPoint(x,y),QPoint());
}

void QCstmGLVisualization::onRequestToUnmaskPixelRemotely(int x, int y)
{
    _maskingRequestFromServer = true;
    _maskOpration = UNMASK;
    pixel_selected(QPoint(x,y),QPoint());
}
