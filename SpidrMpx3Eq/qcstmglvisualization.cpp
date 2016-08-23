#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"
#include "qcstmequalization.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "DataTakingThread.h"

#include "qcstmcorrectionsdialog.h"
#include "statsdialog.h"
#include "profiledialog.h"
#include "testpulses.h"

#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"

#include "color2drecoguided.h"

//#include "mpx3gui.h"
#include "ui_mpx3gui.h"

#include <stdio.h>
#include <QDialog>
#include <QDebug>

QCstmGLVisualization::QCstmGLVisualization(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmGLVisualization)
{

    ui->setupUi(this);
    _dataTakingThread = 0x0;

    FreeBusyState();
    _takingData = false; // important for offline work

    _busyDrawing = false;
    _etatimer = 0x0;
    _timer = 0x0;
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

}

QCstmGLVisualization::~QCstmGLVisualization() {
    delete ui;
}



void QCstmGLVisualization::timerEvent(QTimerEvent *)
{
    refreshScoringInfo();
}

void QCstmGLVisualization::refreshScoringInfo()
{

    updateETA();

    // Progress
    int nTriggers = _mpx3gui->getConfig()->getNTriggers();
    QString prog;
    if ( nTriggers > 0 ) prog = QString("%1/%2").arg( _score.nFramesKept ).arg( nTriggers );
    else prog = QString("%1").arg( _score.nFramesKept ); // nTriggers=0 is keep taking data forever
    ui->frameCntr->setText( prog );

    // Fps
    double fpsVal = ((double)_score.nFramesReceived) / ((double)_etatimer->elapsed() / 1000.); // elapsed() comes in milliseconds
    QString fpsS = QString::number( round( fpsVal ) , 'd', 0 );
    fpsS += " fps";
    ui->fpsLabel->setText( fpsS );

    //
    BuildStatsStringLostFrames( _score.lostFrames );

    //
    BuildStatsStringLostPackets( _score.lostPackets );


    //
    BuildStatsString();

    //qDebug() << "ref .. " << _score.nFramesReceived;

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

    if ( _etatimer ) {
        // show eta in display
        // h must be in the range 0 to 23, m and s must be in the range 0 to 59, and ms must be in the range 0 to 999.
        QTime n(0, 0, 0);                // n == 00:00:00
        QTime t(0, 0, 0);
        int diff = _estimatedETA - _etatimer->elapsed();
        if (diff > 0) t = n.addMSecs( _estimatedETA - _etatimer->elapsed() );
        QString textT = t.toString("hh:mm:ss");
        ui->etaCntr->setText( textT );
    }

}

void QCstmGLVisualization::FinishDataTakingThread() {
    if ( _dataTakingThread ) {
        delete _dataTakingThread;
        _dataTakingThread = nullptr;
    }
}

void QCstmGLVisualization::StopDataTakingThread()
{
    if ( _dataTakingThread ) _dataTakingThread->stop();
}

bool QCstmGLVisualization::DataTakingThreadIsRunning()
{
    return _dataTakingThread->isRunning();
}

bool QCstmGLVisualization::DataTakingThreadIsIdling()
{
    return _dataTakingThread->isIdling();
}


void QCstmGLVisualization::ConfigureGUIForDataTaking() {

    emit taking_data_gui();

    ui->startButton->setText( "Stop" );
    ui->singleshotPushButton->setText( "Stop" );
    emit sig_statusBarAppend("start","blue");

    ui->groupBoxConfigAndStats->setEnabled( false );
    ui->statsLabel->setEnabled( true ); // keep the stats label alive

}

void QCstmGLVisualization::ConfigureGUIForIdling() {

    emit idling_gui();

    ui->startButton->setText( "Start" );
    ui->singleshotPushButton->setText( "single" );
    emit sig_statusBarAppend("done","blue");

    ui->groupBoxConfigAndStats->setEnabled( true );

}

void QCstmGLVisualization::StartDataTaking() {

    if ( !_dataTakingThread ) {
        _dataTakingThread = new DataTakingThread(_mpx3gui, this);
        _dataTakingThread->ConnectToHardware();
    }

    _estimatedETA = _mpx3gui->getConfig()->getTriggerPeriodMS() *  _mpx3gui->getConfig()->getNTriggers(); // ETA in ms
    _estimatedETA += _estimatedETA * __networkOverhead; // add ~10% network overhead.  FIXME  to be calculated at startup

    _dataTakingThread->setFramesRequested( _mpx3gui->getConfig()->getNTriggers() );
    _dataTakingThread->takedata();
    ArmAndStartTimer();

    // GUI
    ConfigureGUIForDataTaking();


    // info refresh
    _timerId = this->startTimer( 100 ); // 100 ms is a good compromise to refresh the scoreing info


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

    _statsString.displayString = "offline";
}

