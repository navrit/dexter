/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#include "mpx3gui.h"
#include "mpx3equalization.h"
#include "qcstmglvisualization.h"
#include "ui_mpx3gui.h"

#include "qcustomplot.h"
#include "mpx3eq_common.h"
//#include "DACs.h"
#include "mpx3defs.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "barchart.h"
#include "ThlScan.h"
#include "gradient.h"

#include <QMessageBox>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

Mpx3GUI::Mpx3GUI(QApplication * coreApp, QWidget * parent) :	QMainWindow(parent), _coreApp(coreApp), _ui(new Ui::Mpx3GUI)
{
	// Instantiate everything in the UI
	_ui->setupUi(this);
	config = new Mpx3Config;
	workingSet = new Dataset(256, 256);
	workingSet->setFramesPerGroup(2,2);
	workingSet->setOrientation(0, Dataset::orientationTtBLtR);
	workingSet->setOrientation(3, Dataset::orientationTtBLtR);
	workingSet->setOrientation(1, Dataset::orientationBtTRtL);
	workingSet->setOrientation(2, Dataset::orientationBtTRtL);
	gradients = Gradient::fromJsonFile("./config/heatmaps.json");
	QStringList gradientNames;
	for(int i = 0; i < gradients.length();i++)
	  gradientNames.append(gradients[i]->getName());

	// Prepare DACs panel
	//_dacs = new DACs(_coreApp, _ui);
	//_dacs->SetMpx3GUI( this );

	_ui->DACsWidget->SetMpx3GUI( this );

	// Prepare Equalization
	_equalization = new Mpx3Equalization(_coreApp, _ui);
	_equalization->SetMpx3GUI( this );

	// Prepare Visualization
	_ui->visualizationGL->SetMpx3GUI(this);
	_ui->CTTab->SetMpx3GUI(this);
	emit availible_gradients_changed(gradientNames);

	// Prepare THL Calibration
	_ui->ThresholdTab->SetMpx3GUI(this);

	//Config & monitoring
	_ui->CnMWidget->SetMpx3GUI(this);
	// Read the configuration
	QString configFile = "./config/mpx3.json";
	if ( ! config->fromJsonFile( configFile ) ) {
	QMessageBox::critical(this, "Loading configuration error",
					QString("Couldn't load the configuration file: %1").arg(configFile));
	}
	// Signals and slots for this part
	SetupSignalsAndSlots();
}

Mpx3GUI::~Mpx3GUI()
{
	delete config;
	delete workingSet;
	delete _ui;
}

Gradient* Mpx3GUI::getGradient(int index){
  return gradients.at(index);
}

void Mpx3GUI::LoadEqualization(){
	_equalization->LoadEqualization();
}

void Mpx3GUI::SetupSignalsAndSlots(){

	std::cout << "[Mpx3GUI] Connecting signals and slots" << std::endl;
	connect( _ui->actionLoad_Equalization, SIGNAL(triggered()), this, SLOT( LoadEqualization() ) );
	connect( _ui->actionSave_DACs, SIGNAL(triggered()), _ui->DACsWidget, SLOT( openWriteMenu() ) );
	connect( _ui->actionConnect, SIGNAL(triggered()), this, SLOT( establish_connection() ) );

	connect(_ui->actionSumming, SIGNAL(triggered()), this, SLOT(set_mode_integral()));
	connect(_ui->actionDiscrete, SIGNAL(triggered()), this, SLOT(set_mode_normal()));

	connect(_ui->actionSave_data, SIGNAL(triggered()), this, SLOT(save_data()));
	connect(_ui->actionSave_Equalization, SIGNAL(triggered()), _equalization, SLOT(SaveEqualization()));
	connect(_ui->actionOpen_data, SIGNAL(triggered()), this, SLOT(open_data()));
	connect(_ui->actionClear_data, SIGNAL(triggered()), this, SLOT(clear_data()));
	connect(_ui->actionClear_configuration, SIGNAL(triggered()), this, SLOT(clear_configuration()) );

	// Inform every module of changes in connection status
	connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->DACsWidget, SLOT( ConnectionStatusChanged() ) );
	connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _equalization, SLOT( ConnectionStatusChanged() ) );
	connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->visualizationGL, SLOT( ConnectionStatusChanged() ) );

}
Mpx3Config* Mpx3GUI::getConfig(){return config;}
void Mpx3GUI::on_openfileButton_clicked() {

}

void Mpx3GUI::set_summing(bool shouldSum){
  if(shouldSum)
    set_mode_integral();
  else
    set_mode_normal();
}

