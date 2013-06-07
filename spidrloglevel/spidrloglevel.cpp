#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"

int my_atoi( const char *c );

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  int log_level = 0;

  if( argc > 1 ) log_level = my_atoi( argv[1] );

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

// ----------------------------------------------------------------------------

int my_atoi( const char *c )
{
  int value = 0;
  while( *c >= '0' && *c <= '9' )
    {
      value *= 10;
      value += (int) (*c-'0');
      ++c;
    }
  return value;
}

// ----------------------------------------------------------------------------
