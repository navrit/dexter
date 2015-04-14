/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */


#include "mpx3equalization.h"
#include "ui_mpx3gui.h"
#include "mpx3gui.h"


#include "qcustomplot.h"
#include "qcstmdacs.h"
#include "mpx3eq_common.h"
//#include "DACs.h"
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

#include <fstream>
#include <iostream>

using namespace std;


Mpx3Equalization::Mpx3Equalization(QApplication * coreApp, Ui::Mpx3GUI * ui)
{
	_coreApp = coreApp;
	_ui = ui;

	// Some defaults
	_deviceIndex = 2;
	_nTriggers = 1;
	_spacing = 4;
	_minScanTHL = 0;
	_maxScanTHL = (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0].size) - 1;
	_stepScan = 4;
	_eqresults = 0x0;

	// Limits in the input widgets
	SetLimits();


	//and set them
	//_ui->heatmapCombobox->addItems(heatmapMap.keys());

	connect( _ui->_startEq, SIGNAL(clicked()), this, SLOT(StartEqualization()) );
	connect(_ui->_intermediatePlot, SIGNAL(mouseOverChanged(QString)), _ui->mouseHoveLabel, SLOT(setText(QString)));
	_ui->_statusLabel->setStyleSheet("QLabel { background-color : gray; color : black; }");

	_ui->_histoWidget->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );


	startTimer( 200 );

	// Signals and slots
	SetupSignalsAndSlots();

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
	_ui->eqMinSpinBox->setValue( _minScanTHL );

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

	// N sets in the plot
	int setId = 0;

	// Preliminary) Find out the equalization range


	// First) DAC_Disc Optimization
	setId = DAC_Disc_Optimization(setId, MPX3RX_DAC_DISC_L);

	// Second) First interpolation.  Coming close to the equalization target
	setId = PrepareInterpolation(setId, MPX3RX_DAC_DISC_L);

	// Third) Fine tunning.
	//setId = FineTunning(setId, MPX3RX_DAC_DISC_L);

	// 4) Write the result
	SaveEqualization();

}

void Mpx3Equalization::SaveEqualization() {

	// Binary file
	_eqresults->WriteAdjBinaryFile("adj");
	// Masked pixels
	_eqresults->WriteMaskBinaryFile("mask");

}

int Mpx3Equalization::FineTunning(int setId, int DAC_Disc_code) {

	// Start from the last scan.
	int lastScanIndex = (int)_scans.size();
	ThlScan * lastScan = 0x0;
	if( lastScanIndex > 0 ) {
		lastScan = _scans[lastScanIndex-1];
	} else {
		return -1;
	}

	// Check how many pixels are more than N*sigmas off the mean
	lastScan->ReAdjustPixelsOff(3, DAC_Disc_code);

	return setId;
}

int Mpx3Equalization::DetectStartEqualizationRange(int setId, int DAC_Disc_code) {

	return setId;
}