void Mpx3GUI::establish_connection() {

	cout << "Connecting ..." << endl;
	_spidrcontrol = config->establishConnection();

	// Check if we are properly connected to the SPIDR module
	if ( _spidrcontrol->isConnected() ) {
		cout << "Connected to SPIDR: " << _spidrcontrol->ipAddressString();
		int ipaddr;
		 // This call takes device number 0 'cause it is not really addressed to a chip in particular
		if( _spidrcontrol->getIpAddrDest( 0, &ipaddr ) )
			cout << ", IP dest: "
			<< ((ipaddr>>24) & 0xFF) << "."
			<< ((ipaddr>>16) & 0xFF) << "."
			<< ((ipaddr>> 8) & 0xFF) << "."
			<< ((ipaddr>> 0) & 0xFF);
		cout <<  endl;
		//_ui->_statusLabel->setText("Connected");
		//_ui->_statusLabel->setStyleSheet("QLabel { background-color : blue; color : white; }");

	} else {
		cout << _spidrcontrol->connectionStateString() << ": "
				<< _spidrcontrol->connectionErrString() << endl;
		//_ui->_statusLabel->setText("Connection failed.");
		//_ui->_statusLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
		QMessageBox::critical(this, "Connection error",
				QString("Couldn't establish a connection to the Spidr controller at %1, %2").arg(QString::fromStdString(_spidrcontrol->ipAddressString())).arg(QString::fromStdString(_spidrcontrol->connectionErrString())));
		emit ConnectionStatusChanged(false);
		return; //No use in continuing if we can't connect.
	}

	// Get version numbers
	cout << "SpidrController class: "
			<< _spidrcontrol->versionToString( _spidrcontrol->classVersion() ) << endl;
	int version;
	if( _spidrcontrol->getFirmwVersion( &version ) )
		cout << "SPIDR firmware  : " << _spidrcontrol->versionToString( version ) << endl;
	if( _spidrcontrol->getSoftwVersion( &version ) )
		cout << "SPIDR software  : " << _spidrcontrol->versionToString( version ) << endl;

	// SpidrDaq
	_spidrdaq = new SpidrDaq( _spidrcontrol );
	cout << "SpidrDaq: ";
	for( int i=0; i<4; ++i ) cout << _spidrdaq->ipAddressString( i ) << " ";
	cout << endl;
	Sleep( 1000 );
	cout << _spidrdaq->errorString() << endl;

	// Emmit
	emit ConnectionStatusChanged(true);

	// A connection to hardware should make aware the DAC panel
	//_moduleConn->GetDACs()->ConnectToHardware(_spidrcontrol, _spidrdaq);
	//_dacs->PopulateDACValues();

}

void Mpx3GUI::generateFrame(){//TODO: put into Dataset
	printf("Generating a frame!\n");
	double fx = ((double)8*rand()/RAND_MAX)/(workingSet->x()), fy = (8*(double)rand()/RAND_MAX)/workingSet->y();
	QVector<int> data(workingSet->x()/2*workingSet->y()/2);
	for(int i = 0; i < workingSet->y()/2; i++)
		for(int j = 0; j < workingSet->x()/2; j++)
			data[i*workingSet->x()/2+j] = 2*j+i;//(int)((1<<14)*sin(fx*j)*(cos(fy*i)));
	addFrame(data.data());
}
void Mpx3GUI::addFrame(int *frame){
  QVector<int*> wrapper(1);
  wrapper[0] = frame;
  this->addFrames(wrapper);
}
void Mpx3GUI::set_active_frame(int index){
  workingSet->setActive(index);
  emit active_frame_changed(index);
}

void Mpx3GUI::addFrames(QVector<int*> frames){
  for(int i = 0; i < frames.length(); i++){
	if(0 == mode || 0 == workingSet->getFrameCount()){//normal mode, or no frame yet
		workingSet->addFrame(frames[i]);
		hists.push_back(new histogram(frames[i], workingSet->x()/2*workingSet->y()/2,  1));
		emit hist_added();
	}
	else if(1 == mode){ // Summing mode
		workingSet->sumFrame(frames[i]);
		histogram *old = hists[workingSet->getActiveIndex()];//TODO: do this better
		delete old;
		old = new histogram(workingSet->getActiveFrame(),workingSet->x()*workingSet->y());
	}
  }
   if(0 == mode || 0 == workingSet->getFrameCount())
    emit frame_added();// --> emit something else.
   else if(1 == mode)
    emit frames_reload();
}

int Mpx3GUI::getPixelAt(int x, int y, int layer){
  return workingSet->sample(x,y, layer);
  //if(layer >= data.length() || x >= nx || y >= ny)
  // return 0;
  //return data[layer][y*nx+x];
}

QPoint Mpx3GUI::getSize(){
  return workingSet->getSize();
}

void Mpx3GUI::getSize(int *x, int *y){
	QPoint dataSize = workingSet->getSize();
	*x = dataSize.x();
	*y = dataSize.y();
}
int Mpx3GUI::getX(){
	return workingSet->getSize().x();
}
int Mpx3GUI::getY(){
	return workingSet->getSize().y();
}

