#include "qcstmthreshold.h"
#include "ui_qcstmthreshold.h"
#include "qcstmdacs.h"
#include <iterator>
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "mpx3dacsdescr.h"

#include "mpx3gui.h"
#include "mpx3config.h"
#include "mpx3defs.h"
#include "mpx3eq_common.h"
#include "ui_mpx3gui.h"

QCstmThreshold::QCstmThreshold(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmThreshold)
{
    ui->setupUi(this);
    QList<int> defaultSizesMain; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".
    defaultSizesMain.append(2971215);
    defaultSizesMain.append(1836312);
    for(int i = 0; i < ui->splitter->count();i++){
        QWidget *child = ui->splitter->widget(i);
        child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        child->setMinimumSize(1,1);
    }
    ui->splitter->setSizes(defaultSizesMain);

    _scanThread = nullptr;
    _plotIdxCntr = 0;

    // Signals & Slots
    SetupSignalsAndSlots();

    ui->plot->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );
    // The legend
    ui->plot->legend->setVisible( false ); // Nothing there yet...
    QFont f = ui->plot->font();
    f.setPointSize( 7 ); // and make a bit smaller for legend
    ui->plot->legend->setFont( f );
    ui->plot->legend->setBrush( QBrush(QColor(255,255,255,230)) );
    // The axes
    ui->plot->xAxis->setRange( 0, 511 ); // Maximum DAC range
    ui->plot->yAxis->setRange( 0, 10000 );
    f = ui->plot->font();  // Start out with Dialog's font..
    f.setBold( true );
    ui->plot->xAxis->setLabelFont( f );
    ui->plot->yAxis->setLabelFont( f );
    // The labels:
    ui->plot->xAxis->setLabel(defaultXLabel);
    ui->plot->yAxis->setLabel(defaultYLabel);

    /*
    ui->framePlot->axisRect()->setupFullAxesBox(true);
    QCPColorScale *colorScale = new QCPColorScale(ui->framePlot);
    ui->framePlot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
     */
    for(int i = 0; i < MPX3RX_DAC_COUNT;i++){
        ui->scanTargetComboBox->addItem(MPX3RX_DAC_TABLE[i].name);
    }

    _scanDescendant = true;
    _keepPlots = true;
    _logyPlot = false;

}

QCstmThreshold::~QCstmThreshold()
{
    delete ui;
}

void QCstmThreshold::on_rangeDirectionCheckBox_toggled(bool checked) {
    _scanDescendant = checked;
}

void QCstmThreshold::on_keepCheckbox_toggled(bool checked) {
    _keepPlots = checked;
}

void QCstmThreshold::on_logyCheckBox_toggled(bool checked) {

    _logyPlot = checked;

    if ( _logyPlot ) ui->plot->yAxis->setScaleType( QCPAxis::stLogarithmic );
    else ui->plot->yAxis->setScaleType( QCPAxis::stLinear );

    ui->plot->replot();
}

