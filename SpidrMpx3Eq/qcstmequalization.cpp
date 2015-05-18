#include "qcstmequalization.h"
#include "ui_qcstmequalization.h"


#include "qcustomplot.h"
#include "qcstmdacs.h"
#include "mpx3gui.h"
#include "ui_mpx3gui.h"
#include "mpx3eq_common.h"
//#include "DACs.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "barchart.h"
#include "ThlScan.h"


QCstmEqualization::QCstmEqualization(QWidget *parent) :
QWidget(parent),
_ui(new Ui::QCstmEqualization)
{
	_ui->setupUi(this);
	QList<int> defaultSizesMain; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".
	defaultSizesMain.append(2971215);
	defaultSizesMain.append(1836312);
	QList<int> defaultSizesHist;
	defaultSizesHist.append(2971215);
	defaultSizesHist.append(1836312);

	for(int i = 0; i < _ui->mainSplitter->count();i++){
		QWidget *child = _ui->mainSplitter->widget(i);
		child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		child->setMinimumSize(1,1);
	}
	for(int i = 0; i < _ui->histSetupSplitter->count();i++){
		QWidget *child = _ui->histSetupSplitter->widget(i);
		child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		child->setMinimumSize(1,1);
	}
	_ui->histSetupSplitter->setSizes(defaultSizesHist);
	_ui->mainSplitter->setSizes(defaultSizesMain);

	// Some defaults
	_deviceIndex = 2;
	_nTriggers = 1;
	_spacing = 4;
	_minScanTHL = 0;
	_maxScanTHL = (1 << MPX3RX_DAC_TABLE[MPX3RX_DAC_THRESH_0].size) - 1;
	_stepScan = 2;
	_eqresults = 0x0;
	_setId = 0;
	_global_adj = 0x0;
	_nChips = 1;
	_eqVector.clear();

	// Limits in the input widgets
	SetLimits();

	// Signals and slots
	SetupSignalsAndSlots();

	_eqStatus = __INIT;
	_stepDone = new bool[__EQStatus_Count];
	for(int i = 0 ; i < __EQStatus_Count ; i++) _stepDone[i] = false;

}

Ui::QCstmEqualization * QCstmEqualization::GetUI() {
	return _ui;
}

QCstmEqualization::~QCstmEqualization()
{
	delete _ui;
}

