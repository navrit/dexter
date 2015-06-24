/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "ThlScan.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include "barchart.h"
#include "qcstmplotheatmap.h"
#include "qcstmequalization.h"
#include "ui_qcstmequalization.h"

#include "mpx3defs.h"
#include "mpx3eq_common.h"
#include "ui_mpx3gui.h"

#include <iostream>
#include <map>
using namespace std;

//ThlScan::ThlScan() {
//}

ThlScan::ThlScan(Mpx3GUI * mpx3gui, QCstmEqualization * ptr) {

	// keep these pointers
	_mpx3gui = mpx3gui;
	_equalization = ptr;
	_chart = 0x0;
	_heatmap = 0x0;
	_spidrcontrol = 0x0; // Assuming no connection yet
	_spidrdaq = 0x0;     // Assuming no connection yet
	_frameId = 0;
	_adjType = __adjust_to_global;
	_stop = false;
	_scanType = __BASIC_SCAN;
	_fineTunningPixelsEqualized.clear();

	// Results of the scan
	_results.weighted_arithmetic_mean = 0.;
	_results.sigma = 0.;

	_detectedScanBoundary_L = 0;
	_detectedScanBoundary_H = 0;

	// Set to true for special scans
	_stopWhenPlateau = false;

	// number of reactive pixels
	_nReactivePixels = 0;

	RewindData();

	// Extract the information needed when the thread will be launched
	_spacing = _equalization->GetSpacing();
	_minScan = _equalization->GetMinScan();
	_maxScan = _equalization->GetMaxScan();
	_stepScan = _equalization->GetStepScan();
	_deviceIndex = _equalization->GetDeviceIndex();


}

void ThlScan::ConnectToHardware(SpidrController * sc, SpidrDaq * sd) {

	_spidrcontrol = sc;
	_spidrdaq = sd;

	// I need to do this here and not when already running the thread
	// Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
	if( _spidrcontrol ) { _spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
	else { _srcAddr = 0; }

	// The chart and the heatmap !
	_chart = _equalization->GetUI()->_histoWidget;
	_heatmap = _equalization->GetUI()->_intermediatePlot;

}

/**
 * Clear data structures and get ready for a new Scan
 */
void ThlScan::RewindData() {

	if ( !_pixelCountsMap.empty() ) {
		_pixelCountsMap.clear();
	}
	if ( !_pixelReactiveTHL.empty() ) {
		_pixelReactiveTHL.clear();
	}
	_nReactivePixels = 0;

	// Create entries at 0 for the whole matric
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		_pixelCountsMap[i] = __NOT_TESTED_YET;
		_pixelReactiveTHL[i] = __UNDEFINED;
	}

	//cout << "[INFO] ThlScan::_pixelCountsMap rewinded. Contains " << _pixelCountsMap.size() << " elements." << endl;
	//cout << "[INFO] ThlScan::_pixelReactiveTHL rewinded. Contains " << _pixelReactiveTHL.size() << " elements." << endl;

	// Nothing should be masked
	_maskedSet.clear();

}

void ThlScan::RewindPixelCountsMap(){

	if ( !_pixelCountsMap.empty() ) {
		_pixelCountsMap.clear();
	}
	// Create entries at 0 for the whole matric
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		_pixelCountsMap[i] = __NOT_TESTED_YET;
	}

	// Nothing should be masked
	_maskedSet.clear();

}

//ThlScan::~ThlScan(){

//}

void ThlScan::DeliverPreliminaryEqualization(Mpx3EqualizationResults * eq, ScanResults scan_res) {

	// Fill the preliminary results
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		// First the adjustment is the same for all
		eq->SetPixelAdj(i, scan_res.global_adj );
		// And here is the reactive threshold
		eq->SetPixelReactiveThl(i, _pixelReactiveTHL[i] );
	}

}

void ThlScan::DoScan(int dac_code, int setId, int DAC_Disc_code, int numberOfLoops, bool blindScan) {

	_dac_code = dac_code;
	_setId = setId;
	_numberOfLoops = numberOfLoops;
	_blindScan = blindScan;
	_DAC_Disc_code = DAC_Disc_code;

}

void ThlScan::on_stop_data_taking_thread() {
	// Used to properly stop the data taking thread
	_stop = true;
}



void ThlScan::run() {

	// The normal scan starting from scratch
	if(_scanType == __BASIC_SCAN) EqualizationScan();

	// This one is a fine tunning scan
	if(_scanType == __FINE_TUNNING1_SCAN) {
		ReAdjustPixelsOff(4, MPX3RX_DAC_DISC_L);
	}
}

