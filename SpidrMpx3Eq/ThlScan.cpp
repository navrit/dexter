/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "ThlScan.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include "mpx3eq_common.h"
#include "barchart.h"
#include "qcstmplotheatmap.h"

#include "mpx3defs.h"
#include "mpx3equalization.h"

#include <iostream>
using namespace std;

ThlScan::ThlScan() {

}

ThlScan::ThlScan(BarChart * bc, QCstmPlotHeatmap * hm, Mpx3Equalization * ptr) {

	// keep these pointers
	_spidrcontrol = 0; // Assuming no connection yet
	_spidrdaq = 0;     // Assuming no connection yet
	_chart = bc;
	_heatmap = hm;
	_equalization = ptr;

	RewindData();

}

void ThlScan::ConnectToHardware(SpidrController * sc, SpidrDaq * sd) {
	_spidrcontrol = sc;
	_spidrdaq = sd;
}

/**
 * Clear data structures and get ready for a new Scan
 */
void ThlScan::RewindData() {

	if ( !_pixelCountsMap.empty() ) {
		_pixelCountsMap.clear();
	}

	// Create entries at 0 for the whole matric
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		_pixelCountsMap[i] = __NOT_TESTED_YET;
	}

	cout << "[INFO] ThlScan::_pixelCountsMap rewinded. Contains " << _pixelCountsMap.size() << " elements." << endl;

	// Nothing should be masked
	_maskedSet.clear();

}

ThlScan::~ThlScan(){

}

void ThlScan::DoScan(int dac_code){

	// Pointer to incoming data
	int * data;
	int spacing = _equalization->GetSpacing();
	int minScan = _equalization->GetMinScan();
	int maxScan = _equalization->GetMaxScan();
	int stepScan = _equalization->GetStepScan();
	int deviceIndex = _equalization->GetDeviceIndex();

	// Prepare the heatmap
	_heatmap->clear();
	// The heatmap can store the images.  This is the indexing.
	int frameId = 0;

	for(int maskOffsetItr_x = 0 ; maskOffsetItr_x < spacing ; maskOffsetItr_x++ ) {

		for(int maskOffsetItr_y = 0 ; maskOffsetItr_y < spacing ; maskOffsetItr_y++ ) {

			// Set mask
			int nMasked = SetEqualizationMask(spacing, maskOffsetItr_x, maskOffsetItr_y);
			cout << "offset_x: " << maskOffsetItr_x << ", offset_y:" << maskOffsetItr_y <<  " | N pixels unmasked = " << __matrix_size - nMasked << endl;

			// Start the Scan for one mask
			for(int i = minScan ; i <= maxScan ; i += stepScan ) {

				//cout << "THL : " << i << endl;

				_spidrcontrol->setDac( deviceIndex, dac_code, i );
				//_spidrcontrol->writeDacs( _deviceIndex );

				// Start the trigger as configured
				_spidrcontrol->startAutoTrigger();
				Sleep( 50 );

				// See if there is a frame available
				// I should get as many frames as triggers

				while ( _spidrdaq->hasFrame() ) {

					int size_in_bytes = -1;
					data = _spidrdaq->frameData(0, &size_in_bytes);

					ExtractScanInfo( data, size_in_bytes );

					_spidrdaq->releaseFrame();
					Sleep( 10 ); // Allow time to get and decode the next frame, if any


					//for(int i = 0 ; i < 256*256 ; i++) {
					//	if( data[i] != 0 ) {
					//		cout << i << ": " << data[i] << endl;
					//	}
					//}
					// Report to heatmap
					_heatmap->addData(data, 256, 256); // Add a new plot/frame.
					_heatmap->setActive(frameId++); // Activate the last plot (the new one)
					//_heatmap->setData( data, 256, 256 );

				}

				// Report to graph
				UpdateChart(0, i);

				// Clean map
				_pixelCountsMap.clear();

			}

			// Try to resize to min max the X axis here
			//_chart->GetBarChartProperties()->max_x[0] = 200;

		}
	}
	///////////////////////////////////////////////////////////////////
	// Scan finished

	// Here's on Scan completed.  Do the stats on it.
	ExtractStatsOnChart(0);

}

void ThlScan::ExtractScanInfo(int * data, int size_in_bytes) {

	int nPixels = size_in_bytes/4;
	// int pixelsActive = 0;
	// Each 32 bits corresponds to the counts in each pixel already
	// in 'int' representation as the decoding has been requested
	for(int i = 0 ; i < nPixels ; i++) {

		// In principle masked pixels should count 0, but seems like it's not the case
		// TODO.
		// I checked that the entry is not zero, and also that is not in the maskedMap
		if ( data[i] != 0 && ( _maskedSet.find( i ) == _maskedSet.end() ) ) {
			// Increase the counting in this pixel if it hasn't already reached the _nTriggers
			if ( _pixelCountsMap[i] < _equalization->GetNTriggers() ) _pixelCountsMap[i]++;
			//pixelsActive++;
		}

	}
	//cout << "!!! pixels active !!! " << pixelsActive << endl;

}

void ThlScan::UpdateChart(int setId, int thlValue) {

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

	_chart->SetValueInSet( setId , thlValue, cntr );

}

void ThlScan::ExtractStatsOnChart(int setId){

	QCPBarDataMap * dataSet = _chart->GetDataSet( setId )->data();
	QCPBarDataMap::iterator i = dataSet->begin();
	for( ; i != dataSet->end() ; i++){
		cout << (*i).key << ", " << (*i).value << endl;
	}

}

/**
 * Create and apply the mask with a given spacing
 *
 */
int ThlScan::SetEqualizationMask(int spacing, int offset_x, int offset_y) {

	// Clear previous mask.  Not sending the configuration yet !
	ClearMask(false);

	for (int i = 0 ; i < __array_size_x ; i++) {

		// For instance if spacing = 4, there should be calls with offset_x=0,1,2,3
		//  in order to cover the whole matrix.
		if ( (i + offset_x) % spacing == 0 ) { // This is the right column

			for (int j = 0 ; j < __array_size_y ; j++) {

				if( (j + offset_y) % spacing != 0 ) { // This one should be masked
					_spidrcontrol->setPixelMaskMpx3rx(i, j);
					_maskedSet.insert( XYtoX(i, j, __array_size_x ) );
				} // leaving unmasked (j + offset_x) % spacing == 0

			}

		} else { // mask the entire column
			for (int j = 0 ; j < __array_size_y ; j++) {
				_spidrcontrol->setPixelMaskMpx3rx(i, j);
				_maskedSet.insert( XYtoX(i, j, __array_size_x ) );
			}
		}

	}

	// And send the configuration
	_spidrcontrol->setPixelConfigMpx3rx( _equalization->GetDeviceIndex() );

	//cout << "N masked = " << _maskedSet.size() << endl;

	return (int) _maskedSet.size();
}

void ThlScan::ClearMask(bool sendToChip){

	for (int i = 0 ; i < __array_size_x ; i++) {
		for (int j = 0 ; j < __array_size_y ; j++) {
			_spidrcontrol->setPixelMaskMpx3rx(i, j, false);
		}
	}
	// And send the configuration
	if ( sendToChip ) _spidrcontrol->setPixelConfigMpx3rx( _equalization->GetDeviceIndex() );

	_maskedSet.clear();
};
