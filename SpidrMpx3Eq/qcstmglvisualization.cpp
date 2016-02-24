#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"
#include "qcstmequalization.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "DataTakingThread.h"

#include "qcstmcorrectionsdialog.h"

#include "color2drecoguided.h"

#include <stdio.h>
#include <QDialog>
#include <QDebug>

QCstmGLVisualization::QCstmGLVisualization(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmGLVisualization)
{

    ui->setupUi(this);
    _dataTakingThread = 0x0;
    _takingData = false;
    _busyDrawing = false;
    // By default don't drop frames
    ui->dropFramesCheckBox->setChecked( false );
    _etatimer = 0x0;
    _timer = 0x0;
    _estimatedETA = 0;

    // Defaults from GUI
    ui->dropFramesCheckBox->setChecked( true );
    ui->summingCheckbox->setChecked( true );

    // packets lost counter
    //QFont font1("Courier New");
    //ui->lostPacketsLabel->setFont( font1 );
    ui->lostPacketsLabel->setTextFormat( Qt::RichText );

    // Range selection on histogram. Init values
    _manualRange = QCPRange( 0, 1 ); // This is quite arbitrary. It doesn't really matter here.
    _percentileRange = QCPRange( 0.025, 0.975 ); // These instead are reasonable percentile cuts
    setRangeSpinBoxesPercentile();

    // Log Scale for histogram
    connect(ui->logCheckBox, SIGNAL(clicked(bool)), this, SLOT(on_logscale(bool)));


}

QCstmGLVisualization::~QCstmGLVisualization() {
    delete ui;
}

void QCstmGLVisualization::FlipBusyState() {

    if( _busyDrawing ) {
        _busyDrawing = false;
        emit free_to_draw();
    } else {
        _busyDrawing = true;
        emit busy_drawing();
    }

}

void QCstmGLVisualization::UnlockWaitingForFrame() {
    cout << "..." << endl;
}

void QCstmGLVisualization::updateETA() {

    // Recalculate ETA first
    //_estimatedETA = _mpx3gui->getConfig()->getTriggerPeriodMS() *  _mpx3gui->getConfig()->getNTriggers(); // ETA in ms
    //_estimatedETA += _estimatedETA * __networkOverhead; // add ~10% network overhead.  FIXME  to be calculated at startup

    // show eta in display
    // h must be in the range 0 to 23, m and s must be in the range 0 to 59, and ms must be in the range 0 to 999.
    QTime n(0, 0, 0);                // n == 00:00:00
    QTime t(0, 0, 0);
    int diff = _estimatedETA - _etatimer->elapsed();
    if (diff > 0) t = n.addMSecs( _estimatedETA - _etatimer->elapsed() );
    QString textT = t.toString("hh:mm:ss");
    ui->etaCntr->setText( textT );

}