void ThlScan::EqualizationScan() {

	// Open a new temporary connection to the spider to avoid collisions to the main one
	// Extract the ip address
	int ipaddr[4] = { 1, 1, 168, 192 };
	if ( _srcAddr != 0 ) {
		ipaddr[3] = (_srcAddr >> 24) & 0xFF;
		ipaddr[2] = (_srcAddr >> 16) & 0xFF;
		ipaddr[1] = (_srcAddr >>  8) & 0xFF;
		ipaddr[0] = (_srcAddr >>  0) & 0xFF;
	}
	SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

	if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
		cout << "[ERR ] Device not connected !" << endl;
		return;
	}

	//_equalization->SetAllAdjustmentBits(spidrcontrol, 0x5, 0x0);
	// Send all the adjustment bits to 0x5
	if ( _adjType == __adjust_to_global ) {
		if( _DAC_Disc_code == MPX3RX_DAC_DISC_L ) _equalization->SetAllAdjustmentBits(spidrcontrol, _equalization->GetGlobalAdj(), 0x0);
		if( _DAC_Disc_code == MPX3RX_DAC_DISC_H ) _equalization->SetAllAdjustmentBits(spidrcontrol, 0x0, _equalization->GetGlobalAdj());
	} else if ( _adjType == __adjust_to_equalizationMatrix ) {
		_equalization->SetAllAdjustmentBits(spidrcontrol);
	}

	// While equalizing one threshold the other can be at a very high value
	// to keep that circuit from reacting
	// Set Dac
	spidrcontrol->setDac( _deviceIndex, MPX3RX_DAC_THRESH_1, 150 );
	// Adjust the sliders and the SpinBoxes to the new value
	connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
	// Get the DAC back just to be sure and then slide&spin
	int dacVal = 0;
	spidrcontrol->getDac( _deviceIndex,  MPX3RX_DAC_THRESH_1, &dacVal);
	// SlideAndSpin works with the DAC index, no the code.
	int dacIndex = _mpx3gui->GetUI()->DACsWidget->GetDACIndex( MPX3RX_DAC_THRESH_1 );
	slideAndSpin( dacIndex,  dacVal );
	disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );


	// Signals to draw out of the worker
	connect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
	connect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
	connect(_equalization, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis

	// Sometimes a reduced loop is selected
	int processedLoops = 0;
	bool finishScan = false;
	bool finishTHLLoop = false;
	// For a truncated scan
	int expectedInOneThlLoop = ( __matrix_size ) / ( _spacing*_spacing );

	// The data buffer id doesn't necessarily corresponds to _deviceIndex
	int idDataFetch = _mpx3gui->getConfig()->getDataBufferId( _deviceIndex );
	cout << "[INFO] Run a Scan. devIndex:" << _deviceIndex << " | databuffer:" << idDataFetch << endl;

	int step = _stepScan;
	bool accelerationApplied = false;
	int accelerationFlagCntr = 0;

	int progressMax = _numberOfLoops;
	if ( _numberOfLoops < 0 ) progressMax = _spacing * _spacing;

	for(int maskOffsetItr_x = 0 ; maskOffsetItr_x < _spacing ; maskOffsetItr_x++ ) {

		for(int maskOffsetItr_y = 0 ; maskOffsetItr_y < _spacing ; maskOffsetItr_y++ ) {

			QString loopProgressS;
			loopProgressS =  QString::number( maskOffsetItr_x * _spacing + maskOffsetItr_y + 1, 'd', 0 );
			loopProgressS += "/";
			loopProgressS += QString::number( progressMax, 'd', 0 );
			connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelLoopProgress, SLOT( setText(QString)) );
			fillText( loopProgressS );
			disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelLoopProgress, SLOT( setText(QString)) );


			// Set mask
			int nMasked = SetEqualizationMask(spidrcontrol, _spacing, maskOffsetItr_x, maskOffsetItr_y);
			cout << "offset_x: " << maskOffsetItr_x << ", offset_y:" << maskOffsetItr_y <<  " | N pixels unmasked = " << __matrix_size - nMasked << endl;


			// Start the Scan for one mask
			_pixelReactiveInScan = 0;
			finishTHLLoop = false;
			accelerationApplied = false;
			accelerationFlagCntr = 0;
			step = _stepScan;
			bool doReadFrames = true;

			for(_thlItr = _minScan ; _thlItr <= _maxScan ; _thlItr += step ) {

				//cout << "------------ THL : " << _thlItr << "----------------" << endl;

				QString thlLabelS;
				thlLabelS = QString::number( _thlItr, 'd', 0 );
				if ( accelerationApplied ) thlLabelS += " acc";
				// Send signal to Labels.  Making connections one by one.
				connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelTHLCurrentValue, SLOT( setText(QString)) );
				fillText( thlLabelS );
				disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelTHLCurrentValue, SLOT( setText(QString)) );

				// Set Dac
				spidrcontrol->setDac( _deviceIndex, _dac_code, _thlItr );
				// Adjust the sliders and the SpinBoxes to the new value
				connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
				// Get the DAC back just to be sure and then slide&spin
				int dacVal = 0;
				spidrcontrol->getDac( _deviceIndex, _dac_code, &dacVal);
				// SlideAndSpin works with the DAC index, no the code.
				int dacIndex = _mpx3gui->GetUI()->DACsWidget->GetDACIndex( _dac_code );
				slideAndSpin( dacIndex,  dacVal );
				disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );

				// Start the trigger as configured
				spidrcontrol->startAutoTrigger();

				// See if there is a frame available
				// I should get as many frames as triggers
				// Assume the frame won't come
				doReadFrames = false;
				while ( _spidrdaq->hasFrame( 25 ) ) { // 5ms for eq + 20ms transfer over the network

					// A frame is here
					doReadFrames = true;
					// Check quality
					if ( _spidrdaq->packetsLostCountFrame() != 0 ) { // from any of the chips connected
						doReadFrames = false;
					}

					if ( doReadFrames ) {
						int size_in_bytes = -1;
						_data = _spidrdaq->frameData(idDataFetch, &size_in_bytes);
						_pixelReactiveInScan += ExtractScanInfo( _data, size_in_bytes, _thlItr );
					}


					// Release
					_spidrdaq->releaseFrame();

					if ( doReadFrames ) {
						// Report to heatmap
						UpdateHeatMapSignal(_mpx3gui->getDataset()->x(), _mpx3gui->getDataset()->y());

						// Report to graph
						if ( !_blindScan ) UpdateChartSignal(_setId, _thlItr);

						//_heatmap->addData(data, 256, 256); // Add a new plot/frame.
						//_heatmap->setActive(_frameId++); // Activate the last plot (the new one)
						//_heatmap->setData( data, 256, 256 );

						// Last scan boundaries
						// This information could be useful for a next scan
						if ( _thlItr < _detectedScanBoundary_L ) _detectedScanBoundary_L = _thlItr;
						if ( _thlItr > _detectedScanBoundary_H ) _detectedScanBoundary_H = _thlItr;
					}

				}

				// Try again if necessary
				if( !doReadFrames ) {
					doReadFrames = true;
					_thlItr -= step;
					if ( _thlItr < _minScan ) _thlItr = _minScan;
					continue;
				}


				/*
				// FIXME
				// FIXME
				// If the scan has reached the total number of pixels expected to react. Stop.
				//if ( pixelReactiveInScan % expectedInScan == 0 && pixelReactiveInScan > 0 ) {
				//	finishScan = true;
				//	cout << "[INFO] All pixels in round found active. Scan stops at THL = " << i << endl;
				//}

				// See if this is a scan which can be aloud to truncate.
				// Useful in certain cases like DiscL optimization
				if ( _stopWhenPlateau ) {
					if ( (double)_pixelReactiveInScan > (double)expectedInOneThlLoop*0.99 ) {
						finishTHLLoop = true;
						cout << "[INFO] Truncate scan. 99% reached. Scan stops at THL = " << _thlItr << endl;
					}
				}
				 */

				QString reactiveLabelS;
				reactiveLabelS = QString::number( _pixelReactiveInScan, 'd', 0 );
				// Send signal to Labels.  Making connections one by one.
				connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelNPixelsReactive, SLOT( setText(QString)) );
				fillText( reactiveLabelS );
				disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelNPixelsReactive, SLOT( setText(QString)) );



				// If done with all pixels
				if ( _pixelReactiveInScan == expectedInOneThlLoop ) {
					finishTHLLoop = true;
				}

				if( finishScan ) break;
				if( finishTHLLoop ) break;

				// Accelerate if the pixels reactive reached a significate fraction
				// Once the 99% limit is reached, wait 50 counts and accelerate
				if ( (double)_pixelReactiveInScan > (double)expectedInOneThlLoop*0.99
						&& !accelerationApplied
						&& accelerationFlagCntr < __accelerationStartLimit
				) {
					accelerationFlagCntr += step;
					if ( accelerationFlagCntr >= __accelerationStartLimit ) {
						step *= 5;
						accelerationApplied = true;
					}
				}

				// If called to Stop
				if ( _stop ) break;
			}

			// A full spacing loop has been achieved here
			processedLoops++;
			if( _numberOfLoops > 0 && _numberOfLoops == processedLoops ) finishScan = true;
			if( finishScan ) break;

			// If called to Stop
			if ( _stop ) break;
		}

		if( finishScan ) break;
		// If called to Stop
		if ( _stop ) break;
	}

	///////////////////////////////////////////////////////////////////
	// Scan finished

	// Here's on Scan completed.  Do the stats on it.
	//ExtractStatsOnChart(_setId);

	// Signals to draw out of the thread
	disconnect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
	disconnect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
	disconnect( _equalization, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread()) );

	// in case the thread is used again
	_stop = false;

	delete spidrcontrol;
}

