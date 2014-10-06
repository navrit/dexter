#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include "tpx3defs.h"

int main( int argc, char *argv[] )
{
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

  int  device_nr = 0;

  bool valid;
    if( !spidrctrl.validDacs( device_nr, &valid ) )
    cout << "###validDacs: " << spidrctrl.errorString() << endl;
  else
    cout << "validDacs: " << valid << endl;

  if( !spidrctrl.storeDacs( device_nr ) )
    cout << "###storeDacs: " << spidrctrl.errorString() << endl;
  else
    cout << "storeDacs" << endl;

  if( !spidrctrl.validDacs( device_nr, &valid ) )
    cout << "###validDacs: " << spidrctrl.errorString() << endl;
  else
    cout << "validDacs: " << valid << endl;

    if( !spidrctrl.eraseDacs( device_nr ) )
    cout << "###eraseDacs: " << spidrctrl.errorString() << endl;
  else
    cout << "eraseDacs" << endl;

  if( !spidrctrl.validDacs( device_nr, &valid ) )
    cout << "###validDacs: " << spidrctrl.errorString() << endl;
  else
    cout << "validDacs: " << valid << endl;

  return 0;
}