void QCstmEqualization::SetLimits(){

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


void QCstmEqualization::InitEqualization() {

	// Rewind state machine variables
	_eqStatus = __INIT;
	for(int i = 0 ; i < __EQStatus_Count ; i++) _stepDone[i] = false;
	GetUI()->_histoWidget->Clean();
	// No sets available
	_setId = 0;
	// Clear the equalization results
	if ( _eqresults ) {
		delete _eqresults;
	}
	_eqresults = new Mpx3EqualizationResults;

	//else _eqresults->Clear();

	// Rewind limits
	SetMinScan( 0 );
	// FIXME ! ... it can be other threshold
	SetMaxScan( (MPX3RX_DAC_TABLE[ MPX3RX_DAC_THRESH_0 ].dflt * 2) - 1 );

}

void QCstmEqualization::StartEqualizationAllChips() {

	// How many ?
	_nChips = _mpx3gui->getConfig()->getNDevicesSupported();

	// Init
	InitEqualization();

	// Start by the first chip
	_deviceIndex = 0;
	_ui->devIdSpinBox->setValue( _deviceIndex );
	StartEqualization( _deviceIndex );

}


void QCstmEqualization::Rewind() {


	// Establish if it is needed to Equalize another chip
	if( _deviceIndex < _nChips - 1  ) {

		InitEqualization();

		// Next chip
		_deviceIndex++;
		_ui->devIdSpinBox->setValue( _deviceIndex );

		// Clear the previous scans !
		_scans.clear();

		StartEqualization( _deviceIndex );

	} else { // when done

		AppendToTextBrowser( "-- DONE ----------------" );
	}

}

void QCstmEqualization::StartEqualization() {

	// Init
	_nChips = 1;
	InitEqualization();

	StartEqualization( _deviceIndex );

}

void QCstmEqualization::StartEqualization(int chipId) {


	// I need to do this here and not when already running the thread
	// Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	if( spidrcontrol ) { spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
	else { _srcAddr = 0; }

	// Check if we can talk to the chip
	if ( ! _mpx3gui->getConfig()->detectorResponds( _deviceIndex ) ) {
		QString startS = "--- CHIP ";
		startS += QString::number(_deviceIndex, 'd', 0);
		startS += " --- NOT RESPONDING -- SKIP --";
		AppendToTextBrowser( startS );
		Rewind();
		return;
	}

	// N sets in the plot
	int DAC_DISC_testValue = 100;

	// Preliminary) Find out the equalization range

	// First) DAC_Disc Optimization
	if( EQ_NEXT_STEP( __INIT) ) {

		// Clean old equalization if any
		GetUI()->_histoWidget->Clean();

		////////////
		// STEP 1 //
		////////////
		if(_nChips > 1 && _deviceIndex == 0) ClearTextBrowser(); // Clear only the first time
		QString startS = "--- CHIP ";
		startS += QString::number(_deviceIndex, 'd', 0);
		startS += " ----------------";
		AppendToTextBrowser( startS );
		AppendToTextBrowser("1) DAC_DiscL optimization ...");
		// CONFIG !
		Configuration(true);

		// Prepare and launch the thread
		DAC_DISC_testValue = 100;
		DAC_Disc_Optimization_100(MPX3RX_DAC_DISC_L, DAC_DISC_testValue);


	} else if ( EQ_NEXT_STEP(__DAC_Disc_Optimization_100 ) ) {

		// Extract results from immediately previous scan. Calc the stats now (this is quick)
		_scans[_eqStatus - 1]->ExtractStatsOnChart(_setId - 1);
		// Show the results
		DAC_Disc_Optimization_DisplayResults( _scans[_eqStatus - 1]->GetScanResults() );

		// And go for next scan
		DAC_DISC_testValue = 150;
		DAC_Disc_Optimization_150(MPX3RX_DAC_DISC_L, DAC_DISC_testValue);


	} else if ( EQ_NEXT_STEP(__DAC_Disc_Optimization_150 ) ) {

		// Extract results from immediately previous scan. Calc the stats now (this is quick)
		_scans[_eqStatus - 1]->ExtractStatsOnChart(_setId - 1);
		// Show the results
		DAC_Disc_Optimization_DisplayResults( _scans[_eqStatus - 1]->GetScanResults() );

		// And calculate the optimal DAC_Disc
		ScanResults res_100 = _scans[_eqStatus - 2]->GetScanResults();
		ScanResults res_150 = _scans[_eqStatus - 1]->GetScanResults();
		DAC_Disc_Optimization(res_100, res_150);

		// I could get rid of the previous two equalizations
		//delete _scans[_eqStatus - 2];
		//delete _scans[_eqStatus - 1];
		////////////
		// STEP 2 //
		////////////
		AppendToTextBrowser("2) Test adj-bits sensibility and extrapolate to target ...");
		PrepareInterpolation_0x0(MPX3RX_DAC_DISC_L);

	} else if ( EQ_NEXT_STEP(__PrepareInterpolation_0x0) ) {

		// Results
		_scans[_eqStatus - 1]->ExtractStatsOnChart(_setId - 1);
		DisplayStatsInTextBrowser(_global_adj, _opt_MPX3RX_DAC_DISC_L, _scans[_eqStatus - 1]->GetScanResults());

		// Now adj=0x5
		PrepareInterpolation_0x5(MPX3RX_DAC_DISC_L);

	} else if ( EQ_NEXT_STEP(__PrepareInterpolation_0x5) ) {

		// Results
		int nNonReactive = _scans[_eqStatus - 1]->NumberOfNonReactingPixels();
		if ( nNonReactive > 0 ) {
			cout << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
		}

		_scans[_eqStatus - 1]->ExtractStatsOnChart(_setId - 1);
		DisplayStatsInTextBrowser(_global_adj, _opt_MPX3RX_DAC_DISC_L, _scans[_eqStatus - 1]->GetScanResults());

		// Interpolate now
		ScanResults res_x0 = _scans[_eqStatus - 2]->GetScanResults();
		ScanResults res_x5 = _scans[_eqStatus - 1]->GetScanResults();
		CalculateInterpolation(res_x0, res_x5);

		// Perform now a scan with the extrapolated adjustments
		//    Here there's absolutely no need to go through the THL range.
		// New limits --> ask the last scan
		ScanOnInterpolation(MPX3RX_DAC_DISC_L);


	} else if ( EQ_NEXT_STEP( __ScanOnInterpolation) ) {

		// Results
		int nNonReactive = _scans[_eqStatus - 1]->NumberOfNonReactingPixels();
		if ( nNonReactive > 0 ) {
			cout << "[WARNING] there are non reactive pixels : " << nNonReactive << endl;
		}
		_scans[_eqStatus - 1]->ExtractStatsOnChart(_setId - 1);
		DisplayStatsInTextBrowser(-1, _opt_MPX3RX_DAC_DISC_L, _scans[_eqStatus - 1]->GetScanResults());

		// Display
		_ui->_intermediatePlot->clear();
		//int lastActiveFrame = _ui->_intermediatePlot->GetLastActive();
		int * adj_matrix = _eqresults->GetAdjustementMatrix();
		_ui->_intermediatePlot->addData( adj_matrix, 256, 256 );
		_ui->_intermediatePlot->setActive( 0 );

		// 4) Write the result
		SaveEqualization(chipId);

		// Continue if multiple chips need to be equalized
		if ( _nChips > 1 ) Rewind();

	}

	// Second) First interpolation.  Coming close to the equalization target
	////setId = PrepareInterpolation(setId, MPX3RX_DAC_DISC_L);

	// Third) Fine tunning.
	//setId = FineTunning(setId, MPX3RX_DAC_DISC_L);


	/////////////////////////////////////////////
	// !!! ATTENTION !!!
	// THL1 Eq, set the Equalization for THL1 eq
	//  but the results comes in the counter 0 !!!

}

void QCstmEqualization::CalculateInterpolation(ScanResults res_x0, ScanResults res_x5) {

	////////////////////////////////////////////////////////////////////////////////////
	// 6) Stablish the dependency THL(Adj). It will be used to extrapolate to the
	//    Equalization target for every pixel
	GetSlopeAndCut_Adj_THL(res_x0, res_x5, _eta_Adj_THL, _cut_Adj_THL);

	////////////////////////////////////////////////////////////////////////////////////
	// 7) Extrapolate to the target using the last scan information and the knowledge
	//    on the Adj_THL dependency.
	_scans[_eqStatus - 1]->DeliverPreliminaryEqualization(_eqresults, res_x5 );
	_eqresults->ExtrapolateAdjToTarget( __equalization_target, _eta_Adj_THL );
	int * adj_matrix = _eqresults->GetAdjustementMatrix();

	// Display
	_ui->_intermediatePlot->clear();
	//int lastActiveFrame = _ui->_intermediatePlot->GetLastActive();
	_ui->_intermediatePlot->addData( adj_matrix, 256, 256 );
	_ui->_intermediatePlot->setActive( 0 );

}

void QCstmEqualization::DAC_Disc_Optimization_DisplayResults(ScanResults res) {

	// Results
	DisplayStatsInTextBrowser(0, res.DAC_DISC_setting, res);

}

void QCstmEqualization::DAC_Disc_Optimization_100(int DAC_Disc_code, int DAC_DISC_testValue) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	_global_adj = 0x0;

	////////////////////////////////////////////////////////////////////////////////////
	// 1) Scan with MPX3RX_DAC_DISC_L = 100
	//_chart = _ui->_histoWidget;
	ThlScan * tscan = new ThlScan(_mpx3gui, this);
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
	cprop.color_g = 127;
	cprop.color_b = 0;
	_ui->_histoWidget->AppendSet( cprop );

	// DAC_DiscL=100
	spidrcontrol->setDac( _deviceIndex, DAC_Disc_code, DAC_DISC_testValue );
	// This is a scan that I can truncate early ... I don't need to go all the way
	tscan->DoScan( MPX3RX_DAC_THRESH_0, _setId++, DAC_Disc_code, 1 );
	tscan->SetConfigurationToScanResults(DAC_DISC_testValue, 0x0);

	// Launch as thread.  Connect the slot which signals when it's done
	_scans.push_back( tscan );
	connect( tscan, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
	tscan->start();

}

void QCstmEqualization::DAC_Disc_Optimization_150(int DAC_Disc_code, int DAC_DISC_testValue) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	////////////////////////////////////////////////////////////////////////////////////
	// 2) Scan with MPX3RX_DAC_DISC_L = 150

	ThlScan * tscan = new ThlScan(_mpx3gui, this);
	tscan->ConnectToHardware(spidrcontrol, spidrdaq);
	BarChartProperties cprop_150;
	cprop_150.name = "DAC_DiscL150";
	cprop_150.min_x = 0;
	cprop_150.max_x = 200;
	cprop_150.nBins = 511;
	cprop_150.color_r = 127;
	cprop_150.color_g = 127;
	cprop_150.color_b = 10;
	_ui->_histoWidget->AppendSet( cprop_150 );

	// DAC_DiscL=150
	spidrcontrol->setDac( _deviceIndex, DAC_Disc_code, DAC_DISC_testValue );
	tscan->DoScan( MPX3RX_DAC_THRESH_0, _setId++, DAC_Disc_code, 1 );
	tscan->SetConfigurationToScanResults(DAC_DISC_testValue, 0x0);

	// Launch as thread.  Connect the slot which signals when it's done
	_scans.push_back( tscan );
	connect( tscan, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
	tscan->start();


}

void QCstmEqualization::DAC_Disc_Optimization (ScanResults res_100, ScanResults res_150) {

	////////////////////////////////////////////////////////////////////////////////////
	// 3) With the results of step 1 and 2 I can obtain the dependency DAC_Disc[L/H](THL)
	GetSlopeAndCut_IDAC_DISC_THL(res_100, res_150, _eta_THL_DAC_DiscL, _cut_THL_DAC_DiscL);

	////////////////////////////////////////////////////////////////////////////////////
	// 4) Now IDAC_DISC optimal is such that:
	//    With an adj-bit of 00101[5] the optimal mean is at __equalization_target + 3.2 sigma

	// Desired mean value = __equalization_target + 3.2 sigma
	// Tomando el sigma del primer scan
	double meanTHL_for_opt_IDAC_DISC = __equalization_target + 3.2*res_100.sigma;
	// Using the relation DAC_Disc[L/H](THL) we can find the value of DAC_Disc
	_opt_MPX3RX_DAC_DISC_L = (int) EvalLinear(_eta_THL_DAC_DiscL, _cut_THL_DAC_DiscL, meanTHL_for_opt_IDAC_DISC);
	// Set the new DAC
	_mpx3gui->GetSpidrController()->setDac( _deviceIndex, MPX3RX_DAC_DISC_L, _opt_MPX3RX_DAC_DISC_L );
	// Adjust the sliders and the SpinBoxes to the new value
	connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
	// Get the DAC back just to be sure and then slide&spin
	int dacVal = 0;
	_mpx3gui->GetSpidrController()->getDac( _deviceIndex, MPX3RX_DAC_DISC_L, &dacVal);
	// SlideAndSpin works with the DAC index, no the code.
	int dacIndex = _mpx3gui->GetUI()->DACsWidget->GetDACIndex( MPX3RX_DAC_DISC_L );
	slideAndSpin( dacIndex,  dacVal );
	disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
	// Now I need to set it in the local data base
	_mpx3gui->GetUI()->DACsWidget->SetDACValueLocalConfig( _deviceIndex, dacIndex, _opt_MPX3RX_DAC_DISC_L);


	QString statsString = "Optimal DAC_DISC_L = ";
	statsString += QString::number(_opt_MPX3RX_DAC_DISC_L, 'd', 0);
	AppendToTextBrowser(statsString);


}

void QCstmEqualization::SaveEqualization(int chipId) {

	QString adjfn = "adj_";
	adjfn += QString::number(chipId, 10);
	QString maskfn = "mask_";
	maskfn += QString::number(chipId, 10);

	// Binary file
	_eqresults->WriteAdjBinaryFile( adjfn );
	// Masked pixels
	_eqresults->WriteMaskBinaryFile( maskfn );

}

int QCstmEqualization::FineTunning(int setId, int DAC_Disc_code) {

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


//int QCstmEqualization::DetectStartEqualizationRange(int setId, int DAC_Disc_code) {
//	return setId;
//}

void QCstmEqualization::PrepareInterpolation_0x0(int DAC_Disc_code) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	_global_adj = 0x0;

	////////////////////////////////////////////////////////////////////////////////////
	// 5)  See where the pixels fall now for adj0 and keep the pixel information
	ThlScan * tscan_opt_adj0 = new ThlScan( _mpx3gui, this);
	tscan_opt_adj0->ConnectToHardware(spidrcontrol, spidrdaq);
	BarChartProperties cprop_opt_adj0;
	cprop_opt_adj0.name = "DAC_DiscL_Opt_adj0";
	cprop_opt_adj0.min_x = 0;
	cprop_opt_adj0.max_x = 511;
	cprop_opt_adj0.nBins = 511;
	cprop_opt_adj0.color_r = 0;
	cprop_opt_adj0.color_g = 10;
	cprop_opt_adj0.color_b = 127;
	_ui->_histoWidget->AppendSet( cprop_opt_adj0 );

	// Send all the adjustment bits to 0x5
	if( DAC_Disc_code == MPX3RX_DAC_DISC_L ) SetAllAdjustmentBits(spidrcontrol, _global_adj, 0x0);
	if( DAC_Disc_code == MPX3RX_DAC_DISC_H ) SetAllAdjustmentBits(spidrcontrol, 0x0, _global_adj);

	// Let's assume the mean falls at the equalization target
	//tscan_opt_adj0->SetStopWhenPlateau(true);
	tscan_opt_adj0->DoScan( MPX3RX_DAC_THRESH_0, _setId++, DAC_Disc_code, -1 ); // -1: Do all loops
	tscan_opt_adj0->SetConfigurationToScanResults(_opt_MPX3RX_DAC_DISC_L, _global_adj);

	// Launch as thread.  Connect the slot which signals when it's done
	_scans.push_back( tscan_opt_adj0 );
	connect( tscan_opt_adj0, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
	tscan_opt_adj0->start();

}

void QCstmEqualization::ScanOnInterpolation(int DAC_Disc_code) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	// New limits --> ask the scan with adj_global = 0x0
	ThlScan * scan_adj5 = _scans[_eqStatus - 2];
	SetMinScan( scan_adj5->GetDetectedLowScanBoundary() );
	SetMaxScan( scan_adj5->GetDetectedHighScanBoundary() );

	ThlScan * tscan_opt_ext = new ThlScan(_mpx3gui, this);
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
	//if( DAC_Disc_code == MPX3RX_DAC_DISC_L ) SetAllAdjustmentBits(spidrcontrol);
	//if( DAC_Disc_code == MPX3RX_DAC_DISC_H ) SetAllAdjustmentBits(spidrcontrol);

	// Let's assume the mean falls at the equalization target
	tscan_opt_ext->DoScan( MPX3RX_DAC_THRESH_0, _setId++, DAC_Disc_code, -1 ); // -1: Do all loops
	tscan_opt_ext->SetAdjustmentType( ThlScan::__adjust_to_equalizationMatrix );
	// A global_adj doesn't apply here anymore.  Passing -1.
	tscan_opt_ext->SetConfigurationToScanResults(_opt_MPX3RX_DAC_DISC_L, -1);

	// Launch as thread.  Connect the slot which signals when it's done
	_scans.push_back( tscan_opt_ext);
	connect( tscan_opt_ext, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
	tscan_opt_ext->start();




}

void QCstmEqualization::PrepareInterpolation_0x5(int DAC_Disc_code) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	////////////////////////////////////////////////////////////////////////////////////
	// 5)  See where the pixels fall now for adj5 and keep the pixel information
	_global_adj = 0x5;

	// New limits
	// The previous scan is a complete scan on all pixels.  Now we can cut the scan up to
	//  a few sigmas from the previous scan.
	SetMinScan( _scans[_eqStatus - 1]->GetDetectedLowScanBoundary() );
	SetMaxScan( _scans[_eqStatus - 1]->GetDetectedHighScanBoundary() );

	ThlScan * tscan_opt_adj5 = new ThlScan(_mpx3gui, this);
	tscan_opt_adj5->ConnectToHardware(spidrcontrol, spidrdaq);
	BarChartProperties cprop_opt_adj5;
	cprop_opt_adj5.name = "DAC_DiscL_Opt_adj5";
	cprop_opt_adj5.min_x = 0;
	cprop_opt_adj5.max_x = 511;
	cprop_opt_adj5.nBins = 511;
	cprop_opt_adj5.color_r = 127;
	cprop_opt_adj5.color_g = 10;
	cprop_opt_adj5.color_b = 0;
	_ui->_histoWidget->AppendSet( cprop_opt_adj5 );

	// Send all the adjustment bits to 0x5
	//if( DAC_Disc_code == MPX3RX_DAC_DISC_L ) SetAllAdjustmentBits(spidrcontrol, _global_adj, 0x0);
	//if( DAC_Disc_code == MPX3RX_DAC_DISC_H ) SetAllAdjustmentBits(spidrcontrol, 0x0, _global_adj);

	// Let's assume the mean falls at the equalization target
	//tscan_opt_adj5->SetStopWhenPlateau(true);
	tscan_opt_adj5->DoScan( MPX3RX_DAC_THRESH_0, _setId++, DAC_Disc_code, -1 ); // -1: Do all loops
	tscan_opt_adj5->SetConfigurationToScanResults(_opt_MPX3RX_DAC_DISC_L, _global_adj);

	// Launch as thread.  Connect the slot which signals when it's done
	_scans.push_back( tscan_opt_adj5 );
	connect( tscan_opt_adj5, SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
	tscan_opt_adj5->start();

}

/*

	// TODO
	// For the moment mask the non reactive pixels
	vector<int> nonReactive = tscan_opt_ext->GetNonReactingPixels();
	vector<int>::iterator nrItr = nonReactive.begin();
	for ( ; nrItr != nonReactive.end() ; nrItr++ ) {
		_eqresults->maskPixel( *nrItr );
	}
	SetAllAdjustmentBits();
 */


void QCstmEqualization::DisplayStatsInTextBrowser(int adj, int dac_disc, ScanResults res) {

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

void QCstmEqualization::ScanThreadFinished(){

	// This step was done
	_stepDone[_eqStatus] = true;
	// disconnect the signal that brought us here
	disconnect( _scans[_eqStatus], SIGNAL( finished() ), this, SLOT( ScanThreadFinished() ) );
	// Go to next step
	_eqStatus++;
	// Now revisit the equalization.
	// It knows where to pick up.
	StartEqualization( _deviceIndex );
}



double QCstmEqualization::EvalLinear(double eta, double cut, double x){
	return x*eta + cut;
}

void QCstmEqualization::GetSlopeAndCut_IDAC_DISC_THL(ScanResults r1, ScanResults r2, double & eta, double & cut) {

	// The slope is =  (THLmean2 - THLmean1) / (DAC_DISC_L_setting_2 - DAC_DISC_L_setting_1)
	eta = (r2.DAC_DISC_setting - r1.DAC_DISC_setting) / (r2.weighted_arithmetic_mean - r1.weighted_arithmetic_mean);
	cut = r2.DAC_DISC_setting - (eta * r2.weighted_arithmetic_mean);

}

void QCstmEqualization::GetSlopeAndCut_Adj_THL(ScanResults r1, ScanResults r2, double & eta, double & cut) {

	eta = (r2.global_adj - r1.global_adj) / (r2.weighted_arithmetic_mean - r1.weighted_arithmetic_mean);
	cut = r2.global_adj - (eta * r2.weighted_arithmetic_mean);

}

/*
 * The way the next two functions relate looks redundant but I need
 * it for when this is called from inside a thread.
 */
void QCstmEqualization::SetAllAdjustmentBits() {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SetAllAdjustmentBits(spidrcontrol);

}

void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol, int chipIndex ) {

	int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
	if ( chipIndex < 0 || chipIndex > nChips - 1) {
		cout << "[ERROR] wrong chip index !" << endl;
		return;
	}
	_deviceIndex = chipIndex;
	_eqresults = _eqVector[chipIndex];

	// This comes from hand-picked masking operation
	// Clean the chip first
	pair<int, int> pix;
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		pix = XtoXY(i, __array_size_x);
		spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, false);
	}
	spidrcontrol->setPixelConfigMpx3rx( _deviceIndex );

	// Now set the right adjustments
	SetAllAdjustmentBits(spidrcontrol);

}