bool ThlScan::OutsideTargetRegion(int pix, double Nsigma){
	return (

			_pixelReactiveTHL[pix] < __equalization_target - ceil(Nsigma*_results.sigma)
			||
			_pixelReactiveTHL[pix] > __equalization_target + ceil(Nsigma*_results.sigma)
			||
			_pixelReactiveTHL[pix] < _equalization->GetNTriggers() // simply never responded
	);
}

/**
 * As the fine tunnig is started.  Tag the pixels in order to know who needs rework.
 * The vetolist has been obtained through ExtractFineTunningVetoList and lists the
 *   pixels which don't need rework.
 */

void ThlScan::TagPixelsEqualizationStatus(set<int> vetoList) {

	// This pixels will be marked in the Mpx3EqualizationResults as __EQUALIZED

	for ( int i = 0 ; i < __matrix_size ; i++ ) {

		if( vetoList.find(i) == vetoList.end() ) {
			// if it is not in the vetolist it needs rework
			_equalization->GetEqualizationResults(_deviceIndex)->SetStatus( i, Mpx3EqualizationResults::__SCHEDULED_FOR_FINETUNING );

		} else {
			// if it is found in the vetolist this pixel is well equalized
			_equalization->GetEqualizationResults(_deviceIndex)->SetStatus( i, Mpx3EqualizationResults::__EQUALIZED );
			// These can be send to the histogram immediately
			_fineTunningPixelsEqualized.insert( i );
		}

	}

	// The pixels which are ready can be drawn immediately in the plot
	emit UpdateChartPixelsReadySignal(_setId);

}

set<int> ThlScan::ExtractFineTunningVetoList(double Nsigma) {

	set<int> vetoList;

	for ( int i = 0 ; i < __matrix_size ; i++ ) {

		if ( ! OutsideTargetRegion(i, Nsigma) ) {

			vetoList.insert( i );

		}

	}

	cout << "[INFO] " << vetoList.size() << " pixels in veto for fine tuning." << endl;

	return vetoList;
}

set<int> ThlScan::ExtractReworkList(double Nsigma) {

	set<int> reworkList;

	for ( int i = 0 ; i < __matrix_size ; i++ ) {

		if ( OutsideTargetRegion(i, Nsigma) ) {

			reworkList.insert( i );

		}

	}

	cout << "[INFO] " << reworkList.size() << " pixels scheduled for fine tuning." << endl;

	return reworkList;
}

/**
 * return <pixid, adj> map
 */
map<int, int> ThlScan::ExtractReworkAdjustments(set<int> reworkPixels) {

	map<int, int> adj;
	set<int>::iterator i;
	set<int>::iterator iE;

	for( ; i != iE ; i++ ) {
		adj[*i] = _equalization->GetEqualizationResults(_deviceIndex)->GetPixelAdj( *i );
	}

	return adj;
}

void ThlScan::RewindReactionCounters(set<int> reworkPixelsSet) {

	set<int>::iterator i = reworkPixelsSet.begin();
	set<int>::iterator iE = reworkPixelsSet.end();

	for( ; i != iE ; i++ ) {
		// Now I can rewind counters for this pixel
		_pixelReactiveTHL[ *i ] = __UNDEFINED;
		_pixelCountsMap[ *i ] = __NOT_TESTED_YET;
	}

}

void ThlScan::DumpSet(set<int> theset, QString name) {

	cout << "-- " << name.toStdString() << " [" << (int)theset.size() << "] --------" << endl;

	if ( theset.empty() ) return;

	set<int>::iterator i = theset.begin();
	set<int>::iterator iE = theset.end();

	cout << "< ";
	for ( ; i != iE ; ) {
		cout << *i << "{" << _pixelReactiveTHL[*i] << ":" << _equalization->GetEqualizationResults(_deviceIndex)->GetPixelAdj( *i ) << "}, ";
		if ( ++i != iE ) cout << ", ";
	}
	cout << " >" << endl;

}

void ThlScan::DumpRework(set<int> reworkSubset, int thl){

	set<int>::iterator i = reworkSubset.begin();
	set<int>::iterator iE = reworkSubset.end();

	int adj = 0;
	Mpx3EqualizationResults::eq_status eqStat;

	// Asume all rework pixels are still outside the target region
	bool anyOutsideTargetRegion = false;

	cout << "THL:" << thl << endl;

	for ( ; i != iE ; i++ ) {
		cout << "   pix:" << *i << " | status:" << _equalization->GetEqualizationResults(_deviceIndex)->GetStatus( *i )
																																									<< " | adj" <<  _equalization->GetEqualizationResults(_deviceIndex)->GetPixelAdj( *i )
																																									<< " | reactive:" << _pixelReactiveTHL[*i]
																																									                                       << endl;
	}

}

