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

  int device_nr = 0, id, config;

  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    cout << "###getDevId: " << spidrctrl.errorString() << endl;
  else
    cout << "DeviceID=" << id << endl;

  if( !spidrctrl.setGenConfig( device_nr,
			       TPX3_POLARITY_HPLUS |
			       TPX3_ACQMODE_TOA_TOT |
			       TPX3_GRAYCOUNT_ENA |
			       TPX3_TESTPULSE_ENA |
			       TPX3_FASTLO_ENA |
			       TPX3_SELECTTP_DIGITAL ) )
    cout << "###setGenCfg: " << spidrctrl.errorString() << endl;

  if( !spidrctrl.getGenConfig( device_nr, &config ) )
    cout << "###getGenConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "GenConfig=" << hex << config << endl;

  if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
    cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "OutConfig=" << hex << config << endl;

  if( !spidrctrl.getPllConfig( device_nr, &config ) )
    cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "PllConfig=" << hex << config << endl;

  if( !spidrctrl.getSlvsConfig( device_nr, &config ) )
    cout << "###getSlvsConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "SlvsConfig=" << hex << config << endl;

  return 0;
}