void QCstmGLVisualization::data_taking_finished(int /*nFramesTaken*/) {

    DestroyTimer();
    ETAToZero();

    ConfigureGUIForIdling();

    rewindScoring();

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
    _timer = 0x0;
    if( _etatimer ) delete _etatimer;
    _etatimer = 0x0;

}

void QCstmGLVisualization::GetAFrame() {

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
        //ui->recoPushButton->setEnabled( true );

        // Report the chip ID's
        // Make space in the dataTakingGridLayout
        QVector<int> devs = _mpx3gui->getConfig()->getActiveDevices();
        QVector<int>::const_iterator i  = devs.begin();
        QVector<int>::const_iterator iE = devs.end();
        _statsString.devicesIdString.clear();
        for ( ; i != iE ; i++ ) {
            int indx = _mpx3gui->getConfig()->getIndexFromID( *i );
            _statsString.devicesIdString.append( _mpx3gui->getConfig()->getDeviceWaferId( indx ) );
            if ( (i+1) != iE ) _statsString.devicesIdString.append( " | " );
        }
        if ( _extraWidgets.devicesNamesLabel == nullptr ) {
            _extraWidgets.devicesNamesLabel = new QLabel(this);
            _extraWidgets.devicesNamesLabel->setAlignment( Qt::AlignRight );
        }
        _extraWidgets.devicesNamesLabel->setText( _statsString.devicesIdString );
        int colCount = ui->dataTakingGridLayout->columnCount();
        ui->dataTakingGridLayout->addWidget( _extraWidgets.devicesNamesLabel, 1, 0, 1, colCount );

    } else {

        FinishDataTakingThread();
        ui->startButton->setEnabled( false );
        ui->singleshotPushButton->setEnabled( false );
        //ui->recoPushButton->setEnabled( false );

    }

    // TODO
    // Configure the chip, provided that the Adj mask is loaded
    // now done from the configuration
    //Configuration( false );

}

void QCstmGLVisualization::setGradient(int index){
    ui->glPlot->setGradient(_mpx3gui->getGradient(index));
}

