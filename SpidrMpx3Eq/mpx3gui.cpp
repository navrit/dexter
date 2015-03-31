/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#include "mpx3gui.h"
#include "mpx3equalization.h"
#include "ui_mpx3gui.h"

#include "qcustomplot.h"
#include "mpx3eq_common.h"
#include "DACs.h"
#include "mpx3defs.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "barchart.h"
#include "ThlScan.h"

#include <QMessageBox>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>


Mpx3GUI::Mpx3GUI(QApplication * coreApp, QWidget * parent) :	QMainWindow(parent), _coreApp(coreApp), _ui(new Ui::Mpx3GUI)
{
	// Instantiate everything in the UI
	_ui->setupUi(this);
	//index various heatmaps
	gradients.insert("Grayscale", QCPColorGradient::gpGrayscale);
	gradients.insert("Jet", QCPColorGradient::gpJet);
	gradients.insert("Ion", QCPColorGradient::gpIon);
	gradients.insert("Candy", QCPColorGradient::gpCandy);
	gradients.insert("Hot", QCPColorGradient::gpHot);
	gradients.insert("Cold", QCPColorGradient::gpCold);
	emit availible_gradients_changed(QStringList(gradients.keys()));
	gradients.insert("Thermal", QCPColorGradient::gpThermal);
	emit gradient_added("Thermal");
	startTimer( 200 );// TODO: use of this?

	// Connectivity between modules
	//_moduleConn = new ModuleConnection;

	// Prepare DACs panel
	_dacs = new DACs(_coreApp, _ui);
	_dacs->SetMpx3GUI( this );

	// Prepare Equalization
	_equalization = new Mpx3Equalization(_coreApp, _ui);
	_equalization->SetMpx3GUI( this );

	// Prepare Visualization
	_ui->visualizationWidget->SetMpx3GUI( this );
	_ui->visualizationWidget->SignalsAndSlots();
	_ui->visualizationWidget->set_gradient("Thermal");

	// Signals and slots for this part
	SetupSignalsAndSlots();

	_ui->glWidget->SetMpx3GUI(this);

	// Connect automatically
	//_moduleConn->Connection();
}

Mpx3GUI::~Mpx3GUI()
{
	for(int i = 0; i < data.size(); i++)
		delete[] data[i];
	delete _ui;
}

void Mpx3GUI::timerEvent( QTimerEvent * /*evt*/ ) {


}

void Mpx3GUI::LoadEqualization(){
	_equalization->LoadEqualization();
}

void Mpx3GUI::SetupSignalsAndSlots(){

	std::cout << "[Mpx3GUI] Connecting signals and slots" << std::endl;
	connect( _ui->actionLoad_Equalization, SIGNAL(triggered()), this, SLOT( LoadEqualization() ) );
	connect( _ui->actionSave_DACs, SIGNAL(triggered()), _dacs, SLOT( openWriteMenu() ) );
	connect( _ui->actionConnect, SIGNAL(triggered()), this, SLOT( establish_connection() ) );

	connect(_ui->actionSumming, SIGNAL(triggered()), this, SLOT(set_mode_integral()));
	connect(_ui->actionDiscrete, SIGNAL(triggered()), this, SLOT(set_mode_normal()));

	connect( this, SIGNAL(frame_changed()), _ui->visualizationWidget, SLOT(on_frame_changed()) );

	connect(_ui->actionSave_data, SIGNAL(triggered()), this, SLOT(save_data()));
	connect(_ui->actionSave_Equalization, SIGNAL(triggered()), _equalization, SLOT(SaveEqualization()));
	connect(_ui->actionOpen_data, SIGNAL(triggered()), this, SLOT(open_data()));
	connect(_ui->actionClear_data, SIGNAL(triggered()), this, SLOT(clear_data()));
	connect(_ui->actionClear_configuration, SIGNAL(triggered()), this, SLOT(clear_configuration()) );

	// Inform every module of changes in connection status
	connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _dacs, SLOT( ConnectionStatusChanged() ) );
	connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _equalization, SLOT( ConnectionStatusChanged() ) );
	connect( this, SIGNAL( ConnectionStatusChanged(bool) ), _ui->visualizationWidget, SLOT( ConnectionStatusChanged(bool) ) );

}