void QCstmThreshold::on_thlCalibDifferentiateCheckBox_toggled(bool checked)
{
    if (checked){
        //! Smooth threshold scan then calculate and show 5 point stencil differentiation.
        //! Change y units to "dC/dTHL"

        ui->plot->yAxis->setLabel(altYLabel);
        //int graphNumber = ui->plot->graphCount();

        for ( int i = 0 ; i < _plotIdxCntr ; i++ ) {
            QCPGraph * graph = ui->plot->graph( i );
            QMap<double, QCPData> * dm = graph->data();
            QMap<double, QCPData>::const_iterator itr  = dm->begin();
            QMap<double, QCPData>::const_iterator itrE = dm->end();
            std::vector<int>    x;
            std::vector<double> y;

            //! Extract points in a useful way
            for ( ; itr != itrE ; itr++ ) {
                x.push_back( int    ((*itr).key)  );
                y.push_back( double ((*itr).value));
            }

            //! Test data
//            std::vector<int>    x = {0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200};
//            std::vector<double> y = {64452,65295.3,65477.7,65505.3,65509,65509,65509,65510,65510,65510,65510,65510,65510,65510,65510,65510,65509.7,65509.3,65509.7,65508.3,65509,65508,65508,65508,65507.7,65507,65506.3,65505.7,65504.3,65505.7,65503.7,65500,65496.3,65499,65491.7,65491,65479,65481.3,65470.7,65462.7,65450.3,65425.7,65406.3,65388.3,65348.7,65310,65273.3,65186.3,65087.3,64962.7,64764.3,64537.3,64228.3,63711,63111,62291,61135.3,59679.3,58064.7,55990.7,53583.7,51015.3,48145.3,45225,42253,39526.3,36757,34447.3,31789,29294.3,26869.3,24247,21741,19423,17131,14856,12924,10889,9241.67,7796,6305,5058,4012.33,3127.33,2390.33,1788.33,1324.67,938.667,678.333,445.333,327.333,216.667,145.667,114.333,80.6667,55.3333,38,26.6667,19,18,12.3333};

            std::vector<double> f = derivativeFivePointStencil(x,y);
//            qDebug() << f.size() << " | " << x.size();

            //! Put points from array on graph
            for ( int i = 0; i < x.size() ; i++ ) {
//                qDebug() << x[j] << f[j];
                setPoint(QPointF(x[i],f[i]), _plotIdxCntr);
            }
            QCPGraph * graphNew = ui->plot->graph( i+1 );
            auto biggest = std::max_element(std::begin(f), std::end(f));
            auto peak = std::min_element(std::begin(f), std::end(f)) + std::distance(std::begin(f), biggest);
            double xAtPeak = 0;

            for ( int i = 0; i < x.size() ; i++ ) {
                if (f[i] == *peak){
                    auto minX = std::min_element(std::begin(x), std::end(x));
                    xAtPeak = i + *minX;
                }
            }
            graphNew->setName( "dC/dTHL Peak @ " + QString::number(xAtPeak) + ", " + QString::number(*peak) );
        }


    } else {
        //! Hide differentiation and switch back to raw data
        ui->plot->yAxis->setLabel(defaultYLabel);

        ui->plot->removeGraph((_plotIdxCntr));
    }

    ui->plot->rescaleAxes();
    ui->plot->replot();
}

int QCstmThreshold::getActiveTargetCode() {
    return MPX3RX_DAC_TABLE[ui->scanTargetComboBox->currentIndex()].code;
}

QString QCstmThreshold::getActiveTargetName() {
    return QString(MPX3RX_DAC_TABLE[ui->scanTargetComboBox->currentIndex()].name);
}

void QCstmThreshold::StartCalibration() {

    //if( _scanThread ) delete _scanThread;
    if ( _scanThread ) {
        if ( _scanThread->isRunning() ) {
            return;
        }
        //disconnect(_senseThread, SIGNAL( progress(int) ), ui->progressBar, SLOT( setValue(int)) );
        delete _scanThread;
        _scanThread = nullptr;
    }

    if ( ! _keepPlots ) {
        // clear graphs and rewind
        ui->plot->clearGraphs();
        _plotIdxMap.clear();
        _plotIdxCntr = 0;
    }

    // Look at the spacing
    if ( ui->spacingSpinBox->value() == 0 ) {

        SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

        for (int i = 0 ; i < __array_size_x ; i++) {
            for (int j = 0 ; j < __array_size_y ; j++) {
                spidrcontrol->setPixelMaskMpx3rx(i, j, false);
            }
        }
        // And send the configuration
        spidrcontrol->setPixelConfigMpx3rx( ui->devIdSpinBox->value() );
    }

    //cout << "adding : " << getActiveTargetName().toStdString() << endl;
    addData( _plotIdxCntr );

    QCPGraph * _graph = ui->plot->graph( _plotIdxCntr-1 ); // the right vector index

    QPen pen( COLOR_TABLE[ _plotIdxCntr-1 ] );
    pen.setWidth( 0.1 );
    _graph->setPen( pen );
    _graph->setName( getActiveTargetName() ); // the dac index

    // Prepare plot
    //ui->plot->xAxis->setLabel( getActiveTargetName() );
    //ui->plot->clearGraphs();
    //ui->plot->legend->setVisible( false );
    ui->plot->replot();

    // Configure the chip.  Custom configuration for a scan
    Mpx3Config::extra_config_parameters expars;
    expars.nTriggers = ui->nTriggersSpinBox->value();
    expars.equalizationBit = false;
    // Debug bits in this context.  Disc_CSM_SPM.
    if ( ui->setDiscCsmSpmBitCheckBox->isChecked() ) { expars.DiscCsmSpm = 0x1; }
    else { expars.DiscCsmSpm = 0x0; }
    // Debug bits in this context.  Equalization bit.
    if ( ui->setEqualizationBitCheckBox->isChecked() ) { expars.equalizationBit = true; }

    // Send the configuration
    _mpx3gui->getConfig()->Configuration( false, ui->devIdSpinBox->value(), expars);

    // Go on with the scan thread
    _scanThread = new CustomScanThread( _mpx3gui, this );
    _scanThread->ConnectToHardware();

    _scanThread->start();

}

