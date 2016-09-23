/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "mpx3gui.h"
#include "qcstmequalization.h"
#include "qcstmglvisualization.h"
#include "ui_mpx3gui.h"
#include "ui_qcstmconfigmonitoring.h"
#include "DataTakingThread.h"

#include "qcustomplot.h"
#include "mpx3eq_common.h"
#include "mpx3defs.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "barchart.h"
#include "ThlScan.h"
#include "gradient.h"
#include "dataset.h"
#include "mpx3config.h"

#include <QMessageBox>
#include <QDebug>
#include <QStatusBar>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <fstream>
#include <iostream>

//Mpx3GUI::Mpx3GUI(QWidget * parent) :	QMainWindow(parent), _coreApp(coreApp), _ui(new Ui::Mpx3GUI)
Mpx3GUI::Mpx3GUI(QWidget * parent) :
    QMainWindow(parent),
    _ui(new Ui::Mpx3GUI)
{

    // Instantiate everything in the UI
    _ui->setupUi(this);
    this->setWindowTitle(_softwareName);
    workingSet = new Dataset(128,128, 4);
    originalSet = new Dataset(128,128, 4);
    config = new Mpx3Config;
    config->SetMpx3GUI( this );

    //workingSet->setFramesPerGroup(1,1)

    // The orientations carry the information of how the information
    //  from a given chip should be drawn in the screen.
    getDataset()->setOrientation(0, _MPX3RX_ORIENTATION[0]);
    getDataset()->setOrientation(1, _MPX3RX_ORIENTATION[1]);
    getDataset()->setOrientation(2, _MPX3RX_ORIENTATION[2]);
    getDataset()->setOrientation(3,_MPX3RX_ORIENTATION[3]);

    // The layout is the position of the chip in the assembly.
    getDataset()->setLayout(0,  _MPX3RX_LAYOUT[0]);
    getDataset()->setLayout(1,  _MPX3RX_LAYOUT[1]);
    getDataset()->setLayout(2,  _MPX3RX_LAYOUT[2]);
    getDataset()->setLayout(3,  _MPX3RX_LAYOUT[3]);

    QString heatmapsFile = "./config/heatmaps.json";
    gradients = Gradient::fromJsonFile( heatmapsFile );
    QStringList gradientNames;

    if ( gradients.empty() ) {
        QMessageBox::critical(this, "Loading configuration error",
                              QString("Couldn't load the following configuration file: %1 . The program won't start. Place the file in the suggested relative path and run again. You are running the program from \"%2\"" ).arg(heatmapsFile).arg(QDir::currentPath()));
        _armedOk = false; // it won't let the application go into the event loop
        return;
    } else {
        for ( int i = 0 ; i < gradients.length() ; i++ )
            gradientNames.append(gradients[i]->getName());
    }




    // Change me when adding extra views
    //! Preparation of extra new views - add a new tab in the UI file.
    //! - Look for the widgets by name from the ones below here
    //! CHANGE the tab class type in the UI file by text

    // Prepare DACs panel
    _ui->DACsWidget->SetMpx3GUI( this );
    _ui->DACsWidget->setWindowWidgetsStatus(); // startup status

    // Prepare Equalization
    _ui->equalizationWidget->SetMpx3GUI( this );
    _ui->equalizationWidget->setWindowWidgetsStatus();

    if ( ! gradients.empty() ) {
        // Prepare Visualization
        _ui->visualizationGL->SetMpx3GUI(this);

        //_ui->CTTab->SetMpx3GUI(this);
        emit availible_gradients_changed(gradientNames);

    }
    //_ui->visualizationGL->ConfigureGUIForIdling();

    // Prepare THL Calibration
    _ui->ThresholdTab->SetMpx3GUI(this);

    //Config & monitoring
    _ui->CnMWidget->SetMpx3GUI(this);
    _ui->CnMWidget->widgetInfoPropagation();

    // CT
    _ui->ctTab->SetMpx3GUI( this );

    // DQE
    _ui->dqeTab->SetMpx3GUI(this);

    // Stepper Motor control view
    _ui->stepperMotorTab->SetMpx3GUI(this);



    // Read the configuration
    QString configFile = "./config/mpx3.json";
    if ( ! config->fromJsonFile( configFile ) ) {
        QMessageBox::critical(this, "Loading configuration error",
                              QString("Couldn't load the following configuration file: %1. The program won't start. Place the file in the suggested relative path and run again. You are running the program from \"%2\"").arg(configFile).arg(QDir::currentPath()));
        _armedOk = false; // it won't let the application go into the event loop
        return;
    }

    // shortcuts
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+1", "Switch to viewer") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+2", "Switch to configuration and monitoring") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+3", "Switch to DAC control") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+4", "Switch to Equalization") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+5", "Switch to DQE calculation") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+6", "Switch to Scans") ), this)  );

    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+7", "Switch to CT") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+8", "Switch to Stepper Motor Control") ), this)  );


    // Signals and slots for this part
    SetupSignalsAndSlots();
    //emit frame_added();

    /////////////////////////////////////////////////////////
    // statusBarMessage

    // I use a QLabel to be able to use rich text inside
    m_statusBarMessageLabel.setTextFormat( Qt::RichText );

    //QRect sgeo = _ui->statusBar->geometry();
    //m_statusBarMessageLabel.setGeometry(sgeo);

    // Add the QLabel permanently to the statusBar
    _ui->statusBar->addPermanentWidget( &m_statusBarMessageLabel, 0 );

    //_ui->statusBar->set
    //m_statusBarMessageLabel.setAlignment( Qt::AlignLeft );
    m_statusBarMessageString.clear( );

}



