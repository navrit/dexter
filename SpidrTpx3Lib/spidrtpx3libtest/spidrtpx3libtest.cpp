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

  int mdegrees, mvolt, mamp, mwatt;

  if( !spidrctrl.restartTimers() )
    cout << "###restartTimers: " << spidrctrl.errorString() << endl;

  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    cout << "###getDevId: " << spidrctrl.errorString() << endl;
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
  unsigned int timer_lo, timer_hi;
  if( !spidrctrl.getTimer( device_nr, &timer_lo, &timer_hi ) )
    cout << "###getTimer: " << spidrctrl.errorString() << endl;
  else
    cout << "Timer = " << timer_lo << ", " << timer_hi <<endl;
  if( !spidrctrl.getShutterStart( device_nr, &timer_lo, &timer_hi ) )
    cout << "###getShutterShart: " << spidrctrl.errorString() << endl;
  else
    cout << "Start = " << timer_lo << ", " << timer_hi <<endl;

  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    cout << "###getDevId: " << spidrctrl.errorString() << endl;
  //Sleep( 50 );
  if( !spidrctrl.getAvdd( &mvolt, &mamp, &mwatt ) )
    cout << "###getAvdd: " << spidrctrl.errorString() << endl;
  //Sleep( 50 );
  if( !spidrctrl.getDvdd( &mvolt, &mamp, &mwatt ) )
    cout << "###getDvdd: " << spidrctrl.errorString() << endl;
    
  // Set some Timepix3 DACs
  int dac_val = spidrctrl.dacMax( TPX3_VTHRESH_COARSE ) / 4;
  if( !spidrctrl.setDac( device_nr, TPX3_VTHRESH_COARSE, dac_val ) )
    cout << "###setDac1: " << spidrctrl.errorString() << endl;
  if( !spidrctrl.setDac( device_nr, TPX3_VTHRESH_FINE, 0 ) )
    cout << "###setDac2: " << spidrctrl.errorString() << endl;
  int code;
  for( code=1; code<=TPX3_DAC_COUNT_TO_SET; ++code )
    if( spidrctrl.getDac( device_nr, code, &dac_val ) )
      cout << "DAC " << code << ": " << dac_val << endl;
    else
      cout << "### DAC " << code << endl;

  spidrctrl.resetPixels( device_nr );

  // Create and upload a Timepix3 pixel configuration
  spidrctrl.resetPixelConfig();          // Reset all values to zero
  spidrctrl.setPixelMask( 254,2 );       // Mask pixel column 34
  spidrctrl.setPixelConfig( device_nr ); // Upload the pixel configuration

  unsigned char *pixconf = spidrctrl.pixelConfig();
  int x, y, cnt=0;
  if( spidrctrl.getPixelConfig( device_nr ) )
    {
      for( y=0; y<256; ++y )
	for( x=0; x<256; ++x )
	  if( pixconf[y*256+x] != 0 )
	    {
	      cout << x << ',' << y << ": " << hex << setw(2) << setfill('0')
		   << (unsigned int) pixconf[y*256+x] << dec << endl;
	      //printf( "%d,%d: %02X\n", y, x,
	      //      (unsigned int) pixconf[y*256+x] );
	      ++cnt;
	    }
      cout << "cnt = " << cnt << endl;
    }
  else
    {
      cout << "###getPixelConfig: " << spidrctrl.errorString() << endl;
    }

  // Set Timepix3 acquisition mode
  if( !spidrctrl.setGenConfig( device_nr,
			       TPX3_POLARITY_HPLUS |
			       TPX3_ACQMODE_TOA_TOT |
			       TPX3_GRAYCOUNT_ENA |
			       TPX3_TESTPULSE_ENA |
			       TPX3_FASTLO_ENA |
			       TPX3_SELECTTP_DIGITAL ) )
    cout << "###setGenCfg: " << spidrctrl.errorString() << endl;

  // Start acquisition
  if( !spidrctrl.datadrivenReadout() )
    cout << "###ddrivenReadout: " << spidrctrl.errorString() << endl;

  return 0;
}
