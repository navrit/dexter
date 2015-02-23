/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#include "mpx3gui.h"
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
	_ui->setupUi(this);

        //index various heatmaps
        heatmapMap.insert("Grayscale", QCPColorGradient::gpGrayscale);
        heatmapMap.insert("Jet", QCPColorGradient::gpJet);
        /*heatmapMap.insert("Ion", QCPColorGradient::gpIon);
        heatmapMap.insert("Candy", QCPColorGradient::gpCandy);*/
        heatmapMap.insert("Hot", QCPColorGradient::gpHot);
        heatmapMap.insert("Cold", QCPColorGradient::gpCold);
        heatmapMap.insert("Thermal", QCPColorGradient::gpThermal);
        //and set them
        _ui->heatmapCombobox->addItems(heatmapMap.keys());

	connect( _ui->_startEq, SIGNAL(clicked()), this, SLOT(StartEqualization()) );
	connect( _ui->_connect, SIGNAL(clicked()), this, SLOT(Connect()) );
	_ui->_statusLabel->setStyleSheet("QLabel { background-color : gray; color : black; }");

	_ui->_histoWidget->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );

	// Set One histogram
	BarChartProperties * cprop = new BarChartProperties;
	cprop->name.push_back("Adj0");
	cprop->min_x.push_back(0);
	cprop->max_x.push_back(511);
	cprop->nBins.push_back(511);
	cprop->color_r.push_back(150);
	cprop->color_g.push_back(222);
	cprop->color_b.push_back(0);

	_ui->_histoWidget->SetBarChartProperties( cprop );
	_ui->_histoWidget->PrepareSets();

	startTimer( 200 );

	// ThlScan
	_tscan = new ThlScan(_ui->_histoWidget);

	// Prepare DACs panel
	_dacs = new DACs(_coreApp, _ui);

	// some randon numbers
	srand (time(NULL));
}


Mpx3GUI::~Mpx3GUI()
{
	delete _ui;
}


void Mpx3GUI::StartEqualization(){

	//int i;
	//int dev_nr = 2;
	//int * data;


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

void Mpx3GUI::Connect() {

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
		_ui->_statusLabel->setText("Connected");
		_ui->_statusLabel->setStyleSheet("QLabel { background-color : blue; color : white; }");

	} else {
		cout << _spidrcontrol->connectionStateString() << ": "
				<< _spidrcontrol->connectionErrString() << endl;
		_ui->_statusLabel->setText("Connection failed.");
		_ui->_statusLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
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
	//_spidrcontrol->setEqThreshH( true );
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


	// Prepare THl scans for equalization
	_tscan->ConnectToHardware(_spidrcontrol, _spidrdaq);

	// Prepare DACs.  Populate default values.
	_dacs->ConnectToHardware(_spidrcontrol, _spidrdaq);
	_dacs->PopulateDACValues();

}


void Mpx3GUI::timerEvent( QTimerEvent * /*evt*/ ) {


	//double val = rand() % 510 + 1 ;
	//cout << "append : " << val << endl;

	// filling the histo with random numbers
	//_ui->_histoWidget->PushBackToSet( 0, val );
	////QCPBarData bin_content = (*(_ui->_histoWidget->GetDataSet(0)->data()))[val];

	//double cont = (*dmap)[val];
	////_ui->_histoWidget->GetDataSet(0)->addData( bin_content.key , bin_content.value +1 );
	//_ui->_histoWidget->DumpData();
	//_ui->_histoWidget->clearGraphs();
	////_ui->_histoWidget->replot();
	//_ui->_histoWidget->update();

	//_ui->_histoWidget->SetValueInSet(0, 100);

}

void Mpx3GUI::PrintFraction(int * buffer, int size, int first_last) {

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

int Mpx3GUI::GetNPixelsActive(int * buffer, int size, verblev verbose) {

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

pair<int, int> Mpx3GUI::XtoXY(int X, int dimX){
	return make_pair(X % dimX, X/dimX);
}

void Mpx3GUI::on_heatmapCombobox_currentIndexChanged(const QString &arg1)//would be more elegant to do with signals and slots, but would require either specalizing the combobox, or making the heatmapMap globally visible.
{
  _ui->heatmap->setHeatmap(heatmapMap[arg1]);
  _ui->_intermediatePlot->setHeatmap(heatmapMap[arg1]);
}

void Mpx3GUI::on_openfileButton_clicked()
{
  QImage image;
  files = QFileDialog::getOpenFileNames(this, tr("Open File"),QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), tr("Images (*.png *.xpm *.jpg *.gif *.png)"));
  if(files.isEmpty())
    return;
  _ui->layerCombobox->clear();
  _ui->layerCombobox->addItems(files);
  _ui->histogramPlot->clear();
  delete[] nx; delete[] ny;
  for(unsigned u = 0; u < nData; u++){
    delete[] data[u];
    delete hists[u];
    }
  delete[] data;
  delete[] hists;
  _ui->heatmap->clear();
  nData = files.length();
  data = new int*[nData];
  hists = new histogram*[nData];
  nx = new unsigned[nData]; ny = new unsigned[nData];
  for(unsigned i = 0; i < nData; i++){
    image.load(files[i]);
      if (image.isNull()) {
          QMessageBox::information(this, tr("Image Viewer"), tr("Cannot load %1.").arg(files[i]));
          return;
      }
      nx[i] = image.width(); ny[i] = image.height();
      data[i] = new int[nx[i]*ny[i]];
      for(unsigned u = 0; u < ny[i]; u++)
        for(unsigned w = 0; w < nx[i];w++){
            QRgb pixel = image.pixel(w,u);
            data[i][u*nx[i]+w] = qGray(pixel);
          }
      hists[i] = new histogram(data[i],nx[i]*ny[i], 1);
      _ui->histogramPlot->addHistogram(hists[i]);
      _ui->heatmap->addData(data[i], nx[i], ny[i]);
  }
  _ui->histogramPlot->setActive(0);
  _ui->heatmap->setActive(0);
  //_ui->histogramPlot->rescaleAxes();
  _ui->heatmap->rescaleAxes();
}
