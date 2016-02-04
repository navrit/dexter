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
  SpidrController spidrctrl( 192, 168, 1, 10 );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
	 << spidrctrl.connectionStateString() << ", "
	 << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  int stat;
  if( !spidrctrl.reset( &stat ) )
    {
      cout << "###reset: :" << spidrctrl.errorString() << endl;
      return 1;
    }
  else
    {
      cout << "reset: stat=" << stat << endl;
    }
  cout << endl;

  int device_cnt, device_nr, id, config;

  if( !spidrctrl.getDeviceCount( &device_cnt ) )
    cout << "###getDeviceCount: " << spidrctrl.errorString() << endl;
  else
    cout << "DeviceCount=" << device_cnt << endl << endl;

  cout << hex;

  for( device_nr=0; device_nr<device_cnt; ++device_nr )
    {
      cout << "DEVICE #" << device_nr << endl;
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

      int speed;
      if( spidrctrl.getReadoutSpeed( device_nr, &speed ) )
	cout << "Link speed: " << dec << speed << hex << endl;
      else
	cout << "###getReadoutSpeed: " << spidrctrl.errorString() << endl;

      if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
	cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "OutConfig=" << config << endl;

      /*
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
      */

      int ena_mask, lock_mask;
      if( !spidrctrl.getLinkStatus( device_nr, &ena_mask, &lock_mask ) )
	cout << "###getLinkStatus: " << spidrctrl.errorString() << endl;
      else
	cout << "linkstatus = " << ena_mask << ", " << lock_mask << endl;

      // --------------------

      if( device_nr == 0 || device_nr == 2 )
	config = 0x11E;
      else
	config = 0x11E;
      if( !spidrctrl.setPllConfig( device_nr, config ) )
	cout << "###setPllConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "setPllConfig " << config << endl;

      if( !spidrctrl.getPllConfig( device_nr, &config ) )
	cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "PllConfig=" << config << endl;

      // --------------------
      /*
      config = 160;
      if( !spidrctrl.setReadoutSpeed( device_nr, config ) )
	cout << "###setSpeed: " << spidrctrl.errorString() << endl;
      else
	cout << "setSpeed " << config << endl;

      if( !spidrctrl.getReadoutSpeed( device_nr, &config ) )
	cout << "###getSpeed: " << spidrctrl.errorString() << endl;
      else
	cout << "ReadoutSpeed=" << config << endl;

      if( !spidrctrl.getPllConfig( device_nr, &config ) )
	cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "PllConfig=" << config << endl;
      */
      // --------------------

      if( !spidrctrl.getSlvsConfig( device_nr, &config ) )
	cout << "###getSlvsConfig: " << spidrctrl.errorString() << endl;
      else
	cout << "SlvsConfig=" << config << endl;

      cout << endl;
    }

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
