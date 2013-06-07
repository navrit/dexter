#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"

int main( int argc, char *argv[] )
{
  int log_level = 1;

  SpidrController spidrcontrol( 192, 168, 1, 10 );

  // Check if we are properly connected to the SPIDR module
  if( spidrcontrol.isConnected() )
    {
      cout << "Connected to SPIDR: " << spidrcontrol.ipAddressString();
      cout <<  endl;
    }
  else
    {
      cout << spidrcontrol.connectionStateString() << ": "
	   << spidrcontrol.connectionErrString() << endl;
      return 1;
    }

  if( spidrcontrol.setLogLevel( log_level ) )
    {
      cout << "SPIDR loglevel set to " << log_level << endl;
    }
  else
    {
      cout << "### Failed to set SPIDR loglevel to " << log_level << endl;
    }

  return 0;
}
