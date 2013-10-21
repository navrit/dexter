#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"

int main( int argc, char *argv[] )
{
  // ----------------------------------------------------------
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

  // Interface to Timepix3 pixel data acquisition
  SpidrDaq spidrdaq( &spidrctrl );
  string errstr = spidrdaq.errorString();
  if( !errstr.empty() ) cout << "### SpidrDaq: " << errstr << endl;

  int device_nr = 0;

  // ----------------------------------------------------------
  // DACs configuration

  if( !spidrctrl.setDacsDflt( device_nr ) )
    cout << "###setDacsDflt: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------
  // Pixel configuration

  if( !spidrctrl.resetPixels( device_nr ) )
    cout << "###resetPixels: " << spidrctrl.errorString() << endl;

  // Enable test-bit in all pixels
  spidrctrl.resetPixelConfig();
  spidrctrl.configPixel( ALL_PIXELS, ALL_PIXELS, 0, true );
  if( !spidrctrl.setPixelConfig( device_nr ) )
    cout << "###setPixelConfig: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------
  // Test pulse and CTPR configuration

  // Timepix3 test pulse configuration
  if( !spidrctrl.setTpPeriodPhase( device_nr, 10, 0 ) )
    cout << "###setTpPeriodPhase: " << spidrctrl.errorString() << endl;
  if( !spidrctrl.setTpNumber( device_nr, 1 ) )
    cout << "###setTpNumber: " << spidrctrl.errorString() << endl;

  // Enable test-pulses for some columns
  int col;
  for( col=0; col<256; ++col )
    if( col >= 10 && col < 11 )
      spidrctrl.configCtpr( device_nr, col, 1 );

  if( !spidrctrl.setCtpr( device_nr ) )
    cout << "###setCtpr: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------

  // SPIDR-TPX3 and Timepix3 timers
  if( !spidrctrl.restartTimers() )
    cout << "###restartTimers: " << spidrctrl.errorString() << endl;

  // Set Timepix3 acquisition mode
  if( !spidrctrl.setGenConfig( device_nr,
                               TPX3_POLARITY_HPLUS |
                               TPX3_ACQMODE_TOA_TOT |
                               TPX3_GRAYCOUNT_ENA |
                               TPX3_TESTPULSE_ENA |
                               TPX3_FASTLO_ENA |
                               TPX3_SELECTTP_DIG_ANALOG ) )
    cout << "###setGenCfg: " << spidrctrl.errorString() << endl;

  // Set Timepix3 into acquisition mode
  if( !spidrctrl.datadrivenReadout( device_nr ) )
    cout << "###ddrivenReadout: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------

  // Configure the shutter trigger
  int trig_mode      = 4;      // SPIDR_TRIG_AUTO;
  int trig_period_us = 100000; // 100 ms
  int trig_freq_hz   = 1;      // 1 Hz
  int nr_of_trigs    = 10;     // 10 triggers
  if( !spidrctrl.setTriggerConfig( trig_mode, trig_period_us,
                                   trig_freq_hz, nr_of_trigs ) )
    cout << "###setTriggerConfig: " << spidrctrl.errorString() << endl;

  // Sample 'frames' as well as write pixel data to file
  spidrdaq.setSampling( true );
  spidrdaq.openFile( "test.dat", true );

  // ----------------------------------------------------------
  // Get frames (data up to the next End-of-Readout packet)
  // and display some pixel data details

  // Start triggers
  if( !spidrctrl.startAutoTrigger() )
    cout << "###startAutoTrigger: " << spidrctrl.errorString() << endl;

  int   framecnt = 0, size, x, y, pixdata, timestamp;
  char *frame;
  bool  next_frame = true;
  while( next_frame )
    {
      next_frame = spidrdaq.getFrame( 3000 );
      if( next_frame )
        {
          ++framecnt;
          frame = spidrdaq.frameData( &size );
	  int pixcnt = 0;
	  /*
          while( spidrdaq.nextPixel( &x, &y, &pixdata, &timestamp ) )
	    {
	      cout << x << "," << y << ": " << hex << pixdata << dec << endl;
	      ++pixcnt;
	    }
	  */
	  unsigned long long pixel;
          while( (pixel = spidrdaq.nextPixel()) != 0 && pixcnt < 5 )
	    {
	      cout << pixcnt << ": " << hex << pixel << dec << endl;
	      ++pixcnt;
	    }
          cout << "Frame " << framecnt << " size=" << size << ": "
	       << pixcnt <<" pixels" << endl;	  
        }
      else
        {
          cout << "### Timeout -> finish after " << framecnt
	       << " frames" << endl;
        }
    }

  // ----------------------------------------------------------
  return 0;
}
