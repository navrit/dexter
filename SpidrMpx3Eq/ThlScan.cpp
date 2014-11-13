/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "ThlScan.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include "mpx3eq_common.h"

ThlScan::ThlScan() {

}

ThlScan::ThlScan(SpidrController * sc, SpidrDaq * sd) {

	// keep these pointers
	_spidrcntrl = sc;
	_spidrdaq = sd;

}

ThlScan::~ThlScan(){

}

void ThlScan::DoScan(){

}

/**
 * Create and apply the mask with a given spacing
 *
 */
void ThlScan::SetMask(int spacing, int offset){

	for(int i = 0 ; i < __array_size_x ; i++) {

		// For instance if spacing = 4, there should be calls with offset=0,1,2,3
		//  in order to cover the whole matrix.
		if( (i + offset) % spacing == 0 ) { // This is the right column

			for(int j = 0 ; j < __array_size_y ; j++) {

				if( (j + offset) % spacing != 0 ) { // This one should be masked
					_spidrcntrl->setPixelMaskMpx3rx(i, j);
					//_spidrcntrl->maskPixelMpx3rx(i,j);
				} // leaving unmasked (j + offset) % spacing == 0

			}

		} else { // mask the entire column
			for(int j = 0 ; j < __array_size_y ; j++) {
				_spidrcntrl->setPixelMaskMpx3rx(i, j);
				//_spidrcntrl->maskPixelMpx3rx(i,j);
			}
		}

	}
}

