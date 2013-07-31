#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include "tpx3defs.h"

int main( int argc, char *argv[] )
{
  // Open a control connection to SPIDR-TPX3 module with address 192.168.100.10,
  // default port number 50000
  SpidrController spidrctrl( 192, 168, 100, 10 );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
	 << spidrctrl.connectionStateString() << ", "
	 << spidrctrl.connectionErrString() << endl;
    exit(1);
  }

  int device_nr = 0;

  // Set some Timepix3 DACs
  int dac_val = spidrctrl.dacMax( TPX3_VTHRESH_COARSE ) / 4;
  spidrctrl.setDac( device_nr, TPX3_VTHRESH_COARSE, dac_val );
  spidrctrl.setDac( device_nr, TPX3_VTHRESH_FINE, 0 );

  // Create and upload a Timepix3 pixel configuration
  spidrctrl.resetPixelConfig();          // Reset all values to zero
  spidrctrl.maskPixel( 34 );             // Mask pixel column 34
  spidrctrl.setPixelConfig( device_nr ); // Upload the pixel configuration

  // Set Timepix3 acquisition mode
  spidrctrl.setGenConfig( device_nr, TPX3_ACQMODE_TOA_TOT );

  exit(0);
}
