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

  int id;

  if( !spidrctrl.getDeviceId( 0, &id ) )
    cout << "###getDevId0: " << spidrctrl.errorString() << endl;
  else
    cout << "DeviceID0=" << hex << id << dec << endl;

  if( !spidrctrl.readEfuses( 0, &id ) )
    cout << "###readEfuses0: " << spidrctrl.errorString() << endl;
  else
    cout << "Efuse0=" << hex << id << dec << endl;

  //int prog_width = 1;
  //int selection  = 0;
  //if( !spidrctrl.burnEfuse( 0, prog_width, selection ) )
  //  cout << "###burnEfuse0: " << spidrctrl.errorString() << endl;
  //else
  //  cout << "burnEfuse0" << endl;

  if( !spidrctrl.readEfuses( 0, &id ) )
    cout << "###readEfuses0: " << spidrctrl.errorString() << endl;
  else
    cout << "Efuse0=" << hex << id << dec << endl;

  // -------------------------------------------

  if( !spidrctrl.getDeviceId( 1, &id ) )
    cout << "###getDevId1: " << spidrctrl.errorString() << endl;
  else
    cout << "DeviceID1=" << hex << id << dec << endl;

  if( !spidrctrl.readEfuses( 1, &id ) )
    cout << "###readEfuses1: " << spidrctrl.errorString() << endl;
  else
    cout << "Efuse1=" << hex << id << dec << endl;

  return 0;
}
