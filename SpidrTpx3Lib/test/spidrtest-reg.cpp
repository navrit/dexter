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
  cout << hex;

  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    cout << "###getDevId: " << spidrctrl.errorString() << endl;
  else
    cout << "DeviceID=" << id << endl;

  config = (TPX3_POLARITY_EMIN |
	    TPX3_ACQMODE_TOA_TOT |
	    TPX3_GRAYCOUNT_ENA |
	    TPX3_TESTPULSE_ENA |
	    TPX3_FASTLO_ENA |
	    TPX3_SELECTTP_DIGITAL );
  if( !spidrctrl.setGenConfig( device_nr, config ) )
    cout << "###setGenCfg: " << spidrctrl.errorString() << endl;

  config = 0;
  if( !spidrctrl.getGenConfig( device_nr, &config ) )
    cout << "###getGenConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "GenConfig=" << config << endl;

  // --------------------

  int stat;
  spidrctrl.reset( &stat );
  cout << "reset: stat=" << stat << endl;

  if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
    cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "OutConfig=" << config << endl;

  config &= ~0xFC;
  //config = 0x03;
  //if( !spidrctrl.setOutBlockConfig( device_nr, config ) )
  if( !spidrctrl.setOutputMask( device_nr, config ) )
    cout << "###setOutputMask: " << spidrctrl.errorString() << endl;
  else
    cout << "OutputMask=" << config << endl;

  if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
    cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "OutConfig=" << config << endl;

  int ena_mask, lock_mask;
  if( !spidrctrl.getLinkStatus( device_nr, &ena_mask, &lock_mask ) )
    cout << "###getLinkStatus: " << spidrctrl.errorString() << endl;
  else
    cout << "linkstatus = " << ena_mask << ", " << lock_mask << endl;

  // --------------------

  if( !spidrctrl.getPllConfig( device_nr, &config ) )
    cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "PllConfig=" << config << endl;

  if( !spidrctrl.getSlvsConfig( device_nr, &config ) )
    cout << "###getSlvsConfig: " << spidrctrl.errorString() << endl;
  else
    cout << "SlvsConfig=" << config << endl;

  int options;
  if( !spidrctrl.getStartupOptions( &options ) )
    cout << "###getStartupOptions: " << spidrctrl.errorString() << endl;
  else
    cout << "getStartupOptions=" << options << endl;
  /*
  if( options != 0x7fff34fe )
    {
      options = 0x7fff34fe;
      if( !spidrctrl.storeStartupOptions( options ) )
        cout << "###setStartupOptions: " << spidrctrl.errorString() << endl;
      else
      cout << "setStartupOptions=" << options << endl;
    }
  */

  return 0;
}