void QCstmGLVisualization::SetMpx3GUI(Mpx3GUI *p){

    _mpx3gui = p;
    setGradient(0);
    changeBinCount(ui->binCountSpinner->value());

    //connect(_mpx3gui, SIGNAL(ConnectionStatusChanged(bool)), ui->startButton, SLOT(setEnabled(bool))); //enable the button on connection
    //connect(_mpx3gui, SIGNAL(ConnectionStatusChanged(bool)), ui->singleshotPushButton, SLOT(setEnabled(bool))); //enable the button on connection

    connect(_mpx3gui, SIGNAL(sizeChanged(int, int)), ui->glPlot, SLOT(setSize(int, int)));
    connect(ui->startButton, SIGNAL(clicked(bool)), this, SLOT(StartDataTaking()));
    connect(this, SIGNAL(mode_changed(bool)), _mpx3gui, SLOT(set_summing(bool)));
    connect(_mpx3gui, SIGNAL(summing_set(bool)), ui->summingCheckbox, SLOT(setChecked(bool)));
    connect(ui->gradientSelector, SIGNAL(activated(int)), this, SLOT(setGradient(int)));
    connect(ui->generateDataButton, SIGNAL(clicked()), _mpx3gui, SLOT(generateFrame()));
    connect(_mpx3gui, SIGNAL(data_cleared()), this, SLOT(on_clear()));
    //connect(_mpx3gui, SIGNAL(frame_added(int)), this, SLOT(on_frame_added(int)));//TODO specify which layer.
    //connect(_mpx3gui, SIGNAL(hist_added(int)), this, SLOT(on_hist_added(int)));
    //connect(_mpx3gui, SIGNAL(hist_changed(int)),this, SLOT(on_hist_changed(int)));
    connect(_mpx3gui, SIGNAL(reload_layer(int)), this, SLOT( reload_layer(int)));
    connect(_mpx3gui, SIGNAL(reload_all_layers()), this, SLOT(reload_all_layers()));
    //connect(_mpx3gui, SIGNAL(frames_reload()),this, SLOT(on_frame_updated()));
    connect(_mpx3gui, SIGNAL(availible_gradients_changed(QStringList)), this, SLOT(availible_gradients_changed(QStringList)));
    connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), this, SLOT(range_changed(QCPRange)));
    //connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), this, SLOT(on_new_range_dragged(QCPRange)));
    // connect(ui->layerSelector, SIGNAL(activated(QString)), ui->glPlot->getPlot(), SLOT()

    connect(ui->binCountSpinner, SIGNAL(valueChanged(int)), this, SLOT(changeBinCount(int)));
    connect(ui->glPlot->getPlot(), SIGNAL(hovered_pixel_changed(QPoint)),this, SLOT(hover_changed(QPoint)));
    connect(ui->glPlot->getPlot(), SIGNAL(pixel_selected(QPoint,QPoint)), this, SLOT(pixel_selected(QPoint,QPoint)));
    connect(ui->glPlot->getPlot(), SIGNAL(region_selected(QPoint,QPoint,QPoint)), this, SLOT(region_selected(QPoint,QPoint,QPoint)));

    connect(this, SIGNAL(change_hover_text(QString)), ui->mouseOverLabel, SLOT(setText(QString)));
    //connect(ui->fullRangeRadio, SIGNAL(pressed()), ui->histPlot, SLOT(set_scale_full()));
    connect(ui->histPlot, SIGNAL(new_range_dragged(QCPRange)), this, SLOT(new_range_dragged(QCPRange)));

    connect( this, &QCstmGLVisualization::sig_statusBarAppend, _mpx3gui, &Mpx3GUI::statusBarAppend );
    connect( this, &QCstmGLVisualization::sig_statusBarWrite, _mpx3gui, &Mpx3GUI::statusBarWrite );
    connect( this, &QCstmGLVisualization::sig_statusBarClean, _mpx3gui, &Mpx3GUI::statusBarClean );

    // Connection to configuration
    connect( ui->nTriggersSpinBox, SIGNAL(editingFinished()),
             this, SLOT(ntriggers_edit()) );

    connect( ui->triggerLengthSpinBox, SIGNAL(editingFinished()),
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


    //
    connect(ui->glPlot->getPlot(), &QCstmGLPlot::double_click,
            this, &QCstmGLVisualization::reload_all_layers);

    // DataTaking / idling actions
    connect( this, &QCstmGLVisualization::taking_data_gui,
            _mpx3gui->getConfigMonitoring(), &QCstmConfigMonitoring::on_taking_data_gui );
    connect( this, &QCstmGLVisualization::idling_gui,
            _mpx3gui->getConfigMonitoring(), &QCstmConfigMonitoring::on_idling_gui );


    // Defaults
    emit mode_changed( ui->summingCheckbox->isChecked() );

}

