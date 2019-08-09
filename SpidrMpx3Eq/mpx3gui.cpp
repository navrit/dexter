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
#include "thresholdscan.h"
#include "tcpserver.h"

#include "mpx3eq_common.h"
#include "mpx3defs.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "ThlScan.h"
#include "gradient.h"
#include "dataset.h"
#include "mpx3config.h"
#include "datacontrollerthread.h"
#include "hdmiConfig.h"

#include <QMessageBox>
#include <QDebug>
#include <QStatusBar>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QTimer>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <climits>


static Mpx3GUI *mpx3GuiInstance;

Mpx3GUI::Mpx3GUI(QWidget * parent) :
    QMainWindow(parent),
    _ui(new Ui::Mpx3GUI)
{
    qDebug().noquote() << "[VERSION]\t" << _softwareVersion;

    // Instantiate everything in the UI
    _ui->setupUi(this);
    this->setWindowTitle(_softwareName);
    workingSet = new Dataset(128, 128, 4);
    originalSet = new Dataset(128, 128, 4);
    config = new Mpx3Config;
    config->SetMpx3GUI( this );

    dataControllerThread = new DataControllerThread(this);
    _generalSettings = new GeneralSettings;
    _energyCalibrator = new EnergyCalibrator;

    //! FleXray - ZMQ to XRE/TeSCAN Acquila interface
    m_zmqController = new zmqController(this);
    m_zmqController->SetMpx3GUI(this);

//    tcpServer = new TcpServer;
//    if(!tcpServer->listen(QHostAddress::Any,6351))
//    {
//        qDebug()<< "Server can not be started...!";
//        return;
//    }
//    commandHandlerWrapper = new CommandHandlerWrapper;
//    connect(tcpServer,SIGNAL(dataReceived(QString)),commandHandlerWrapper,SLOT(on_dataReceived(QString)));
//    connect(commandHandlerWrapper,SIGNAL(responseIsReady(QString)),tcpServer,SLOT(on_responseIsReady(QString)));
//    dataServer = new DataServer;

//    if(!dataServer->listen(QHostAddress::Any,6352))
//    {
//        qDebug()<< "Data Server can not be started...!";
//        return;
//    }

    //connect(tcpServer,SIGNAL(requestAnotherServer(int)),dataServer,SLOT(on_requestAnotherServer(int)));


    // The orientations carry the information of how the information
    //  from a given chip should be drawn in the screen.
    auto ds = getDataset();
    ds->setOrientation(0, _MPX3RX_ORIENTATION[0]);
    ds->setOrientation(1, _MPX3RX_ORIENTATION[1]);
    ds->setOrientation(2, _MPX3RX_ORIENTATION[2]);
    ds->setOrientation(3, _MPX3RX_ORIENTATION[3]);

    // The layout is the position of the chip in the assembly.
    ds->setLayout(0,  _MPX3RX_LAYOUT[0]);
    ds->setLayout(1,  _MPX3RX_LAYOUT[1]);
    ds->setLayout(2,  _MPX3RX_LAYOUT[2]);
    ds->setLayout(3,  _MPX3RX_LAYOUT[3]);
    ds->computeBoundingBox();

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

    // Prepare Equalisation
    _ui->equalizationWidget->SetMpx3GUI( this );
    _ui->equalizationWidget->setWindowWidgetsStatus();

    if ( ! gradients.empty() ) {
        // Prepare Visualization
        _ui->visualizationGL->SetMpx3GUI(this);

        emit availible_gradients_changed(gradientNames);
    }

    // Prepare THL Calibration
    _ui->ThresholdTab->SetMpx3GUI(this);

    //Config & monitoring
    _ui->CnMWidget->SetMpx3GUI(this);
    _ui->CnMWidget->widgetInfoPropagation();

    // Stepper Motor control view
    _ui->stepperMotorTab->SetMpx3GUI(this);

    // CT
    _ui->ctTab->SetMpx3GUI( this );

    // Threshold scan
    _ui->THScan->SetMpx3GUI( this );

    // Read the configuration
    //QString configFile = "./config/mpx3.json";

    //if ( ! config->fromJsonFile( configFile ) ) {
    _loadingBeforeConnecting = true;
    bool configFileLoaded =  _loadConfigsFromGeneralSettings();
    if (!configFileLoaded)
    {
        qDebug() << "["<< _generalSettings->getConfigPath() << "] does not exist. Config file loaded from defualt path.";
        configFileLoaded = load_config_remotely("./config/mpx3.json");

    }

    if (! configFileLoaded) {
        QMessageBox::critical(this, "Loading configuration error",
                              QString("Couldn't load the following configuration file: %1. The program won't start. Place the file in the suggested relative path and run again. You are running the program from \"%2\"").arg(_generalSettings->getConfigPath()).arg(QDir::currentPath()));
        _armedOk = false; // it won't let the application go into the event loop
        return;
    }
    _loadingBeforeConnecting = false;
    // View keyboard shortcuts
    // NOTE: Change on_shortcutsSwithPages to match this
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+1", "Switch to viewer") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+2", "Switch to configuration and monitoring") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+3", "Switch to DAC control") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+4", "Switch to Equalization") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+5", "Switch to Threshold Scan") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+D, Ctrl+Alt+6", "Switch to Scans") ), this)  );

    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+D, Ctrl+Alt+7", "Switch to CT") ), this)  );
    _shortcutsSwitchPages.push_back( new QShortcut( QKeySequence( tr("Ctrl+D, Ctrl+Alt+8", "Switch to Stepper Motor Control") ), this)  );

    // Make Dexter somewhat scriptable via the GUI

    // Connections to Visualisation
    QShortcut *shortcutStart = new QShortcut(QKeySequence("s"), this);
    connect(shortcutStart, SIGNAL(activated()), _ui->visualizationGL, SLOT(shortcutStart()));
    QShortcut *shortcutIntegrate = new QShortcut(QKeySequence("i"), this);
    connect(shortcutIntegrate, SIGNAL(activated()), _ui->visualizationGL, SLOT(shortcutIntegrate()));
    QShortcut *shortcutIntegrateToggle = new QShortcut(QKeySequence("Alt+i"), this);
    connect(shortcutIntegrateToggle, SIGNAL(activated()), _ui->visualizationGL, SLOT(shortcutIntegrateToggle()));
    QShortcut *shortcutFrameLength = new QShortcut(QKeySequence("l"), this);
    connect(shortcutFrameLength, SIGNAL(activated()), _ui->visualizationGL, SLOT(shortcutFrameLength()));
    QShortcut *shortcutFrameNumber = new QShortcut(QKeySequence("f"), this);
    connect(shortcutFrameNumber, SIGNAL(activated()), _ui->visualizationGL, SLOT(shortcutFrameNumber()));

    // Connections to Configuration and settings
    QShortcut *shortcutGainModeSLGM = new QShortcut(QKeySequence("g, 4"), this);
    connect(shortcutGainModeSLGM, SIGNAL(activated()), _ui->CnMWidget, SLOT(shortcutGainModeSLGM()));
    QShortcut *shortcutGainModeLGM = new QShortcut(QKeySequence("g, 3"), this);
    connect(shortcutGainModeLGM, SIGNAL(activated()), _ui->CnMWidget, SLOT(shortcutGainModeLGM()));
    QShortcut *shortcutGainModeHGM = new QShortcut(QKeySequence("g, 2"), this);
    connect(shortcutGainModeHGM, SIGNAL(activated()), _ui->CnMWidget, SLOT(shortcutGainModeHGM()));
    QShortcut *shortcutGainModeSHGM = new QShortcut(QKeySequence("g, 1"), this);
    connect(shortcutGainModeSHGM, SIGNAL(activated()), _ui->CnMWidget, SLOT(shortcutGainModeSHGM()));

    QShortcut *shortcutCSMOff = new QShortcut(QKeySequence("c, 0"), this);
    connect(shortcutCSMOff, SIGNAL(activated()), _ui->CnMWidget, SLOT(shortcutCSMOff()));
    QShortcut *shortcutCSMOn = new QShortcut(QKeySequence("c, 1"), this);
    connect(shortcutCSMOn, SIGNAL(activated()), _ui->CnMWidget, SLOT(shortcutCSMOn()));

    // Connections to DACs
    QShortcut *shortcutTH0 = new QShortcut(QKeySequence("t, 0"), this);
    connect(shortcutTH0, SIGNAL(activated()), _ui->DACsWidget, SLOT(shortcutTH0()));
    QShortcut *shortcutIkrum = new QShortcut(QKeySequence("Ctrl+D, i"), this);
    connect(shortcutIkrum, SIGNAL(activated()), _ui->DACsWidget, SLOT(shortcutIkrum()));

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

    initialiseServers();
    updateEnergyCalibratorParameters();
    mpx3GuiInstance = this;
    shutterOpenTimer  = new QTimer(this);
    shutterCloseTimer = new QTimer(this);
    QTimer::singleShot(0, this, SLOT(autoConnectToDetector()));
}

