/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#include <stdlib.h>     /* srand, rand */


#include "spidrmpx3eq.h"
#include <unistd.h>
//#define Sleep(ms) usleep(ms*1000)

using namespace std;
#include <iostream>

#include "qcustomplot.h"

#include "stdio.h"

#include "mpx3eq_common.h"

SpidrMpx3Eq::SpidrMpx3Eq(QWidget *parent) :
    												QMainWindow(parent),
    												ui(new Ui::SpidrMpx3Eq)
{
	ui->setupUi(this);

	connect( ui->_startEq, SIGNAL(clicked()), this, SLOT(StartEqualization()) );
	connect( ui->_connect, SIGNAL(clicked()), this, SLOT(Connect()) );
	ui->_statusLabel->setStyleSheet("QLabel { background-color : gray; color : black; }");

	// Create the bar chart
	_chart = new BarChart( ui->_histoFrame );

	//_customPlot = new QCustomPlot;
	//_chart = new BarChart( this );
	_chart->setParent( ui->_histoFrame );
	// Make it fit in it's parent
	QRect hrect = ui->_histoFrame->geometry();
	_chart->resize( hrect.size().rwidth() , hrect.size().rheight() );

	// Set One histogram
	BarChartProperties * cprop = new BarChartProperties;
	cprop->name.push_back("Adj0");
	cprop->min_x.push_back(0);
	cprop->max_x.push_back(511);
	cprop->nBins.push_back(511);
	cprop->color_r.push_back(150);
	cprop->color_g.push_back(222);
	cprop->color_b.push_back(0);

	_chart->SetBarChartProperties( cprop );
	_chart->PrepareSets();


	//startTimer( 100 );


	//
	Connect();

	// Ready for scans
	_tscan = new ThlScan(_spidrcontrol, _spidrdaq, _chart);

	// some randon numbers
	srand (time(NULL));
}


SpidrMpx3Eq::~SpidrMpx3Eq()
{
	delete _chart;
	delete ui;
}


void SpidrMpx3Eq::StartEqualization(){

	int i;
	int dev_nr = 2;
	int * data;


	_tscan->DoScan();

	/*
	_spidrcontrol->setDac( dev_nr, MPX3_DAC_THRESH_1, 511 );

	for( i=0; i < 511; i+=100 ) {

		cout << "THL : " << i << " -------------------------" << endl;

		_spidrcontrol->setDac( dev_nr, MPX3_DAC_THRESH_0, i );

		_spidrcontrol->writeDacs( dev_nr );

		_spidrcontrol->startAutoTrigger();
		Sleep( 50 );

		while ( _spidrdaq->hasFrame() ) {

			int size_in_bytes = -1;
			data = _spidrdaq->frameData(0, &size_in_bytes);
			//cout << "Size in bytes = " << size_in_bytes << endl;
			//PrintFraction(data, size_in_bytes, 100);
			int nActive = GetNPixelsActive(data, size_in_bytes, __VERBL_DEBUG);

			cout << ", active : " << nActive << " ";

			double t = _spidrdaq->frameTimestampDouble();
			unsigned int secs = (unsigned int) t;
			unsigned int msecs = (unsigned int) ((t - secs)*1000.0+0.5);
			//cout << "T=" << fixed << setprecision(9)
			// << spidrdaq.frameTimestampDouble() << endl;

			//cout << "T=" << secs << "."
			//		<< setfill('0') << setw(3) << msecs
			//		<< ", " << spidrdaq.framesProcessedCount()
			//		<< ", Ts=" << hex << spidrdaq.frameTimestampSpidr()
			//		<< dec << endl;

			//for	( int j = 0 ; j < N ; j++ ) {
			//	cout << data[j] << ", ";
			//}

			_spidrdaq->releaseFrame();
			Sleep( 50 ); // Allow time to get and decode the next frame, if any

		}

		cout << " " << endl;

		//cout << "DAQ frames: " << spidrdaq.framesCount() << ", lost "
		//		<< spidrdaq.framesLostCount() << ", lost pkts "
		//		<< spidrdaq.packetsLostCount() << ", exp seqnr (dev 0) "
		//		<< spidrdaq.expSequenceNr( 0 ) << endl;
	}

	*/

}

