#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(1000*ms)
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
  spidrctrl.setPixelTestEna( ALL_PIXELS, ALL_PIXELS );
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
  //spidrctrl.setCtprBits( 0 );
  int col;
  for( col=0; col<256; ++col )
    //if( (col >= 10 && col < 12) || (col >= 100 && col < 102) )
      spidrctrl.setCtprBit( col );

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
  if( !spidrctrl.datadrivenReadout() )
  //if( !spidrctrl.sequentialReadout( 127 ) )
    cout << "###xxxxReadout: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------

  // Configure the shutter trigger
  int trig_mode      = 4;      // SPIDR_TRIG_AUTO;
  int trig_period_us = 100000; // 100 ms
  int trig_freq_hz   = 3;      // 3 Hz
  //int nr_of_trigs    = 10;     // 10 triggers
  int nr_of_trigs    = 1;
  if( !spidrctrl.setTriggerConfig( trig_mode, trig_period_us,
                                   trig_freq_hz, nr_of_trigs ) )
    cout << "###setTriggerConfig: " << spidrctrl.errorString() << endl;

  // Interface to Timepix3 pixel data acquisition
  SpidrDaq spidrdaq( &spidrctrl );
  string errstr = spidrdaq.errorString();
  if( !errstr.empty() ) cout << "### SpidrDaq: " << errstr << endl;

  // Sample 'frames' as well as write pixel data to file
  spidrdaq.setSampling( true );
  spidrdaq.setSampleAll( true );
  //spidrdaq.openFile( "test.dat", true );

  // ----------------------------------------------------------
  // Get frames (data up to the next End-of-Readout packet)
  // and display some pixel data details

  // Start triggers
  if( !spidrctrl.startAutoTrigger() )
    cout << "###startAutoTrigger: " << spidrctrl.errorString() << endl;

  int   framecnt = 0;
  int   total_size = 0, total_pixcnt = 0;
  int   size, x, y, pixdata, timestamp;
  char *frame;
  bool  next_frame = true;
  while( next_frame )
    {
      next_frame = spidrdaq.getSample( 2*256*256*8, 3000 );
      //next_frame = spidrdaq.getFrame( 3000 );
      if( next_frame )
        {
          ++framecnt;
	  size  = spidrdaq.frameSize();
          frame = spidrdaq.frameData();
	  int pixcnt = 0;
          while( spidrdaq.nextPixel( &x, &y, &pixdata, &timestamp ) )
	    {
	      //if( pixcnt < 5 )
	      //cout << x << "," << y << ": " << hex << pixdata << dec << endl;
	      ++pixcnt;
	    }
	  total_pixcnt += pixcnt;
	  /*
	  unsigned long long pixel;
          while( (pixel = spidrdaq.nextPixel()) != 0 && pixcnt < 5 )
	    {
	      cout << pixcnt << ": " << hex << pixel << dec << endl;
	      ++pixcnt;
	    }
	  */
	  spidrdaq.freeSample();

	  total_size += size;
	  if( pixcnt > 0 )
	    cout << "Frame " << framecnt << " size=" << size << " (total="
		 << total_size << "): " << pixcnt <<" pixels" << endl;
        }
      else
        {
          cout << "### Timeout -> finish after " << framecnt
	       << " samples, " << total_pixcnt
	       << " pix, bytes r=" << spidrdaq.bytesReceivedCount()
	       << ", s=" << spidrdaq.bytesSampledCount()
	       << ", w=" << spidrdaq.bytesWrittenCount() << endl;
        }
    }

  if( !spidrctrl.pauseReadout() )
    cout << "###pauseReadout: " << spidrctrl.errorString() << endl;
  Sleep(100);

  // ----------------------------------------------------------
  return 0;
}