Mpx3GUI::~Mpx3GUI()
{
    delete config;
    delete workingSet;
    delete originalSet;
    delete _ui;
}

void Mpx3GUI::resize(int x, int y) {
    getDataset()->resize(x, y, getConfig()->isConnected());
    QRectF bbox = getDataset()->getBoundingBox();
    emit sizeChanged(int(bbox.width() * x), int(bbox.height() * y)); // goes to qcstmglplot
}

void Mpx3GUI::addLayer(int * data, int layer) {

    if (mode == 1) { //! Summing/integral
        getDataset()->addLayer(data, layer);
    } else { //! Not summing, normal mode
        getDataset()->setLayer(data, layer);
    }
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

void Mpx3GUI::loadEqualisationFromPath(){
    bool getPath = true;
    _ui->equalizationWidget->LoadEqualization(getPath,false);
}

void Mpx3GUI::loadEqualisationFromPathRemotely(QString path)
{
    bool getPath = true;
    _ui->equalizationWidget->LoadEqualization(getPath,true,path);
}

void Mpx3GUI::onEqualizationPathExported(QString path)
{
    _generalSettings->setEqualisationPath(path);
}

void Mpx3GUI::sendingShutter()
{
    qDebug() << "ShutterOpen_us  : " << getConfig()->getTriggerLength_64();
    qDebug() << "ShutterClose_us : " << getConfig()->getTriggerDowntime_64();
    //GetSpidrController()->setShutterTriggerConfig(SHUTTERMODE_AUTO,0,(int)((1./(double)getConfig()->getTriggerPeriodMS())*1000000),getConfig()->getNTriggers(),0);
    //GetSpidrController()->startAutoTrigger(); //openshutter
    connect(shutterOpenTimer,SIGNAL(timeout()),this,SLOT(shutterOpenTimer_timeout()));
    connect(shutterCloseTimer,SIGNAL(timeout()),this,SLOT(shutterCloseTimer_timeout()));
    _timerStop = false;
    shutterCloseTimer_timeout();
    qDebug() << "_timerStop on_sending: " << _timerStop;
}

bool Mpx3GUI::equalizationLoaded(){
    return _ui->equalizationWidget->getEqualisationHasBeenLoaded();
}

int Mpx3GUI::getStepperMotorPageID()
{
    return __stepperMotor_page_Id;
}

void Mpx3GUI::loadLastConfiguration()
{
    //! If ./last_configuration.ini file exists, try to load it.
    //!
    //! Prompt user to choose to always auto-load last configuration or not.
    //! Save the auto-load parameter in the .ini file

    QFileInfo check_file(settingsFile);

    if (check_file.exists() && check_file.isFile() && check_file.isReadable() ) {
        QSettings settings(settingsFile, QSettings::NativeFormat);
        QString lastConfigurationPath = settings.value("last_configuration_path", "").toString();
        QString autoLoad = settings.value("auto_load_last_configuration", "").toString();

        //! Check folder exists before continuing
        QFileInfo check_folder(lastConfigurationPath);
        if (check_folder.exists() && check_folder.isReadable()) {
            autoLoad = autoLoad.toLower();

            //! The file has just been made, force the user to make a decision
            if (autoLoad == "") {
                //! Make dialog asking if you want to always auto-load the last configuration
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, "Question", "Always auto-load the last configuration?",
                                              QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    settings.setValue("auto_load_last_configuration", "true");
                } else {
                    settings.setValue("auto_load_last_configuration", "false");
                }

            //! Let's check autoLoad now
            } else {
                if (autoLoad == "true" || autoLoad == "yes" || autoLoad == "1") {
                    //! Load the last configuration
                    config->fromJsonFile(lastConfigurationPath + QDir::separator() + "config.json");
                    _ui->DACsWidget->PopulateDACValues();

                    emit sig_statusBarClean();
                    emit sig_statusBarAppend(QString("Autoloading last configuration file from " + lastConfigurationPath), "green");

                } else {
                    //! Don't autoLoad, do nothing. Just tell the user nothing was changed
                    emit sig_statusBarClean();
                    emit sig_statusBarAppend(QString("Not autoloading last configuration file from " + lastConfigurationPath), "black");
                }
            }

        } else {
            QString msg;
            if (lastConfigurationPath == "") {
                msg = QString("last_configuration_path is unset in " + settingsFile);
            } else {
                msg = QString("Auto-load configuration folder does not exist : " + lastConfigurationPath);
            }
            qDebug().noquote() << msg;
            emit sig_statusBarClean();
            emit sig_statusBarAppend(msg, "red");
        }

    } else {
        //! File doesn't exist, let's make it?
        QString msg = QString(settingsFile + " does not exist. Making it now.");
        qDebug() << msg;
        emit sig_statusBarAppend(msg, "red");
        QSettings settings(settingsFile, QSettings::NativeFormat);
        settings.setValue("last_configuration_path", "");
        settings.setValue("auto_load_last_configuration", "");

        loadLastConfiguration();
    }
}

void Mpx3GUI::stopTriggerTimers()
{
    //GetSpidrController()->stopAutoTrigger();
    disconnect(shutterOpenTimer,SIGNAL(timeout()),this,SLOT(shutterOpenTimer_timeout()));
    disconnect(shutterCloseTimer,SIGNAL(timeout()),this,SLOT(shutterCloseTimer_timeout()));
    _timerStop = true;
    qDebug() << "_timerStop stopTriggerTimers: " << _timerStop;
}

CommandHandlerWrapper *Mpx3GUI::getCommandHandlerWrapper()
{
    return commandHandlerWrapper;
}

