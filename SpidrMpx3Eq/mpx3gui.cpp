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

	startTimer( 200 );

	// Connectivity between modules
	_moduleConn = new ModuleConnection;

	// Prepare DACs panel
	_dacs = new DACs(_coreApp, _ui);
	_dacs->SetMpx3GUI( this );

	// Prepare Equalization
	_equalization = new Mpx3Equalization(_coreApp, _ui);
	_equalization->SetMpx3GUI( this );

	// Prepare Visualization
	_ui->widget->SetMpx3GUI( this );

	// Signals and slots for this part
	SetupSignalsAndSlots();
}

Mpx3GUI::~Mpx3GUI()
{
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
	connect( _ui->actionConnect, SIGNAL(triggered()), _moduleConn, SLOT( Connection() ) );

	// Inform every module of changes in connection status
	connect( _moduleConn, SIGNAL( ConnectionStatusChanged() ), _dacs, SLOT( ConnectionStatusChanged() ) );
	connect( _moduleConn, SIGNAL( ConnectionStatusChanged() ), _equalization, SLOT( ConnectionStatusChanged() ) );
	connect( _moduleConn, SIGNAL( ConnectionStatusChanged() ), _ui->widget, SLOT( ConnectionStatusChanged() ) );

}

void Mpx3GUI::on_openfileButton_clicked() {

}

void ModuleConnection::Connection() {

	int dev_nr = 2;
	cout << "Connecting ..." << endl;
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
	emit ConnectionStatusChanged();

	// A connection to hardware should make aware the DAC panel
	//_moduleConn->GetDACs()->ConnectToHardware(_spidrcontrol, _spidrdaq);
	//_moduleConn->GetDACs()->PopulateDACValues();

}
