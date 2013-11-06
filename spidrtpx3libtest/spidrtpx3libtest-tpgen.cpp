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

  //if( !spidrctrl.setDacsDflt( device_nr ) )
  //  cout << "###setDacsDflt: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------
  // Pixel configuration

  if( !spidrctrl.resetPixels( device_nr ) )
    cout << "###resetPixels: " << spidrctrl.errorString() << endl;

  spidrctrl.resetPixelConfig();
  spidrctrl.setPixelTestEna( ALL_PIXELS, ALL_PIXELS );
  if( !spidrctrl.setPixelConfig( device_nr ) )
    cout << "###setPixelConfig: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------
  // Test pulse and CTPR configuration

  // Timepix3 test pulse configuration
  if( !spidrctrl.setTpPeriodPhase( device_nr, 10, 0 ) )
    cout << "###setTpPeriodPhase: " << spidrctrl.errorString() << endl;
  //if( !spidrctrl.setTpNumber( device_nr, 50000 ) )
  if( !spidrctrl.setTpNumber( device_nr, 1 ) )
    cout << "###setTpNumber: " << spidrctrl.errorString() << endl;

  // Enable test-pulses for (some or all) columns
  int col;
  for( col=0; col<256; ++col )
    //if( col >= 10 && col < 16 )
    if( (col & 1) == 1 )
      spidrctrl.setCtprBit( col );

  if( !spidrctrl.setCtpr( device_nr ) )
    cout << "###setCtpr: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------

  // Configure the shutter trigger
  int trig_mode      = 4;      // SPIDR_TRIG_AUTO;
  int trig_period_us = 500000; // 500 ms
  int trig_freq_hz   = 1;      // Hz
  //int nr_of_trigs    = 200;
  int nr_of_trigs    = 1;
  if( !spidrctrl.setTriggerConfig( trig_mode, trig_period_us,
                                   trig_freq_hz, nr_of_trigs ) )
    cout << "###setTriggerConfig: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------

  // SPIDR-TPX3 and Timepix3 timers
  if( !spidrctrl.restartTimers() )
    cout << "###restartTimers: " << spidrctrl.errorString() << endl;

  /*
  // Set Timepix3 acquisition mode: ToA-ToT
  if( !spidrctrl.setGenConfig( device_nr,
                               TPX3_POLARITY_HPLUS |
                               TPX3_ACQMODE_TOA_TOT |
                               TPX3_GRAYCOUNT_ENA |
                               TPX3_FASTLO_ENA ) )
  */
  // Set Timepix3 acquisition mode: ToA-ToT, test-pulses, digital-in
  if( !spidrctrl.setGenConfig( device_nr,
                               TPX3_POLARITY_HPLUS |
                               TPX3_ACQMODE_TOA_TOT |
                               TPX3_GRAYCOUNT_ENA |
                               TPX3_TESTPULSE_ENA |
                               TPX3_FASTLO_ENA |
			       TPX3_SELECTTP_DIG_ANALOG ) )
    cout << "###setGenCfg: " << spidrctrl.errorString() << endl;

  // Set Timepix3 into acquisition mode
  //if( !spidrctrl.sequentialReadout() )
  if( !spidrctrl.datadrivenReadout() )
    cout << "###ddrivenReadout: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------

  // Open the shutter
  //if( !spidrctrl.openShutter() )
  //cout << "###openShutter: " << spidrctrl.errorString() << endl;
  // OR
  // Start triggers
  if( !spidrctrl.startAutoTrigger() )
    cout << "###startAutoTrigger: " << spidrctrl.errorString() << endl;

  cout << "Type any to continue..." << endl;
  char ch;
  cin >> ch;

  // Close the shutter
  if( !spidrctrl.closeShutter() )
    cout << "###closeShutter: " << spidrctrl.errorString() << endl;

  // ----------------------------------------------------------
  return 0;
}