void Mpx3GUI::SetupSignalsAndSlots(){

    connect( _ui->actionLoad_Equalization, SIGNAL(triggered()), this, SLOT( LoadEqualization() ) );
    connect( _ui->actionLoad_equalisation_from_folder, SIGNAL(triggered()), this, SLOT( loadEqualisationFromPath()) );

    connect( _ui->actionSave_DACs, SIGNAL(triggered()), this, SLOT( save_config()) );
    connect( _ui->actionLoad_DACs, SIGNAL(triggered()), this, SLOT( load_config()) );
    //connect( _ui->actionConnect, SIGNAL(triggered()), this, SLOT( establish_connection() ) );

    connect(_ui->actionSumming, SIGNAL(triggered()), this, SLOT(set_mode_integral()));
    connect(_ui->actionDiscrete, SIGNAL(triggered()), this, SLOT(set_mode_normal()));

    connect(_ui->actionSave_data, SIGNAL(triggered(bool)), this, SLOT(save_data(bool)));
    connect(_ui->actionSave_Equalization, SIGNAL(triggered()), _ui->equalizationWidget, SLOT(SaveEqualization()));
    connect(_ui->actionOpen_data, SIGNAL(triggered()), this, SLOT(open_data()));
    connect(_ui->actionClear_data, SIGNAL(triggered()), this, SLOT(zero_data()));
    connect(_ui->actionClear_configuration, SIGNAL(triggered()), this, SLOT(clear_configuration()) );

    connect(_ui->actionDeveloper_mode, SIGNAL(triggered()), this, SLOT(developerMode()));

    // Change me when adding extra views
    // Inform every module of changes in connection status
    connect( this, &Mpx3GUI::ConnectionStatusChanged, this, &Mpx3GUI::onConnectionStatusChanged );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->DACsWidget, SLOT( ConnectionStatusChanged(bool)) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->equalizationWidget, SLOT( ConnectionStatusChanged(bool)) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->visualizationGL, SLOT( ConnectionStatusChanged(bool)) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->stepperMotorTab, SLOT( ConnectionStatusChanged(bool)) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->CnMWidget, SLOT( ConnectionStatusChanged(bool)) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->hdmiConfigTab, SLOT(ConnectionStatusChanged(bool)) );
    connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->THScan, SLOT(ConnectionStatusChanged(bool)) );

    connect( this, &Mpx3GUI::sig_statusBarAppend, this, &Mpx3GUI::statusBarAppend );
    connect( this, &Mpx3GUI::sig_statusBarWrite, this, &Mpx3GUI::statusBarWrite );
    connect( this, &Mpx3GUI::sig_statusBarClean, this, &Mpx3GUI::statusBarClean );

    //! Part 4: Send equalisation loaded from ... to mpx3gui status bar
    // Connect signal from QCstmEqualization widget to slot here
    connect( _ui->equalizationWidget, &QCstmEqualization::sig_statusBarAppend, this, &Mpx3GUI::statusBarAppend);
    connect( _ui->equalizationWidget, &QCstmEqualization::sig_statusBarClean, this, &Mpx3GUI::statusBarClean);

    connect( _ui->stepperMotorTab, &QCstmStepperMotor::sig_statusBarAppend , this, &Mpx3GUI::statusBarAppend);

    connect( getConfig(), SIGNAL(colourModeChanged(bool)), getTHScan(), SLOT(slot_colourModeChanged()));
    connect( getConfig(), SIGNAL(readBothCountersChanged(bool)), getTHScan(), SLOT(slot_doubleCounterModeChanged()));

    for ( int i = 0 ; i < _shortcutsSwitchPages.size() ; i++ ) {
        connect( _shortcutsSwitchPages[i], &QShortcut::activated,
                 this, &Mpx3GUI::on_shortcutsSwitchPages );
    }

}

Mpx3GUI *Mpx3GUI::getInstance()
{
  return mpx3GuiInstance;
}

// Change me when adding extra views
void Mpx3GUI::on_shortcutsSwitchPages() {

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
        _ui->stackedWidget->setCurrentIndex( __equalisation_page_Id );
        _ui->actionEqualization->setChecked(1);

    } else if ( k.matches( QKeySequence(tr("Ctrl+5")) ) ) {
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __thresholdScan_page_Id );
        _ui->actionThreshold_Scan->setChecked(1);

    } else if ( k.matches( QKeySequence(tr("Ctrl+D, Ctrl+Alt+6")) ) ){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __scans_page_Id );
        //_ui->actionScans->setChecked(1);
    } else if ( k.matches( QKeySequence(tr("Ctrl+D, Ctrl+Alt+7")) ) ){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __ct_page_Id );
        //_ui-> actionXXX ->setChecked(1);
    } else if ( k.matches( QKeySequence(tr("Ctrl+D, Ctrl+Alt+8")) ) ){
        uncheckAllToolbarButtons();
        _ui->stackedWidget->setCurrentIndex( __stepperMotor_page_Id );
        _ui->actionStepper_Motor->setChecked(1);
    }

}

Mpx3Config* Mpx3GUI::getConfig() {
    return config;
}

