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


	// Prepare DACs panel
	_dacs = new DACs(_coreApp, _ui);

	// Prepare Equalization
	_equalization = new Mpx3Equalization(_coreApp, _ui);


	// Connectivity between modules
	_moduleConn = new ModuleConnection;
	_moduleConn->SetDACs( _dacs );
	_moduleConn->SetEqualization( _equalization );

	// Pulling down the cables
	_dacs->SetModuleConnection( _moduleConn );
	_equalization->SetModuleConnection( _moduleConn );

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
	std::cout << "Connecting signals and slots" << std::endl;
	connect( _ui->actionLoad_Equalization, SIGNAL(triggered()), _equalization, SLOT( LoadEqualization() ) );
	connect( _ui->actionSave_DACs, SIGNAL(triggered()), _dacs, SLOT( openWriteMenu() ) );
}