void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol) {

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
			pix = XtoXY( (*i), __matrix_size_x );
			cout << "devid:" << _deviceIndex << " | " << pix.first << "," << pix.second << endl;
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

void QCstmEqualization::ClearAllAdjustmentBits() {

	// Clear all data structures
	_eqresults->ClearAdj();
	_eqresults->ClearMasked();
	_eqresults->ClearReactiveThresholds();

	// And now set it up
	SetAllAdjustmentBits();

}

void QCstmEqualization::SetAllAdjustmentBits(SpidrController * spidrcontrol, int val_L, int val_H) {

	//SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

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

void QCstmEqualization::Configuration(bool reset) {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	// Reset pixel configuration
	if ( reset ) spidrcontrol->resetPixelConfig();

	// All adjustment bits to zero
	SetAllAdjustmentBits(spidrcontrol, 0x0, 0x0);

	// OMR
	//_spidrcontrol->setPolarity( true );		// Holes collection
	//_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse
	spidrcontrol->setPixelDepth( _deviceIndex, 12 );
	spidrcontrol->setColourMode( _deviceIndex, false ); 	// Fine Pitch
	spidrcontrol->setCsmSpm( _deviceIndex, 0 );			// Single Pixel mode


	// For Equalization
	spidrcontrol->setEqThreshH( _deviceIndex, true );
	spidrcontrol->setDiscCsmSpm( _deviceIndex, 0 );		// DiscL used
	//spidrcontrol->setDiscCsmSpm( 1 );		// DiscH used


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
	// Sequential R/W
	int trig_mode      = SHUTTERMODE_AUTO;     // Auto-trigger mode
	int trig_length_us = 5000;  // This time shouldn't be longer than the period defined by trig_freq_hz
	int trig_freq_hz   = 100;   // One trigger every 10ms
	int nr_of_triggers = _nTriggers;    // This is the number of shutter open i get
	//int trig_pulse_count;
	spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

	// Continues R/W
	// One counter at a time, but no dead-time



}

Mpx3EqualizationResults * QCstmEqualization::GetEqualizationResults(int chipIndex) {
	int nChips = _mpx3gui->getConfig()->getNDevicesSupported();
	if ( chipIndex < 0 || chipIndex > nChips - 1) return 0x0;
	return _eqVector[chipIndex];
}

void QCstmEqualization::LoadEqualization(){

	int nChips = _mpx3gui->getConfig()->getNDevicesSupported();


	// FIXME !!!!
	// For display.  Memory should be taken care properly and the layout is not correct here
	int * displaymatrix = new int[__matrix_size_x * __matrix_size_y * nChips];


	for(int i = 0 ; i < nChips ; i++) {

		// Next equalization
		_eqVector.push_back( new Mpx3EqualizationResults );
		_eqresults = _eqVector[i];

		// And clear all previous adjustements
		// In case an equalization was done in the same session
		//ClearAllAdjustmentBits();

		// Check if the device is alive
		if ( ! _mpx3gui->getConfig()->detectorResponds( i ) ) {
			cout << "[ERR ] Device " << i << " not responding." << endl;
			continue;
		}

		// Go to this device index
		ChangeDeviceIndex( i );
		_ui->devIdSpinBox->setValue( i );

		QString adjfn = "adj_";
		adjfn += QString::number(i, 10);
		QString maskfn = "mask_";
		maskfn += QString::number(i, 10);

		_eqresults->ReadAdjBinaryFile( adjfn );
		_eqresults->ReadMaskBinaryFile( maskfn );

		// And talk to the hardware
		SetAllAdjustmentBits( );

		// Display the equalization
		int * adj_matrix = _eqresults->GetAdjustementMatrix();

		for (int j = 0 ; j < __matrix_size_x * __matrix_size_y ; j++) {
			displaymatrix[i*(__matrix_size_x * __matrix_size_y) + j ] = adj_matrix[j];
		}

	}


	_ui->_intermediatePlot->clear();
	//int lastActiveFrame = _ui->_intermediatePlot->GetLastActive();
	QSize boundingBox = _mpx3gui->getDataset()->computeBoundingBox();
	cout << boundingBox.width() << ", " << boundingBox.height() << endl;
	_ui->_intermediatePlot->addData( displaymatrix, boundingBox.width(), boundingBox.height() );
	_ui->_intermediatePlot->setActive( 0 );


}

void QCstmEqualization::SetupSignalsAndSlots() {

	connect( _ui->_startEq, SIGNAL(clicked()), this, SLOT(StartEqualization()) );
	connect( _ui->_startEqAll, SIGNAL(clicked()), this, SLOT(StartEqualizationAllChips()) );
	connect( _ui->_stopEq, SIGNAL(clicked()), this, SLOT(StopEqualization()) );
	connect( _ui->_cleanEq, SIGNAL(clicked()), this, SLOT(CleanEqualization()) );

	connect(_ui->_intermediatePlot, SIGNAL(mouseOverChanged(QString)), _ui->mouseHoveLabel, SLOT(setText(QString)));
	//_ui->_statusLabel->setStyleSheet("QLabel { background-color : gray; color : black; }");
	_ui->_histoWidget->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );

	// Spinboxes
	connect( _ui->nTriggersSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeNTriggers(int) ) );
	connect( _ui->devIdSpinBox, SIGNAL(valueChanged(int)), this, SLOT(  ChangeDeviceIndex(int) ) );
	connect( _ui->spacingSpinBox, SIGNAL(valueChanged(int)), this, SLOT(  ChangeSpacing(int) ) );

	connect( _ui->eqMinSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeMin(int) ) );
	connect( _ui->eqMaxSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeMax(int) ) );
	connect( _ui->eqStepSpinBox, SIGNAL(valueChanged(int)), this, SLOT( ChangeStep(int) ) );



}

