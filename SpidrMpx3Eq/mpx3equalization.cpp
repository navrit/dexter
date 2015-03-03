/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#include "mpx3equalization.h"
#include "ui_mpx3gui.h"
#include "mpx3gui.h"


#include "qcustomplot.h"
#include "mpx3eq_common.h"
#include "DACs.h"
#include "mpx3defs.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "barchart.h"
#include "ThlScan.h"

#include <QMessageBox>
#include <QFileDialog>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

Mpx3Equalization::Mpx3Equalization(QApplication * coreApp, Ui::Mpx3GUI * ui)
{
	_coreApp = coreApp;
	_ui = ui;

	// Some defaults
	_deviceIndex = 2;
	_nTriggers = 1;
	_spacing = 4;
	_minScan = 0;
	_maxScanTHL = 200;
	_stepScan = 16;

	// Limits in the input widgets
	SetLimits();

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
	connect(_ui->_intermediatePlot, SIGNAL(mouseOverChanged(QString)), _ui->mouseHoveLabel, SLOT(setText(QString)));
	_ui->_statusLabel->setStyleSheet("QLabel { background-color : gray; color : black; }");

	_ui->_histoWidget->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );


	startTimer( 200 );

	// Signals and slots
	SetupSignalsAndSlots();

	// some randon numbers
	/*
	srand (time(NULL));
	int noise[256*256] = {0};
	for(unsigned u = 0; u < 4;u++){
	    for(unsigned w = 0; w < 256*256;w++)
	      noise[w] = rand()%64;
	_ui->_intermediatePlot->addData(noise,256,256); //Add a new plot/frame.
	_ui->_intermediatePlot->setActive(u); //Activate the last plot (the new one)
	  }
	 */
}
Mpx3Equalization::Mpx3Equalization() {

}
void Mpx3Equalization::SetLimits(){

	//
	_ui->devIdSpinBox->setMinimum( 0 );
	_ui->devIdSpinBox->setMaximum( 3 );
	_ui->devIdSpinBox->setValue( _deviceIndex );

	_ui->nTriggersSpinBox->setMinimum( 1 );
	_ui->nTriggersSpinBox->setMaximum( 1000 );
	_ui->nTriggersSpinBox->setValue( _nTriggers );

	_ui->spacingSpinBox->setMinimum( 1 );
	_ui->spacingSpinBox->setMaximum( 64 );
	_ui->spacingSpinBox->setValue( _spacing );

	_ui->eqMinSpinBox->setMinimum( 0 );
	_ui->eqMinSpinBox->setMaximum( (MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].dflt * 2) - 1 );
	_ui->eqMinSpinBox->setValue( _minScan );

	_ui->eqMaxSpinBox->setMinimum( 0 );
	_ui->eqMaxSpinBox->setMaximum( (MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].dflt * 2) - 1 );
	_ui->eqMaxSpinBox->setValue( _maxScanTHL );

	_ui->eqStepSpinBox->setMinimum( 1 );
	_ui->eqStepSpinBox->setMaximum( (MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].dflt * 2) - 1 );
	_ui->eqStepSpinBox->setValue( _stepScan );

}

Mpx3Equalization::~Mpx3Equalization()
{

}

void Mpx3Equalization::StartEqualization() {

	// First) DAC_Disc Optimization
	DAC_Disc_Optimization(MPX3RX_DAC_DISC_L);

	// Second)
	//

}