int Mpx3Equalization::PrepareInterpolation(int setId, int DAC_Disc_code) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	AppendToTextBrowser("2) Test adj-bits sensibility and extrapolate to target ...");

	int global_adj = 0x0;

	////////////////////////////////////////////////////////////////////////////////////
	// 5)  See where the pixels fall now for adj0 and keep the pixel information
	ThlScan * tscan_opt_adj0 = new ThlScan(_ui->_histoWidget, _ui->_intermediatePlot, this);
	tscan_opt_adj0->ConnectToHardware(spidrcontrol, spidrdaq);
	BarChartProperties cprop_opt_adj0;
	cprop_opt_adj0.name = "DAC_DiscL_Opt_adj0";
	cprop_opt_adj0.min_x = 0;
	cprop_opt_adj0.max_x = 511;
	cprop_opt_adj0.nBins = 511;
	cprop_opt_adj0.color_r = 0;
	cprop_opt_adj0.color_g = 127;
	cprop_opt_adj0.color_b = 0;
	_ui->_histoWidget->AppendSet( cprop_opt_adj0 );

	// Send all the adjustment bits to 0x5
	if( DAC_Disc_code == MPX3RX_DAC_DISC_L ) SetAllAdjustmentBits(global_adj, 0x0);
	if( DAC_Disc_code == MPX3RX_DAC_DISC_H ) SetAllAdjustmentBits(0x0, global_adj);

	// Let's assume the mean falls at the equalization target
	tscan_opt_adj0->SetStopWhenPlateau(true);
	tscan_opt_adj0->DoScan( MPX3RX_DAC_THRESH_0, setId++, -1 ); // -1: Do all loops
	tscan_opt_adj0->SetConfigurationToScanResults(_opt_MPX3RX_DAC_DISC_L, global_adj);
	ScanResults res_opt_adj0 = tscan_opt_adj0->GetScanResults();

	// Results
	DisplayStatsInTextBrowser(global_adj, _opt_MPX3RX_DAC_DISC_L, res_opt_adj0);

	////////////////////////////////////////////////////////////////////////////////////
	// 5)  See where the pixels fall now for adj5 and keep the pixel information
	global_adj = 0x5;

	// New limits
	// The previous scan is a complete scan on all pixels.  Now we can cut the scan up to
	//  a few sigmas from the previous scan.
	SetMinScan( tscan_opt_adj0->GetDetectedLowScanBoundary() );
	SetMaxScan( tscan_opt_adj0->GetDetectedHighScanBoundary() );

	ThlScan * tscan_opt_adj5 = new ThlScan(_ui->_histoWidget, _ui->_intermediatePlot, this);
	tscan_opt_adj5->ConnectToHardware(spidrcontrol, spidrdaq);
	BarChartProperties cprop_opt_adj5;
	cprop_opt_adj5.name = "DAC_DiscL_Opt_adj5";
	cprop_opt_adj5.min_x = 0;
	cprop_opt_adj5.max_x = 511;
	cprop_opt_adj5.nBins = 511;
	cprop_opt_adj5.color_r = 127;
	cprop_opt_adj5.color_g = 127;
	cprop_opt_adj5.color_b = 10;
	_ui->_histoWidget->AppendSet( cprop_opt_adj5 );

	// Send all the adjustment bits to 0x5
	if( DAC_Disc_code == MPX3RX_DAC_DISC_L ) SetAllAdjustmentBits(global_adj, 0x0);
	if( DAC_Disc_code == MPX3RX_DAC_DISC_H ) SetAllAdjustmentBits(0x0, global_adj);

	// Let's assume the mean falls at the equalization target
	tscan_opt_adj5->SetStopWhenPlateau(true);
	tscan_opt_adj5->DoScan( MPX3RX_DAC_THRESH_0, setId++, -1 ); // -1: Do all loops
	int nNonReactive = tscan_opt_adj5->NumberOfNonReactingPixels();
	if ( nNonReactive > 0 ) {
		cout << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
	}
	tscan_opt_adj5->SetConfigurationToScanResults(_opt_MPX3RX_DAC_DISC_L, global_adj);
	ScanResults res_opt_adj5 = tscan_opt_adj5->GetScanResults();

	// Results
	DisplayStatsInTextBrowser(global_adj, _opt_MPX3RX_DAC_DISC_L, res_opt_adj5);

	////////////////////////////////////////////////////////////////////////////////////
	// 6) Stablish the dependency THL(Adj). It will be used to extrapolate to the
	//    Equalization target for every pixel
	GetSlopeAndCut_Adj_THL(res_opt_adj0, res_opt_adj5, _eta_Adj_THL, _cut_Adj_THL);

	////////////////////////////////////////////////////////////////////////////////////
	// 7) Extrapolate to the target using the last scan information and the knowledge
	//    on the Adj_THL dependency.
	_eqresults = tscan_opt_adj5->DeliverPreliminaryEqualization( res_opt_adj5 );
	_eqresults->ExtrapolateAdjToTarget( __equalization_target, _eta_Adj_THL );
	int * adj_matrix = _eqresults->GetAdjustementMatrix();

	// Display
	_ui->_intermediatePlot->clear();
	//int lastActiveFrame = _ui->_intermediatePlot->GetLastActive();
	_ui->_intermediatePlot->addData( adj_matrix, 256, 256 );
	_ui->_intermediatePlot->setActive( 0 );

	////////////////////////////////////////////////////////////////////////////////////
	// 8) Perform now a scan with the extrapolated adjustments
	//    Here there's absolutely no need to go through the THL range.
	// New limits --> ask the last scan
	SetMinScan( tscan_opt_adj5->GetDetectedLowScanBoundary() );
	SetMaxScan( tscan_opt_adj5->GetDetectedHighScanBoundary() );

	ThlScan * tscan_opt_ext = new ThlScan(_ui->_histoWidget, _ui->_intermediatePlot, this);
	tscan_opt_ext->ConnectToHardware(spidrcontrol, spidrdaq);
	BarChartProperties cprop_opt_ext;
	cprop_opt_ext.name = "DAC_DiscL_Opt_ext";
	cprop_opt_ext.min_x = 0;
	cprop_opt_ext.max_x = 511;
	cprop_opt_ext.nBins = 511;
	cprop_opt_ext.color_r = 0;
	cprop_opt_ext.color_g = 0;
	cprop_opt_ext.color_b = 0;
	_ui->_histoWidget->AppendSet( cprop_opt_ext );

	// Send all the adjustment bits to the adjusted values
	if( DAC_Disc_code == MPX3RX_DAC_DISC_L ) SetAllAdjustmentBits();
	if( DAC_Disc_code == MPX3RX_DAC_DISC_H ) SetAllAdjustmentBits();

	// Let's assume the mean falls at the equalization target
	tscan_opt_ext->DoScan( MPX3RX_DAC_THRESH_0, setId++, -1 ); // -1: Do all loops
	nNonReactive = tscan_opt_ext->NumberOfNonReactingPixels();
	if ( nNonReactive > 0 ) {
		cout << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
	}
	tscan_opt_ext->SetConfigurationToScanResults(_opt_MPX3RX_DAC_DISC_L, global_adj);
	ScanResults res_opt_ext = tscan_opt_ext->GetScanResults();

	// Results
	DisplayStatsInTextBrowser(-1, _opt_MPX3RX_DAC_DISC_L, res_opt_ext);

	// Display
	_ui->_intermediatePlot->clear();
	//int lastActiveFrame = _ui->_intermediatePlot->GetLastActive();
	_ui->_intermediatePlot->addData( adj_matrix, 256, 256 );
	_ui->_intermediatePlot->setActive( 0 );

	// This last scan is in principle the good Scan.
	// It may need fine tuning.  Append it to the scan list
	_scans.push_back( tscan_opt_ext );

	// TODO
	// For the moment mask the non reactive pixels
	vector<int> nonReactive = tscan_opt_ext->GetNonReactingPixels();
	vector<int>::iterator nrItr = nonReactive.begin();
	for ( ; nrItr != nonReactive.end() ; nrItr++ ) {
		_eqresults->maskPixel( *nrItr );
	}
	SetAllAdjustmentBits();

	return setId;
}