void QCstmEqualization::ChangeNTriggers( int nTriggers ) {
	if( nTriggers < 0 ) return; // can't really happen cause the SpinBox has been limited
	_nTriggers = nTriggers;
}

void QCstmEqualization::ChangeDeviceIndex( int index ) {
	if( index < 0 ) return; // can't really happen cause the SpinBox has been limited
	_deviceIndex = index;
}

void QCstmEqualization::ChangeSpacing( int spacing ) {
	if( spacing < 0 ) return;
	_spacing = spacing;
}
void QCstmEqualization::ChangeMin(int min) {
	if( min < 0 ) return;
	_minScanTHL = min;
}
void QCstmEqualization::ChangeMax(int max) {
	if( max < 0 ) return;
	_maxScanTHL = max;
}
void QCstmEqualization::ChangeStep(int step) {
	if( step < 0 ) return;
	_stepScan = step;
}

void QCstmEqualization::StopEqualization() {

	//GetUI()->_histoWidget->Clean();

}

void QCstmEqualization::CleanEqualization() {

	// Clean histograms
	GetUI()->_histoWidget->Clean();

	// Clean text browser
	_ui->eqTextBrowser->clear();

}

void QCstmEqualization::ConnectionStatusChanged() {

	// WARNING
	// This could imply talking to the chip at the same time than the dacscontrol or
	//  other widget.  Careful !

	//_ui->_statusLabel->setText("Connected");
	//_ui->_statusLabel->setStyleSheet("QLabel { background-color : blue; color : white; }");

}

void QCstmEqualization::AppendToTextBrowser(QString s){

	_ui->eqTextBrowser->append( s );

}

void QCstmEqualization::ClearTextBrowser(){
	_ui->eqTextBrowser->clear();
}

void QCstmEqualization::timerEvent( QTimerEvent * /*evt*/ ) {


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

void QCstmEqualization::PrintFraction(int * buffer, int size, int first_last) {

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

int QCstmEqualization::GetNPixelsActive(int * buffer, int size, verblev verbose) {

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

pair<int, int> QCstmEqualization::XtoXY(int X, int dimX){
	return make_pair(X % dimX, X/dimX);
}

void QCstmEqualization::SetMinScan(int min) {
	_minScanTHL = min;
	_ui->eqMinSpinBox->setValue( _minScanTHL );
}

void QCstmEqualization::SetMaxScan(int max) {
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
void Mpx3EqualizationResults::Clear() {
	ClearAdj();
	ClearMasked();
	ClearReactiveThresholds();
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

void QCstmEqualization::on_heatmapCombobox_currentIndexChanged(const QString &arg1)//would be more elegant to do with signals and slots, but would require either specalizing the combobox, or making the heatmapMap globally visible.
{
}

void QCstmEqualization::on_openfileButton_clicked()
{
}
