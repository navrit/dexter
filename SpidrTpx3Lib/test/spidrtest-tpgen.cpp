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
  SpidrController spidrctrl( 192, 168, 10, 10 );

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
    if( col >= 10 && col < 16 )
    //if( (col & 1) == 1 )
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
  int trig_mode      = SHUTTERMODE_AUTO;
  int trig_length_us = 10000;  // 10 ms
  int trig_delay_ns  = 10000;  // 10 us
  int trig_freq_hz   = 1;      // 1 Hz
  //int trig_freq_hz   = 3;    // 3 Hz
  int trig_count   = 10;     // 10 triggers
  //int trig_count     = 1;
  if( !spidrctrl.setShutterTriggerConfig( trig_mode, trig_length_us,
					  trig_freq_hz, trig_count,
					  trig_delay_ns ) )
    error_out( "###setShutterTriggerConfig" );

  // ----------------------------------------------------------

  // Open the shutter
  //if( !spidrctrl.openShutter() )
  //cout << "###openShutter: " << spidrctrl.errorString() << endl;
  // OR
  // Start triggers
  if( !spidrctrl.startAutoTrigger() )
    error_out( "###startAutoTrigger" );

  cout << "Type any to continue..." << endl;
  char ch;
  cin >> ch;

  // Close the shutter
  if( !spidrctrl.closeShutter() )
    error_out( "###closeShutter" );

  // ----------------------------------------------------------
  int cntr;
  spidrctrl.getDataPacketCounter( &cntr );
  cout << "SPIDR packet cntr   : " << cntr << endl;

  return 0;
}