void Mpx3GUI::rebuildCurrentSets(int x, int y, int framesPerLayer)
{
    delete workingSet;
    workingSet = new Dataset(x, y, framesPerLayer);
    workingSet->setOrientation(0, _MPX3RX_ORIENTATION[0]);
    workingSet->setLayout(0,  _MPX3RX_LAYOUT[0]);
    workingSet->computeBoundingBox();
    delete originalSet;
    originalSet = new Dataset(x, y, framesPerLayer);
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

        _ui->actionVisualization->setChecked(1);
        break;

    case win_status::connected:
        // Startup status
        _ui->actionConnect->setVisible(1);
        _ui->actionDisconnect->setVisible(0);
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

    SpidrController * spidrcontrol = config->establishConnection();
    QString msg = "";

    // See if there is any chips connected
    if ( config->getActiveDevices().size() == 0 ) {
        msg = "[ERROR]\tCould not find any devices";

        config->destroyController();
        emit ConnectionStatusChanged(false);

        qDebug().noquote().nospace() << msg;
        return false;
    }

    // Check if we are properly connected to the SPIDR module
    if ( spidrcontrol->isConnected() ) {
        msg += "[INFO]\tConnected to SPIDR: ";
        msg += QString::fromStdString(spidrcontrol->ipAddressString());
        msg += " - [";
        msg += QString::number(config->getNDevicesPresent());

        if(config->getNDevicesPresent() > 1) {
            msg += " chips]";
        } else {
            msg += " chip]";
        }

        m_numberOfChipsFound = QString("\nNumber of chips: ")
                + QString::number( config->getNDevicesPresent() );

        int ipaddr;
        // This call takes device number 0 because it is not really addressed to a chip in particular
        if( spidrcontrol->getIpAddrDest( 0, &ipaddr ) ){
            msg += " - Computer IP: " +
                    QString::number((ipaddr>>24) & 0xFF) + "." +
                    QString::number(((ipaddr>>16) & 0xFF)) + "." +
                    QString::number(((ipaddr>> 8) & 0xFF)) + "." +
                    QString::number(((ipaddr>> 0) & 0xFF));
        }

    } else {
        QMessageBox::critical(this, "Connection error",
                              tr("Could not establish a connection to the SPIDR controller.\n\nStatus: %1"
                                 "\nError message: %2"
                                 "\n\n"
                                 "If unsure about the network setup you can listen to the SPIDR during "
                                 "the power cycle through the USB port (8-N-1 115200)."
                                 ).
                              arg(QString::fromStdString(spidrcontrol->connectionStateString())).
                              arg(QString::fromStdString(spidrcontrol->connectionErrString())));

        config->destroyController();
        emit ConnectionStatusChanged(false);

        qDebug().noquote().nospace() << msg;
        return false; // No use in continuing if we can't connect.
    }

    // Get version numbers
    int version;
    m_SPIDRControllerVersion = QString("\nSPIDR Controller version: ") +
            QString (spidrcontrol->versionToString( spidrcontrol->classVersion() ).c_str());

    if( spidrcontrol->getFirmwVersion( &version ) ) {
        m_SPIDRFirmwareVersion = QString("\nSPIDR Firmware version: ") +
                QString(spidrcontrol->versionToString( version ).c_str());
    }

    if( spidrcontrol->getSoftwVersion( &version ) ){
        m_SPIDRSoftwareVersion = QString("\nSPIDR Software version: ") +
                QString(spidrcontrol->versionToString( version ).c_str())
                +QString("\n");
    }


    // SpidrDaq
    _spidrdaq = new SpidrDaq( spidrcontrol );

    for( int i=0; i<4; ++i ){
        msg += " " + QString::fromStdString(_spidrdaq->ipAddressString(i)) + " ";
    }

    if ( _spidrdaq->hasError() ) {
        QMessageBox::critical(this, "Connection error",
                              tr("Couldn't open an UDP connection. Error message:\n\n%1"
                                 "\n\nThis could be due to:\n- A program using the required UDP ports."
                                 "\n- Problems with the network between this system and SPIDR.").
                              arg(QString::fromStdString(_spidrdaq->errorString()))
                              );

        delete _spidrdaq;
        config->destroyController();

        emit ConnectionStatusChanged(false);
        qDebug().noquote().nospace() << msg;
        return false; // No use in continuing if we can't connect.
    }
    qDebug().noquote().nospace() << msg;

    ///////////////////////////////////////////////////
    // Done connecting.
    // If passed this point we are ready to work !

    // Here the chips can be configured
    // Default operation mode
    getConfigMonitoring()->OperationModeSwitched( Mpx3Config::__operationMode_SequentialRW );
    getConfig()->SendConfiguration( Mpx3Config::__ALL );
    getVisualization()->initialiseThresholdsVector();

    emit ConnectionStatusChanged( true );

    // A working set had been instantiated before just to have a Dataset
    //  working on startup.  Now upon connection a new one will be
    //  instantiated.
    delete workingSet;
    delete originalSet;

    int chipSize = config->getColourMode()? __matrix_size_x /2: __matrix_size_x ;
    workingSet = new Dataset(chipSize, chipSize, config->getNActiveDevices(), config->getPixelDepth()); //TODO: get framesize from config, load offsets & orientation from config
    originalSet = new Dataset(chipSize, chipSize, config->getNActiveDevices(), config->getPixelDepth());

    clear_data( false );

    QVector<int> activeDevices = config->getActiveDevices();

    for ( int i = 0 ; i < activeDevices.size(); i++ ) {
        workingSet->setLayout(i, _MPX3RX_LAYOUT[std::size_t(activeDevices[i])]);
        workingSet->setOrientation(i, _MPX3RX_ORIENTATION[std::size_t(activeDevices[i])]);
    }
    workingSet->computeBoundingBox();

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

//! Debugging function to generate test patterns, used to verify files are being saved correctly
//! Note: This is configured for the current orientation scheme for 4 chips.
void Mpx3GUI::generateFrame(){

    int y = getDataset()->y();
    int x = getDataset()->x();

    //int thresholds = getDataset()->getLayerCount();
    int chips = getDataset()->getFrameCount();

    //! Total data - chip dimensions * number of chips
    QVector<int> data(x * y * chips);

//    for(int t=0; t < thresholds; ++t) {
        for(int k=0; k < chips; ++k) {
            //! Only do this for the first threshold (layer) because it will always be there
            //! and cannot mess with the other thresholds if they exist...
            //!
            //! This could be improved later if desired
            //for(int t=0; t < 4; ++t) {
            int offset = k * x * y;

            for(int i = 0; i < y; i++) {
                for(int j = 0; j < x; j++) {
                    //! Generate border pixels
                    if ( (k==0 || k==2) && (i==0 || j==0) ) {
                        data[offset + i*x+j] = k+1;
                    } else if ( (k==1 || k==3) && (i==0 || j==x-1) ) {
                        data[offset + i*x+j] = k+1;
                    //! Generate centre pixels
                    } else if ( ((k==0 || k==2) && (i==x-1 && j==x-1)) ||
                                ((k==1 || k==3) && (i==x-1 && j==0  )) ) {
                        data[offset + i*x+j] = 5;
                    } else {
                        data[offset + i*x+j] = int((k+1)*((int(m_offset)+i+j)%2));
                    }
                }
            }
        }
        addLayer(data.data(), 0);
//    }

    m_offset = !m_offset;

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

QString Mpx3GUI::getLoadButtonFilename() {
    return loadButtonFilenamePath;
}

void Mpx3GUI::saveMetadataToJSON(QString filename){
    if (filename.toLower().contains(".bin")){
        filename = filename.replace(".bin",".json");
    } else if (filename.toLower().contains(".tiff")){
        filename = filename.replace(".tiff",".json");
    } else if (filename.toLower().contains(".txt")) {
        filename = filename.replace(".txt",".json");
    } else {
        qDebug() << "[ERROR]\tFailed writing the JSON file, input name didn't include .tiff, .bin or .txt";
        return;
    }

    //! Get configuration JSON document
    QJsonDocument docConfig = getConfig()->buildConfigJSON(true);
    //! Extract config JSON object
    QJsonObject objParent = docConfig.object();

    QJsonDocument doc;

    //QJsonArray objBinsPathArray;
    // Which bin(s) are we taling about
    objParent.insert("binPath", filename);

    /* Use this as inspiration for when multiple bins are saved
     * Dump bin paths into a QJsonArray

    QJsonArray objDacsArray;
    objBin.insert("binFilePath", );
    objBin.insert("", );

    objParent.insert("IPConfig", objIp);

    if(includeDacs){
        for(int j = 0; j < this->getDacCount(); j++){
            QJsonObject obj;
            for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++)
                obj.insert(MPX3RX_DAC_TABLE[i].name, _dacVals[i][j]);
            objDacsArray.insert(j, obj);
        }
        objParent.insert("DACs", objDacsArray);
    }
    */

    //-----------
    objParent.insert("Build_ABI", QSysInfo::buildAbi());
    objParent.insert("CPU_arch_current", QSysInfo::currentCpuArchitecture());
    objParent.insert("CPU_arch_build", QSysInfo::buildCpuArchitecture());
    objParent.insert("WindowsVersion", QSysInfo::WindowsVersion);
    objParent.insert("OS_productVersion", QSysInfo::productVersion());
    objParent.insert("OS_productType", QSysInfo::productType());
    objParent.insert("OS_prettyProductName", QSysInfo::prettyProductName());
    objParent.insert("Machine_hostName", QSysInfo::machineHostName());
    objParent.insert("Kernel_typeAndVersion", (QSysInfo::kernelType() + QSysInfo::kernelVersion()));
    objParent.insert("Compile_dateTime", compileDateTime);
    objParent.insert("C++_version", QString::fromStdString(to_string(__cplusplus)));
    objParent.insert("Unix_time_s", (QDateTime::currentMSecsSinceEpoch() / 1000));
    objParent.insert("Date_time_local", ( QDateTime::currentDateTime().toString(Qt::ISODate) ));
    objParent.insert("Date_time_UTC", ( QDateTime::currentDateTimeUtc().toString(Qt::ISODate) ));

    objParent.insert("SPIDR_Controller_version", m_SPIDRControllerVersion);
    objParent.insert("SPIDR_Firmware_version", m_SPIDRFirmwareVersion);
    objParent.insert("SPIDR_Software_version", m_SPIDRSoftwareVersion);

    //objParent.insert("", );

    // ------------------ Calculations -------------------------------
    int sizex = getDataset()->x();
    int sizey = getDataset()->y();
    int nchipsx =  getDataset()->getNChipsX();
    int nchipsy = getDataset()->getNChipsY();

    QList <int> thresholds = getDataset()->getThresholds();
    QString thresholds_str;
    for (int i=0; i<thresholds.size(); i++){
        thresholds_str += QString::number(thresholds[i]);
        if(i < (thresholds.size()-1)){
            thresholds_str += ", ";
        }
    }
    // ----------------------------------------------------------------
    objParent.insert("ChipSize_x", sizex);
    objParent.insert("ChipSize_y", sizey);
    objParent.insert("nChips_x", nchipsx);
    objParent.insert("nChips_y", nchipsy);
    objParent.insert("Length_x", sizex*nchipsx);
    objParent.insert("Length_y", sizey*nchipsy);
    objParent.insert("Thresholds", thresholds_str);

    // TODO JSON
    //    objParent.insert("Number of frames", );


    doc.setObject(objParent);

    QFile saveFile(filename);
    if(!saveFile.open(QIODevice::WriteOnly)){
        qDebug() << "[WARN]\tJSON file CANNOT be opened as QIODevice::WriteOnly, aborting";
        sig_statusBarAppend(tr("Cannot write JSON metadata, skipping"),"red");
        return;
    }
    saveFile.write(doc.toJson());
    saveFile.close();

    qDebug() << "[INFO]\tJSON File saved";
}

void Mpx3GUI::initialiseServers()
{
    //! Diamond - Merlin interface commands socket
    tcpServer = new TcpServer;
    if (!tcpServer->listen(QHostAddress::Any, tcpCommandPort)) {
        qWarning() << "[ERROR]\tTCP Command server cannot listen on port:" << tcpCommandPort;
        qDebug() << "[ERROR]\tCheck if there is another process already bound to port:" << tcpCommandPort;
        return;
    } else {
        qDebug().nospace() << "[INFO]\tTCP Command server listening on \"tcp://*:" << tcpCommandPort << "\"";
    }
    commandHandlerWrapper = new CommandHandlerWrapper(this);
    connect(tcpServer,SIGNAL(dataReceived(QString)),commandHandlerWrapper,SLOT(on_dataReceived(QString)));
    connect(commandHandlerWrapper,SIGNAL(responseIsReady(QString)),tcpServer,SLOT(on_responseIsReady(QString)));

    //! Diamond - Merlin interface data socket
    dataServer = new TcpServer;
    if(!dataServer->listen(QHostAddress::Any, tcpDataPort)) {
        qWarning() << "[ERROR]\tTCP Data server cannot listen on port:" << tcpDataPort;
        qDebug() << "[ERROR]\tCheck if there is another process already bound to port:" << tcpDataPort;
        return;
    } else {
        qDebug().nospace() << "[INFO]\tTCP Data server listening on \"tcp://*:" << tcpDataPort << "\"";
    }
    connect(commandHandlerWrapper,SIGNAL(imageIsReady(QByteArray,Canvas)),dataServer,SLOT(on_imageIsReady(QByteArray,Canvas)));

}

void Mpx3GUI::_loadEqualizationFromGeneralSettings()
{
    QDir eqDir(_generalSettings->getEqualisationPath());
    if(eqDir.exists()){
        getEqualization()->LoadEqualization(false, false, _generalSettings->getEqualisationPath());
        qDebug() << "[INFO]\tEqualization loaded from" << _generalSettings->getEqualisationPath();
    }
}

bool Mpx3GUI::_loadConfigsFromGeneralSettings()
{
    bool res = false;
    QStringList pathList = _generalSettings->getConfigPath().split(QDir::separator());
    QString filename = pathList.last();
    // Construct the path directory again
    QString path; path.clear();
    for (int i = 0; i < pathList.length() - 1; ++i) {
         path += pathList.at(i) + QDir::separator();
    }
    QDir confDir(path);
    if(confDir.exists() && filename.split(".").last() == "json"){
        res = load_config_remotely(_generalSettings->getConfigPath());
        qDebug() << "[INFO]\tConfig file loaded from : " << path + filename;
    }
    return res;
}

void Mpx3GUI::updateEnergyCalibratorParameters()
{
    _generalSettings->readSetting();
    for(int i = 0; i < NUMBER_OF_CHIPS; i++){
        _energyCalibrator->setSlope(i,_generalSettings->getSlope(i));
        //qDebug() << "load slope " + QString::number(i) +" : " << _energyCalibrator->getSlope(i);
        _energyCalibrator->setOffset(i,_generalSettings->getOffset(i));
    }

}

void Mpx3GUI::developerMode()
{
    if (devMode){
        //! Have to click on menu item to enable/disable buttons
        //! Disables a bunch of GUI items

        qDebug() << "[INFO]\tDisabling items";
        _ui->visualizationGL->developerMode(false);
        getConfigMonitoring()->developerMode(false);
        _ui->actionDefibrillator->setVisible(false);
        devMode = false;
    } else {
        qDebug() << "[INFO]\tEnabling items";
        _ui->visualizationGL->developerMode(true);
        getConfigMonitoring()->developerMode(true);
        _ui->actionDefibrillator->setVisible(true);
        devMode = true;
    }
}

void Mpx3GUI::save_data(bool requestPath, int frameId, QString selectedFileType) {

    QString filename, path, selectedFilter;
    //! Manual or autosave
    if (!requestPath){
        //! Native format - User dialog
        filename = QFileDialog::getSaveFileName(this, tr("Save Data"), ".", BIN_FILES ";;" TIFF_FILES ";;" RAW_TIFF_FILES ";;" BOTH_TIFF_FILES ";;" SPATIAL_TIFF_FILES ";;" ASCII_FILES, &selectedFilter);

        if (filename.isNull()){
            return;
        }
        if (selectedFilter == BIN_FILES && !filename.toLower().toUtf8().contains(".bin")){
            filename.append(".bin");
        } else if (selectedFilter == SPATIAL_TIFF_FILES && !filename.toLower().toUtf8().contains("_spatialCorrected.tiff")){
            filename.append("_spatialCorrected.tiff");
        } else if (selectedFilter == RAW_TIFF_FILES && !filename.toLower().toUtf8().contains("_raw.tiff")){
            filename.append("_raw.tiff");
        } else if (selectedFilter == BOTH_TIFF_FILES && !filename.toLower().toUtf8().contains(".tiff")){
            filename.append("_both.tiff");
        } else if (selectedFilter == TIFF_FILES && !filename.toLower().toUtf8().contains(".tiff")){
            filename.append(".tiff");
        } else if (selectedFilter == ASCII_FILES && !filename.toLower().toUtf8().contains(".txt")){
            filename.append(".txt");
        }

    } else {
        //! Autosave
        //!
        //! Get the visualisation dialog UI saveLineEdit text and assign to filename
        path = getVisualization()->getsaveLineEdit_Text(); // TODO optimise this, no need to call this every loop
        //! Build the filename+path string up by adding  "/", the current UTC date in ISO format and ".bin"
        filename = path;
        filename.append("/");
        filename.append( QString::number( QDateTime::currentMSecsSinceEpoch()) );

        //! if saving all frames, append the frame ID too - more than 1 frame may be saved within 1 ms
        if ( getVisualization()->isSaveAllFramesChecked() ) { // TODO optimise this, no need to call this every loop
            filename.append( QString("_%1").arg(frameId) );
        }

        //! Handle the different file types that you can autosave to via
        //! the QCstmGLVisualization saveFileComboBox and propagate correct
        //! information forwards.

        if (selectedFileType == ""){
            filename.append(".txt");
            selectedFilter = ASCII_FILES;
        } else if (selectedFileType == "TIFF"){
            filename.append(".tiff");
            selectedFilter = TIFF_FILES;
        } else if (selectedFileType == "Raw TIFF"){
            filename.append("_raw.tiff");
            selectedFilter = RAW_TIFF_FILES;
        } else if (selectedFileType == "Raw TIFF16"){
            filename.append("_raw.tiff");
            selectedFilter = RAW_TIFF16_FILES;
        } else if (selectedFileType == "PGM16"){
            filename.append("_raw.pgm");
            selectedFilter = PGM16_FILES;
        } else if (selectedFileType == "Text"){
            filename.append(".txt");
            selectedFilter = ASCII_FILES;
        } else {
            filename.append(".txt");
            selectedFilter = ASCII_FILES;
        }
    }

    //! Send data to be saved by the relevant function with the correct arguments etc.
    const QString unmodifiedFilename = filename;

    Dataset* dataset = getDataset();
    if (selectedFilter == BIN_FILES) {
        dataset->saveBIN(filename);
    } else if (selectedFilter == SPATIAL_TIFF_FILES
            || selectedFilter == RAW_TIFF_FILES
            || selectedFilter == RAW_TIFF16_FILES
            || selectedFilter == PGM16_FILES
            || selectedFilter == TIFF_FILES
            || selectedFilter == BOTH_TIFF_FILES) {

        //! This is a shitty way of doing it but whatever the disk is still the bottleneck... maybe fix it later
        //!

        foreach (int key, dataset->getThresholds()) {
            QString tmpFilename = unmodifiedFilename;
            Canvas frame;

            if (selectedFilter == SPATIAL_TIFF_FILES) {
                frame = dataset->makeFrameForSaving(key, true, true);
                tmpFilename = tmpFilename.replace("_spatialCorrected.tiff", QString("-th"+ QString::number(key) +"_spatialCorrected.tiff"));

            } else if (selectedFilter == RAW_TIFF_FILES) {
                frame = dataset->getFullImageAsArrayWithLayout(key, 4);
                tmpFilename = tmpFilename.replace("_raw.tiff", QString("-th"+ QString::number(key) +"_raw.tiff"));

            } else if (selectedFilter == RAW_TIFF16_FILES) {
                frame = dataset->getFullImageAsArrayWithLayout(key, 2);
                tmpFilename = tmpFilename.replace("_raw.tiff", QString("-th"+ QString::number(key) +"_raw.tiff"));

            } else if (selectedFilter == PGM16_FILES) {
                frame = dataset->getFullImageAsArrayWithLayout(key, 2);
                tmpFilename = tmpFilename.replace("_raw.pgm", QString("-th"+ QString::number(key) +"_raw.pgm"));
                QtConcurrent::run( dataControllerThread, &DataControllerThread::savePGMParallel, tmpFilename, frame);
                continue;

            } else if (selectedFilter == TIFF_FILES) {
                frame = dataset->makeFrameForSaving(key);
                tmpFilename = tmpFilename.replace(".tiff", QString("-th"+ QString::number(key) +".tiff"));

            } else if (selectedFilter == BOTH_TIFF_FILES) {
                frame = dataset->getFullImageAsArrayWithLayout(key, 4);
                tmpFilename = tmpFilename.replace("_both.tiff", QString("-th"+ QString::number(key) +"_raw.tiff"));
                QtConcurrent::run( dataControllerThread, &DataControllerThread::saveTIFFParallel, tmpFilename, frame);

                tmpFilename = unmodifiedFilename;

                frame = dataset->makeFrameForSaving(key);
                tmpFilename = tmpFilename.replace("_both.tiff", QString("-th"+ QString::number(key) +".tiff"));
            }

            QtConcurrent::run( dataControllerThread, &DataControllerThread::saveTIFFParallel, tmpFilename, frame);
        }
    } else if (selectedFilter == ASCII_FILES) {
        dataset->toASCII(filename);
    } else {
        dataset->toTIFF(filename, false);
    }

    if (!requestPath || frameId == 0){
        // Manual saving
        //qDebug() << "[INFO] File saved: ..." << filename;
        QString msg = "Saved: ...";
        msg.append(filename.section('/',-3,-1));
        emit sig_statusBarAppend(msg,"green");

        saveMetadataToJSON(filename);
    } else {
        // Autosave

        // Not good for high speed without some buffer system
        //saveMetadataToJSON(filename);
    }

    return;
}

void Mpx3GUI::save_config() {
    QString filename = QFileDialog::getSaveFileName(this, tr("Save config"), tr("."), tr("json files (*.json)"));
    if (!filename.endsWith(".json", Qt::CaseInsensitive) ) {
        filename.append(".json");
    }
    config->toJsonFile(filename);
    return;
}

bool Mpx3GUI::load_config() {

    bool res = false;
    if(!_loadConfigRemotely){
        _configPath  = QFileDialog::getOpenFileName(this, tr("Load config (DACs)"), tr("."), tr("json files (*.json)"));
    }
    QFileInfo configFileInfo(_configPath);
    if (configFileInfo.exists() && configFileInfo.isFile()) {
        res = config->fromJsonFile(_configPath);
        _generalSettings->setConfigPath(_configPath);
        // update the dacs
        if (!_loadingBeforeConnecting) {
            _ui->DACsWidget->PopulateDACValues();
            getConfig()->SendConfiguration( Mpx3Config::__ALL );
        }
    } else {
        res = false;
    }

    _loadConfigRemotely = false; //reset the flag

    return res;
}

bool Mpx3GUI::load_config_remotely(QString path)
{
    _loadConfigRemotely = true;
    _configPath = path;
    return load_config();
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

        // Resets About dialog variables to reflect current program state
        m_SPIDRControllerVersion = "";
        m_SPIDRFirmwareVersion = "";
        m_SPIDRSoftwareVersion = "";
        m_numberOfChipsFound = "";
    }

}


void Mpx3GUI::open_data(bool saveOriginal){

    QString selectedFilter;
    QString filename = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("Native binary files (*.bin);;ASCII matrix (*.txt)"), &selectedFilter);
    if (filename.isEmpty() || filename.isNull()){
        return;
    }
    QFile * toOpenFile = new QFile(filename);
    if ( ! toOpenFile->open(QIODevice::ReadOnly) ) {
        string messg = "Couldn't open: ";
        messg += filename.toStdString();
        messg += "\nNo output written!";
        QMessageBox::warning ( this, tr("Error opening data"), tr( messg.c_str() ) );
        emit open_data_failed();
        return;
    }
    clear_data();

    if ( selectedFilter.contains("Native binary") ) {
        getDataset()->fromByteArray( toOpenFile->readAll() );
    } else if ( selectedFilter.contains("ASCII matrix") ) {

        // For now I read a simple ASCII matrix format
        // No info about the number of layers.
        // No info about the number of chips
        // Single layer
        int x,y,framesPerLayer;
        getDataset()->fromASCIIMatrixGetSizeAndLayers( toOpenFile,
                                                       &x,
                                                       &y,
                                                       &framesPerLayer);
        rebuildCurrentSets(x, y, framesPerLayer);
        getDataset()->fromASCIIMatrix( toOpenFile , x, y, framesPerLayer);
    }

    toOpenFile->close();
    delete toOpenFile;

    set_mode_normal();
    //QList<int> thresholds = getDataset()->getThresholds();

    // required signals
    QRectF bbox = getDataset()->getBoundingBox();
    emit sizeChanged(int(bbox.width() * getDataset()->x()), int(bbox.height() * getDataset()->y())); // goes to qcstmglplot
    emit reload_all_layers();

    // And keep a copy just as in QCstmGLVisualization::data_taking_finished
    if ( saveOriginal ) saveOriginalDataset();

    this->setWindowTitle( _softwareName + QString(": ")+ filename);

    //Ask whether the loaded data is already OBcorrected or not.
    QMessageBox::StandardButton reply = QMessageBox::question( this, tr("Specify data"), tr("Is this data corrected?"), QMessageBox::Yes | QMessageBox::No);
    if(reply == QMessageBox::Yes) getDataset()->setCorrected(true);
    else getDataset()->setCorrected(false);

    return;
}