void QCstmGLVisualization::ntriggers_edit() {

    // Modify the spinner on the config side
    _mpx3gui->getConfigMonitoring()->getUI()->nTriggersSpinner->setValue(
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

    _mpx3gui->open_data_with_path(false, true, "icons/startupimage.bin" );

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
    _statsString.displayString  = "fired: ";
    _statsString.displayString += _statsString.counts;

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

void QCstmGLVisualization::on_user_accepted_stats()
{
    // delete the corresponding window
    if ( _statsdialog ) {
        delete _statsdialog;
        _statsdialog = nullptr;
        _mpx3gui->getDataset()->bstats.mean_v.clear();
        _mpx3gui->getDataset()->bstats.stdev_v.clear();
    }

}

void QCstmGLVisualization::on_user_accepted_profile()
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

        ui->triggerLengthSpinBoxLabel->setText( "Length" );
        ui->triggerLengthSpinBox->setValue( _mpx3gui->getConfig()->getTriggerLength() );

    } else if ( indx == Mpx3Config::__operationMode_ContinuousRW ) {

        ui->triggerLengthSpinBoxLabel->setText( "CRW freq(Hz)" );
        ui->triggerLengthSpinBox->setValue( _mpx3gui->getConfig()->getContRWFreq() );

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
    ui->layerSelector->clear();
    ui->histPlot->clear();
    ui->layerSelector->clear();
    //on_reload_all_layers();
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

    /*
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
*/

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

void QCstmGLVisualization::active_frame_changed(){

    //ui->layerSelector->addItem(QString("%1").arg(threshold));
    int layer = _mpx3gui->getDataset()->thresholdToIndex(this->getActiveThreshold());
    ui->glPlot->getPlot()->setActive(layer);
    ui->histPlot->setActive(layer);

    BuildStatsStringCounts( _mpx3gui->getDataset()->getActivePixels(getActiveThreshold()) );
    //ui->countsLabel->setText(QString("%1").arg(_mpx3gui->getDataset()->getActivePixels(getActiveThreshold())));

    if(ui->percentileRangeRadio->isChecked())
        on_percentileRangeRadio_toggled(true);
    else if(ui->fullRangeRadio->isChecked())
        on_fullRangeRadio_toggled(true);

}

void QCstmGLVisualization::region_selected(QPoint pixel_begin, QPoint pixel_end, QPoint position){

    //if(!_mpx3gui->getConfig()->isConnected())
    //    return;

    //int frameIndex = _mpx3gui->getDataset()->getContainingFrame(pixel_begin);
    //QPoint naturalCoords = _mpx3gui->getDataset()->getNaturalCoordinates(pixel_begin, frameIndex);
    //int naturalFlatCoord = naturalCoords.y()*_mpx3gui->getDataset()->x()+naturalCoords.x();
    //if(_mpx3gui->getConfig()->getColourMode()) {
    //	naturalFlatCoord = 4*naturalCoords.y()*_mpx3gui->getDataset()->x() + 2*naturalCoords.x();
    //}

    //int deviceID = _mpx3gui->getConfig()->getActiveDevices()[frameIndex];

    int threshold = getActiveThreshold();

    QMenu contextMenu;

    //Have the region only in the header:
    QLabel* label = new QLabel(QString("For region (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y())
                               , this);
    QWidgetAction wid(&contextMenu);
    wid.setDefaultWidget(label);
    contextMenu.addAction(&wid);

    QAction calcStats(QString("Calc stats"), &contextMenu);    //QAction calcStats(QString("Calc stats (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y()), &contextMenu);
    contextMenu.addAction(&calcStats);

    QAction calcProX(QString("ProfileX"), &contextMenu);    //QAction calcProX(QString("ProfileX (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y()), &contextMenu);
    contextMenu.addAction(&calcProX);

    QAction calcProY(QString("ProfileY"), &contextMenu);    //QAction calcProY(QString("ProfileY (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y()), &contextMenu);
    contextMenu.addAction(&calcProY);

    QAction gotoDQE(QString("Use for DQE"),&contextMenu);    //QAction gotoDQE(QString("Use for DQE (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y()), &contextMenu);
    contextMenu.addAction(&gotoDQE);


    // Show the menu
    QAction * selectedItem = contextMenu.exec(position);

    // Selected item
    if (selectedItem == &calcStats) {

        // Calc basic stats
        _mpx3gui->getDataset()->calcBasicStats(pixel_begin, pixel_end);

        // Now display it

        _statsdialog = new StatsDialog(this);
        _statsdialog->SetMpx3GUI(_mpx3gui);
        _statsdialog->changeText();
        _statsdialog->show();

    }

    else if (selectedItem == &gotoDQE){
        _mpx3gui->GetUI()->dqeTab->clearDataAndPlots();
        _mpx3gui->GetUI()->dqeTab->setRegion(pixel_begin, pixel_end);
        _mpx3gui->GetUI()->dqeTab->setSelectedThreshold(threshold);
        _mpx3gui->getDataset()->collectPointsROI(threshold, pixel_begin, pixel_end);
        _mpx3gui->GetUI()->stackedWidget->setCurrentIndex(__dqe_page_Id);
        _mpx3gui->GetUI()->dqeTab->plotESF();
    }

    else if(selectedItem == &calcProX || selectedItem == &calcProY) {

        QString axis;
        if(selectedItem == &calcProX) axis = "X";
        if(selectedItem == &calcProY) axis = "Y";

        //Display
        _profiledialog = new ProfileDialog(this);
        _profiledialog->SetMpx3GUI(_mpx3gui);
        _profiledialog->setRegion(pixel_begin, pixel_end);
        _profiledialog->setAxis(axis);
        _profiledialog->changeTitle();

        //        QList<int> thresholdlist = _mpx3gui->getDataset()->getThresholds();
        //        QStringList combolist;
        //        for(int i = 0; i < thresholdlist.length(); i++)
        //            combolist.append(QString("Threshold %1").arg(thresholdlist[i]));

        //Calculate the profile of the selected region of the selected layer

        _profiledialog->setSelectedThreshold(threshold);
        _profiledialog->setAxisMap(_mpx3gui->getDataset()->calcProfile(axis, threshold, pixel_begin, pixel_end));
        _profiledialog->plotProfile();

        _profiledialog->show();

    }

    //    delete label;
    //    delete a;
}

void QCstmGLVisualization::pixel_selected(QPoint pixel, QPoint position){

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
    QMenu contextMenu;
    QAction mask(QString("Mask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu), unmask(QString("Unmask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu);
    contextMenu.addAction(&mask);
    contextMenu.addAction(&unmask);
    QAction* selectedItem = contextMenu.exec(position);
    if(!_mpx3gui->getConfig()->isConnected())
        return;
    if(selectedItem == &mask){
        if(_mpx3gui->getConfig()->getColourMode()){

            qDebug() << "Nat[" << deviceID << "]:"
                     << naturalFlatCoord
                     << naturalFlatCoord+1
                     << naturalFlatCoord+_mpx3gui->getX()*2
                     << naturalFlatCoord+1+_mpx3gui->getX()*2;

            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord);
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord+1);
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord+_mpx3gui->getX()*2);
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord+1+_mpx3gui->getX()*2);
        }
        else
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->maskPixel(naturalFlatCoord);
    }
    else if(selectedItem == &unmask){
        if(_mpx3gui->getConfig()->getColourMode()){
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord);
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord+1);
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord+_mpx3gui->getX()*2);
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord+1+_mpx3gui->getX()*2);
        }
        else
            _mpx3gui->getEqualization()->GetEqualizationResults(deviceID)->unmaskPixel(naturalFlatCoord);
    }
    if(selectedItem != nullptr)
        _mpx3gui->getEqualization()->SetAllAdjustmentBits( _mpx3gui->getConfig()->getController(), deviceID, true);

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
            int size = _mpx3gui->getDataset()->getPixelsPerLayer();
            int min = INT_MAX, max = INT_MIN;
            for(int i = 0; i < size; i++) {
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
    cout << "[INDEX] " << threshold << " --> " << layer << endl;

    ui->glPlot->getPlot()->setActive(layer);
    ui->histPlot->setActive(layer);
    //_mpx3gui->set_active_frame(threshold);
    this->active_frame_changed();
}