void Mpx3Equalization::DAC_Disc_Optimization(int DAC_Disc_code) {

	ClearTextBrowser();
	AppendToTextBrowser("1) DAC_DiscL optimization ...");
	//int setId = 0;

	////////////////////////////////////////////////////////////////////////////////////
	// 1) Scan with MPX3RX_DAC_DISC_L = 100
	ThlScan * tscan = new ThlScan(_ui->_histoWidget, _ui->_intermediatePlot, this);
	tscan->ConnectToHardware(_spidrcontrol, _spidrdaq);
	// Append the data set which will be used for this scan
	BarChartProperties cprop;
	cprop.name = "DAC_DiscL100";
	cprop.xAxisLabel = "THL";
	cprop.yAxisLabel = "entries";
	cprop.min_x = 0;
	cprop.max_x = 200;
	cprop.nBins = 511;
	cprop.color_r = 0;
	cprop.color_g = 10;
	cprop.color_b = 127;
	_ui->_histoWidget->AppendSet( cprop );

	// DAC_DiscL=100
	Configuration(true);
	_spidrcontrol->setDac( _deviceIndex, DAC_Disc_code, 100 );
	//_tscan->DoScan( MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].code );
	tscan->DoScan( MPX3RX_DAC_THRESH_0, 0 );
	ScanResults res = tscan->GetScanResults();
	res.DAC_DISC_setting = 100;

	QString statsString = "Adj=0x0 | DAC_DISC_L=100 | Mean = ";
	statsString += QString::number(res.weighted_arithmetic_mean, 'f', 1);
	statsString += ", Sigma = ";
	statsString += QString::number(res.sigma, 'f', 1);
	AppendToTextBrowser(statsString);

	////////////////////////////////////////////////////////////////////////////////////
	// 2) Scan with MPX3RX_DAC_DISC_L = 150
	ThlScan * tscan_150 = new ThlScan(_ui->_histoWidget, _ui->_intermediatePlot, this);
	tscan_150->ConnectToHardware(_spidrcontrol, _spidrdaq);
	BarChartProperties cprop_150;
	cprop_150.name = "DAC_DiscL150";
	cprop_150.min_x = 0;
	cprop_150.max_x = 200;
	cprop_150.nBins = 511;
	cprop_150.color_r = 127;
	cprop_150.color_g = 10;
	cprop_150.color_b = 0;
	_ui->_histoWidget->AppendSet( cprop_150 );

	// DAC_DiscL=150
	_spidrcontrol->setDac( _deviceIndex, DAC_Disc_code, 150 );
	//_tscan->DoScan( MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].code );
	tscan_150->DoScan( MPX3RX_DAC_THRESH_0, 1 );
	ScanResults res_150 = tscan_150->GetScanResults();
	res_150.DAC_DISC_setting = 150;

	statsString = "Adj=0x0 | DAC_DISC_L=150 | Mean = ";
	statsString += QString::number(res_150.weighted_arithmetic_mean, 'f', 1);
	statsString += ", Sigma = ";
	statsString += QString::number(res_150.sigma, 'f', 1);
	AppendToTextBrowser(statsString);

	////////////////////////////////////////////////////////////////////////////////////
	// 3) With the results of step 1 and 2 I can obtain the dependency DAC_Disc[L/H](THL)
	GetSlopeAndCut_THL_IDAC_DISC(res, res_150, _eta_THL_DAC_DiscL, _cut_THL_DAC_DiscL);

	////////////////////////////////////////////////////////////////////////////////////
	// 4) Now IDAC_DISC optimal is such that:
	//    With an adj-bit of 00101[5] the optimal mean is at __equalization_target + 3.2 sigma

	// Desired mean value = __equalization_target + 3.2 sigma
	double mean_for_opt_IDAC_DISC = __equalization_target + 3.2*res.sigma;
	// Using the relation DAC_Disc[L/H](THL) we can find the value of DAC_Disc
	_opt_MPX3RX_DAC_DISC_L = (int) EvalLinear(_eta_THL_DAC_DiscL, _cut_THL_DAC_DiscL, mean_for_opt_IDAC_DISC);
	// Set the new DAC
	_spidrcontrol->setDac( _deviceIndex, DAC_Disc_code, _opt_MPX3RX_DAC_DISC_L );
	statsString = "Optimal DAC_DISC_L = ";
	statsString += QString::number(_opt_MPX3RX_DAC_DISC_L, 'd', 0);
	AppendToTextBrowser(statsString);

	////////////////////////////////////////////////////////////////////////////////////
	// 5)  See where the pixels fall now
	ThlScan * tscan_opt = new ThlScan(_ui->_histoWidget, _ui->_intermediatePlot, this);
	tscan_opt->ConnectToHardware(_spidrcontrol, _spidrdaq);
	BarChartProperties cprop_opt;
	cprop_opt.name = "DAC_DiscL_Opt";
	cprop_opt.min_x = 0;
	cprop_opt.max_x = 511;
	cprop_opt.nBins = 511;
	cprop_opt.color_r = 0;
	cprop_opt.color_g = 127;
	cprop_opt.color_b = 0;
	_ui->_histoWidget->AppendSet( cprop_opt );

	// Send all the adjustment bits to 0x5
	if( DAC_Disc_code == MPX3RX_DAC_DISC_L ) SetAllAdjustmentBits(0x5, 0x0);
	if( DAC_Disc_code == MPX3RX_DAC_DISC_H ) SetAllAdjustmentBits(0x0, 0x5);

	// Let's assume the mean falls at the equalization target
	tscan_opt->DoScan( MPX3RX_DAC_THRESH_0, 2 );
	ScanResults res_opt = tscan_opt->GetScanResults();
	res_opt.DAC_DISC_setting = _opt_MPX3RX_DAC_DISC_L;

	statsString = "Adj=0x5 | DAC_DISC_L=";
	statsString += QString::number(_opt_MPX3RX_DAC_DISC_L, 'd', 0);
	statsString += " | Mean = ";
	statsString += QString::number(res_opt.weighted_arithmetic_mean, 'f', 1);
	statsString += ", Sigma = ";
	statsString += QString::number(res_opt.sigma, 'f', 1);
	AppendToTextBrowser(statsString);

}

double Mpx3Equalization::EvalLinear(double eta, double cut, double x){
	return x*eta + cut;
}

void Mpx3Equalization::GetSlopeAndCut_THL_IDAC_DISC(ScanResults r1, ScanResults r2, double & eta, double & cut) {

	// The slope is =  (THLmean2 - THLmean1) / (DAC_DISC_L_setting_2 - DAC_DISC_L_setting_1)
	eta = (r2.weighted_arithmetic_mean - r1.weighted_arithmetic_mean) / (r2.DAC_DISC_setting - r1.DAC_DISC_setting);
	cut = eta * 150 - r2.weighted_arithmetic_mean;

}