void Mpx3Equalization::DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults res) {

	QString statsString = "Adj=0x";
	if (adj >= 0) statsString += QString::number(adj, 'd', 0);
	else statsString += "X";
	statsString += " | DAC_DISC_L=";
	statsString += QString::number(dac_disc, 'd', 0);
	statsString += " | Mean = ";
	statsString += QString::number(res.weighted_arithmetic_mean, 'f', 1);
	statsString += ", Sigma = ";
	statsString += QString::number(res.sigma, 'f', 1);
	AppendToTextBrowser(statsString);

}

int Mpx3Equalization::DAC_Disc_Optimization(int setId, int DAC_Disc_code) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	ClearTextBrowser();
	AppendToTextBrowser("1) DAC_DiscL optimization ...");

	////////////////////////////////////////////////////////////////////////////////////
	// 1) Scan with MPX3RX_DAC_DISC_L = 100
	ThlScan * tscan = new ThlScan(_ui->_histoWidget, _ui->_intermediatePlot, this);
	tscan->ConnectToHardware(spidrcontrol, spidrdaq);
	// Append the data set which will be used for this scan
	BarChartProperties cprop;
	cprop.name = "DAC_DiscL100";
	cprop.xAxisLabel = "THL";
	cprop.yAxisLabel = "entries";
	cprop.min_x = 0;
	cprop.max_x = 511;
	cprop.nBins = 511;
	cprop.color_r = 0;
	cprop.color_g = 10;
	cprop.color_b = 127;
	_ui->_histoWidget->AppendSet( cprop );

	// DAC_DiscL=100
	Configuration(true);
	spidrcontrol->setDac( _deviceIndex, DAC_Disc_code, 100 );
	// This is a scan that I can truncate early ... I don't need to go all the way
	tscan->DoScan( MPX3RX_DAC_THRESH_0, setId++, 1 );
	tscan->SetConfigurationToScanResults(100, 0);
	ScanResults res = tscan->GetScanResults();

	// Results
	DisplayStatsInTextBrowser(0, res.DAC_DISC_setting, res);

	////////////////////////////////////////////////////////////////////////////////////
	// 2) Scan with MPX3RX_DAC_DISC_L = 150

	ThlScan * tscan_150 = new ThlScan(_ui->_histoWidget, _ui->_intermediatePlot, this);
	tscan_150->ConnectToHardware(spidrcontrol, spidrdaq);
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
	spidrcontrol->setDac( _deviceIndex, DAC_Disc_code, 150 );
	tscan_150->DoScan( MPX3RX_DAC_THRESH_0, setId++, 1 );
	tscan_150->SetConfigurationToScanResults(150, 0);
	ScanResults res_150 = tscan_150->GetScanResults();

	// Results
	DisplayStatsInTextBrowser(0, res_150.DAC_DISC_setting, res_150);

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
	spidrcontrol->setDac( _deviceIndex, DAC_Disc_code, _opt_MPX3RX_DAC_DISC_L );
	QString statsString = "Optimal DAC_DISC_L = ";
	statsString += QString::number(_opt_MPX3RX_DAC_DISC_L, 'd', 0);
	AppendToTextBrowser(statsString);

	return setId;
}