void Mpx3GUI::open_data_with_path(bool saveOriginal, bool requestPath, QString path)
{
    QString filename;
    if(!requestPath) {
        filename = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("Native binary files (*.bin);;ASCII matrix (*.txt)"));
    } else {
        filename = path;
        loadButtonFilenamePath = path;
    }

//    qDebug() << "[INFO] loading image: " << filename;

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

    // required signals
    QRectF bbox = getDataset()->getBoundingBox();
    emit sizeChanged(int(bbox.width() * getDataset()->x()), int(bbox.height() * getDataset()->y())); // goes to qcstmglplot
    emit reload_all_layers();

    // And keep a copy just as in QCstmGLVisualization::data_taking_finished
    if ( saveOriginal ) saveOriginalDataset();

    /*if(getDataset()->getLayer(0)[0] == 0) {
        qDebug() << "Mpx3GUI::open_data_with_path : "<< getDataset()->getLayer(0)[0];
    }*/

    if(!requestPath) {
        emit returnFilename(filename);
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

    //! None of the following code should be run unless connected (Connect button is visible)
    if (_ui->actionConnect->isVisible()){
        sig_statusBarAppend(tr("Clear equalisation only works when connected"),"black");
        return;
    }
    // Clear adjustment bits
    QMessageBox::StandardButton ans = QMessageBox::question(this, tr("Clear configuration"), tr("The adjustment matrix and the pixel mask will be cleared.  Continue ?") );
    if ( ans == QMessageBox::No ) return;

    bool noEqualization = false;
    if ( _ui->equalizationWidget ) {

        int ndev = config->getNDevicesSupported();
        qDebug() << "[INFO] Number of devices:" << ndev;

        QProgressDialog pd("Clear adjustment bits ... ", "Cancel", 0, ndev, this);
        pd.setCancelButton( nullptr ); // no cancel button
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

void Mpx3GUI::clear_data(bool printToStatusBar) {

    getDataset()->clear();
    emit data_cleared();
    this->setWindowTitle( _softwareName);

    if ( printToStatusBar ) {
        emit sig_statusBarAppend("Clear data","orange");
    }
}

void Mpx3GUI::zero_data(bool printToStatusBar)
{
    getDataset()->zero();
    emit data_zeroed();
    this->setWindowTitle( _softwareName);

    if ( printToStatusBar ) {
        emit sig_statusBarAppend("Zeroed data", "orange");
    }
}

// Change me when adding extra views???
QCstmEqualization * Mpx3GUI::getEqualization(){return _ui->equalizationWidget;}
QCstmGLVisualization * Mpx3GUI::getVisualization() { return _ui->visualizationGL; }
QCstmDacs * Mpx3GUI::getDACs() { return _ui->DACsWidget; }
QCstmConfigMonitoring * Mpx3GUI::getConfigMonitoring() { return _ui->CnMWidget; }
QCstmStepperMotor * Mpx3GUI::getStepperMotor() {return _ui->stepperMotorTab; }
QCstmCT * Mpx3GUI::getCT() { return _ui->ctTab; }
thresholdScan * Mpx3GUI::getTHScan() { return _ui->THScan; }
hdmiConfig * Mpx3GUI::getHdmiConfig() { return _ui->hdmiConfigTab; }

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

//        // Save data --> dialogue
//        save_data(false);
    }

    emit exitApp( 0 );
}