void Mpx3Equalization::SetAllAdjustmentBits(int val_L, int val_H) {

	// Adjustment bits
	pair<int, int> pix;
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		pix = XtoXY(i, __array_size_x);
		_spidrcontrol->configPixelMpx3rx(pix.first, pix.second, val_L, val_H ); // 0x1F = 31 is the max adjustment for 5 bits
	}
	_spidrcontrol->setPixelConfigMpx3rx( _deviceIndex );

}

void Mpx3Equalization::Configuration(bool reset) {

	// Reset pixel configuration
	if ( reset )_spidrcontrol->resetPixelConfig();

	// All adjustment bits to zero
	SetAllAdjustmentBits(0x0, 0x0);

	// OMR
	//_spidrcontrol->setPolarity( true );		// Holes collection
	//_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
	//_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse
	_spidrcontrol->setPixelDepth( _deviceIndex, 12 );

	_spidrcontrol->setColourMode( _deviceIndex, false ); 	// Fine Pitch
	_spidrcontrol->setCsmSpm( _deviceIndex, 0 );			// Single Pixel mode
	_spidrcontrol->setEqThreshH( _deviceIndex, true );
	_spidrcontrol->setDiscCsmSpm( _deviceIndex, 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
	//_spidrcontrol->setGainMode( 1 );

	// Gain ?!
	// 00: SHGM  0
	// 10: HGM   2
	// 01: LGM   1
	// 11: SLGM  3
	_spidrcontrol->setGainMode( _deviceIndex, 3 );

	// Other OMR
	_spidrdaq->setDecodeFrames( true );
	_spidrcontrol->setPixelDepth( _deviceIndex, 12 );
	_spidrdaq->setPixelDepth( 12 );
	_spidrcontrol->setMaxPacketSize( 1024 );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = 4;     // Auto-trigger mode
	int trig_length_us = 5000;  // This time shouldn't be longer than the period defined by trig_freq_hz
	int trig_freq_hz   = 100;   // One trigger every 10ms
	int nr_of_triggers = _nTriggers;    // This is the number of shutter open i get
	//int trig_pulse_count;
	_spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

}


void Mpx3Equalization::SetupSignalsAndSlots() {

	// Spinboxes
	connect( _ui->nTriggersSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeNTriggers(int) ) );
	connect( _ui->devIdSpinBox, SIGNAL(valueChanged(int)), this, SLOT(  ChangeDeviceIndex(int) ) );
	connect( _ui->spacingSpinBox, SIGNAL(valueChanged(int)), this, SLOT(  ChangeSpacing(int) ) );

	connect( _ui->eqMinSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeMin(int) ) );
	connect( _ui->eqMaxSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeMax(int) ) );
	connect( _ui->eqStepSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeStep(int) ) );

}

void Mpx3Equalization::ChangeNTriggers( int nTriggers ) {
	if( nTriggers < 0 ) return; // can't really happen cause the SpinBox has been limited
	_nTriggers = nTriggers;
}

void Mpx3Equalization::ChangeDeviceIndex( int index ) {
	if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
	_deviceIndex = index;
}

void Mpx3Equalization::ChangeSpacing( int spacing ) {
	if( spacing < 0 ) return;
	_spacing = spacing;
}
void Mpx3Equalization::ChangeMin(int min) {
	if( min < 0 ) return;
	_minScan = min;
}
void Mpx3Equalization::ChangeMax(int max) {
	if( max < 0 ) return;
	_maxScanTHL = max;
}
void Mpx3Equalization::ChangeStep(int step) {
	if( step < 0 ) return;
	_stepScan = step;
}

void Mpx3Equalization::Connect() {

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


	// A connection to hardware should make aware the DAC panel
	_moduleConn->GetDACs()->ConnectToHardware(_spidrcontrol, _spidrdaq);
	_moduleConn->GetDACs()->PopulateDACValues();

}

void Mpx3Equalization::AppendToTextBrowser(QString s){

	_ui->eqTextBrowser->append( s );

}

void Mpx3Equalization::ClearTextBrowser(){
	_ui->eqTextBrowser->clear();
}

void Mpx3Equalization::timerEvent( QTimerEvent * /*evt*/ ) {


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

void Mpx3Equalization::PrintFraction(int * buffer, int size, int first_last) {

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

int Mpx3Equalization::GetNPixelsActive(int * buffer, int size, verblev verbose) {

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

pair<int, int> Mpx3Equalization::XtoXY(int X, int dimX){
	return make_pair(X % dimX, X/dimX);
}

void Mpx3Equalization::on_heatmapCombobox_currentIndexChanged(const QString &arg1)//would be more elegant to do with signals and slots, but would require either specalizing the combobox, or making the heatmapMap globally visible.
{
	_ui->heatmap->setHeatmap(heatmapMap[arg1]);
	_ui->_intermediatePlot->setHeatmap(heatmapMap[arg1]);
}

void Mpx3Equalization::on_openfileButton_clicked()
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
