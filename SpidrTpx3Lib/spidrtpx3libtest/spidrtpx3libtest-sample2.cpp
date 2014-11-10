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

  int devnr;
  char ch;

  // ----------------------------------------------------------

  for( devnr=0; devnr<2; ++devnr )
    {
      // DACs configuration
      if( !spidrctrl.setDacsDflt( devnr ) )
	error_out( "###setDacsDflt" );
      // The following setting is necessary (in combination with
      // the setGenConfig() setting EMIN or HPLUS) in order to prevent
      // noisy pixels also generating pixel data!
      if( !spidrctrl.setDac( devnr, TPX3_VTHRESH_COARSE, 9 ) )
	error_out( "###setDac" );

      // ----------------------------------------------------------
      // Pixel configuration

      if( !spidrctrl.resetPixels( devnr ) )
	error_out( "###resetPixels" );

      // Enable test-bit in pixels
      spidrctrl.resetPixelConfig();
      spidrctrl.setPixelTestEna();
      /*
	int i;
	for( i=0; i<250; ++i )
	{
	spidrctrl.setPixelTestEna( 0, i );
	spidrctrl.setPixelTestEna( 1, i );
	}
      */
      if( !spidrctrl.setPixelConfig( devnr ) )
	error_out( "###setPixelConfig" );

      // ----------------------------------------------------------
      // Test pulse and CTPR configuration

      // Timepix3 test pulse configuration
      if( !spidrctrl.setTpPeriodPhase( devnr, 10, 0 ) )
	error_out( "###setTpPeriodPhase" );

      if( !spidrctrl.setTpNumber( devnr, 1 ) )
	error_out( "###setTpNumber" );

      // Enable test-pulses for (some or all) columns
      //spidrctrl.setCtprBits( 0 );
      int col;
      for( col=0; col<256; ++col )
	if( !((col >= 10 && col < 12) || (col >= 100 && col < 102)) )
	  spidrctrl.setCtprBit( col );

      if( !spidrctrl.setCtpr( devnr ) )
	error_out( "###setCtpr" );

      // ----------------------------------------------------------

      // Set Timepix3 acquisition mode
      if( !spidrctrl.setGenConfig( devnr,
				   TPX3_POLARITY_EMIN |
				   TPX3_ACQMODE_TOA_TOT |
				   TPX3_GRAYCOUNT_ENA |
				   TPX3_TESTPULSE_ENA |
				   TPX3_FASTLO_ENA |
				   TPX3_SELECTTP_DIGITAL ) )
	error_out( "###setGenConfig" );
      else
	cout << "setGenConfig" << endl;

      cout << "Type any to continue.." << endl;
      cin >> ch;
    }

  // ----------------------------------------------------------

  // Configure the shutter trigger
  int trig_mode      = SHUTTERMODE_AUTO;
  int trig_length_us = 10000;  // 10 ms
  int trig_freq_hz   = 3;      // 3 Hz
  //int trig_count   = 10;     // 10 triggers
  int trig_count     = 1;
  if( !spidrctrl.setShutterTriggerConfig( trig_mode, trig_length_us,
					  trig_freq_hz, trig_count ) )
    error_out( "###setShutterTriggerConfig" );
  else
    cout << "setShutterTriggerConfig" << endl;

  // ----------------------------------------------------------

  // Interface to Timepix3 pixel data acquisition
  SpidrDaq *spidrdaq[2];
  spidrdaq[0] = new SpidrDaq( &spidrctrl, 0x10000000, 0 );
  spidrdaq[1] = new SpidrDaq( &spidrctrl, 0x10000000, 1 );

  for( devnr=0; devnr<2; ++devnr )
    {
      string errstr = spidrdaq[devnr]->errorString();
      if( !errstr.empty() ) cout << "###SpidrDaq: " << errstr << endl;
      cout << "bufsize=" << spidrdaq[devnr]->bufferSize() << endl;
      
      // Sample 'frames' as well as write pixel data to file
      //spidrdaq[devnr]->setSampling( true );
      //spidrdaq[devnr]->setSampleAll( true );
      ostringstream oss;
      oss << "test/data" << devnr; // To distinguish between 2 devices!
      if( !spidrdaq[devnr]->startRecording( oss.str(),
					    123, "Eerste testjes" ) )
	cout << "###SpidrDaq.startRecording: "
	     << spidrdaq[devnr]->errorString() << endl;

      cout << "Filename dev " << devnr << ": "
	   << spidrdaq[devnr]->fileName() << endl;

      cout << "Type any to continue.." << endl;
      cin >> ch;
    }

  // ----------------------------------------------------------

  // SPIDR-TPX3 and Timepix3 timers
  if( !spidrctrl.restartTimers() )
    error_out( "###restartTimers" );
  else
    cout << "restartTimers" << endl;

  // Set Timepix3 into acquisition mode
  if( !spidrctrl.datadrivenReadout() )
  //if( !spidrctrl.sequentialReadout( 1 ) )
    error_out( "###xxxxReadout" );
  else
    cout << "seqReadout" << endl;

  // ----------------------------------------------------------
  // Get frames (data up to the next End-of-Readout packet)
  // and display some pixel data details

  // Start triggers
  if( !spidrctrl.startAutoTrigger() )
    error_out( "###startAutoTrigger" );

  cout << "Type any to continue.." << endl;
  cin >> ch;

  int cntr;
  spidrctrl.getDataPacketCounter( &cntr );
  cout << "SPIDR packet cntr   : " << cntr << endl;
  for( devnr=0; devnr<2; ++devnr )
    {
      cout << "SpidrDaq " << devnr << " packet cntr: "
	   << spidrdaq[devnr]->packetsReceivedCount() << endl;
    }

  //if( !spidrctrl.pauseReadout() )
  //  error_out( "###pauseReadout" );
  //Sleep(100);

  // Revert changes made due to sequentialReadout()
  for( devnr=0; devnr<2; ++devnr )
    {
      if( !spidrctrl.setHeaderFilter( devnr, 0x0C00, 0xF3FF ) )
	error_out( "###setHeaderFilter" );
    }
  // ----------------------------------------------------------
  return 0;
}
