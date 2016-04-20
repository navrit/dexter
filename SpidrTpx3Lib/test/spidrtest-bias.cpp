#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <iomanip>
using namespace std;

#include <QString>

#include "SpidrController.h"
#include "tpx3defs.h"

quint32 get_addr_and_port(const char *str, int *portnr);
void usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;

  // Check argument count
  if( !(argc == 2) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port(argv[1], &portnr);

  // ----------------------------------------------------------
  // Open a control connection to the SPIDR module
  // with the given address and port, or -if the latter was not provided-
  // the default port number 50000
  SpidrController spidrctrl((ipaddr >> 24) & 0xFF,
    (ipaddr >> 16) & 0xFF,
    (ipaddr >> 8) & 0xFF,
    (ipaddr >> 0) & 0xFF, portnr);

  // Are we connected ?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
      << spidrctrl.connectionStateString() << ", "
      << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  if( !spidrctrl.setBiasSupplyEna( true ) )
    cout << "###setBiasSupplyEna: " << spidrctrl.errorString() << endl;

  int volts;
  for( volts=0; volts<110; ++volts )
    {
      if( !spidrctrl.setBiasVoltage( volts ) )
	cout << "###setBiasVoltage " << volts << ": "
	     << spidrctrl.errorString() << endl;
      if( (volts % 10) == 0 ) cout << volts << endl;
      Sleep( 300 );
    }
  Sleep( 5000 );

  volts = 0;
  if( !spidrctrl.setBiasVoltage( volts ) )
    cout << "###setBiasVoltage " << volts << ": "
	 << spidrctrl.errorString() << endl;

  if( !spidrctrl.setBiasSupplyEna( false ) )
    cout << "###setBiasSupplyEna: " << spidrctrl.errorString() << endl;

  return 0;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrtest-bias <ipaddr>[:<portnr>]\n";
}

// ----------------------------------------------------------------------------