void QCstmGLVisualization::StartDataTaking(){

    // The Start button becomes the Stop button

    if ( !_takingData ) {

        // Clear previous data first
        GetMpx3GUI()->clear_data( false );

        // Threads
        if ( _dataTakingThread ) {
            if ( _dataTakingThread->isRunning() ) {
                return;
            }
            //disconnect(_senseThread, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );
            delete _dataTakingThread;
            _dataTakingThread = 0x0;
        }

        // Create the thread
        _dataTakingThread = new DataTakingThread(_mpx3gui, this);
        _dataTakingThread->ConnectToHardware();

        // Change the Start button to Stop
        ui->startButton->setText( "Stop" );
        ui->overflowLabel->setText( "" );

        // Start data taking
        // FIXME, depends on the mode !
        _estimatedETA = _mpx3gui->getConfig()->getTriggerPeriodMS() *  _mpx3gui->getConfig()->getNTriggers(); // ETA in ms
        _estimatedETA += _estimatedETA * __networkOverhead; // add ~10% network overhead.  FIXME  to be calculated at startup

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
        _takingData = false;

        // force the GUI to update
        update();

        // Finish
        DestroyTimer();
        ETAToZero();

        emit sig_statusBarAppend("stop","orange");
    }

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

void QCstmGLVisualization::data_taking_finished(int /*nFramesTaken*/) {

    if ( _takingData ) {

        _takingData = false;
        DestroyTimer();
        ETAToZero();

        // When finished taking data save the original data
        _mpx3gui->saveOriginalDataset();

        // Corrections
        if( _corrdialog ) _mpx3gui->getDataset()->applyCorrections( _corrdialog );

        // And replot
        reload_all_layers();

        // Change the Stop button to Start
        ui->startButton->setText( "Start" );

        // If single shot, recover previous NTriggers
        if ( _singleShot ) {
            _mpx3gui->getConfig()->setNTriggers( _singleShotSaveNTriggers );
            _singleShot = false;
            _singleShotSaveNTriggers = 0;
        }

        emit sig_statusBarAppend("done","blue");

    }

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


void QCstmGLVisualization::ConnectionStatusChanged() {

    ui->startButton->setEnabled(true); // Enable or disable the button depending on the connection status.
    ui->singleshotPushButton->setEnabled(true);

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

    // Defaults
    emit mode_changed(ui->summingCheckbox->isChecked());
}

void QCstmGLVisualization::changeBinCount(int count){
    ui->histPlot->changeBinCount(count);
    QList<int> thresholds = _mpx3gui->getDataset()->getThresholds();
    for(int i = 0; i < thresholds.size(); i++){
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

        QString ovfS = "<font color=\"red\">";
        ovfS += "overflow</font>";
        ui->overflowLabel->setText( ovfS );

        // Bring the user to full range so the hot spots can be seen
        ui->fullRangeRadio->setChecked( true );
        on_fullRangeRadio_toggled(true);

    } else {
        ui->overflowLabel->setText( "" );
    }

}

void QCstmGLVisualization::lost_packets(int packetsLost) {

    // Increase the current packet loss
    _mpx3gui->getDataset()->increasePacketsLost( packetsLost );

    // Retrieve the counter and display
    QString plS = "<font color=\"red\">";
    plS += QString::number( _mpx3gui->getDataset()->getPacketsLost(), 'd', 0 );
    plS += "</font>";

    ui->lostPacketsLabel->setText( plS );

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
    //int layer = _mpx3gui->getDataset()->thresholdToIndex(threshold);
    ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset()); //TODO: only read specific layer.
    ui->histPlot->setHistogram(threshold, _mpx3gui->getDataset()->getLayer(threshold), _mpx3gui->getDataset()->getPixelsPerLayer());
    setThreshold(threshold);
    active_frame_changed();
}


void QCstmGLVisualization::progress_signal(int framecntr) {

    QString prog = QString("%1/%2").arg( framecntr ).arg(_mpx3gui->getConfig()->getNTriggers() );
    ui->frameCntr->setText( prog );

}


void QCstmGLVisualization::reload_all_layers(){

    // Get busy
    emit FlipBusyState();

    ui->glPlot->getPlot()->readData(*_mpx3gui->getDataset()); //TODO: only read specific layer.
    QList<int> thresholds = _mpx3gui->getDataset()->getThresholds();
    for(int i = 0; i < thresholds.size(); i++){
        addThresholdToSelector(thresholds[i]);
        ui->histPlot->setHistogram(thresholds[i], _mpx3gui->getDataset()->getLayer(thresholds[i]), _mpx3gui->getDataset()->getPixelsPerLayer());
    }
    //setThreshold(thresholds[0]);
    active_frame_changed();

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
    //ui->overflowLabel->setText(QString("%1").arg(_mpx3gui->getDataset()->getOverflow(getActiveThreshold())));
    ui->countsLabel->setText(QString("%1").arg(_mpx3gui->getDataset()->getActivePixels(getActiveThreshold())));
    if(ui->percentileRangeRadio->isChecked())
        on_percentileRangeRadio_toggled(true);
    else if(ui->fullRangeRadio->isChecked())
        on_fullRangeRadio_toggled(true);

    // Get free
    emit FlipBusyState();

}

void QCstmGLVisualization::region_selected(QPoint pixel_begin, QPoint pixel_end, QPoint position){

    if(!_mpx3gui->getConfig()->isConnected())
        return;

    //int frameIndex = _mpx3gui->getDataset()->getContainingFrame(pixel_begin);
    //QPoint naturalCoords = _mpx3gui->getDataset()->getNaturalCoordinates(pixel_begin, frameIndex);
    //int naturalFlatCoord = naturalCoords.y()*_mpx3gui->getDataset()->x()+naturalCoords.x();
    //if(_mpx3gui->getConfig()->getColourMode()) {
    //	naturalFlatCoord = 4*naturalCoords.y()*_mpx3gui->getDataset()->x() + 2*naturalCoords.x();
    //}

    //int deviceID = _mpx3gui->getConfig()->getActiveDevices()[frameIndex];
    QMenu contextMenu;
    QAction calcStats(QString("Calc stats (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y()), &contextMenu);
    contextMenu.addAction(&calcStats);

    // Show the menu
    QAction * selectedItem = contextMenu.exec(position);

    // Selected item
    if (selectedItem == &calcStats) {
        // Basic stats
        _mpx3gui->getDataset()->calcBasicStats(pixel_begin, pixel_end);
    }

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

void QCstmGLVisualization::on_lowerPercentileSpin_editingFinished()
{
    //if(ui->lowerPercentileSpin->value() > ui->upperPercentileSpin->value())
    //    ui->upperPercentileSpin->setValue(ui->lowerPercentileSpin->value());

    if(ui->lowerSpin->value() > ui->upperSpin->value())
        ui->upperSpin->setValue(ui->lowerSpin->value());

    if(ui->percentileRangeRadio->isChecked())
        on_percentileRangeRadio_toggled(ui->percentileRangeRadio->isChecked());
}

void QCstmGLVisualization::on_upperPercentileSpin_editingFinished()
{
    //if(ui->upperPercentileSpin->value() < ui->lowerPercentileSpin->value())
    //    ui->lowerPercentileSpin->setValue(ui->upperPercentileSpin->value());

    if(ui->upperSpin->value() < ui->lowerSpin->value())
        ui->lowerSpin->setValue(ui->upperSpin->value());

    if(ui->percentileRangeRadio->isChecked())
        on_percentileRangeRadio_toggled(ui->percentileRangeRadio->isChecked());
}

void QCstmGLVisualization::on_lowerManualSpin_editingFinished()
{
    //if(ui->upperManualSpin->value() < ui->lowerManualSpin->value())
    //    ui->upperManualSpin->setValue(ui->lowerManualSpin->value());

    if(ui->upperSpin->value() < ui->lowerSpin->value())
        ui->upperSpin->setValue(ui->lowerSpin->value());

    if(ui->manualRangeRadio->isChecked())
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());

}

void QCstmGLVisualization::on_upperManualSpin_editingFinished()
{
    //if(ui->lowerManualSpin->value() > ui->upperManualSpin->value())
    //    ui->lowerManualSpin->setValue(ui->upperManualSpin->value());

    if(ui->lowerSpin->value() > ui->upperSpin->value())
        ui->lowerSpin->setValue(ui->upperSpin->value());

    if(ui->manualRangeRadio->isChecked())
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
}

void QCstmGLVisualization::on_manualRangeRadio_toggled(bool checked)
{

    // Before toogle save the current range to percentile
    // If unselecting save the info
    if ( !checked ) _manualRange = QCPRange( ui->lowerSpin->value(), ui->upperSpin->value() );

    if ( checked ) {

        // When toggling here recompute the min and max
        // if the range is still the initial (0,1)

        if ( _manualRange == QCPRange( 0,1 ) ) {
            // current threshold
            int activeTHL = getActiveThreshold();
            if ( activeTHL >= 0 ) {
                _manualRange = QCPRange( ui->histPlot->getMin( activeTHL ),
                                         ui->histPlot->getMax( activeTHL )
                                         );
            } else {
                _manualRange = QCPRange( 0,1 );
            }
        }

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
            // when toggling here recompute the min and max
            _manualRange = QCPRange( ui->histPlot->getMin( activeTHL ),
                                     ui->histPlot->getMax( activeTHL )
                                     );
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
        ui->histPlot->set_scale_percentile(getActiveThreshold(),
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

    //  Activate or deactivate the corrections
    _corrdialog->setCorrectionsActive( checked );

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
    _singleShotSaveNTriggers =  _mpx3gui->getConfig()->getNTriggers();
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
        _manualRange = QCPRange( ui->lowerSpin->value(), ui->upperSpin->value() );
        ui->manualRangeRadio->setChecked( true );
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
        return;
    }

    if(ui->manualRangeRadio->isChecked()) {
        _manualRange = QCPRange( ui->lowerSpin->value(), ui->upperSpin->value() );
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
    }

    if(ui->percentileRangeRadio->isChecked()) {
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
        _manualRange = QCPRange( ui->lowerSpin->value(), ui->upperSpin->value() );
        ui->manualRangeRadio->setChecked( true );
        on_manualRangeRadio_toggled(ui->manualRangeRadio->isChecked());
        return;
    }

    if(ui->manualRangeRadio->isChecked()) {
        _manualRange = QCPRange( ui->lowerSpin->value(), ui->upperSpin->value() );
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