int Mpx3GUI::getFrameCount(){
	return workingSet->getFrameCount();
}


void Mpx3GUI::save_data(){//TODO: does not append file suffix, error checking.
	QString filename = QFileDialog::getSaveFileName(this, tr("Save Data"), tr("."), tr("binary files (*.bin)"));
	QFile saveFile(filename);
	if (!saveFile.open(QIODevice::WriteOnly)) {
		string messg = "Couldn't open: ";
		messg += filename.toStdString();
		messg += "\nNo output written!";
		QMessageBox::warning ( this, tr("Error saving data"), tr( messg.c_str() ) );
		return;
	}
	QPoint dataSize = workingSet->getSize();
	int nx = dataSize.x(); int ny = dataSize.y();
	if(-1 == saveFile.write((const char*)&nx, sizeof(nx))) QMessageBox::warning ( this, tr("Error saving data"), tr( "Couldn't save nx!") );
	saveFile.write((const char*)&ny, sizeof(ny));
	int nLayers = workingSet->getFrameCount();
	saveFile.write((const char*)&nLayers, sizeof(nLayers));
	for(int i = 0; i < nLayers;i++){
		saveFile.write((const char*)workingSet->getFrame(i), nx*ny*sizeof(*workingSet->getFrame(i)));
	}
	saveFile.close();
	return;
}

void Mpx3GUI::open_data(){
	QString filename = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("binary files (*.bin)"));
	QFile saveFile(filename);
	if (!saveFile.open(QIODevice::ReadOnly)) {
		string messg = "Couldn't open: ";
		messg += filename.toStdString();
		messg += "\nNo output written!";
		QMessageBox::warning ( this, tr("Error opening data"), tr( messg.c_str() ) );
		return;
	}
	clear_data();
	int nx, ny;
	saveFile.read((char*)&nx, sizeof(nx));
	saveFile.read((char*)&ny, sizeof(ny));
	delete workingSet;
	workingSet = new Dataset(nx, ny);
	workingSet->setFramesPerGroup(2,2);
	workingSet->setOrientation(0, Dataset::orientationLtRTtB);
	workingSet->setOrientation(3, Dataset::orientationLtRTtB);
	workingSet->setOrientation(1, Dataset::orientationLtRTtB);
	workingSet->setOrientation(2, Dataset::orientationLtRTtB);
	int nLayers;
	saveFile.read((char*)&nLayers, sizeof(nLayers));
	QVector<int*> newFrames(nLayers);
	for(int i = 0; i < nLayers;i++){
	  newFrames[i] = new int[nx*ny];
	  saveFile.read((char*)newFrames[i], nx*ny*sizeof(int));
	  this->addFrame(newFrames[i]);
	}
	saveFile.close();
	//this->addFrames(newFrames);
	for(int i = 0; i < nLayers;i++)
	  delete[] newFrames[i];
	return;
}

void Mpx3GUI::set_mode_integral(){
	if(mode != 1){
	  mode = 1;
	  emit set_summing(true);
	}
}

void Mpx3GUI::set_mode_normal(){
  if(0 != mode){
	mode = 0;
	emit set_summing(false);
    }
}

void Mpx3GUI::clear_configuration(){

	// Clear adjustement bits
	QMessageBox::StandardButton ans = QMessageBox::question(this, tr("Clear configuration"), tr("The adjustment matrix and the pixel mask will be cleared.  Continue ?") );
	if ( ans == QMessageBox::No ) return;

	bool noEqualization = false;
	if ( _equalization ) {
		if ( _equalization->GetEqualizationResults() ) {

			cout << "[INFO] clearing adjustment bits and mask." << endl;

			_equalization->ClearAllAdjustmentBits();

		} else {
			noEqualization = true;
		}

	} else { noEqualization = true; }

	if ( noEqualization ) {
		cout << "[INFO] No equalization has been loaded. Nothing to clear." << endl;
	}

}

void Mpx3GUI::clear_data(){
  workingSet->clear();
  for(int i = 0; i < hists.length(); i++)
    delete hists[i];
  hists.clear();
  emit(data_cleared());
}

void Mpx3GUI::start_data_taking(){
    SpidrController * spidrcontrol = this->GetSpidrController();
    SpidrDaq * spidrdaq = this->GetSpidrDaq();

    cout << "Acquiring ... ";

    // Start the trigger as configured
    spidrcontrol->startAutoTrigger();//getNTriggers
    Sleep( 50 );
    // See if there is a frame available
    // I should get as many frames as triggers
    int * framedata;
    while ( spidrdaq->hasFrame() ) {
            //cout << "capture ..." << endl;
            int size_in_bytes = -1;
            framedata = spidrdaq->frameData(0, &size_in_bytes);

            this->addFrame( framedata );

	    spidrdaq->releaseFrame();
	    Sleep( 10 ); // Allow time to get and decode the next frame, if any
    }

    cout << "done." << endl;

}