void QCstmThreshold::SetupSignalsAndSlots() {

    //std::cout << "[QCstmThreshold] Connecting signals and slots \n";
    connect( ui->thlCalibStart, SIGNAL(clicked()), this, SLOT( StartCalibration() ) );

}

int QCstmThreshold::ExtractScanInfo(int * data, int size_in_bytes, int /*thl*/) {

    int nPixels = size_in_bytes/4;
    int pixelsActive = 0;
    // Each 32 bits corresponds to the counts in each pixel already
    // in 'int' representation as the decoding has been requested
    for(int i = 0 ; i < nPixels ; i++) {
        if ( data[i] != 0 ) {
            pixelsActive++;
        }
    }

    return pixelsActive;
}

void QCstmThreshold::UpdateChart(int /*setId*/, int /*thlValue*/) {
    /*
    map<int, int>::iterator itr = _pixelCountsMap.begin();
    map<int, int>::iterator itrE = _pixelCountsMap.end();

    // I am going to plot for this threshold the number of
    //  pixels which reached _nTriggers counts.  The next time
    //  they won't be considered.
    int cntr = 0;
    for( ; itr != itrE ; itr++ ) {

        if( (*itr).second ==  _equalization->GetNTriggers() ) {
            cntr++;
            (*itr).second++; // This way we avoid re-ploting next time. The value _nTriggers+1 identifies these pixels
        }

    }

    _chart->SetValueInSet( setId , thlValue, cntr );
     */
}

//void QCstmThreshold::UpdateHeatMap() {
//	addFrame(QPoint(0,0), 0, _data);
//}

/**
 * 		offset: corner of the quad
 * 		layer: threshold layer
 * 		data: the actual data
 */

void QCstmThreshold::addFrame(QPoint offset, int layer, int* data){

    while(layer >= ui->framePlot->plottableCount())
        ui->framePlot->addPlottable(new QCPColorMap(ui->framePlot->xAxis, ui->framePlot->yAxis));
    int nx = _mpx3gui->getDataset()->x(),  ny =_mpx3gui->getDataset()->y(); //TODO: grab from config.
    for(int i = 0; i < ny; i++) {
        for(int j = 0; j < nx; j++){
            ((QCPColorMap*)ui->framePlot->plottable(layer))->data()->setCell(j+offset.x()*nx, ny-1-i+offset.y()*ny, data[i*nx+j]);
        }
    }

    ui->framePlot->rescaleAxes();
    ui->framePlot->replot();
    ui->framePlot->repaint();

}

void QCstmThreshold::setPoint(QPointF data, int plot){
    while(plot >= ui->plot->graphCount())
        ui->plot->addGraph();
    ui->plot->graph(plot)->removeData(data.x());
    ui->plot->graph(plot)->addData(data.x(), data.y());
    ui->plot->rescaleAxes();
    ui->plot->replot();
}

double QCstmThreshold::getPoint(int x, int plot){
    if(plot >= ui->plot->graphCount())
        return 0;
    ui->plot->graph(plot)->data()->find(x);
    if(ui->plot->graph(plot)->data()->find(x) == ui->plot->graph(plot)->data()->end())
        return 0;
    return ui->plot->graph(plot)->data()->find(x).value().value;
}

void QCstmThreshold::addPoint(QPointF data, int plot){
    return setPoint(QPointF(data.x(), data.y()+getPoint(data.x(),plot)), plot);
}