void Mpx3GUI::on_openfileButton_clicked() {

}

void Mpx3GUI::establish_connection() {
	int dev_nr = 2;
	cout << "Connecting ..." << endl;
	delete _spidrcontrol;
	_spidrcontrol = new SpidrController( 192, 168, 1, 10 );

	// Check if we are properly connected to the SPIDR module
	if ( _spidrcontrol->isConnected() ) {
		cout << "Connected to SPIDR: " << _spidrcontrol->ipAddressString();
		int ipaddr;
		if( _spidrcontrol->getIpAddrDest( dev_nr, &ipaddr ) )
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

void Mpx3GUI::generateFrame(){
	printf("Generating a frame!\n");
	double fx = ((double)8*rand()/RAND_MAX)/(nx), fy = (8*(double)rand()/RAND_MAX)/ny;
	QVector<int> data(nx*ny);
	for(int i = 0; i < ny; i++)
		for(int j = 0; j < nx; j++)
			data[i*nx+j] = (int)((1<<14)*sin(fx*j)*(cos(fy*i)));
	addFrame(data.data());
}

void Mpx3GUI::addFrame(int * origframe){

	// Make a local copy first
	int * frame = new int[nx*ny];
	for(int i = 0; i < nx*ny; i++) frame[i] = origframe[i];


	if(0 == mode || 0 == data.size()){//normal mode, or no frame yet
		data.push_back(frame);
		hists.push_back(new histogram(frame, nx*ny, 1));
		emit frame_added();
		emit frames_reload(data);
	}
	else if(1 == mode){ // Summing mode

		for(int i = 0; i < ny; i++) {

			for(int j = 0; j < nx; j++){

				data[data.size()-1][i*nx+j] += frame[i*nx+j];

			}
		}
		histogram *last = hists[hists.size()-1];
		hists.pop_back();
		delete last;
		last = new histogram(data[data.size()-1],nx*ny);
		hists.push_back(last);
		//hists.last()->addCount(frame, nx*ny);
		emit frame_changed();
	}
}

QPoint Mpx3GUI::getSize(){
  return QPoint(nx, ny);
}

void Mpx3GUI::getSize(int *x, int *y){
	*x = nx;
	*y = ny;
}
int Mpx3GUI::getX(){
	return nx;
}
int Mpx3GUI::getY(){
	return ny;
}

int Mpx3GUI::getFrameCount(){
	return (int)data.size();
}

QCPColorGradient Mpx3GUI::getGradient(QString index){
	return gradients[index];
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
	if(-1 == saveFile.write((const char*)&nx, sizeof(nx))) QMessageBox::warning ( this, tr("Error saving data"), tr( "Couldn't save nx!") );
	saveFile.write((const char*)&ny, sizeof(ny));
	int nLayers = (int)data.size();
	saveFile.write((const char*)&nLayers, sizeof(nLayers));
	for(int i = 0; i < nLayers;i++){
		saveFile.write((const char*)data[i], nx*ny*sizeof(*data[i]));
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
	saveFile.read((char*)&nx, sizeof(nx));
	saveFile.read((char*)&ny, sizeof(ny));
	int nLayers;
	saveFile.read((char*)&nLayers, sizeof(nLayers));
	for(int i = 0; i < nLayers;i++){
		int* newFrame = new int[nx*ny];
		saveFile.read((char*)newFrame, nx*ny*sizeof(int));
		this->addFrame(newFrame);
	}
	saveFile.close();
	return;
}

void Mpx3GUI::set_mode_integral(){
	mode = 1;
}

void Mpx3GUI::set_mode_normal(){
	mode = 0;
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
  for(int i = 0; i < data.size(); i++)
    delete[] data[i];
  data.clear();
  hists.clear();
  emit(data_cleared());
}