/*
 * Returns a new reworkSubset with those still needing re-adjustment
 */
set<int> ThlScan::NeedsReadjustment(set<int> reworkSubset, set<int> & doneAndNoisySet, int Nsigma) {

	set<int> rw;

	// For every interesting pixels this needs to hold
	set<int>::iterator i = reworkSubset.begin();
	set<int>::iterator iE = reworkSubset.end();

	bool outsideTargetRegion = false;
	for ( ; i != iE ; i++ ) {

		////////////////////////////////////////////////////////////////////////////
		// See if it is still outside the target region and that it hasn't been
		//  tagged as equalized or impossible-to-equalize
		if( _equalization->GetEqualizationResults(_deviceIndex)->GetStatus( *i ) >= Mpx3EqualizationResults::__EQUALIZED ) {
			// Done AND Noisy pixels are reported here (N.B >= Mpx3EqualizationResults::__EQUALIZED) !
			doneAndNoisySet.insert( *i );
			continue;
		}
		if ( ! OutsideTargetRegion( *i, Nsigma ) ) {
			// this might have happened in the last scan, tag it here
			_equalization->GetEqualizationResults(_deviceIndex)->SetStatus( *i,  Mpx3EqualizationResults::__EQUALIZED );
			// Done pixels are reported here
			doneAndNoisySet.insert( *i );
		} else {
			// These keep going
			rw.insert( *i );
		}

	}

	return rw;
}

/**
 * The THL loop for every new adjustment test ends when this member returns false.
 */
bool ThlScan::ThlScanEndConditionFineTuning(set<int> reworkSubset, int thl, int Nsigma) {

	// For every interesting pixels this needs to hold
	set<int>::iterator i = reworkSubset.begin();
	set<int>::iterator iE = reworkSubset.end();

	int adj = 0;
	Mpx3EqualizationResults::eq_status eqStat;

	// Asume all rework pixels are still outside the target region
	bool anyPendingToReact = false;

	for ( ; i != iE ; i++ ) {

		eqStat =  _equalization->GetEqualizationResults(_deviceIndex)->GetStatus( *i );

		// If the pixel is already equalized or failed then continue
		if ( eqStat > Mpx3EqualizationResults::__EQUALIZED ) continue;

		////////////////////////////////////////////////////////////////////////////
		// See if it is still outside the target region
		bool outsideTargetRegion = OutsideTargetRegion( *i, Nsigma );

		// If the pixel entered the target region, mark it as __EQUALIZED
		if( ! outsideTargetRegion ) {

			_equalization->GetEqualizationResults(_deviceIndex)->SetStatus( *i, Mpx3EqualizationResults::__EQUALIZED );

			/*
			// If it entered the target region, it could still be that we can get even closer to the target.
			// 1) If it is right at the target we are done here
			if ( _pixelReactiveTHL[pix] == __equalization_target ) P
				_equalization->GetEqualizationResults(_deviceIndex)->SetStatus( *i, Mpx3EqualizationResults::__EQUALIZED );
			} else {
				// 2) Otherwise start keeping track of that's happening here

			}
			*/

		}

		// Is this pixel pending to react ?
		bool pendingToReact = false;
		if ( _pixelReactiveTHL[ *i ] == __UNDEFINED ) pendingToReact = true;

		// If there is any pixel pending to react then keep going
		if ( !anyPendingToReact && pendingToReact ) anyPendingToReact = true;

		////////////////////////////////////////////////////////////////////////////
		// Get the next adjustment to try
		adj = _equalization->GetEqualizationResults(_deviceIndex)->GetPixelAdj( *i );

		// If the adjustment we are about to try is out of bounds
		//  this pixel will be discarded as __EQUALIZATION_FAILED_ADJ_OUTOFBOUNDS
		if ( ( adj > __max_adj_val || adj < 0 ) && eqStat < Mpx3EqualizationResults::__EQUALIZED ) {
			_equalization->GetEqualizationResults(_deviceIndex)->SetStatus( *i, Mpx3EqualizationResults::__EQUALIZATION_FAILED_ADJ_OUTOFBOUNDS );
		}

	}


	// Check the limits of the thl scan.  If there's nowhere else to go then this scan is done.
	if ( thl > _maxScan ) return false;

	return anyPendingToReact;
}

void ThlScan::UnmaskPixelsInLocalSet(set<int> reworkPixelsSet) {

	set<int>::iterator i = reworkPixelsSet.begin();
	set<int>::iterator iE = reworkPixelsSet.end();

	for( ; i != iE ; i++ ) {
		set<int>::iterator iS = _maskedSet.find( *i );
		if ( iS != _maskedSet.end() ) _maskedSet.erase( iS );
	}

}

void ThlScan::ShiftAdjustments(SpidrController * spidrcontrol, set<int> reworkSubset) {

	set<int>::iterator i = reworkSubset.begin();
	set<int>::iterator iE = reworkSubset.end();

	pair<int, int> pix;
	int adj = 0, newadj = 0;

	for( ; i != iE ; i++ ) {

		// If the pixel is already equalized or failed this is not needed anymore
		if ( _equalization->GetEqualizationResults(_deviceIndex)->GetStatus( *i ) >= Mpx3EqualizationResults::__EQUALIZED ) continue;

		// In this case the pixel never actuall fired cause it was probably confined
		//  to the negative THL values.  We try here the minimum possible adjustment
		//  which effectively pushes a pixel to the right.
		adj = _equalization->GetEqualizationResults(_deviceIndex)->GetPixelAdj( *i );

		// Check if the current adjustment is already at the limits
		if ( adj == 0x0 || adj == __max_adj_val ) {
			_equalization->GetEqualizationResults(_deviceIndex)->SetStatus( *i, Mpx3EqualizationResults::__EQUALIZATION_FAILED_ADJ_OUTOFBOUNDS );
			continue;
		}

		//if ( _pixelReactiveTHL[ i->first ] == __UNDEFINED ) i->second = 0x0;

		// Otherwise see where it is standing and try the next value
		// - If the pixel is at the right of the equalization target try
		//		a higher adjustment until it reaches the max Adj.
		//- If the pixel is at the left, try a lower adjustment.
		//if ( _pixelReactiveTHL[ i->first ] > __equalization_target ) i->second++;
		//else i->second--;
		if ( _pixelReactiveTHL[ *i ] == __UNDEFINED ) newadj = 0x0;
		else if ( _pixelReactiveTHL[ *i ] > __equalization_target ) newadj = adj + 1;
		else newadj = adj - 1;

		// Now set the new value
		// Set the new adjustment for this particular pixel.
		pix = XtoXY(*i, __matrix_size_x);
		_equalization->GetEqualizationResults(_deviceIndex)->SetPixelAdj(*i, newadj);
		// Write the adjustment
		spidrcontrol->configPixelMpx3rx(pix.first, pix.second, newadj, 0x0 );

	}

	// send to chip
	spidrcontrol->setPixelConfigMpx3rx( _deviceIndex );

}