double Mpx3Equalization::EvalLinear(double eta, double cut, double x){
	return x*eta + cut;
}

void Mpx3Equalization::GetSlopeAndCut_THL_IDAC_DISC(ScanResults r1, ScanResults r2, double & eta, double & cut) {

	// The slope is =  (THLmean2 - THLmean1) / (DAC_DISC_L_setting_2 - DAC_DISC_L_setting_1)
	eta = (r2.weighted_arithmetic_mean - r1.weighted_arithmetic_mean) / (r2.DAC_DISC_setting - r1.DAC_DISC_setting);
	cut = r2.weighted_arithmetic_mean - (eta * r2.DAC_DISC_setting);

}

void Mpx3Equalization::GetSlopeAndCut_Adj_THL(ScanResults r1, ScanResults r2, double & eta, double & cut) {

	eta = (r2.global_adj - r1.global_adj) / (r2.weighted_arithmetic_mean - r1.weighted_arithmetic_mean);
	cut = r2.global_adj - (eta * r2.weighted_arithmetic_mean);

}

void Mpx3Equalization::SetAllAdjustmentBits() {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

	if( !spidrcontrol ) {
		QMessageBox::information(this, tr("Clear configuration"), tr("The system is disconnected. Nothing to clear.") );
		return;
	}

	// Adj bits
	pair<int, int> pix;

	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		pix = XtoXY(i, __array_size_x);
		spidrcontrol->configPixelMpx3rx(pix.first, pix.second, _eqresults->GetPixelAdj(i), 0x0 );
	}

	// Mask
	if ( _eqresults->GetNMaskedPixels() > 0 ) {
		QSet<int> tomask = _eqresults->GetMaskedPixels();
		QSet<int>::iterator i = tomask.begin();
		QSet<int>::iterator iE = tomask.end();
		pair<int, int> pix;
		for ( ; i != iE ; i++ ) {
			pix = XtoXY( (*i), __matrix_size_x ); //John, What coordinate system does the chip use? Top left = (0,0)?
			spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second);
		}
	} else { // When the mask is empty go ahead and set all to zero
		for ( int i = 0 ; i < __matrix_size ; i++ ) {
			pix = XtoXY(i, __array_size_x);
			spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, false);
		}
	}

	spidrcontrol->setPixelConfigMpx3rx( _deviceIndex );

}

void Mpx3Equalization::ClearAllAdjustmentBits() {

	// Clear all data structures
	_eqresults->ClearAdj();
	_eqresults->ClearMasked();
	_eqresults->ClearReactiveThresholds();

	// And now set it up
	SetAllAdjustmentBits();

}

void Mpx3Equalization::SetAllAdjustmentBits(int val_L, int val_H) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

	if( !spidrcontrol ) {
		QMessageBox::information(this, tr("Clear configuration"), tr("The system is disconnected. Nothing to clear.") );
		return;
	}

	// Adjustment bits
	pair<int, int> pix;
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		pix = XtoXY(i, __array_size_x);
		spidrcontrol->configPixelMpx3rx(pix.first, pix.second, val_L, val_H ); // 0x1F = 31 is the max adjustment for 5 bits
	}
	spidrcontrol->setPixelConfigMpx3rx( _deviceIndex );

}