Mpx3GUI::~Mpx3GUI()
{
    delete config;
    delete workingSet;
    delete originalSet;
    delete _ui;
}

void Mpx3GUI::resize(int x, int y) {
    getDataset()->resize(x, y);
    QRectF bbox = getDataset()->computeBoundingBox();
    emit sizeChanged(bbox.width() * x, bbox.height() * y); // goes to qcstmglplot
}

unsigned int Mpx3GUI::addLayer(int *data) {
    return addLayer(data, -1);
}

unsigned int Mpx3GUI::dataReady(int layer)
{

    QVector<int> dataLayer = getVisualization()->dataTakingThread()->getData(layer);

    unsigned int ovflcntr = addLayer( dataLayer.data(), layer );

    return ovflcntr;
}

unsigned int Mpx3GUI::addFrame(int * frame, int index, int layer) {

    //cout << "index : " << index << " | layer : " << layer << " | mode : " << mode << endl;

    unsigned int ovfcntr = 0;
    if ( mode == 1 ) {
        ovfcntr = getDataset()->sumFrame(frame, index, layer);
    } else {
        ovfcntr = getDataset()->setFrame(frame, index, layer);
    }
    return ovfcntr;
}

unsigned int Mpx3GUI::addLayer(int * data, int layer) {

    unsigned int ovfcntr = 0;
    if(mode == 1){
        ovfcntr = getDataset()->addLayer(data, layer);
    }
    else {
        ovfcntr = getDataset()->setLayer(data, layer);
    }
    //emit reload_layer(layer);
    return ovfcntr;
}

Gradient* Mpx3GUI::getGradient(int index){
    return gradients.at(index);
}

SpidrController * Mpx3GUI::GetSpidrController(){
    return config->getController();
}

void Mpx3GUI::LoadEqualization(){
    _ui->equalizationWidget->LoadEqualization();
}

bool Mpx3GUI::equalizationLoaded(){
    return _ui->equalizationWidget->equalizationHasBeenLoaded();
}

void Mpx3GUI::setTestPulses() {

    int devId = 0;

    if ( _ui->equalizationWidget->equalizationHasBeenLoaded() ) {

        SpidrController * spidrcontrol = GetSpidrController();

        pair<int, int> pix;
        bool testbit = false;
        for ( int i = 0 ; i < __matrix_size ; i++ ) {

            pix = XtoXY(i, __array_size_x);


            if ( pix.first % 2 == 0 && pix.second % 2 == 0 ) {
                testbit = true;
                if ( pix.second == 0 ) spidrcontrol->configCtpr( devId, pix.first, 1 );
            } else {
                testbit = false;
                if ( pix.second == 0 ) spidrcontrol->configCtpr( devId, pix.first, 1 );
            }

            spidrcontrol->configPixelMpx3rx(pix.first,
                                            pix.second,
                                            _ui->equalizationWidget->getEqMap()[devId]->GetPixelAdj(i),
                                            _ui->equalizationWidget->getEqMap()[devId]->GetPixelAdj(i, Mpx3EqualizationResults::__ADJ_H),
                                            testbit);

        }

        spidrcontrol->setPixelConfigMpx3rx( devId );
        spidrcontrol->setCtpr( devId );

        spidrcontrol->setTpSwitch( 1, 200000 );

    }

}