/**
 * reworkPixelsSet:	Contains the pixels which need rework.
 * reworkSubset: 	Will contain only those from reworkPixelsSet which respect the spacing.
 * 		Every time a pixel is picked from reworkPixelsSet, it also gets removed from it.
 * 		reworkSubset gets shrink by other functions below when rework pixels are tuned.
 */
int ThlScan::ExtractReworkSubsetSpacingAware(set<int> & reworkPixelsSet, set<int> & reworkSubset, int spacing) {

	// Iterate on the input set
	set<int>::iterator i = reworkPixelsSet.begin();
	set<int>::iterator iE = reworkPixelsSet.end();

	// Iterate on the output set
	set<int>::iterator ri = reworkSubset.begin();
	set<int>::iterator riE = reworkSubset.end();

	// Count the number of pixels moved to the reworkSubset
	int cntrToSubset = 0;

	// Keep a list of those pixels sent to the reworkSubset which need to be erased
	//  from the reworkPixelsSet.
	set<int> scheduleToErase;

	// Chances are the reworkSubset is empty.  Let's insert one value to get it started.
	if ( reworkSubset.empty() ) {
		reworkSubset.insert ( *i );
		scheduleToErase.insert ( *i );
		cntrToSubset++;
		i++;
	}

	bool clearToAdd = true;

	for ( ; i != iE ; i++ ) {

		// Rewind bounds on reworkSubset
		ri = reworkSubset.begin();
		riE = reworkSubset.end();

		// See if the current pixel is at a safe distance from all pixels in reworkSubset
		clearToAdd = true;
		for ( ; ri != riE ; ri++ ) {

			if ( ! TwoPixelsRespectMinimumSpacing( *i, *ri, spacing) ) clearToAdd = false;

		}

		// If it respects the minimum spacing, then put it in the reworkSubset and
		//  schedule for erasing.
		if ( clearToAdd ) {
			reworkSubset.insert ( *i );
			scheduleToErase.insert ( *i );
			cntrToSubset++;
		}

	}

	// Erase from reworkPixelsSet what was tossed in reworkSubset
	// Iterate on the output set
	set<int>::iterator ei = scheduleToErase.begin();
	set<int>::iterator eiE = scheduleToErase.end();
	set<int>::iterator itoerase;
	for ( ; ei != eiE ; ei++ ) {
		itoerase = reworkPixelsSet.find( *ei );
		if ( itoerase != reworkPixelsSet.end() ) reworkPixelsSet.erase( itoerase );
	}

	return cntrToSubset;
}

bool ThlScan::TwoPixelsRespectMinimumSpacing(int pix1, int pix2, int spacing) {

	pair<int, int> p1 = XtoXY(pix1, __matrix_size_x);
	pair<int, int> p2 = XtoXY(pix2, __matrix_size_x);

	if (
			abs(p1.first  - p2.first) < spacing
			&&
			abs(p1.second - p2.second) < spacing
	) return false;

	return true;
}