void Mpx3Equalization::Configuration(bool reset) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	// Reset pixel configuration
	if ( reset ) spidrcontrol->resetPixelConfig();

	// All adjustment bits to zero
	SetAllAdjustmentBits(0x0, 0x0);

	// OMR
	//_spidrcontrol->setPolarity( true );		// Holes collection
	//_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
	//_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse
	spidrcontrol->setPixelDepth( _deviceIndex, 12 );

	spidrcontrol->setColourMode( _deviceIndex, false ); 	// Fine Pitch
	spidrcontrol->setCsmSpm( _deviceIndex, 0 );			// Single Pixel mode
	spidrcontrol->setEqThreshH( _deviceIndex, true );
	spidrcontrol->setDiscCsmSpm( _deviceIndex, 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
	//_spidrcontrol->setGainMode( 1 );

	// Gain ?!
	// 00: SHGM  0
	// 10: HGM   2
	// 01: LGM   1
	// 11: SLGM  3
	spidrcontrol->setGainMode( _deviceIndex, 3 );

	// Other OMR
	spidrdaq->setDecodeFrames( true );
	spidrcontrol->setPixelDepth( _deviceIndex, 12 );
	spidrdaq->setPixelDepth( 12 );
	spidrcontrol->setMaxPacketSize( 1024 );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = 4;     // Auto-trigger mode
	int trig_length_us = 5000;  // This time shouldn't be longer than the period defined by trig_freq_hz
	int trig_freq_hz   = 100;   // One trigger every 10ms
	int nr_of_triggers = _nTriggers;    // This is the number of shutter open i get
	//int trig_pulse_count;
	spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

}

void Mpx3Equalization::LoadEqualization(){

	if ( _eqresults == 0x0 ) {
		_eqresults = new Mpx3EqualizationResults;
	}
	_eqresults->ReadAdjBinaryFile("adj");
	_eqresults->ReadMaskBinaryFile("mask");


	// Display the equalization
	int * adj_matrix = _eqresults->GetAdjustementMatrix();
	_ui->_intermediatePlot->clear();
	//int lastActiveFrame = _ui->_intermediatePlot->GetLastActive();
	_ui->_intermediatePlot->addData( adj_matrix, 256, 256 );
	_ui->_intermediatePlot->setActive( 0 );

	// And talk to the hardware
	SetAllAdjustmentBits( ); //_eqresults );

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
	_minScanTHL = min;
}
void Mpx3Equalization::ChangeMax(int max) {
	if( max < 0 ) return;
	_maxScanTHL = max;
}
void Mpx3Equalization::ChangeStep(int step) {
	if( step < 0 ) return;
	_stepScan = step;
}