void Mpx3GUI::SetupSignalsAndSlots(){

    connect( _ui->actionLoad_Equalization, SIGNAL(triggered()), this, SLOT( LoadEqualization() ) );
    connect( _ui->actionSave_DACs, SIGNAL(triggered()), this, SLOT( save_config()) );
    connect( _ui->actionLoad_DACs, SIGNAL(triggered()), this, SLOT( load_config()) );
    //connect( _ui->actionConnect, SIGNAL(triggered()), this, SLOT( establish_connection() ) );

    connect(_ui->actionSumming, SIGNAL(triggered()), this, SLOT(set_mode_integral()));
    connect(_ui->actionDiscrete, SIGNAL(triggered()), this, SLOT(set_mode_normal()));

    connect(_ui->actionSave_data, SIGNAL(triggered()), this, SLOT(save_data()));
    connect(_ui->actionSave_Equalization, SIGNAL(triggered()), _ui->equalizationWidget, SLOT(SaveEqualization()));
    connect(_ui->actionOpen_data, SIGNAL(triggered()), this, SLOT(open_data()));
    connect(_ui->actionClear_data, SIGNAL(triggered()), this, SLOT(clear_data()));
    connect(_ui->actionClear_configuration, SIGNAL(triggered()), this, SLOT(clear_configuration()) );

    // Change me when adding extra views
    // Inform every module of changes in connection status
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->DACsWidget, SLOT( ConnectionStatusChanged(bool) ) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->equalizationWidget, SLOT( ConnectionStatusChanged(bool) ) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->visualizationGL, SLOT( ConnectionStatusChanged(bool) ) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->dqeTab, SLOT( ConnectionStatusChanged(bool) ) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->stepperMotorTab, SLOT( ConnectionStatusChanged(bool) ) );
    connect( this, &Mpx3GUI::ConnectionStatusChanged, this, &Mpx3GUI::onConnectionStatusChanged );

    connect( this, &Mpx3GUI::sig_statusBarAppend, this, &Mpx3GUI::statusBarAppend );
    connect( this, &Mpx3GUI::sig_statusBarWrite, this, &Mpx3GUI::statusBarWrite );
    connect( this, &Mpx3GUI::sig_statusBarClean, this, &Mpx3GUI::statusBarClean );

    for ( int i = 0 ; i < _shortcutsSwitchPages.size() ; i++ ) {
        connect( _shortcutsSwitchPages[i], &QShortcut::activated,
                 this, &Mpx3GUI::on_shortcutsSwithPages );
    }

}

// Change me when adding extra views
void Mpx3GUI::on_shortcutsSwithPages() {

    // figure out who sent it
    QShortcut * sc = static_cast<QShortcut*> ( QObject::sender() );
    if ( ! sc ) return;

    QKeySequence k = sc->key();
    if ( k.matches( QKeySequence(tr("Ctrl+1")) ) ) {
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __visualization_page_Id );
        _ui->actionVisualization->setChecked(1);

    } else if ( k.matches( QKeySequence(tr("Ctrl+2")) ) ) {
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __configuration_page_Id );
        _ui->actionConfiguration->setChecked(1);

    } else if ( k.matches( QKeySequence(tr("Ctrl+3")) ) ) {
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __dacs_page_Id );
        _ui->actionDACs->setChecked(1);

    } else if ( k.matches( QKeySequence(tr("Ctrl+4")) ) ) {
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __equalization_page_Id );
        _ui->actionEqualization->setChecked(1);

    } else if ( k.matches( QKeySequence(tr("Ctrl+5")) ) ){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __dqe_page_Id );
        //_ui->actionDQE->setChecked(1);

    } else if ( k.matches( QKeySequence(tr("Ctrl+6")) ) ){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __scans_page_Id );
        //_ui->actionScans->setChecked(1);
    /*} else if ( k.matches( QKeySequence(tr("Ctrl+7")) ) ){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __ct_page_Id );
        //_ui-> actionXXX ->setChecked(1);*/
    } else if ( k.matches( QKeySequence(tr("Ctrl+8")) ) ){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __stepperMotor_page_Id );
        _ui->actionStepper_Motor->setChecked(1);
    }

}

Mpx3Config* Mpx3GUI::getConfig() {
    return config;
}

void Mpx3GUI::startupActions()
{

    /////////////////////////////////////////////////////////
    // startup actions
    if ( ! gradients.empty() ) {
        _ui->visualizationGL->startupActions();

    }

}

void Mpx3GUI::saveOriginalDataset(){
    *originalSet = *workingSet;
}

void Mpx3GUI::rewindToOriginalDataset(){
    *workingSet = *originalSet;
}

void Mpx3GUI::setWindowWidgetsStatus(win_status s)
{
    switch (s) {

    case win_status::startup:
        // Startup status
        _ui->actionConnect->setVisible(1);
        _ui->actionDisconnect->setVisible(0);
        _ui->actionDefibrillator->setVisible(0);

        _ui->actionVisualization->setChecked(1);
        break;

    case win_status::connected:
        // Startup status
        _ui->actionConnect->setVisible(1);
        _ui->actionDisconnect->setVisible(0);
        _ui->actionDefibrillator->setVisible(1);
        break;

    default:
        break;

    }
}

void Mpx3GUI::set_summing(bool shouldSum){
    if(shouldSum)
        set_mode_integral();
    else
        set_mode_normal();
}