// Change me when adding extra views
void Mpx3GUI::uncheckAllToolbarButtons(){
    _ui->actionVisualization->setChecked(0);
    _ui->actionConfiguration->setChecked(0);
    _ui->actionDACs->setChecked(0);
    _ui->actionEqualization->setChecked(0);
    _ui->actionThreshold_Scan->setChecked(0);
    _ui->actionStepper_Motor->setChecked(0);
    _ui->actionHDMI_Config->setChecked(0);
    // _ui-> NEW ACTION ->setChecked(0);
}


void Mpx3GUI::on_actionConnect_triggered() {
    const QString ipAddr_10Gbit = "192.168.100.10";
    const QString ipAddr_1Gbit  = "192.168.1.10";
    const QString ipAddr_local  = "127.0.0.10";

    // The connection status signal will be sent from establish_connection
    if ( establish_connection() ) {
        emit sig_statusBarAppend( "Connected!", "green" );
        // Can exit now
    } else {
        // Cycle through the 3 common and expected IP addresses
        if ( _lastTriedIPAddress == "" ) {
            getConfig()->setIpAddress(ipAddr_10Gbit);
            _lastTriedIPAddress = ipAddr_10Gbit;
            qDebug() << "[INFO]\tTry to connect to the 10 Gbit interface @" << ipAddr_10Gbit;
            on_actionConnect_triggered();
        } else if ( _lastTriedIPAddress == ipAddr_10Gbit ) {
            getConfig()->setIpAddress(ipAddr_1Gbit);
            _lastTriedIPAddress = ipAddr_1Gbit;
            qDebug() << "[INFO]\tTry to connect to the 1 Gbit interface @" << ipAddr_1Gbit;
            on_actionConnect_triggered();
        } else if ( _lastTriedIPAddress == ipAddr_1Gbit ) {
            getConfig()->setIpAddress(ipAddr_local);
            _lastTriedIPAddress = ipAddr_local;
            qDebug() << "[INFO]\tTry to connect to localhost @" << ipAddr_local;
            on_actionConnect_triggered();
        } else if ( _lastTriedIPAddress == ipAddr_local ) {
            // All options failed, exit with the status bar error message
            emit sig_statusBarAppend( "Auto connection failed", "red" );
        }
    }

    if ( getConfig()->isConnected() ) {
    // Reset _lastTriedIPAddress so that this function can auto search IP Addresses without restarting the program
        _lastTriedIPAddress = "";
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
    _ui->stackedWidget->setCurrentIndex( __equalisation_page_Id );
    _ui->actionEqualization->setChecked(1);
}

//void Mpx3GUI::on_actionScans_triggered(){
//    uncheckAllToolbarButtons();
//    _ui->stackedWidget->setCurrentIndex( __scans_page_Id );
//    // ->setChecked(1);
//}

void Mpx3GUI::on_actionAbout_triggered(bool){
    QMessageBox msgBox;
    msgBox.setWindowTitle("About");
    msgBox.setTextFormat(Qt::RichText);
    QString newLine = "<br>";


    QString frameworks = QString("This program uses the following frameworks and libraries with their respective licences listed:")
                        + newLine
                        + QString("<ul>")
                        + QString("<li> Qt - Commercial - <a href='https://doc.qt.io/qt-5/licensing.html'>doc.qt.io/qt-5/licensing.html</a> </li>")
                        + QString("<li> Boost - Boost - <a href='http://www.boost.org/LICENSE_1_0.txt'>boost.org/LICENSE_1_0.txt</a> </li>")
                        + QString("<li> LibTiff - BSD like - <a href='http://simplesystems.org/libtiff/misc.html'>simplesystems.org/libtiff/misc.html</a> </li>")
                        + QString("</ul>")
                        + newLine
                        + QString("For the BSD licensed software, see the BSD license : <a href='https://opensource.org/licenses/BSD-3-Clause'>opensource.org/licenses/BSD-3-Clause</a>");



    QString msg = QString("Version: ") + _softwareVersion +
            newLine +
            newLine +
            QString("Compiled: ") + compileDateTime +
            newLine +
            QString("C++: ") + QString::fromStdString(to_string(__cplusplus)) +
            newLine +
            m_SPIDRControllerVersion +
            newLine +
            m_SPIDRFirmwareVersion +
            newLine +
            m_SPIDRSoftwareVersion +
            newLine +
            m_numberOfChipsFound +
            newLine +
            newLine +
            QString("Authors: ") +
            QString("Navrit Bal (2016-), Bram Bouwens (2018-), Kiavash Mortezavi Matin (2018-), John Idarraga (2014-2017), Amber van Keeken (2016), Roel Deckers, Cyrano Chatziantoniou") +
            newLine +
            newLine +
            QString("Contributors: ") +
            QString("Frans Schreuder, Henk Boterenbrood, Martin van Beuzekom") +
            newLine +
            newLine +
            newLine +
            frameworks +
            newLine +
            newLine +
            newLine +
            QString("ASI B.V. All rights reserved.") ;
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
    if ( _spidrdaq ) {
        _spidrdaq->stop();
        delete _spidrdaq;
        _spidrdaq = nullptr;
    }
    getConfig()->closeConnection();

    emit ConnectionStatusChanged( checked ); // false when disconnecting

    emit sig_statusBarAppend( "Disconnected", "red" );
}

void Mpx3GUI::on_actionDefibrillator_triggered(bool checked){
    Q_UNUSED(checked);

    if ( getConfig()->isConnected() ) {
        QProgressDialog pd("System reset in progress ... ", "Cancel", 0, 3, this);
        pd.setCancelButton( nullptr ); // no cancel button
        pd.setWindowModality(Qt::WindowModal);
        pd.setMinimumDuration( 0 ); // show immediately
        pd.setWindowTitle("Reset");

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

        // Done
        pd.setValue( 3 );
    }
}

void Mpx3GUI::on_actionThreshold_Scan_triggered()
{
    uncheckAllToolbarButtons();
    _ui->stackedWidget->setCurrentIndex( __thresholdScan_page_Id );
    _ui->actionThreshold_Scan->setChecked(1);
}

void Mpx3GUI::on_actionHDMI_Config_triggered()
{
    uncheckAllToolbarButtons();
    _ui->stackedWidget->setCurrentIndex( __hdmi_config_page_Id );
    _ui->actionHDMI_Config->setChecked(1);
}

void Mpx3GUI::autoConnectToDetector() {
  qDebug() << "[INFO]\tConnecting automatically to detector";

  on_actionConnect_triggered();
  if (GetSpidrController() != nullptr && GetSpidrController()->isConnected()) {
    connect(getEqualization(), SIGNAL(equalizationPathExported(QString)), this,
            SLOT(onEqualizationPathExported(QString)));
    _loadEqualizationFromGeneralSettings();
  }
}

void Mpx3GUI::shutterOpenTimer_timeout() {
  qDebug() << "[INFO]\tShutter timer stop open:" << _timerStop;
  if (_timerStop) {
    shutterCloseTimer->stop();
    shutterOpenTimer->stop();
    return;
  }
  GetSpidrController()->stopAutoTrigger(); // Close shutter
  shutterOpenTimer->stop();                // Stop shutter open timer

  shutterCloseTimer->start(int(
      getConfig()->getTriggerDowntime_ms_64())); // Start shutter close timer
  //! NOTE 1. This loses precision (uint64_t to int)
  //!      2. The maximum value will be not as intended, it will be clipped if
  //!      over 2^32-1
  //! Suboptimal solution: For clarity - cast as an int here, it is cast as
  //! an int later anyway...

  //! Not expecting this to happen
  if (getConfig()->getTriggerDowntime_ms_64() >= INT_MAX) {
      qDebug() << "[WARNING]\tShutter close timer argument has lost precision and the maximum value has been limited to" << INT_MAX;
  }
}

void Mpx3GUI::shutterCloseTimer_timeout() {
  qDebug() << "[INFO]\tShutter timer stop close:" << _timerStop;
  if (_timerStop) {
    shutterCloseTimer->stop();
    shutterOpenTimer->stop();
    return;
  }
  auto config = getConfig();
  GetSpidrController()->startAutoTrigger(); // Open shutter

  shutterOpenTimer->start(int(config->getTriggerLength_ms_64()));
  //! NOTE 1. This loses precision (uint64_t to int)
  //!      2. The maximum value will be not as intended, it will be clipped if
  //!      over 2^32-1
  //! Suboptimal solution: For clarity - cast as an int here, it is cast as
  //! an int later anyway...
  if (config->getTriggerDowntime_ms_64() >= INT_MAX) {
      qDebug() << "[WARNING]\tShutter open timer argument has lost precision and the maximum value has been limited to" << INT_MAX;
  }

  shutterCloseTimer->stop();
}