void QCstmGLVisualization::on_summingCheckbox_toggled(bool checked)
{
    emit mode_changed(checked);
    //on_reload_all_layers();
}


void QCstmGLVisualization::on_saveBitmapPushButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Data"), QStandardPaths::displayName(QStandardPaths::PicturesLocation), tr("location"));

    // Get the image
    if ( _savePNGWithScales ) ui->glPlot->grab().save(filename+".png"); // with all children
    else ui->glPlot->getPlot()->grabFramebuffer().save(filename+".png"); // only image

    // And the histogram
    ui->histPlot->savePng(filename+"_histogram.png");
    QFile histogramDat(filename+"_histogram.dat");
    if(!histogramDat.open(QIODevice::WriteOnly)){
        qDebug() << histogramDat.errorString();
        return;
    }
    Histogram *hist = ui->histPlot->getHistogram(getActiveThreshold());
    for(int i = 0; i < hist->size();i++)
        histogramDat.write(QString("%1 %2\n").arg(hist->keyAt(i)).arg(hist->atIndex(i)).toStdString().c_str());

}


void QCstmGLVisualization::on_noisyPixelMeanMultiplier_valueChanged(double arg1)
{

}

void QCstmGLVisualization::on_correctionsDialogCheckBox_toggled(bool checked)
{

    if ( !_corrdialog && checked ) {

        // Setup corrections and be ready to apply them
        _corrdialog = new QCstmCorrectionsDialog(this);
        _corrdialog->SetMpx3GUI( _mpx3gui );

        // non Modal
        _corrdialog->show();
        _corrdialog->raise();
        _corrdialog->activateWindow();

    } else if ( _corrdialog && checked ) {

        // If already created
        _corrdialog->show();
        _corrdialog->raise();
        _corrdialog->activateWindow();

    }

    // Create the button to change corrections settings
    //  in case the corrections are selected but the dialogue
    //  has been closed by the user
    if ( checked && ! _extraWidgets.correctionsDialogueButton ) {
        _extraWidgets.correctionsDialogueButton = new QPushButton( "...", this );
        _extraWidgets.correctionsDialogueButton->setToolTip("change corrections settings");
        ui->dataTakingOptionsLayout->addWidget(
                    _extraWidgets.correctionsDialogueButton
                    );
        _extraWidgets.correctionsDialogueButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        // make a connection to functionality
        connect( _extraWidgets.correctionsDialogueButton , &QPushButton::clicked,
                 this, &QCstmGLVisualization::correctionDialogueButtonClicked);

    }
    // Get rid of the button if not checked
    if ( ! checked ) {
        int col_count = ui->dataTakingOptionsLayout->count();
        // we are trying to remove the very last item
        QLayoutItem * it = ui->dataTakingOptionsLayout->itemAt( col_count - 1 );
        ui->dataTakingOptionsLayout->removeItem( it );
        // disconnect and delete the widget
        disconnect( _extraWidgets.correctionsDialogueButton , &QPushButton::clicked,
                    this, &QCstmGLVisualization::correctionDialogueButtonClicked);

        delete _extraWidgets.correctionsDialogueButton;
        _extraWidgets.correctionsDialogueButton = nullptr;
    }

    //  Activate or deactivate the corrections
    _corrdialog->setCorrectionsActive( checked );

}