bool Mpx3GUI::establish_connection() {

    qDebug() << "[INFO] Connecting ...";

    SpidrController * spidrcontrol = config->establishConnection();

    QDebug * dbg = new QDebug(QtInfoMsg);

    // See if there is any chips connected
    if ( config->getActiveDevices().size() == 0 ) {

        *dbg << "[ERROR] Could not find any devices";
        // Dump the verbose here.
        delete dbg;

        QMessageBox::critical(this, "Connection error",
                              tr("Could not find any devices.")
                              );
        config->destroyController();
        emit ConnectionStatusChanged(false);

        return false;
    }

    // Check if we are properly connected to the SPIDR module
    if ( spidrcontrol->isConnected() ) {
        *dbg << "Connected to SPIDR: " << spidrcontrol->ipAddressString().c_str() << "[" << config->getNDevicesPresent();
        if(config->getNDevicesPresent() > 1) *dbg << " chips found] ";
        else *dbg << " chip found] ";

        int ipaddr;
        // This call takes device number 0 'cause it is not really addressed to a chip in particular
        if( spidrcontrol->getIpAddrDest( 0, &ipaddr ) )
            *dbg << ", IP dest: "
                 << ((ipaddr>>24) & 0xFF) << "."
                 << ((ipaddr>>16) & 0xFF) << "."
                 << ((ipaddr>> 8) & 0xFF) << "."
                 << ((ipaddr>> 0) & 0xFF);

    } else {

        QMessageBox::critical(this, "Connection error",
                              tr("Could not establish a connection to the SPIDR controller.\n\nStatus: %1"
                                 "\nError message: %2"
                                 "\n\n"
                                 "If unsure about the network setup you can listen to the SPIDR during "
                                 "the power cycle through the USB port (8-N-1 115200)."
                                 ).
                              arg(QString::fromStdString(spidrcontrol->connectionStateString())).
                              arg(QString::fromStdString(spidrcontrol->connectionErrString()))
                              );

        config->destroyController();
        emit ConnectionStatusChanged(false);

        return false; // No use in continuing if we can't connect.
    }

    // Get version numbers
    *dbg << "\n";
    *dbg << "SpidrController class: "
         << spidrcontrol->versionToString( spidrcontrol->classVersion() ).c_str() << "\n";
    int version;
    if( spidrcontrol->getFirmwVersion( &version ) )
        *dbg << "SPIDR firmware  : " << spidrcontrol->versionToString( version ).c_str() << "\n";
    if( spidrcontrol->getSoftwVersion( &version ) )
        *dbg << "SPIDR software  : " << spidrcontrol->versionToString( version ).c_str() << "\n";


    // SpidrDaq
    _spidrdaq = new SpidrDaq( spidrcontrol );
    *dbg << "SpidrDaq[" << _spidrdaq << "]";

    for( int i=0; i<4; ++i ) *dbg << _spidrdaq->ipAddressString( i ).c_str() << " ";
    *dbg << "\n";

    if ( _spidrdaq->hasError() ) {
        // Dump the verbose here.
        delete dbg;

        QMessageBox::critical(this, "Connection error",
                              tr("Couldn't open an UDP connection. Error message:\n\n%1"
                                 "\n\nThis could be due to:\n- A program using the required UDP ports."
                                 "\n- Problems with the network between this system and SPIDR.").
                              arg(QString::fromStdString(_spidrdaq->errorString()))
                              );

        delete _spidrdaq;
        config->destroyController();

        emit ConnectionStatusChanged(false);
        return false; // No use in continuing if we can't connect.
    }

    // Dump the verbose here.
    delete dbg;

    ///////////////////////////////////////////////////
    // Done connecting.
    // If passed this point we are ready to work !

    // Here the chips can be configured
    // Default operation mode
    getConfigMonitoring()->OperationModeSwitched( Mpx3Config::__operationMode_SequentialRW );
    getConfig()->SendConfiguration( Mpx3Config::__ALL );

    // Load equalization if possible
    //LoadEqualization();

    // Emit
    emit ConnectionStatusChanged( true );

    // A working set had been instantiated before just to have a Dataset
    //  working on startup.  Now upon connection a new one will be
    //  instantiated.
    delete workingSet;
    delete originalSet;

    int chipSize = config->getColourMode()? __matrix_size_x /2: __matrix_size_x ;
    workingSet = new Dataset(chipSize, chipSize, config->getNActiveDevices(), config->getPixelDepth()); //TODO: get framesize from config, load offsets & orientation from config
    originalSet = new Dataset(chipSize, chipSize, config->getNActiveDevices(), config->getPixelDepth());
    //KoreaHack workingSet = new Dataset(chipSize, chipSize, 4, config->getPixelDepth()); //TODO: get framesize from config, load offsets & orientation from config
    //KoreaHack originalSet = new Dataset(chipSize, chipSize, 4, config->getPixelDepth());

    clear_data( false );

    QVector<int> activeDevices = config->getActiveDevices();

    for ( int i = 0 ; i < activeDevices.size() ; i++ ) {
        getDataset()->setLayout(i, _MPX3RX_LAYOUT[activeDevices[i]]);
        getDataset()->setOrientation(i, _MPX3RX_ORIENTATION[activeDevices[i]]);
    }

    //KoreaHack
    /*
    for ( int i = 0 ; i < 4 ; i++ ) {
        getDataset()->setLayout(i, _MPX3RX_LAYOUT[i]);
        getDataset()->setOrientation(i, _MPX3RX_ORIENTATION[i]);
    }
    */

    /*for(int i = 0; i < workingSet->getLayerCount();i++)
    updateHistogram(i);*/
    //emit frames_reload();

    return true;
}