int ThlScan::ReAdjustPixelsOff(double Nsigma, int dac_code) {

	int ipaddr[4] = { 1, 1, 168, 192 };
	if( _srcAddr != 0 ) {
		ipaddr[3] = (_srcAddr >> 24) & 0xFF;
		ipaddr[2] = (_srcAddr >> 16) & 0xFF;
		ipaddr[1] = (_srcAddr >>  8) & 0xFF;
		ipaddr[0] = (_srcAddr >>  0) & 0xFF;
	}

	SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

	connect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
	connect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
	connect( this, SIGNAL( UpdateChartPixelsReadySignal(int) ), this, SLOT( UpdateChartPixelsReady(int) ) );


	// I re-scan only pixels that are off the target by N*sigma.
	// But that scan should also keep into account the spacing.
	// The idea is to re-do the spacing scan putting a veto on
	//  the pixels which don't need fine tunning.

	// Extract list of pixels to veto 'cause they are already ok
	set<int> vetoPixels = ExtractFineTunningVetoList(Nsigma);
	// Now that we know which pixels are not-to-be reworked.  let's apply tags
	TagPixelsEqualizationStatus( vetoPixels );
	// Get now the pixels which need rework
	set<int> reworkPixelsSet = ExtractReworkList(Nsigma);

	int adjustedPixels = 0;
	int * data;
	int frameId = 0;
	bool doReadFrames = true;

	// The data buffer id doesn't necessarily corresponds to _deviceIndex
	int idDataFetch = _mpx3gui->getConfig()->getDataBufferId( _deviceIndex );
	cout << "[INFO] Run a Scan. devIndex:" << _deviceIndex << " | databuffer:" << idDataFetch << endl;

	// Loop over the pixels off the adjustment
	//for ( int i = 0 ; i < __matrix_size ; i++ ) {
	//
	int stepScan = _equalization->GetStepScan();
	int deviceIndex = _equalization->GetDeviceIndex();
	//int i = 0;
	//int reworkPixels = 0;
	bool accelerationApplied = false;

	//for (int maskOffsetItr_x = 0 ; maskOffsetItr_x < _spacing ; maskOffsetItr_x++ ) {
	//for (int maskOffsetItr_y = 0 ; maskOffsetItr_y < _spacing ; maskOffsetItr_y++ ) {

	// Mask as usual but consider the veto list because we don't need to work on those pixels
	//  as they are already properly adjusted.
	//int nMasked = SetEqualizationMask(spidrcontrol, _spacing, maskOffsetItr_x, maskOffsetItr_y);
	//cout << "offset_x: " << maskOffsetItr_x << ", offset_y:" << maskOffsetItr_y <<  " | N pixels unmasked = " << __matrix_size - nMasked << endl;
	//int nExtraMasked = SetEqualizationVetoMask(spidrcontrol, vetoPixels, false);

	// reworkPixels are the number of pixels unmask and due for rework in this loop
	//reworkPixels = ( (__matrix_size_x * __matrix_size_y ) / ( _spacing * _spacing ) ) - nExtraMasked;
	//cout << "extra masked = " << nExtraMasked << " | rework = " << reworkPixels << endl;

	// There is going to be a subset of the rework pixels which is active in this particular mask.
	//set<int> reworkSubset = GetReworkSubset(reworkPixelsSet, _spacing, maskOffsetItr_x, maskOffsetItr_y);

	int reworkCntr = 0;

	// Out of this reworkPixelsSet I need a subset which respects the spacing
	set<int> reworkSubset;

	// Keep track of what has been tuned, or goes as noisy.
	set<int> doneAndNoisySet;

	// This is the original number of pixels which need rework
	int nPixelsRework = (int)reworkPixelsSet.size();

	// Stop when they have all been processed
	while ( (int) doneAndNoisySet.size() < nPixelsRework ) {

		reworkCntr += ExtractReworkSubsetSpacingAware(reworkPixelsSet, reworkSubset, _spacing);
		DumpSet( reworkPixelsSet, "reworkPixelsSet" );
		DumpSet( reworkSubset,  "reworkSubset" );
		DumpSet( doneAndNoisySet, "doneAndNoisySet");

		// Adjust again if needed
		while ( ! reworkSubset.empty() ) {

			// Here doneAndNoisySet contains the list of pixels already worked out
			cout << "[INFO] progress : " << (int)doneAndNoisySet.size() << "/" << nPixelsRework << endl;

			reworkCntr += ExtractReworkSubsetSpacingAware(reworkPixelsSet, reworkSubset, _spacing);
			DumpSet( reworkPixelsSet, "reworkPixelsSet" );
			DumpSet( reworkSubset,  "reworkSubset" );
			DumpSet( doneAndNoisySet, "doneAndNoisySet");

			// At this point reworkSubset already shrinked by exactly doneAndNoisySet
			// 1) doneAndNoisySet should be masked
			// 2) reworkSubset can be unmasked
			int unmasked = SetEqualizationMask(spidrcontrol, reworkSubset);
			cout << "[INFO] leaving " << unmasked << " pixels unmasked" << endl;

			// At this point only pixels needing rework are unmasked.
			// We are doing the rework on all these pixels at the same time.
			// 1) Decide on the start adjustment point for all the rework pixels
			// 		- If the pixel is at the right of the equalization target try
			//  		a higher adjustment until it reaches the max Adj.
			// 		- If the pixel is at the left, try a lower adjustment.
			ShiftAdjustments(spidrcontrol, reworkSubset); // ! THE PIXELS OUT OF BOUNDS ARE TAGGED HERE !

			// Time to rewind the counters for these particular pixels
			RewindReactionCounters(reworkSubset);
			// Unmask pixels in the local set
			UnmaskPixelsInLocalSet(reworkSubset);


			/////////////////////////
			// Thl scan
			int thlItr = 0;
			stepScan = _equalization->GetStepScan();
			accelerationApplied = false;
			while (  ThlScanEndConditionFineTuning( reworkSubset, thlItr, Nsigma )  ) { // ! THE PIXELS EQUALIZED ARE TAGGED HERE !

				QString thlLabelS;
				thlLabelS = QString::number( thlItr, 'd', 0 );
				if ( accelerationApplied ) thlLabelS += " acc";
				// Send signal to Labels.  Making connections one by one.
				connect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelTHLCurrentValue, SLOT( setText(QString)) );
				fillText( thlLabelS );
				disconnect( this, SIGNAL( fillText(QString) ), _equalization->GetUI()->eqLabelTHLCurrentValue, SLOT( setText(QString)) );

				//////////////////////////////////////////////////////
				// Now ready to scan on this unique pixel !

				spidrcontrol->setDac( deviceIndex, dac_code, thlItr );
				//_spidrcontrol->writeDacs( _deviceIndex );

				// Start the trigger as configured
				spidrcontrol->startAutoTrigger();

				// See if there is a frame available. I should get as many frames as triggers
				// Assume the frame won't come intact
				doReadFrames = false;

				while ( _spidrdaq->hasFrame( 25 ) ) { // 5ms for eq + 20ms transfer over the network

					doReadFrames = true;
					if ( _spidrdaq->packetsLostCountFrame() != 0 ) { // from any of the chips connected
						doReadFrames = false;
					}


					if ( doReadFrames ) {

						int size_in_bytes = -1;
						_data = _spidrdaq->frameData(idDataFetch, &size_in_bytes);
						ExtractScanInfo( _data, size_in_bytes, thlItr );

						// Report to heatmap
						UpdateHeatMapSignal(_mpx3gui->getDataset()->x(), _mpx3gui->getDataset()->y());

						UpdateChartSignal(_setId, _thlItr);

						// Continue thl scan
						thlItr += stepScan;


					} else {
						// otherwise try again on the same thl
					}

					// Release frame whatever happens
					_spidrdaq->releaseFrame();

				}

				// In the fine tuning the acceleration can take place at a reasonable
				// deviation from the target.  I pick half range or the dac.
				if ( thlItr > MPX3RX_DAC_TABLE[dac_code].dflt && !accelerationApplied ) {
					stepScan *= __step_scan_boostfactor;
					accelerationApplied = true;
				}

			} // THL loop

			// Here is where the reworkSubset shrinks.  Ready to take new elements if any.
			reworkSubset = NeedsReadjustment( reworkSubset, doneAndNoisySet, Nsigma ); // ! HERE THE reworkSubset SHRINKS for both Equalized and Noisy !
			cout << "[INFO] Rework " << (int) reworkSubset.size() << endl;

		} // Adjust loop


		//} // mask loop
		//} // mask loop

	}

	cout << "[INFO] done with fine tuning. " << reworkCntr << " pixels considered." << endl;

	disconnect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );
	disconnect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
	disconnect( this, SIGNAL( UpdateChartPixelsReadySignal(int) ), this, SLOT( UpdateChartPixelsReady(int) ) );

	return adjustedPixels;
}


