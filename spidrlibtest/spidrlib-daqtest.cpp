#include <iostream>
using namespace std;

#include "SpidrControl.h"
#include "SpidrDaq.h"

int main( int argc, char *argv[] )
{
  // Open a control connection to SPIDR module with address 192.168.1.10, port 50000 (default)
  SpidrControl spidr( 192, 168, 1, 10 );

  // Check if we are properly connected to the SPIDR module
  if( !spidr.isConnected() )
    {
      cout << spidr.connectionStateString() << ": "
           << spidr.connectionErrString() << endl;
      return 1;
    }

  // Start frame-acquisition on interface 192.168.1.1, port 8192 (default)
  SpidrDaq spidrdaq( 192, 168, 1, 1 );

  spidrdaq.openFile( "test.dat" ); // To be implemented...

  // Configure the trigger, then generate some triggers
  spidr.configTrigger( 4, 100000, 3, 2 );
  for( int i=0; i<10; ++i )
    {
      spidr.startAutoTrigger();
      Sleep( 2000 );
    }

  spidrdaq.closeFile(); // To be implemented...

  cout << "DAQ frames: " << spidrdaq.framesCount() << ", "
       << spidrdaq.framesLostCount() << ", "
       << spidrdaq.packetsLostCount() << endl;

  spidrdaq.stop();

  return 0;
}