void Mpx3GUI::statusBarAppend(QString mess, QString colorString)
{

    QString toappend;
    if ( ! m_statusBarMessageString.isEmpty() ) {
        toappend += " | ";
    }

    toappend += "<font color=\"";
    toappend += colorString;
    toappend += "\">";
    toappend += mess;
    toappend += "</font>";

    // Append
    m_statusBarMessageString.append( toappend );

    // Associate to the label
    m_statusBarMessageLabel.setText( m_statusBarMessageString );

    // See if it fits in the status bar other wise cut stuff
    QRect messRect = m_statusBarMessageLabel.geometry();
    QRect statusRect = _ui->statusBar->geometry();

    //qDebug() << "mess:" << messRect.width() << "stat:" << statusRect.width() << " | " << m_statusBarMessageString << "\n";

    while ( (messRect.width() + 100) > statusRect.width() ) {

        m_statusBarMessageString = removeOneMessage( m_statusBarMessageString );
        m_statusBarMessageLabel.setText( m_statusBarMessageString );
        m_statusBarMessageLabel.adjustSize();

        messRect = m_statusBarMessageLabel.geometry();
        statusRect = _ui->statusBar->geometry();

        //qDebug() << "mess:" << messRect.width() << "stat:" << statusRect.width() << " | " << m_statusBarMessageString << "\n";

    }

    _ui->statusBar->update();
    m_statusBarMessageLabel.update();

}

void Mpx3GUI::statusBarWrite(QString mess, QString colorString)
{

    QString toappend = "<font color=\"";
    toappend += colorString;
    toappend += "\">";
    toappend += mess;
    toappend += "</font>";


    // clear all previous messages
    m_statusBarMessageString.clear();

    //
    m_statusBarMessageLabel.setText( m_statusBarMessageString );

    _ui->statusBar->update();
    m_statusBarMessageLabel.update();

}

void Mpx3GUI::statusBarClean() {

    m_statusBarMessageString.clear();
    m_statusBarMessageLabel.setText( m_statusBarMessageString );
    _ui->statusBar->clearMessage();

    _ui->statusBar->update();
    m_statusBarMessageLabel.update();

}

QString Mpx3GUI::removeOneMessage(QString fullMess){

    int indx = fullMess.indexOf(" | ");
    if ( indx != -1 ) {
        fullMess = fullMess.right( fullMess.size() - indx - 3 );
    }

    return fullMess;
}

void Mpx3GUI::on_applicationStateChanged(Qt::ApplicationState s) {

    // When the application comes in Active for the first time
    //  take some startup actions
    if ( s == Qt::ApplicationActive && !m_appActiveFirstTime ) {
        startupActions();
        m_appActiveFirstTime = true;
    }

}

//Debugging function to generate data when not connected
void Mpx3GUI::generateFrame(){//TODO: put into Dataset
    //int thresholds[] = {0,1,2,3};
    QVector<int> data(getDataset()->x()*getDataset()->y()*getDataset()->getFrameCount());
    for(int t = 0; t < config->getNTriggers();t++){
        for(int k = 0; k < getDataset()->getFrameCount();k++){
            for(int t = 0; t < 4;t++){
                double fx = ((double)8*rand()/RAND_MAX)/(getDataset()->x()), fy = (8*(double)rand()/RAND_MAX)/getDataset()->y();
                for(int i = 0; i < getDataset()->y(); i++)
                    for(int j = 0; j < getDataset()->x(); j++)
                        //data[k*workingSet->x()*workingSet->y()+i*workingSet->x()+j] = (int)((1<<14)*sin(fx*j)*(cos(fy*i)));
                        data[i*getDataset()->x()+j] = (int)((1<<14)*sin(fx*j)*(cos(fy*i)));
                addFrame(data.data(), k, t);
            }
        }
    }
    //reloadLayer(0);reloadLayer(1);reloadLayer(2);reloadLayer(3);
    emit reload_all_layers();
}

int Mpx3GUI::getPixelAt(int x, int y, int layer){
    return getDataset()->sample(x,y, layer);
    //if(layer >= data.length() || x >= nx || y >= ny)
    // return 0;
    //return data[layer][y*nx+x];
}

QPoint Mpx3GUI::getSize(){
    return getDataset()->getSize();
}

void Mpx3GUI::getSize(int *x, int *y){
    QPoint dataSize = getDataset()->getSize();
    *x = dataSize.x();
    *y = dataSize.y();
}
int Mpx3GUI::getX(){
    return getDataset()->getSize().x();
}
int Mpx3GUI::getY(){
    return getDataset()->getSize().y();
}

int Mpx3GUI::getFrameCount(){
    return getDataset()->getFrameCount();
}

