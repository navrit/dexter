/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "ThlScan.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include "mpx3eq_common.h"
#include "barchart.h"

#include "mpx3defs.h"

#include <iostream>
using namespace std;

ThlScan::ThlScan() {

}

ThlScan::ThlScan(BarChart * bc) {

	// keep these pointers
	_spidrcontrol = 0; // Assuming no connection yet
	_spidrdaq = 0;     // Assuming no connection yet
	_chart = bc;
	_nTriggers = 10;

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

void ThlScan::DoScan(){

	int * data;
	int dev_nr = 2;

	// Set the right configuration
	Configuration();

	for(int maskOffsetItr = 0 ; maskOffsetItr < 4 ; maskOffsetItr++ ) {

		// Set mask
		int nMasked = SetEqualizationMask(4, maskOffsetItr);
		cout << "N pixels unmasked = " << __matrix_size - nMasked << endl;

		// Start the Scan for one mask
		for(int i = 511 ; i >= 0 ; i -= 10 ) {

			//cout << "THL : " << i << endl;

			_spidrcontrol->setDac( dev_nr, MPX3RX_DAC_THRESH_0, i );
			//_spidrcontrol->writeDacs( dev_nr );

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

			}

			// Report to graph
			UpdateChart(i);

			// Clean map
			_pixelCountsMap.clear();

		}

	}


}

void ThlScan::ExtractScanInfo(int * data, int size_in_bytes) {

	int nPixels = size_in_bytes/4;
	//int pixelsActive = 0;
	// Each 32 bits corresponds to the counts in each pixel alread
	// in 'int' representation as the decoding has been requested
	for(int i = 0 ; i < nPixels ; i++) {

		// In principle masked pixels should count 0, but seems like it's not the case
		// TODO.
		// I checked that the entry is not zero, and also that is not in the maskedMap
		if ( data[i] != 0 && ( _maskedSet.find( i ) == _maskedSet.end() ) ) {
			// Increase the counting in this pixel if it hasn't already reached the _nTriggers
			if ( _pixelCountsMap[i] < _nTriggers ) _pixelCountsMap[i]++;
			//pixelsActive++;
		}

	}
	//cout << "!!! pixels active !!! " << pixelsActive << endl;

}

void ThlScan::UpdateChart(int thlValue) {

	map<int, int>::iterator itr = _pixelCountsMap.begin();
	map<int, int>::iterator itrE = _pixelCountsMap.end();

	// I am going to plot for this threshold the number of
	//  pixels which reached _nTriggers counts.  The next time
	//  they won't be considered.
	int cntr = 0;
	for( ; itr != itrE ; itr++ ) {

		if( (*itr).second == _nTriggers ) {
			cntr++;
			(*itr).second++; // This way we avoid re-ploting next time. The value _nTriggers+1 identifies these pixels
		}
	}

	_chart->SetValueInSet( 0 , thlValue, cntr );

}

void ThlScan::Configuration(){

	int dev_nr = 2;

	// Reset pixel configuration
	_spidrcontrol->resetPixelConfig();
	pair<int, int> pix;
	for ( int i = 0 ; i < __matrix_size ; i++ ) {
		pix = XtoXY(i, __array_size_x);
		_spidrcontrol->configPixelMpx3rx(pix.first, pix.second, 0, 0x0 ); // 0x1F = 31 is the max adjustment for 5 bits
	}
	_spidrcontrol->setPixelConfigMpx3rx(dev_nr);

	// OMR
	//_spidrcontrol->setPolarity( true );		// Holes collection
	//_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
	//_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse
	_spidrcontrol->setPixelDepth( 12 );

	_spidrcontrol->setColourMode( false ); 	// Fine Pitch
	_spidrcontrol->setCsmSpm( 0 );			// Single Pixel mode
	_spidrcontrol->setEqThreshH( true );
	_spidrcontrol->setDiscCsmSpm( 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
	//_spidrcontrol->setGainMode( 1 );

	// Gain ?!

	// Other OMR
	_spidrdaq->setDecodeFrames( true );
	_spidrcontrol->setPixelDepth( 12 );
	_spidrdaq->setPixelDepth( 12 );
	_spidrcontrol->setMaxPacketSize( 1024 );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = 4;     // Auto-trigger mode
	int trig_length_us = 1000;  // This time shouldn't be longer than the period defined by trig_freq_hz
	int trig_freq_hz   = 100;   // One trigger every 10ms
	int nr_of_triggers = _nTriggers;    // This is the number of shutter open i get
	//int trig_pulse_count;
	_spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

}

/**
 * Create and apply the mask with a given spacing
 *
 */
int ThlScan::SetEqualizationMask(int spacing, int offset) {

	// Clear previous mask
	ClearMask();

	for (int i = 0 ; i < __array_size_x ; i++) {

		// For instance if spacing = 4, there should be calls with offset=0,1,2,3
		//  in order to cover the whole matrix.
		if ( (i + offset) % spacing == 0 ) { // This is the right column

			for (int j = 0 ; j < __array_size_y ; j++) {

				if( (j + offset) % spacing != 0 ) { // This one should be masked
					_spidrcontrol->setPixelMaskMpx3rx(i, j);
					_maskedSet.insert( XYtoX(i, j, __array_size_x ) );
				} // leaving unmasked (j + offset) % spacing == 0

			}

		} else { // mask the entire column
			for (int j = 0 ; j < __array_size_y ; j++) {
				_spidrcontrol->setPixelMaskMpx3rx(i, j);
				_maskedSet.insert( XYtoX(i, j, __array_size_x ) );
			}
		}

	}

	cout << "N masked = " << _maskedSet.size() << endl;

	return (int) _maskedSet.size();
}

