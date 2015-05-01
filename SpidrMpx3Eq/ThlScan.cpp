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

	cout << "[INFO] ThlScan::_pixelCountsMap rewinded. Contains " << _pixelCountsMap.size() << " elements." << endl;
	cout << "[INFO] ThlScan::_pixelReactiveTHL rewinded. Contains " << _pixelReactiveTHL.size() << " elements." << endl;

	// Nothing should be masked
	_maskedSet.clear();

}

//ThlScan::~ThlScan(){

//}

Mpx3EqualizationResults * ThlScan::DeliverPreliminaryEqualization(ScanResults scan_res) {

	Mpx3EqualizationResults * eq = new Mpx3EqualizationResults;

	// Fill the preliminary results
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		// First the adjustment is the same for all
		eq->SetPixelAdj(i, scan_res.global_adj );
		// And here is the reactive threshold
		eq->SetPixelReactiveThl(i, _pixelReactiveTHL[i] );
	}

	return eq;
}

void ThlScan::DoScan(int dac_code, int setId, int DAC_Disc_code, int numberOfLoops, bool blindScan) {

	_dac_code = dac_code;
	_setId = setId;
	_numberOfLoops = numberOfLoops;
	_blindScan = blindScan;
	_DAC_Disc_code = DAC_Disc_code;

}