void Mpx3Equalization::ConnectionStatusChanged() {

	_mpx3gui->GetUI()->_statusLabel->setText("Connected");
	_mpx3gui->GetUI()->_statusLabel->setStyleSheet("QLabel { background-color : blue; color : white; }");

	// Load the adj bits
	LoadEqualization();

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

void Mpx3Equalization::SetMinScan(int min) {
	_minScanTHL = min;
	_ui->eqMinSpinBox->setValue( _minScanTHL );
}

void Mpx3Equalization::SetMaxScan(int max) {
	_maxScanTHL = max;
	_ui->eqMaxSpinBox->setValue( _maxScanTHL );
}

Mpx3EqualizationResults::Mpx3EqualizationResults(){

}

Mpx3EqualizationResults::~Mpx3EqualizationResults(){

}
void Mpx3EqualizationResults::SetPixelAdj(int pixId, int adj) {
	_pixId_Adj[pixId] = adj;
}
void Mpx3EqualizationResults::SetPixelReactiveThl(int pixId, int thl) {
	_pixId_Thl[pixId] = thl;
}
int Mpx3EqualizationResults::GetPixelAdj(int pixId) {
	return _pixId_Adj[pixId];
}
int Mpx3EqualizationResults::GetPixelReactiveThl(int pixId) {
	return _pixId_Thl[pixId];
}

void Mpx3EqualizationResults::ReadAdjBinaryFile(QString fn) {

	cout << "[INFO] Read Adj binary file: " << fn.toStdString() << endl;
	QFile file(fn);
	if (!file.open(QIODevice::ReadOnly)) return;
	_pixId_Adj = file.readAll();
	file.close();

}

void Mpx3EqualizationResults::WriteAdjBinaryFile(QString fn) {

	//ofstream fd;
	cout << "Writing adj file to: " << fn.toStdString() << endl;
	QFile file(fn);
	if (!file.open(QIODevice::WriteOnly)) return;
	file.write(_pixId_Adj);
	/*fd.open (fn.toStdString().c_str(), ios::out | ios::binary);
	cout << "Writing adjustment matrix to: " << fn.toStdString() << endl;
	// Each adjustment value is written as 8 bits val.  Each value is actually 5 bits.
	char buffer;
	for( int j = 0 ; j < __matrix_size ; j++ ){
		buffer = (char) ( _pixId_Adj[j] & 0xFF );
		fd.write( &buffer, 1 );   // _pixId_Adj[j];
	}

	fd.close();*/
	file.close();

}

void Mpx3EqualizationResults::WriteMaskBinaryFile(QString fn) {

	ofstream fd;
	fd.open (fn.toStdString().c_str(), ios::out);
	cout << "Writing mask file to: " << fn.toStdString() << endl;

	QSet<int>::iterator i = maskedPixels.begin();
	QSet<int>::iterator iE = maskedPixels.end();

	for ( ; i != iE ; i++ ) {
		fd << (*i) << endl;
	}

}

void Mpx3EqualizationResults::ReadMaskBinaryFile(QString fn) {

	ifstream fd;
	fd.open (fn.toStdString().c_str(), ios::out);
	cout << "Reading mask file from: " << fn.toStdString() << endl;

	int val;
	while ( fd.good() ) {

		if( fd.eof() ) break;

		fd >> val;
		//cout << val << endl;
		maskedPixels.insert( val );

	}

}




/**
 * Convert the map to an array to feed the heatmap in the display
 */
int * Mpx3EqualizationResults::GetAdjustementMatrix() {

	int * mat = new int[__matrix_size];
	for( int j = 0 ; j < __matrix_size ; j++ ){
		mat[j] = _pixId_Adj[j];
	}

	return mat;
}

void Mpx3EqualizationResults::ClearAdj(){

	if ( _pixId_Adj.size() == __matrix_size ) {
		for ( int i = 0 ; i < __matrix_size ; i++ ) {
			_pixId_Adj[i] = 0x0;
		}
	} else {
		cout << "[ERROR] the pixAdj ByteArray doesn't match the matrix size Mpx3EqualizationResults::ClearAdj()" << endl;
	}

}
void Mpx3EqualizationResults::ClearMasked(){
	maskedPixels.clear();
}
void Mpx3EqualizationResults::ClearReactiveThresholds(){
	_pixId_Thl.clear();
}

void Mpx3EqualizationResults::ExtrapolateAdjToTarget(int target, double eta_Adj_THL) {

	// Here simply I go through every pixel and decide, according to the behaviour of
	//   Adj(THL), which is the best adjustement.

	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		// Every pixel has a different reactive thl.  At this point i know the behavior of
		//  Adj(THL) based on the mean.  I will assume that the slope is the same, but now
		//  I need to adjust the linear behaviour to every particular pixel.  Let's obtain the
		//  cut.
		double pixel_cut = _pixId_Adj[i] - (eta_Adj_THL * _pixId_Thl[i]);
		// Now I can throw the extrapolation for every pixel to the equalization target
		int adj = (int)(eta_Adj_THL * (double)target + pixel_cut);
		// Replace the old matrix with this adjustment
		if ( adj > 0x1F ) { 			// Deal with an impossibly high adjustement
			_pixId_Adj[i] = 0x1F;
		} else if ( adj < 0x0 ) { 		// Deal with an impossibly low adjustement
			_pixId_Adj[i] = 0x0;
		} else {
			_pixId_Adj[i] = adj;
		}

	}

}

void Mpx3Equalization::on_heatmapCombobox_currentIndexChanged(const QString &arg1)//would be more elegant to do with signals and slots, but would require either specalizing the combobox, or making the heatmapMap globally visible.
{
}

void Mpx3Equalization::on_openfileButton_clicked()
{
}