void Mpx3GUI::save_data(){//TODO: REIMPLEMENT

    //! Native format
    // User dialog
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Data"), tr("."), tr("binary files (*.bin)"));

    // Force the .bin in the data filename
    if ( ! filename.contains(".bin")  && !filename.isEmpty()) {
        filename.append(".bin");
    }

    // And save
    QFile saveFile(filename);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        string messg = "Couldn't open: ";
        messg += filename.toStdString();
        messg += "\nNo output written!";
        QMessageBox::warning ( this, tr("Error saving data"), tr( messg.c_str() ) );
        return;
    }
    saveFile.write(getDataset()->toByteArray());
    saveFile.close();

    /*

    ///////////////////////////////////////////////////////////
    // ASCII

    int sizex = getDataset()->x();
    int sizey = getDataset()->y();
    int nchipsx =  getDataset()->getNChipsX();
    int nchipsy = getDataset()->getNChipsY();
    int len = sizex * sizey * nchipsx * nchipsy;

    QList <int> thresholds = getDataset()->getThresholds();
    QList<int>::iterator it = thresholds.begin();
    QList<int>::iterator itE = thresholds.end();

    // Do the different thresholds
    for (; it != itE; it++) {

        int * fullFrame = getDataset()->getFullImageAsArrayWithLayout(*it, this);


        // Create string containing file location
        QString plS = filename;
        plS.remove(plS.size() - 4, 4); // get rid of the .bin extension
        plS += "_frame_ascii_";
        plS += QString::number(*it, 'd', 0);
        plS += ".txt";
        string saveLoc = plS.toStdString();

        //qDebug() << "nchipsx : " << nchipsx << " | nchipsy : " << nchipsy << " --> " << getDataset()->getPixelsPerLayer();

        // Save file
        ofstream of;
        of.open(saveLoc);
        if (of.is_open()) {
            for (int i = 0; i < len; i++) {

                of << fullFrame[i] << " ";

                // new line
                if ((i + 1) % (sizex*nchipsx) == 0) of << "\r\n";

            }
        }

        of.close();

    }


    // end of for loop that cycles through the layers (threshold)

    */

    return;
}

void Mpx3GUI::save_config(){
    QString filename = QFileDialog::getSaveFileName(this, tr("Save config"), tr("."), tr("json files (*.json)"));
    config->toJsonFile(filename);
    return;
}

void Mpx3GUI::load_config(){
    QString filename = QFileDialog::getOpenFileName(this, tr("Save config"), tr("."), tr("json files (*.json)"));
    config->fromJsonFile(filename);

    // update the dacs
    _ui->DACsWidget->PopulateDACValues();

    return;
}

void Mpx3GUI::onConnectionStatusChanged(bool conn)
{
    // Specific to the main window
    if( conn ) {
        _ui->actionConnect->setVisible(0);
        _ui->actionDisconnect->setVisible(1);

    } else {
        _ui->actionConnect->setVisible(1);
        _ui->actionDisconnect->setVisible(0);
    }

}


void Mpx3GUI::open_data(bool saveOriginal){

    QString filename = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("binary files (*.bin)"));
    QFile saveFile(filename);
    if ( ! saveFile.open(QIODevice::ReadOnly) ) {
        string messg = "Couldn't open: ";
        messg += filename.toStdString();
        messg += "\nNo output written!";
        QMessageBox::warning ( this, tr("Error opening data"), tr( messg.c_str() ) );
        emit open_data_failed();
        return;
    }
    clear_data();
    getDataset()->fromByteArray( saveFile.readAll() );
    saveFile.close();
    set_mode_normal();
    //QList<int> thresholds = getDataset()->getThresholds();

    // required signals
    QRectF bbox = getDataset()->computeBoundingBox();
    emit sizeChanged(bbox.width() * getDataset()->x(), bbox.height() * getDataset()->y() ); // goes to qcstmglplot
    emit reload_all_layers();

    // And keep a copy just as in QCstmGLVisualization::data_taking_finished
    if ( saveOriginal ) saveOriginalDataset();

    this->setWindowTitle( _softwareName + filename);

//    // If not in DQE - change back to Visualisation
//    if (!(_ui->stackedWidget->currentIndex() == __dqe_page_Id)){
    if(!_ui->dqeTab->openingNPSfile){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex(__visualization_page_Id);
        _ui->actionVisualization->setChecked(1);
    }

    //Ask whether the loaded data is already OBcorrected or not.
    QMessageBox::StandardButton reply = QMessageBox::question( this, tr("Specify data"), tr("Is this data corrected?"), QMessageBox::Yes | QMessageBox::No);
    if(reply == QMessageBox::Yes) getDataset()->setCorrected(true);
    else getDataset()->setCorrected(false);

    emit returnFilename(filename);

    return;
}