void QCstmThreshold::addData(int dacIdx, int dacVal, double adcVal ) {

    _graph = ui->plot->graph( _plotIdxMap[dacIdx] ); // the right vector index
    _graph->addData( dacVal, adcVal );
    ui->plot->yAxis->rescale( true );
    ui->plot->xAxis->rescale( true );

    ui->plot->replot();

    // Actualize Slider, SpinBoxes, and Labels
    //_dacSpinBoxes[dacIdx]->setValue( dacVal );
    //_dacSliders[dacIdx]->setValue( dacVal );
    // Labels are updated already through a SIGNAL/SLOT from the thread.

}

void QCstmThreshold::addData(int dacIdx) {

    _plotIdxMap[dacIdx] = _plotIdxCntr;
    _plotIdxCntr++;

    // If starting a plot, create it first
    ui->plot->addGraph();
    _graph = ui->plot->graph( _plotIdxMap[dacIdx] ); // the right vector index

    QPen pen( COLOR_TABLE[ dacIdx ] );
    pen.setWidth( 0.1 );
    _graph->setPen( pen );
    _graph->setName( QString(MPX3RX_DAC_TABLE[dacIdx].name) ); // the dac index

    // Do not use legend.  I will color the text boxes instead.
    //_graph->addToLegend();
    ui->plot->legend->setVisible( true );

}

std::vector<double> QCstmThreshold::derivativeFivePointStencil(std::vector<int> x, std::vector<double> f)
{
    std::vector<double> f_prime;

    for (int i = 0 ; i < x.size() ; i++) {
        if (i < (x.size() - 2)){
            double der;
            der +=   -f[i + 2]; // f1
            der +=  8.*f[i    ]; // f2
            der += -8.*f[i    ]; // f3
            der +=    f[i - 2]; // f4
            der /= 12.;
            //qDebug() << x[i] << ", "<< f[i] << ", " << der;
            // We don't care about negative values at all
            if (der < 0){
                der = 0;
            }
            f_prime.push_back(der);
        } else {
            f_prime.push_back(0);
        }
    }

    auto f_primeMax = std::max_element(std::begin(f_prime), std::end(f_prime));
    auto distToMax = std::distance(std::begin(f_prime), f_primeMax);
    double fPeak = f[distToMax];
    auto a = *f_primeMax / fPeak;

    //! Normalise to max of input (x)
    // Meant to be f_prime = f_prime * a but this isn't Python
    for (int i = 0 ; i < x.size() ; i++) {
        f_prime[i] = f_prime[i] / a;
    }
    return f_prime;
}

void QCstmThreshold::on_pushButtonSave_clicked()
{
    // Save the different groups of data
    for ( int i = 0 ; i < _plotIdxCntr ; i++ ) {

        QString fn = "scan_";
        fn += QString::number(i);
        fn += ".txt";
        qDebug() << "[DUMP] plot : " << i << " | file : " << fn;

        std::ofstream ofs (fn.toStdString(), std::ofstream::out);

        QCPGraph * graph = ui->plot->graph( i );
        QMap<double, QCPData> * dm = graph->data();
        QMap<double, QCPData>::const_iterator itr  = dm->begin();
        QMap<double, QCPData>::const_iterator itrE = dm->end();
        for ( ; itr != itrE ; itr++ ) {
            ofs << (*itr).key << "\t" << (*itr).value << "\n";
        }
        ofs.close();
    }

}

void QCstmThreshold::on_thlCalibStop_clicked()
{
    //qDebug() << ">> Stop clicked";
    if (_scanThread->isRunning()) {
        //qDebug() << ">> Scan is running, let's kill this biatch";
        _scanThread->setAbort(true);
    } else {
        //qDebug() << ">> Scan is not running, do NOTHING";
        return;
    }
}



CustomScanThread::CustomScanThread(Mpx3GUI * mpx3gui, QCstmThreshold * cstmThreshold) {

    _mpx3gui = mpx3gui;
    _cstmThreshold = cstmThreshold;
    _ui = _cstmThreshold->GetUI();

}

void CustomScanThread::ConnectToHardware() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    if( spidrcontrol ) { spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

    // The chart and the heatmap !
    //_chart = _equalization->GetUI()->_histoWidget;
    _heatmap = _cstmThreshold->GetUI()->framePlot;

}

void CustomScanThread::UpdateHeatMap(int sizex, int sizey) {

    //_heatmap->addData(_data, sizex, sizey);	// Add a new plot/frame.
    _heatmap->setData( _data, sizex, sizey);
    //_heatmap->setActive(_frameId++); 		// Activate the last plot (the new one)

}

