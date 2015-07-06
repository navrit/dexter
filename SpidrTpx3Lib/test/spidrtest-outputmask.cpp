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

  int speed;
  if( spidrctrl.getReadoutSpeed( device_nr, &speed ) )
    cout << "Link speed (before reset): " << dec << speed << hex << endl;
  else
    cout << "###getReadoutSpeed: " << spidrctrl.errorString() << endl;

  int stat;
  spidrctrl.reset( &stat, -1 ); // Force low-speed readout
  //spidrctrl.reset( &stat );     // Default readout speed
  //spidrctrl.reset( &stat,  1 );   // Force high-speed readout
  cout << "reset: stat=" << stat << endl;

  speed = 640;
  if( spidrctrl.setReadoutSpeed( device_nr, speed ) )
    cout << "Speed=> " << dec << speed << hex << endl;
  else
    cout << "###setReadoutSpeed " << dec << speed << hex << ": "
	 << spidrctrl.errorString() << endl;

  if( spidrctrl.getReadoutSpeed( device_nr, &speed ) )
    cout << "Link speed: " << dec << speed << hex << endl;
  else
    cout << "###getReadoutSpeed: " << spidrctrl.errorString() << endl;

  int config;
  if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
    cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "OutConfig=" << config << endl;

  if( !spidrctrl.getPllConfig( device_nr, &config ) )
    cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "PllConfig=" << config << endl;

  int linkcount;
  if( spidrctrl.getLinkCount( &linkcount ) )
    cout << "Link count: " << dec << linkcount << hex << endl;
  else
    cout << "###getLinkCount: " << spidrctrl.errorString() << endl;

  int i;
  int ena_mask, lock_mask, output_mask = 0x00;
  for( i=0; i<linkcount*2; ++i )
    {
      cout << endl;

      if( i >= linkcount )
	{
	  if( i == linkcount ) output_mask = 0x00;
	  output_mask |= (1 << (i-linkcount)); // Additional link each time
	}
      else
	{
	  output_mask = (1 << i); // 1 link at a time
	}
      if( !spidrctrl.setOutputMask( device_nr, output_mask ) )
	cout << "###setOutputMask: " << spidrctrl.errorString() << endl;
      else
	cout << "OutputMask=> " << output_mask << endl;

      if( !spidrctrl.getLinkStatus( device_nr, &ena_mask, &lock_mask ) )
	cout << "###getLinkStatus: " << spidrctrl.errorString() << endl;
      else
	cout << "LinkStatus = " << ena_mask << ", " << lock_mask << endl;

      if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
	cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "OutConfig=" << config << endl;

      if( !spidrctrl.getPllConfig( device_nr, &config ) )
	cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "PllConfig=" << config << endl;

      Sleep( 800 );
    }

  return 0;
}