void Mpx3GUI::open_data_with_path(bool saveOriginal, bool requestPath, QString path)
{
    QString filename;
    if(!requestPath)
    {
        filename = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("binary files (*.bin)"));
    }else{
        filename = path;
    }

    qDebug() << "[INFO] loading splash image: " << filename;

    QFile saveFile(filename);
    if ( ! saveFile.open(QIODevice::ReadOnly) ) {
        string messg = "Couldn't open: ";
        messg += path.toStdString();
        messg += "\nNo output written!";
        QMessageBox::warning ( this, tr("Error opening data"), tr( messg.c_str() ) );
        emit open_data_failed();
        return;
    }
    clear_data();
    getDataset()->fromByteArray( saveFile.readAll() );
    saveFile.close();
    set_mode_normal();
    QList<int> thresholds = getDataset()->getThresholds();

    // required signals
    QRectF bbox = getDataset()->computeBoundingBox();
    emit sizeChanged(bbox.width() * getDataset()->x(), bbox.height() * getDataset()->y() ); // goes to qcstmglplot
    emit reload_all_layers();

    // And keep a copy just as in QCstmGLVisualization::data_taking_finished
    if ( saveOriginal ) saveOriginalDataset();

    if(getDataset()->getLayer(0)[0]==0) {
        qDebug() << "Mpx3GUI::open_data_with_path : "<< getDataset()->getLayer(0)[0];
    }

    if(!requestPath) {
        emit returnFilename(filename);
    }

    // If not in DQE - change back to Visualisation
//    if (!(_ui->stackedWidget->currentIndex() == __dqe_page_Id)){
//        _ui->stackedWidget->setCurrentIndex(__visualization_page_Id);
//    }
    if(!_ui->dqeTab->openingNPSfile){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex(__visualization_page_Id);
        _ui->actionVisualization->setChecked(1);
    }

    return;
}
void Mpx3GUI::set_mode_integral(){
    if(mode != 1){
        mode = 1;
        emit summing_set(true);
    }
}

void Mpx3GUI::set_mode_normal(){
    if(0 != mode){
        mode = 0;
        emit summing_set(false);
    }
}

void Mpx3GUI::clear_configuration(){


    // Clear adjustement bits
    QMessageBox::StandardButton ans = QMessageBox::question(this, tr("Clear configuration"), tr("The adjustment matrix and the pixel mask will be cleared.  Continue ?") );
    if ( ans == QMessageBox::No ) return;

    bool noEqualization = false;
    if ( _ui->equalizationWidget ) {

        int ndev = config->getNDevicesSupported();

        QProgressDialog pd("Clear adjustment bits ... ", "Cancel", 0, ndev, this);
        pd.setCancelButton( 0 ); // no cancel button
        pd.setWindowModality(Qt::WindowModal);
        pd.setMinimumDuration( 0 ); // show immediately
        pd.setWindowTitle("Clear equalization");

        pd.setValue( 0 );

        for ( int i = 0 ; i < ndev ; i++ ) {

            if ( ! config->detectorResponds( i ) )
                continue;

            if ( _ui->equalizationWidget->GetEqualizationResults( i ) ) {

                qDebug() << "[INFO] clearing adjustment bits and mask for devId : " << i;

                _ui->equalizationWidget->ClearAllAdjustmentBits( i );

            } else {
                noEqualization = true;
            }

            pd.setValue( i+1 );
        }

    } else {
        noEqualization = true;
    }

    if ( noEqualization ) {
        qDebug() << "[INFO] No equalization has been loaded. Nothing to clear.";
    }

}

void Mpx3GUI::clear_data(bool clearStatusBar) {
    getDataset()->clear();
//    _ui->dqeTab->clearDataAndPlots(true);
    //getVisualization()->cle
    emit data_cleared();

    if ( clearStatusBar )
        emit sig_statusBarAppend("Clear data","orange");

}

// Change me when adding extra views???
QCstmEqualization * Mpx3GUI::getEqualization(){return _ui->equalizationWidget;}
QCstmGLVisualization * Mpx3GUI::getVisualization() { return _ui->visualizationGL; }
QCstmDacs * Mpx3GUI::getDACs() { return _ui->DACsWidget; }
QCstmConfigMonitoring * Mpx3GUI::getConfigMonitoring() { return _ui->CnMWidget; }
//QCstmDQE * Mpx3GUI::getDQE(){ return _ui->dqeTab; }
QCstmStepperMotor * Mpx3GUI::getStepperMotor() {return _ui->stepperMotorTab; }

void Mpx3GUI::on_actionExit_triggered()
{

    // Check if something is running
    if ( getVisualization()->DataTakingThreadIsRunning() ) { // This means there's a thread ongoing

        if ( ! getVisualization()->DataTakingThreadIsIdling() ) { // actually taking data
            getVisualization()->StopDataTakingThread();
            QMessageBox::warning ( this,
                                   tr("Exit - pending actions"),
                                   tr("Attempting to exit while taking data.\n"
                                      "Data taking has been stopped." ) );
        }
        // Now just kill the data taking thread and consumer thread
        getVisualization()->FinishDataTakingThread();
    }

    // Check if it's connected
    if ( getConfig()->isConnected() ) {
        on_actionDisconnect_triggered( false );
    }

    // Save data --> dialogue
    save_data();

    emit exitApp( 0 );
}

// Change me when adding extra views
void Mpx3GUI::uncheckAllToolbarButtons(){
    _ui->actionVisualization->setChecked(0);
    _ui->actionConfiguration->setChecked(0);
    _ui->actionDACs->setChecked(0);
    _ui->actionEqualization->setChecked(0);

    _ui->actionStepper_Motor->setChecked(0);
    //TODO _ui-> NEW ACTION ->setChecked(0);
}