void ThlScan::ExtractStatsOnChart(int setId) {

	double weigtedSum = 0.;
	double weights = 0.;
	// Normalization value (p val) used later when calculating the variance
	double norm_val = 0.;
	// Calculate the weighted arithmetic mean and standard deviation
	QCPBarDataMap * dataSet = _chart->GetDataSet( setId )->data();
	QCPBarDataMap::iterator i = dataSet->begin();

	for( ; i != dataSet->end() ; i++) {

		if( (*i).value != 0 ) {
			weigtedSum += ( (*i).key * (*i).value );
			weights += (*i).value;

			norm_val += (*i).value;
		}
	}
	norm_val = 1. / norm_val;

	if ( weights != 0.) _results.weighted_arithmetic_mean = weigtedSum / weights;
	else _results.weighted_arithmetic_mean = 0.;

	// Rewind the iterator and get the sigma
	// I need to weight the sigmas first

	i = dataSet->begin();
	for( ; i != dataSet->end() ; i++) {
		_results.sigma += ( (*i).value * norm_val ) * (
				( (*i).key - _results.weighted_arithmetic_mean )
				*
				( (*i).key - _results.weighted_arithmetic_mean )
		);
	}
	_results.sigma = sqrt( _results.sigma );

}


int ThlScan::ExtractScanInfo(int * data, int size_in_bytes, int thl) {

	int nPixels = size_in_bytes/4;
	int pixelsActive = 0;
	// Each 32 bits corresponds to the counts in each pixel already
	// in 'int' representation as the decoding has been requested

	for(int i = 0 ; i < nPixels ; i++) {

		// I checked that the entry is not zero, and also that is not in the maskedMap
		if ( data[i] != 0 && ( _maskedSet.find( i ) == _maskedSet.end() ) ) {

			// Increase the counting in this pixel if it hasn't already reached the _nTriggers
			if ( _pixelCountsMap[i] < _equalization->GetNTriggers() ) {
				_pixelCountsMap[i]++;
			}

			// It it reached the number of triggers, set this Threshold as the reactive threshold
			if ( _pixelCountsMap[i] == _equalization->GetNTriggers() ) {

				// This way I mark the pixel as reactive +1
				_pixelCountsMap[i]++;

				//cout << i << " | " << _pixelCountsMap[i] << " -- data --> " << data[i] << " | triggers : " << _equalization->GetNTriggers() << endl;

				_pixelReactiveTHL[i] = thl;
				_nReactivePixels++;
				pixelsActive++;
			}

		}

	}

	return pixelsActive;
}

int ThlScan::ExtractScanInfo(int * data, int size_in_bytes, int thl, int interestingpix) {

	int nPixels = size_in_bytes/4;
	int pixelsActive = 0;
	// Each 32 bits corresponds to the counts in each pixel already
	// in 'int' representation as the decoding has been requested

	for(int i = 0 ; i < nPixels ; i++) {

		if ( i == interestingpix ) {
			cout << " { " << i << " | counts: " << _pixelCountsMap[i] << " -- data --> " << data[i] << " | reactive: " << _pixelReactiveTHL[i] << "} " << endl;
		}

		// I checked that the entry is not zero, and also that is not in the maskedMap
		if ( data[i] != 0 && ( _maskedSet.find( i ) == _maskedSet.end() ) ) {
			// Increase the counting in this pixel if it hasn't already reached the _nTriggers
			if ( _pixelCountsMap[i] < _equalization->GetNTriggers() ) {
				_pixelCountsMap[i]++;
			}
			// It it reached the number of triggers, set this Threshold as the reactive threshold
			if ( _pixelCountsMap[i] == _equalization->GetNTriggers() ) {

				// This way I mark the pixel as reactive +1
				_pixelCountsMap[i]++;

				//cout << i << " | " << _pixelCountsMap[i] << " -- data --> " << data[i] << " | triggers : " << _equalization->GetNTriggers() << endl;

				_pixelReactiveTHL[i] = thl;
				_nReactivePixels++;
				pixelsActive++;
			}


		}

	}

	return pixelsActive;
}


int ThlScan::NumberOfNonReactingPixels() {

	int nNonReactive = 0;
	map<int, int>::iterator i = _pixelReactiveTHL.begin();
	map<int, int>::iterator iE = _pixelReactiveTHL.end();

	// Test for non reactive pixels
	for ( ; i != iE ; i++ ) {
		if( (*i).second == __UNDEFINED ) nNonReactive++;
	}

	return nNonReactive;
}

vector<int> ThlScan::GetNonReactingPixels() {

	vector<int> nonReactive;
	map<int, int>::iterator i = _pixelReactiveTHL.begin();
	map<int, int>::iterator iE = _pixelReactiveTHL.end();

	// Test for non reactive pixels
	for ( ; i != iE ; i++ ) {
		if( (*i).second == __UNDEFINED ) nonReactive.push_back( (*i).first );
	}
	return nonReactive;
}

void ThlScan::SetConfigurationToScanResults(int DAC_DISC_setting, int global_adj) {

	_results.DAC_DISC_setting = DAC_DISC_setting;
	_results.global_adj = global_adj;

}


void ThlScan::UpdateHeatMap(int sizex, int sizey) {

	//_heatmap->addData(_data, sizex, sizey);	// Add a new plot/frame.
	_heatmap->setData( _data, sizex, sizey);
	//_heatmap->setActive(_frameId++); 		// Activate the last plot (the new one)

}

void ThlScan::UpdateChart(int setId, int thlValue) {

	map<int, int>::iterator itr = _pixelCountsMap.begin();
	map<int, int>::iterator itrE = _pixelCountsMap.end();

	// I am going to plot for this threshold the number of
	//  pixels which reached _nTriggers counts.  The next time
	//  they won't be considered.
	int cntr = 0;
	for( ; itr != itrE ; itr++ ) {

		if( (*itr).second ==  _equalization->GetNTriggers() + 1 ) { // Marked as +1 := reactive
			// Count how many pixels counted at this threshold
			cntr++; // Marked as +2 --> taken into account in Chart
			(*itr).second++; // This way we avoid re-ploting next time. The value _nTriggers+2 identifies these pixels
		}

	}

	if ( cntr > 0 ) _chart->SetValueInSet( setId , thlValue, cntr );

}