void SpidrMpx3Eq::Connect() {

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
		ui->_statusLabel->setText("Connected");
		ui->_statusLabel->setStyleSheet("QLabel { background-color : blue; color : white; }");

	} else {
		cout << _spidrcontrol->connectionStateString() << ": "
				<< _spidrcontrol->connectionErrString() << endl;
		ui->_statusLabel->setText("Connection failed.");
		ui->_statusLabel->setStyleSheet("QLabel { background-color : red; color : black; }");

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
	cout << _spidrdaq->errString() << endl;


	/*
	// Reset pixel configuration
	_spidrcontrol->resetPixelConfig();
	//_spidrcontrol->writePixelConfigMpx3rx( dev_nr );

	// OMR
	//_spidrcontrol->setPolarity( true );		// Holes collection
	//_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
	//_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse
	_spidrcontrol->setPixelDepth( 12 );

	_spidrcontrol->setColourMode( false ); 	// Fine Pitch
	_spidrcontrol->setCsmSpm( 0 );			// Single Pixel mode
	_spidrcontrol->setEqThreshH( true );
	_spidrcontrol->setDiscCsmSpm( 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH

	// Gain ?!

	// Other OMR
	_spidrdaq->setDecodeFrames( true );
	_spidrcontrol->setPixelDepth( 12 );
	_spidrdaq->setPixelDepth( 12 );
	_spidrcontrol->setMaxPacketSize( 1024 );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = 4;      // Auto-trigger mode
	int trig_length_us = 10000;  // This time shouldn't be longer than the period defined by trig_freq_hz
	int trig_freq_hz   = 5;
	int nr_of_triggers = 1;		 // This is the number of shutter open i get
	int trig_pulse_count;
	_spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );
*/


}


void SpidrMpx3Eq::timerEvent( QTimerEvent * evt ) {


	//double val = rand() % 510 + 1 ;
	//cout << "append : " << val << endl;

	// filling the histo with random numbers
	//_chart->PushBackToSet( 0, val );
	////QCPBarData bin_content = (*(_chart->GetDataSet(0)->data()))[val];

	//double cont = (*dmap)[val];
	////_chart->GetDataSet(0)->addData( bin_content.key , bin_content.value +1 );
	//_chart->DumpData();
	//_chart->clearGraphs();
	////_chart->replot();
	//_chart->update();

	//_chart->SetValueInSet(0, 100);

}

void SpidrMpx3Eq::PrintFraction(int * buffer, int size, int first_last) {

	cout << "< ";
	for(int i = 0 ; i < first_last ; i++) {
		cout << buffer[i];
		if(i<first_last-1) cout << ", ";
	}
	cout << " . . . ";
	for(int i = size-1-first_last ; i < size ; i++) {
		cout << buffer[i];
		if(i<size-1) cout << ", ";
	}
	cout << " >" << endl;
}

int SpidrMpx3Eq::GetNPixelsActive(int * buffer, int size, verblev verbose) {

	int NPixelsActive = 0;

	// 4 bits per pixel
	int nPixels = size/4;
	for(int i = 0 ; i < nPixels ; i++) {
		pair<int, int> pix = XtoXY(i, __matrix_size_x);
		if(buffer[i] != 0) {
			if ( verbose == __VERBL_DEBUG ) {
				printf("(%d,%d) counts: %d \n",pix.first, pix.second, buffer[i]);
			}
			NPixelsActive++;
		}
	}

	return NPixelsActive;
}

pair<int, int> SpidrMpx3Eq::XtoXY(int X, int dimX){
	return make_pair(X % dimX, X/dimX);
}