void CustomScanThread::run() {

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

    //! Send the existing equalization
    //! You actually need this for some reason...
    _mpx3gui->getEqualization()->SetAllAdjustmentBits(spidrcontrol);

    // Configure, no reset
    //_mpx3gui->getConfig()->Configuration( false, 2, _ui->nTriggersSpinBox->value() );
    //_mpx3gui->getConfig()->Configuration( false, 3, _ui->nTriggersSpinBox->value() );

    //SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    //SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    //connect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );

    connect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
    connect( this, SIGNAL( addData(int, int, double) ), _cstmThreshold, SLOT( addData(int, int, double) ) );
    connect( this, SIGNAL( addData(int) ), _cstmThreshold, SLOT( addData(int) ) );

    int dacCodeToScan = _cstmThreshold->getActiveTargetCode();
    int minScan = _ui->eqMinSpinBox->value();
    int maxScan = _ui->eqMaxSpinBox->value();
    int stepScan = _ui->eqStepSpinBox->value();
    int deviceIndex = _ui->devIdSpinBox->value();

    bool doReadFrames = true;
    int pixelsReactive = 0;
    int startReacting = 0, turnonTHL = 0;
    bool turnonReached = false;

    int nReps = 0;



    // Scan iterator observing direction
    int dacItr = minScan;
    if ( _cstmThreshold->isScanDescendant() ) dacItr = maxScan;

    for ( ; scanContinue ; ) {


        // Set Dac
        if ( _cstmThreshold->GetUI()->onAllChipsCheckBox->isChecked() ) {
            for(int i = 0 ; i < _mpx3gui->getConfig()->getNActiveDevices() ; i++) {
                if ( ! _cstmThreshold->GetMpx3GUI()->getConfig()->detectorResponds( i ) ) {
                    qDebug() << "[ERR ] Device " << i << " not responding.";
                } else {
                    spidrcontrol->setDac( i, dacCodeToScan, dacItr );
                }
            }
        } else {
            spidrcontrol->setDac( deviceIndex, dacCodeToScan, dacItr );
        }

        // Adjust the sliders and the SpinBoxes to the new value
        connect( _cstmThreshold, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
        // Get the DAC back just to be sure and then slide&spin
        int dacVal = 0;
        spidrcontrol->getDac( deviceIndex, dacCodeToScan, &dacVal);
        // SlideAndSpin works with the DAC index, no the code.
        int dacIndex = _mpx3gui->GetUI()->DACsWidget->GetDACIndex( dacCodeToScan );
        _cstmThreshold->slideAndSpin( dacIndex,  dacVal );
        disconnect( _cstmThreshold, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );


        // Scan info
        QString scanVal;
        scanVal =  QString::number( dacItr, 'd', 0 );
        connect( this, SIGNAL( fillText(QString) ), _cstmThreshold->GetUI()->scanningLabel, SLOT( setText(QString)) );
        fillText( scanVal );
        disconnect( this, SIGNAL( fillText(QString) ), _cstmThreshold->GetUI()->scanningLabel, SLOT( setText(QString)) );

        // Measure
        // Start the trigger as configured
        spidrcontrol->startAutoTrigger();

        // See if there is a frame available
        // I should get as many frames as triggers

        //while ( _spidrdaq->hasFrame() ) {
        doReadFrames = true;
        nReps = 0;
        pixelsReactive = 0;

        // Timeout
        int timeOutTime =
                _mpx3gui->getConfig()->getTriggerLength_ms()
                + _mpx3gui->getConfig()->getTriggerDowntime_ms()
                + 50; // ms
        // TODO ! The extra 500ms is a combination of delay in the network plus
        // system overhead.  This should be predicted and not hard-coded. TODO !

        while ( spidrdaq->hasFrame( timeOutTime ) ) {

            QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();

            // A frame is here
            doReadFrames = true;

            // Check quality, if packets lost don't consider the frame
            //for(int i = 0 ; i < _mpx3gui->getConfig()->getNActiveDevices() ; i++) {
            //if ( ! _cstmThreshold->GetMpx3GUI()->getConfig()->detectorResponds( i ) ) {
            //if ( spidrdaq->packetsLostCountFrame( ) != 0 ) { // from any of the chips connected
            if ( spidrdaq->lostCountFrame() != 0 ) {
                doReadFrames = false;
            }
            //}
            //}

            if ( doReadFrames ) {
                int size_in_bytes = -1;

                // On all active chips
                if ( _cstmThreshold->GetUI()->onAllChipsCheckBox->isChecked() ) {
                    for(int i = 0 ; i < activeDevices.size() ; i++) {
                        //cout << i << endl;
                        _data = spidrdaq->frameData(i, &size_in_bytes);
                        pixelsReactive += PixelsReactive( _data, size_in_bytes, dacItr );
                    }
                } else {

                    // On a single chip scan
                    int chipScanId = _cstmThreshold->GetUI()->devIdSpinBox->value();
                    int dataIdForChip = _mpx3gui->getConfig()->getIndexFromID( chipScanId );
                    _data = spidrdaq->frameData(dataIdForChip, &size_in_bytes);
                    pixelsReactive += PixelsReactive( _data, size_in_bytes, dacItr );

                }

                // TEMP .. TODO
                // Tring to find the turn-on point
                if ( startReacting < 10 ) {
                    startReacting += PixelsReactive( _data, size_in_bytes, dacItr );
                } else {
                    if ( ! turnonReached ) {
                        turnonTHL = dacItr;
                        turnonReached = true;
                    }
                }
                nReps++;
            }

            // Release
            spidrdaq->releaseFrame();


            // Report to heatmap
            if ( doReadFrames ) {
                UpdateHeatMapSignal(_mpx3gui->getDataset()->x(), _mpx3gui->getDataset()->y());
            }
            /*
            // plot
            if ( _ui->sumCheckbox->isChecked() )
                _cstmThreshold->addPoint(QPointF(itr, cntr), 0);
            else
                _cstmThreshold->setPoint( QPointF(itr, cntr), 0);
             */

            // Report to heatmap
            //UpdateHeatMapSignal();

        }

        qDebug() << "Reactive THL = " << turnonTHL << " | pixelsFired = " << startReacting;
        qDebug() << "nReps = " << nReps;

        double nFiredAverage = ((double)pixelsReactive) / ((double)nReps);

        // Scan info
        QString firedVal;
        firedVal =  QString::number( nFiredAverage, 'f', 0 );
        connect( this, SIGNAL( fillText(QString) ), _cstmThreshold->GetUI()->firedLabel, SLOT( setText(QString)) );
        fillText( firedVal );
        disconnect( this, SIGNAL( fillText(QString) ), _cstmThreshold->GetUI()->firedLabel, SLOT( setText(QString)) );

        // plot only if data is available
        if ( nReps > 0 ) {
            addData( _cstmThreshold->getCurrentPlotIndex() , dacItr, nFiredAverage );
        }

        // increment
        if( _cstmThreshold->isScanDescendant() ) dacItr -= stepScan;
        else dacItr += stepScan;

        // See the termination condition
        if ( _cstmThreshold->isScanDescendant() ) {
            if ( dacItr >= minScan && !abort ) scanContinue = true;
            else scanContinue = false;
        } else {
            if ( dacItr <= maxScan && !abort ) scanContinue = true;
            else scanContinue = false;
        }

    }

    disconnect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
    disconnect( this, SIGNAL( addData(int, int, double) ), _cstmThreshold, SLOT( addData(int, int, double) ) );
    disconnect( this, SIGNAL( addData(int) ), _cstmThreshold, SLOT( addData(int) ) );

    //disconnect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
    //disconnect( this, SIGNAL( UpdateHeatMapSignal() ), this, SLOT( UpdateHeatMap() ) );

    //cout << "[INFO] Scan finished" << endl;

    delete spidrcontrol;
}

int CustomScanThread::PixelsReactive(int * data, int size_in_bytes, int /*thl*/) {

    int nPixels = size_in_bytes/4;
    int pixelsActive = 0;
    // Each 32 bits corresponds to the counts in each pixel already
    // in 'int' representation as the decoding has been requested
    for(int i = 0 ; i < nPixels ; i++) {
        if ( data[i] > minimumTurnOnValue ) {
            pixelsActive++;
        }
    }

    return pixelsActive;
}

bool CustomScanThread::getAbort()
{
    return abort;
}

void CustomScanThread::setAbort(bool arg)
{
    abort = arg;
    return;
}
