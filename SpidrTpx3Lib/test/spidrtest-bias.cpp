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

  int device_nr = 0, id;

  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    cout << "###getDevId: " << spidrctrl.errorString() << endl;
  else
    cout << "DeviceID=" << id << endl;

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