void QCstmGLVisualization::correctionDialogueButtonClicked()
{

    // Simply show the dialogue
    if ( _corrdialog ) {

        // If already created
        _corrdialog->show();
        _corrdialog->raise();
        _corrdialog->activateWindow();

    }

}


void QCstmGLVisualization::on_recoPushButton_clicked()
{

    _mpx3gui->rewindToOriginalDataset();

    // TODO, the location of the corrections will be moved to a separate window
    // Try the reconstruction

    // If previous reco available, delete.
    if ( _reco_Color2DRecoGuided ) delete _reco_Color2DRecoGuided;

    // Prepare the reconstruction handler
    _reco_Color2DRecoGuided = new Color2DRecoGuided( _mpx3gui );
    _reco_Color2DRecoGuided->LoadCrossSectionData();
    _reco_Color2DRecoGuided->BuildAndInvertMuMatrix();
    // Send off for reco
    _mpx3gui->getDataset()->applyColor2DRecoGuided( _reco_Color2DRecoGuided );

    // Now change layer names
    // This maps indx to material name
    QMap<int, QString> materials =_reco_Color2DRecoGuided->getMaterialMap();
    // This is the list of thresholds
    QList<int> thls = _mpx3gui->getDataset()->getThresholds();

    for ( int i = 0 ; i < thls.size() ; i++ ) {
        changeThresholdToNameAndUpdateSelector( thls[i], materials[i] );
    }

    reload_all_layers();

}

void QCstmGLVisualization::on_saveWithScaleCheckBox_toggled(bool checked)
{
    _savePNGWithScales = checked;
}

void QCstmGLVisualization::on_singleshotPushButton_clicked()
{
    // Temporarily change the configuration to a single shot
    _singleShot = true;
    _singleShotSaveCurrentNTriggers = _mpx3gui->getConfig()->getNTriggers();

    // Select only one trigger
    ui->nTriggersSpinBox->setValue( 1 );

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
}


void QCstmGLVisualization::on_multiThresholdAnalysisPushButton_clicked()
{

    if ( ! _mtadialog ) {
        _mtadialog = new MTADialog(_mpx3gui, this);
        connect(_mtadialog, &MTADialog::finished, this, &QCstmGLVisualization::on_MTAClosed);
    }

    _mtadialog->show(); // modeless

}

void QCstmGLVisualization::on_MTAClosed()
{

    if ( _mtadialog ) {
        disconnect(_mtadialog, &MTADialog::finished, this, &QCstmGLVisualization::on_MTAClosed);
        delete _mtadialog;
        _mtadialog = nullptr;
    }

}

void QCstmGLVisualization::on_testPulsesClosed()
{

    if ( _testPulsesDialog ) {
        disconnect(_mtadialog, &MTADialog::finished, this, &QCstmGLVisualization::on_testPulsesClosed);
        delete _testPulsesDialog;
        _testPulsesDialog = nullptr;
    }

}

void QCstmGLVisualization::on_testPulsesPushButton_clicked()
{

    if ( ! _testPulsesDialog ) {

        _testPulsesDialog = new TestPulses(_mpx3gui, this);
        connect(_testPulsesDialog, &MTADialog::finished, this, &QCstmGLVisualization::on_testPulsesClosed);

    }

    _testPulsesDialog->show(); // modeless

}


void QCstmGLVisualization::on_dropFramesCheckBox_clicked(bool checked)
{
    _dropFrames = checked;
}