/**
 * Only for plotting purposes.  Take the pixels already counted as done and plot.
 */
void ThlScan::UpdateChartPixelsReady(int setId) {

	// <thl, cntr>
	map<int, int> thlCntr;
	set<int>::iterator i = _fineTunningPixelsEqualized.begin();
	set<int>::iterator iE = _fineTunningPixelsEqualized.end();

	// initialize
	for (i = _fineTunningPixelsEqualized.begin() ; i != iE ; i++ ) {
		thlCntr[  _pixelReactiveTHL[*i] ] = 0;
	}

	// build the histogram
	for (i = _fineTunningPixelsEqualized.begin() ; i != iE ; i++ ) {
		thlCntr[  _pixelReactiveTHL[*i] ]++;
	}

	// Draw
	map<int, int>::iterator im = thlCntr.begin();
	map<int, int>::iterator imE = thlCntr.end();
	for ( ; im != imE ; im++ ) {
		_chart->SetValueInSet( setId , im->first, im->second );
	}

}

void ThlScan::UpdateChart(int thlValue) {

	map<int, int>::iterator itr = _pixelCountsMap.begin();
	map<int, int>::iterator itrE = _pixelCountsMap.end();

	// I am going to plot for this threshold the number of
	//  pixels which reached _nTriggers counts.  The next time
	//  they won't be considered.
	int cntr = 0;
	for( ; itr != itrE ; itr++ ) {

		if( (*itr).second == _equalization->GetNTriggers() ) {
			cntr++;
			(*itr).second++; // This way we avoid re-ploting next time. The value _nTriggers+1 identifies these pixels
		}

	}

	_chart->SetValueInSet( _setId , thlValue, cntr );

}

/**
 *  Once a mask is prepared only a subset of the rework pixels fall in.
 *  Get that set.
 */
set<int> ThlScan::GetReworkSubset(set<int> reworkSet, int spacing, int offset_x, int offset_y) {

	set<int> subset;

	for (int i = 0 ; i < __array_size_x ; i++) {

		// For instance if spacing = 4, there should be calls with offset_x=0,1,2,3
		//  in order to cover the whole matrix.
		if ( (i + offset_x) % spacing == 0 ) { // This is the right column

			for (int j = 0 ; j < __array_size_y ; j++) {

				if( (j + offset_y) % spacing == 0 ) { // Unmasked
					int pix = XYtoX(i,j, __array_size_x);
					if( reworkSet.find( pix ) != reworkSet.end() ) subset.insert( pix );


				}
			}
		}

	}

	return subset;
}

/**
 * Create and apply the mask with a given spacing
 */
int ThlScan::SetEqualizationMask(SpidrController * spidrcontrol, int spacing, int offset_x, int offset_y) {

	// Clear previous mask.  Not sending the configuration yet !
	ClearMask(spidrcontrol, false);

	for (int i = 0 ; i < __array_size_x ; i++) {

		// For instance if spacing = 4, there should be calls with offset_x=0,1,2,3
		//  in order to cover the whole matrix.
		if ( (i + offset_x) % spacing == 0 ) { // This is the right column

			for (int j = 0 ; j < __array_size_y ; j++) {

				if( (j + offset_y) % spacing != 0 ) { // This one should be masked
					spidrcontrol->setPixelMaskMpx3rx(i, j);
					_maskedSet.insert( XYtoX(i, j, __array_size_x ) );
				} // leaving unmasked (j + offset_x) % spacing == 0

			}

		} else { // mask the entire column
			for (int j = 0 ; j < __array_size_y ; j++) {
				spidrcontrol->setPixelMaskMpx3rx(i, j);
				_maskedSet.insert( XYtoX(i, j, __array_size_x ) );
			}
		}

	}

	// And send the configuration
	spidrcontrol->setPixelConfigMpx3rx( _equalization->GetDeviceIndex() );

	//cout << "N masked = " << _maskedSet.size() << endl;

	return (int) _maskedSet.size();
}

/**
 * Leave reworkPixels unmasked.  Returns number of unmaksed pixels.
 */
int ThlScan::SetEqualizationMask(SpidrController * spidrcontrol, set<int> reworkPixels) {

	// Clear previous mask.  Not sending the configuration yet !
	ClearMask(spidrcontrol, false);

	int cntr = 0, pix = 0;
	for (int i = 0 ; i < __array_size_x ; i++) {

		for (int j = 0 ; j < __array_size_y ; j++) {

			pix = XYtoX(i, j, __array_size_x);

			// The pixels NOT in the reworkPixels set, must be masked
			if ( reworkPixels.find( pix ) == reworkPixels.end() ) {
				spidrcontrol->setPixelMaskMpx3rx(i,j);
				_maskedSet.insert( pix ); // this is a set, entries are unique
			} else {
				cntr++;
			}

		}

	}
	// And send the configuration
	spidrcontrol->setPixelConfigMpx3rx( _equalization->GetDeviceIndex() );

	return cntr;
}

int ThlScan::SetEqualizationVetoMask(SpidrController * spidrcontrol, set<int> vetolist, bool clear) {

	// Clear previous mask.  Not sending the configuration yet !
	if ( clear ) ClearMask(spidrcontrol, false);

	set<int>::iterator itr = vetolist.begin();
	set<int>::iterator itrE = vetolist.end();

	pair<int, int> pix;
	int cntr = 0;
	for ( ; itr != itrE ; itr++ ) {

		pix = XtoXY( *itr, __array_size_x );

		// If the pixel we are masking here was not in the masked let's count it
		if ( _maskedSet.find( *itr ) == _maskedSet.end() ) cntr++;

		// The pixels coming in the vetolist must be masked
		spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second);
		_maskedSet.insert( *itr ); // this is a set, entries are unique

	}

	// And send the configuration
	spidrcontrol->setPixelConfigMpx3rx( _equalization->GetDeviceIndex() );

	return cntr;
}


void ThlScan::ClearMask(SpidrController * spidrcontrol, bool sendToChip){

	for (int i = 0 ; i < __array_size_x ; i++) {
		for (int j = 0 ; j < __array_size_y ; j++) {
			spidrcontrol->setPixelMaskMpx3rx(i, j, false);
		}
	}
	// And send the configuration
	if ( sendToChip ) spidrcontrol->setPixelConfigMpx3rx( _equalization->GetDeviceIndex() );

	_maskedSet.clear();
};
