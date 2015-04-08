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
  // with address 192.168.1(00).10, default port number 50000
  SpidrController spidrctrl( 192, 168, 100, 10 );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
	 << spidrctrl.connectionStateString() << ", "
	 << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  int device_nr = 0, id;
  cout << hex;

  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    cout << "###getDevId: " << spidrctrl.errorString() << endl;
  else
    cout << "DeviceID=" << id << endl;

  // --------------------

  //if( !spidrctrl.setTpxPowerEna( true ) )
  //  cout << "###setTpxPowerEna: " << spidrctrl.errorString() << endl;

  int stat, config;
  spidrctrl.reset( &stat, -1 );
  cout << "reset: stat=" << stat << endl;

  if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
    cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "OutConfig=" << config << endl;

  if( !spidrctrl.getPllConfig( device_nr, &config ) )
    cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "PllConfig=" << config << endl;

  int i, s, speed[] = { 640, 80, 160, 320, 40, 40 };
  int ena_mask, lock_mask, output_mask = 0xC0;
  for( i=0; i<sizeof(speed)/sizeof(int); ++i )
    {
      cout << endl;

      if( spidrctrl.setReadoutSpeed( device_nr, speed[i] ) )
	cout << "Speed=> " << dec << speed[i] << hex << endl;
      else
	cout << "###setReadoutSpeed " << dec << speed[i] << hex << ": "
	     << spidrctrl.errorString() << endl;

      if( spidrctrl.getReadoutSpeed( device_nr, &s ) )
	cout << "Get speed: " << dec << s << hex << endl;
      else
	cout << "###getReadoutSpeed " << dec << speed[i] << hex << ": "
	     << spidrctrl.errorString() << endl;

      if( !spidrctrl.getLinkStatus( device_nr, &ena_mask, &lock_mask ) )
	cout << "###getLinkStatus: " << spidrctrl.errorString() << endl;
      else
	cout << "linkstatus = " << ena_mask << ", " << lock_mask << endl;

      if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
	cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "OutConfig=" << config << endl;

      if( !spidrctrl.getPllConfig( device_nr, &config ) )
	cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "PllConfig=" << config << endl;

      output_mask |= (1 << i);
      if( !spidrctrl.setOutputMask( device_nr, output_mask ) )
	cout << "###setOutputMask: " << spidrctrl.errorString() << endl;
      else
	cout << "OutputMask=> " << output_mask << endl;

      Sleep( 2000 );
    }

  return 0;
}