void ThlScan::run() {

	// Open a new temporary connection to the spider to avoid collisions to the main one
	// Extract the ip address
	int ipaddr[4] = { 1, 1, 168, 192 };
	if( _srcAddr != 0 ) {
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

	// Signals to draw out of the worker
	connect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
	connect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );

	// Sometimes a reduced loop is selected
	int processedLoops = 0;
	bool finishScan = false;
	bool finishTHLLoop = false;
	// For a truncated scan
	int expectedInOneThlLoop = ( _equalization->GetNTriggers() * __matrix_size ) / ( _spacing*_spacing );
	// For a full matrix scan
	//if( _numberOfLoops <= 0) expectedInScan = __matrix_size;

	//int nMasked = SetEqualizationMask(spidrcontrol, _spacing, 0,0);

	int idDataFetch = 0;
	int step = _stepScan;
	bool accelerationApplied = false;
	int accelerationFlagCntr = 0;
	if( _mpx3gui->getFrameCount() > 1 ) idDataFetch = _deviceIndex;

	for(int maskOffsetItr_x = 0 ; maskOffsetItr_x < _spacing ; maskOffsetItr_x++ ) {

		for(int maskOffsetItr_y = 0 ; maskOffsetItr_y < _spacing ; maskOffsetItr_y++ ) {

			QString loopProgressS;
			loopProgressS =  QString::number( maskOffsetItr_x * _spacing + maskOffsetItr_y + 1, 'd', 0 );
			loopProgressS += "/";
			loopProgressS += QString::number( _spacing * _spacing, 'd', 0 );
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
			for(_thlItr = _minScan ; _thlItr <= _maxScan ; _thlItr += step ) {

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
				//StartAutoTriggerSignal();
				Sleep( 50 );

				// See if there is a frame available
				// I should get as many frames as triggers

				while ( _spidrdaq->hasFrame() ) { //HasFrameSignal(_thlItr) ) {
					// if HasFrameSignal(_thlItr) is true there will be a signal back
					// which will process the given frame

					int size_in_bytes = -1;

					_data = _spidrdaq->frameData(idDataFetch, &size_in_bytes);

					_pixelReactiveInScan += ExtractScanInfo( _data, size_in_bytes, _thlItr );

					//ReleaseFrameSignal();
					//Sleep( 10 ); // Allow time to get and decode the next frame, if any

					// Report to heatmap
					UpdateHeatMapSignal(_mpx3gui->getDataset()->x(), _mpx3gui->getDataset()->y());

					//_heatmap->addData(data, 256, 256); // Add a new plot/frame.
					//_heatmap->setActive(_frameId++); // Activate the last plot (the new one)
					//_heatmap->setData( data, 256, 256 );

					// Last scan boundaries
					// This information could be useful for a next scan
					if ( _thlItr < _detectedScanBoundary_L ) _detectedScanBoundary_L = _thlItr;
					if ( _thlItr > _detectedScanBoundary_H ) _detectedScanBoundary_H = _thlItr;

					//
					_spidrdaq->releaseFrame();
					Sleep(10);

				}
				//Sleep(10);

				// Report to graph
				//cout << _setId << "," << _thlItr << endl;
				if ( !_blindScan ) UpdateChartSignal(_setId, _thlItr);

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


			}


			// A full spacing loop has been achieved here
			processedLoops++;
			if( _numberOfLoops > 0 && _numberOfLoops == processedLoops ) finishScan = true;
			if( finishScan ) break;


		}
		if( finishScan ) break;
	}

	///////////////////////////////////////////////////////////////////
	// Scan finished

	// Here's on Scan completed.  Do the stats on it.
	//ExtractStatsOnChart(_setId);

	// Signals to draw out of the thread
	disconnect( this, SIGNAL( UpdateChartSignal(int, int) ), this, SLOT( UpdateChart(int, int) ) );
	disconnect( this, SIGNAL( UpdateHeatMapSignal(int, int) ), this, SLOT( UpdateHeatMap(int, int) ) );

	delete spidrcontrol;
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
			if ( _pixelCountsMap[i] < _equalization->GetNTriggers() ) _pixelCountsMap[i]++;
			// It it reached the number of triggers, stablish this Threshold as the reactive threshold
			if ( _pixelCountsMap[i] == _equalization->GetNTriggers() ) {
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

int ThlScan::ReAdjustPixelsOff(double N, int dac_code) {

	int adjustedPixels = 0;
	int * data;
	// The heatmap can store the images.  This is the indexing.
	int frameId = 0;

	// Loop over the pixels off the adjustment
	for ( int i = 0 ; i < __matrix_size ; i++ ) {

		if
		(
				_pixelReactiveTHL[i] < __equalization_target - N*_results.sigma
				||
				_pixelReactiveTHL[i] > __equalization_target + N*_results.sigma
		) {


			int startAdj = _equalization->GetEqualizationResults()->GetPixelAdj( i );
			bool outsideRegion = true;

			cout << "Reprocess pixel [" << i << "] | ";

			// Now scan from 0 to 10 sigma away from the last extrapolation adjustment
			int thlItr = 0;
			while ( startAdj < __max_adj_val && outsideRegion &&  thlItr <= __equalization_target + 10 * _results.sigma ) {

				// Mask the whole pad except by this pixel
				ClearMask(false); // don't send to the chip yet
				pair<int, int> unmaskPix = XtoXY(i, __matrix_size_x);
				_spidrcontrol->setPixelMaskMpx3rx(unmaskPix.first, unmaskPix.second, true);
				// Set the new adjustment for this particular pixel.
				// If the pixel is at the right of the equalization target try
				//  a higher adjustment bit until it reaches the max Adj. If the
				//  pixel is at the left, try a lower adjustment.
				if ( _pixelReactiveTHL[i] > __equalization_target ) startAdj++;
				else startAdj--;
				_equalization->GetEqualizationResults()->SetPixelAdj(i, startAdj);
				// Write to the spidrControl
				_equalization->SetAllAdjustmentBits( ); // _equalization->GetEqualizationResults() );
				// And send
				_spidrcontrol->setPixelConfigMpx3rx( _equalization->GetDeviceIndex() );

				//////////////////////////////////////////////////////
				// Now ready to scan on this unique pixel !
				int stepScan = _equalization->GetStepScan();
				int deviceIndex = _equalization->GetDeviceIndex();

				cout << "THL : " << thlItr << ", ";

				_spidrcontrol->setDac( deviceIndex, dac_code, thlItr );
				//_spidrcontrol->writeDacs( _deviceIndex );

				// Start the trigger as configured
				_spidrcontrol->startAutoTrigger();
				Sleep( 50 );

				// See if there is a frame available
				// I should get as many frames as triggers

				while ( _spidrdaq->hasFrame() ) {

					int size_in_bytes = -1;
					data = _spidrdaq->frameData(0, &size_in_bytes);

					ExtractScanInfo( data, size_in_bytes, thlItr );

					_spidrdaq->releaseFrame();
					Sleep( 10 ); // Allow time to get and decode the next frame, if any

					// Report to heatmap
					_heatmap->addData(data, 256, 256); // Add a new plot/frame.
					_heatmap->setActive(frameId++); // Activate the last plot (the new one)
					//_heatmap->setData( data, 256, 256 );

				}

				// Report to graph
				//UpdateChart(setId, i);

				//////////////////////////////////////////////////////


				// Is it still outside the region
				outsideRegion =
						(
								_pixelReactiveTHL[i] < __equalization_target - N*_results.sigma
								||
								_pixelReactiveTHL[i] > __equalization_target + N*_results.sigma
						);
				// THL scan
				thlItr += stepScan;
			}

			cout << endl;

			adjustedPixels++;
		}

	}

	return adjustedPixels;
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

		if( (*itr).second ==  _equalization->GetNTriggers() ) {
			cntr++;
			(*itr).second++; // This way we avoid re-ploting next time. The value _nTriggers+1 identifies these pixels
		}

	}

	_chart->SetValueInSet( setId , thlValue, cntr );

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
 * Create and apply the mask with a given spacing
 *
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