void Mpx3GUI::on_actionConnect_triggered() {

    // The connection status signal will be sent from establish_connection
    if ( establish_connection() ) {
        emit sig_statusBarAppend( "Connected", "green" );
    } else {
        emit sig_statusBarAppend( "Connection failed", "red" );
    }

}

void Mpx3GUI::on_actionVisualization_triggered(){
    uncheckAllToolbarButtons();
    _ui->stackedWidget->setCurrentIndex( __visualization_page_Id );
    _ui->actionVisualization->setChecked(1);
}

void Mpx3GUI::on_actionConfiguration_triggered(){
    uncheckAllToolbarButtons();
    _ui->stackedWidget->setCurrentIndex( __configuration_page_Id );
    _ui->actionConfiguration->setChecked(1);
}

void Mpx3GUI::on_actionDACs_triggered(){
    uncheckAllToolbarButtons();
    _ui->stackedWidget->setCurrentIndex( __dacs_page_Id );
    _ui->actionDACs->setChecked(1);
}

void Mpx3GUI::on_actionEqualization_triggered(){
    uncheckAllToolbarButtons();
    _ui->stackedWidget->setCurrentIndex( __equalization_page_Id );
    _ui->actionEqualization->setChecked(1);
}

void Mpx3GUI::on_actionScans_triggered(){
    uncheckAllToolbarButtons();
    _ui->stackedWidget->setCurrentIndex( __scans_page_Id );
    //TODO ->setChecked(1);
}

//! TODO: Implement revert more fully?
void Mpx3GUI::on_actionRevert_triggered(bool) {
    qDebug() << ">> Rewinding to original dataset? TEST ME";
    rewindToOriginalDataset();
    _ui->visualizationGL->reload_all_layers(false);
}

void Mpx3GUI::on_actionAbout_triggered(bool){
    QMessageBox msgBox;
    msgBox.setWindowTitle("About");

    QString newDate =  QDate::currentDate().toString("yyyy-MM-dd");
    QString msg = QString("Compiled: ") + newDate + QString(" - ") + QString(__TIME__) +
            QString("\n C++: ") + QString::fromStdString(to_string(__cplusplus)) +
//            QString("\n STDC: ") + QString::fromStdString(to_string(__STDC__)) +
            QString("\n\n ASI B.V. All rights reserved.") ;
    msgBox.setText( msg );
    msgBox.exec();
}

void Mpx3GUI::on_actionStepper_Motor_triggered(bool)
{
    uncheckAllToolbarButtons();
    _ui->stackedWidget->setCurrentIndex( __stepperMotor_page_Id );
}


void Mpx3GUI::on_actionDisconnect_triggered(bool checked){

    // See if there is anything running
    // Check if something is running
    if ( getVisualization()->DataTakingThreadIsRunning() ) { // This means there's a thread ongoing

        if ( ! getVisualization()->DataTakingThreadIsIdling() ) { // actually taking data
            getVisualization()->StopDataTakingThread();
            QMessageBox::warning ( this,
                                   tr("Exit - pending actions"),
                                   tr("Attempting to disconnect while taking data.\n"
                                      "Data taking has been stopped." ) );
        }
        // Now just kill the data taking thread
        getVisualization()->FinishDataTakingThread();
    }

    // Go through the process of disconnecting
    // SpidrDaq
    if ( _spidrdaq ) {
        _spidrdaq->stop();
        delete _spidrdaq;
        _spidrdaq = nullptr;
    }
    // SpirdController
    getConfig()->closeConnection();

    //
    emit ConnectionStatusChanged( checked ); // false when disconnecting

    emit sig_statusBarAppend( "Disconnected", "red" );
}

void Mpx3GUI::on_actionDefibrillator_triggered(bool checked){

    if ( getConfig()->isConnected() ) {


        QProgressDialog pd("System reset in progress ... ", "Cancel", 0, 3, this);
        pd.setCancelButton( 0 ); // no cancel button
        pd.setWindowModality(Qt::WindowModal);
        pd.setMinimumDuration( 0 ); // show immediately
        pd.setWindowTitle("Reset");
        //pd.setAutoReset( false );
        //pd.setAutoClose( false );

        //pd.setWindowTitle("Reset");
        //pd.show();

        // Hot reset
        pd.setValue( 0 );
        SpidrController * sc = config->getController();
        int errorstat;
        if ( sc ) {
            pd.setValue( 1 );
            qDebug() << "[INFO] Trying to hot-reset ...";
            sc->reset( &errorstat );
            emit sig_statusBarAppend( "Reset", "black" );

            // Hardware reset
            pd.setValue( 2 );
            sc->setSpidrReg( 0x814, 1 ); // write only register
        }

        // Disconnect
        //pd.setValue( 2 );
        //on_actionDisconnect_triggered( false );
        // Reconnnect
        //pd.setValue( 3 );
        //on_actionConnect_triggered();

        // Done
        pd.setValue( 3 );

    }

}



