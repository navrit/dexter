#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(ms*1000)
#endif
#include <iostream>
using namespace std;

#include "SpidrController.h"
#include "SpidrDaq.h"

int main( int argc, char *argv[] )
{
  // Open a control connection to SPIDR module with address 192.168.1.10,
  // port 50000 (default)
  SpidrController spidrctrl( 192, 168, 1, 10 );

  // Check the connection to the SPIDR module
  if( !spidrctrl.isConnected() )
    {
      // No ?
      cout << spidrctrl.connectionStateString() << ": "
           << spidrctrl.connectionErrString() << endl;
      return 1;
    }

  // Start frame-acquisition
  SpidrDaq spidrdaq( &spidrctrl );
  // On interface 192.168.1.1, port 8192 (default)
  //SpidrDaq spidrdaq( 0 );
  //SpidrDaq spidrdaq( 192, 168, 1, 1 );

  bool overwrite = true;
  spidrdaq.openFile( "test.dat", overwrite );

  // Configure the trigger, then generate some triggers
  int trig_mode      = 4;
  int trig_period_us = 100000; // 100 ms
  int trig_freq_hz   = 3;
  int nr_of_triggers = 2;
  spidrctrl.setTriggerConfig( trig_mode, trig_period_us,
			      trig_freq_hz, nr_of_triggers );
  for( int i=0; i<5; ++i )
    {
      spidrctrl.startAutoTrigger();
      Sleep( 2000 );
      cout << i << endl;
    }

  spidrdaq.closeFile();

  cout << "DAQ frames: " << spidrdaq.framesCount() << ", lost frames "
       << spidrdaq.framesLostCount() << ", lost packets "
       << spidrdaq.packetsLostCount() << endl;

  // Must call stop() to terminate threads cleanly
  spidrdaq.stop();

  return 0;
}
