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

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

//int main( int argc, char *argv[] )
int main()
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

  int errstat;
  if( spidrctrl.reset( &errstat ) ) {
    cout << "errorstat " << hex << errstat << dec << endl;
  }

  int device_nr = 0;

  // ----------------------------------------------------------
  // DACs configuration

  if( !spidrctrl.setDacsDflt( device_nr ) )
    error_out( "###setDacsDflt" );
  // The following setting is necessary (in combination with
  // the setGenConfig() setting EMIN or HPLUS) in order to prevent
  // noisy pixels also generating pixel data!
  if( !spidrctrl.setDac( device_nr, TPX3_VTHRESH_COARSE, 9 ) )
    error_out( "###setDac" );

  // ----------------------------------------------------------
  // Pixel configuration

  if( !spidrctrl.resetPixels( device_nr ) )
    error_out( "###resetPixels" );

  // Enable test-bit in pixels
  spidrctrl.resetPixelConfig();
  spidrctrl.setPixelTestEna();
  if( !spidrctrl.setPixelConfig( device_nr ) )
    error_out( "###setPixelConfig" );

  // ----------------------------------------------------------
  // Test pulse and CTPR configuration

  // Timepix3 test pulse configuration
  if( !spidrctrl.setTpPeriodPhase( device_nr, 10, 0 ) )
    error_out( "###setTpPeriodPhase" );

  if( !spidrctrl.setTpNumber( device_nr, 1 ) )
    error_out( "###setTpNumber" );

  // Enable test-pulses for (some or all) columns
  int col;
  for( col=0; col<256; ++col )
    if( !((col >= 10 && col < 12) || (col >= 100 && col < 102)) )
    spidrctrl.setCtprBit( col );

  if( !spidrctrl.setCtpr( device_nr ) )
    error_out( "###setCtpr" );

  // ----------------------------------------------------------

  // SPIDR-TPX3 and Timepix3 timers
  if( !spidrctrl.restartTimers() )
    error_out( "###restartTimers" );

  // Set Timepix3 acquisition mode: ToA-ToT, test-pulses, digital-in
  if( !spidrctrl.setGenConfig( device_nr,
                               TPX3_POLARITY_EMIN |
                               TPX3_ACQMODE_TOA_TOT |
                               TPX3_GRAYCOUNT_ENA |
                               TPX3_TESTPULSE_ENA |
                               TPX3_FASTLO_ENA |
                               TPX3_SELECTTP_DIGITAL ) )
    error_out( "###setGenCfg" );

  // Set Timepix3 into acquisition mode
  //if( !spidrctrl.sequentialReadout() )
  if( !spidrctrl.datadrivenReadout() )
    error_out( "###xxxxReadout" );

  // ----------------------------------------------------------

  // Configure the shutter trigger
  int trig_mode      = 4;      // SPIDR_TRIG_AUTO;
  int trig_length_us = 10000;  // 10 ms
  int trig_freq_hz   = 3;      // 3 Hz
  //int trig_count   = 10;     // 10 triggers
  int trig_count     = 1;
  if( !spidrctrl.setShutterTriggerConfig( trig_mode, trig_length_us,
					  trig_freq_hz, trig_count ) )
    error_out( "###setShutterTriggerConfig" );

  // ----------------------------------------------------------

  // Interface to Timepix3 pixel data acquisition
  SpidrDaq spidrdaq( &spidrctrl );
  string errstr = spidrdaq.errorString();
  if( !errstr.empty() ) cout << "###SpidrDaq: " << errstr << endl;
  cout << "bufsize=" << spidrdaq.bufferSize() << endl;
  /*
  spidrdaq.setBufferSize( 0x080000000 );
  errstr = spidrdaq.errorString();
  if( !errstr.empty() ) cout << "###SpidrDaq: " << errstr << endl;
  cout << "bufsize=" << spidrdaq.bufferSize() << endl;
  spidrdaq.setBufferSize( 0x010000000 );
  errstr = spidrdaq.errorString();
  if( !errstr.empty() ) cout << "###SpidrDaq: " << errstr << endl;
  cout << "bufsize=" << spidrdaq.bufferSize() << endl;
  */
  // Sample 'frames' as well as write pixel data to file
  spidrdaq.setSampling( true );
  spidrdaq.setSampleAll( true );
  if( !spidrdaq.startRecording( "test/test.dt", 123, "Eerste testjes" ) )
    cout << "###SpidrDaq.startRecording: " << spidrdaq.errorString() << endl;

  cout << "Filename: " << spidrdaq.fileName() << endl;

  // ----------------------------------------------------------
  // Get frames (data up to the next End-of-Readout packet)
  // and display some pixel data details

  // Start triggers
  if( !spidrctrl.startAutoTrigger() )
    error_out( "###startAutoTrigger" );

  int   framecnt = 0;
  int   total_size = 0, total_pixcnt = 0;
  int   size, x, y, pixdata, timestamp;
  char *frame;
  bool  next_frame = true;
  bool  pix[256][256];
  for( x=0; x<256; ++x )
    for( y=0; y<256; ++y )
      pix[x][y] = false;
  while( next_frame )
    {
      next_frame = spidrdaq.getSampleMin( 256*8, 2*256*256*8, 1000 );
      //next_frame = spidrdaq.getSampleMin( 256*256*8, 2*256*256*8, 1000 );
      //next_frame = spidrdaq.getFrame( 3000 );
      if( next_frame )
        {
          ++framecnt;
	  size  = spidrdaq.sampleSize();
          frame = spidrdaq.sampleData();
	  int pixcnt = 0;
          while( spidrdaq.nextPixel( &x, &y, &pixdata, &timestamp ) )
	    {
	      if( pixcnt < 5 )
	       cout << x << "," << y << ": " << hex << pixdata << dec << endl;
	      pix[x][y] = true;
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

	  total_size += size;
	  if( pixcnt > 0 )
	    cout << "Sample " << framecnt << " size=" << size << " (total="
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

  if( total_pixcnt > 65400 && total_pixcnt <= 65536 )
    {
      // Display absent pixels
      int i = 0;
      for( x=0; x<256; ++x )
	{
	  if( i > 0 ) cout << endl;
	  i = 0;
	  for( y=0; y<256; ++y )
	    if( pix[x][y] == false )
	      {
		cout << '(' << x << ',' << y << ')';
		++i;
	      }
	}
    }

  // ----------------------------------------------------------
  int cntr;
  spidrctrl.getDataPacketCounter( &cntr );
  cout << "SPIDR packet cntr   : " << cntr << endl;
  cout << "SpidrDaq packet cntr: " << spidrdaq.packetsReceivedCount() << endl;

  //if( !spidrctrl.pauseReadout() )
  //  error_out( "###pauseReadout" );
  //Sleep(100);

  // Revert changes made due to sequentialReadout()
  //if( !spidrctrl.setHeaderFilter( device_nr, 0x0C00, 0xF3FF ) )
  //  error_out( "###setHeaderFilter" );

  return 0;
}
