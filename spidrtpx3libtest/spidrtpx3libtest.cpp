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
  // Open a control connection to SPIDR-TPX3 module
  // with address 192.168.100.10, default port number 50000
  SpidrController spidrctrl( 192, 168, 100, 10 );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
	 << spidrctrl.connectionStateString() << ", "
	 << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  int device_nr = 0, id;

  /*
  int mdegrees, mvolt, mamp, mwatt;

  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    cout << "###setCfg: " << spidrctrl.errorString() << endl;
  //Sleep( 50 );
  if( !spidrctrl.getRemoteTemp( &mdegrees ) )
    cout << "###getRt: " << spidrctrl.errorString() << endl;
  else
    cout << "rtemp=" << mdegrees << endl;
  //Sleep( 50 );
  if( !spidrctrl.getLocalTemp( &mdegrees ) )
    cout << "###getLt: " << spidrctrl.errorString() << endl;
  else
    cout << "ltemp=" << mdegrees << endl;
  //Sleep( 50 );
  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    cout << "###setCfg: " << spidrctrl.errorString() << endl;
  //Sleep( 50 );
  if( !spidrctrl.getAvdd( &mvolt, &mamp, &mwatt ) )
    cout << "###getAvdd: " << spidrctrl.errorString() << endl;
  //Sleep( 50 );
  if( !spidrctrl.getDvdd( &mvolt, &mamp, &mwatt ) )
    cout << "###getDvdd: " << spidrctrl.errorString() << endl;
  */
    
  // Set some Timepix3 DACs
  int dac_val = spidrctrl.dacMax( TPX3_VTHRESH_COARSE ) / 4;
  if( !spidrctrl.setDac( device_nr, TPX3_VTHRESH_COARSE, dac_val ) )
    cout << "###setDac1: " << spidrctrl.errorString() << endl;
  if( !spidrctrl.setDac( device_nr, TPX3_VTHRESH_FINE, 0 ) )
    cout << "###setDac2: " << spidrctrl.errorString() << endl;

  // Create and upload a Timepix3 pixel configuration
  //spidrctrl.resetPixelConfig();          // Reset all values to zero
  //spidrctrl.maskPixel( 34 );             // Mask pixel column 34
  //spidrctrl.setPixelConfig( device_nr ); // Upload the pixel configuration

  // Set Timepix3 acquisition mode
  //if( !spidrctrl.setGenConfig( device_nr, TPX3_ACQMODE_TOA_TOT ) )
  //  cout << "###setCfg: " << spidrctrl.errorString() << endl;

  return 0;
}
