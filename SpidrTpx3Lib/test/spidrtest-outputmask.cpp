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

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr_and_port(const char *str, int *portnr);
void usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;

  // Check argument count
  if( argc < 2 )
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

  int device_nr = 0, id;
  cout << hex;

  if( !spidrctrl.getDeviceId( device_nr, &id ) )
    error_out( "###getDeviceId" );
  else
    cout << "DeviceID=" << id << endl;

  // --------------------

  //if( !spidrctrl.setTpxPowerEna( true ) )
  //  error_out( "###setTpxPowerEna" );

  int speed;
  if( spidrctrl.getReadoutSpeed( device_nr, &speed ) )
    cout << "Link speed (before reset): " << dec << speed << hex << endl;
  else
    error_out( "###getReadoutSpeed" );

  int stat;
  //spidrctrl.reset( &stat );     // Default readout speed
  if( argc > 2 )
    spidrctrl.reset( &stat, -1 ); // Force low-speed readout
  else
    spidrctrl.reset( &stat,  1 ); // Force high-speed readout
  cout << "reset: stat=" << stat << endl;

  if( spidrctrl.getReadoutSpeed( device_nr, &speed ) )
    cout << "Link speed: " << dec << speed << hex << endl;
  else
    error_out( "###getReadoutSpeed" );

  int config;
  if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
    error_out( "###getOutBlockConfig" );
  else
    cout << "OutConfig=" << config << endl;

  if( !spidrctrl.getPllConfig( device_nr, &config ) )
    error_out( "###getPllConfig" );
  else
    cout << "PllConfig=" << config << endl;

  int linkcount;
  if( spidrctrl.getLinkCount( &linkcount ) )
    cout << "Link count: " << dec << linkcount << hex << endl;
  else
    error_out( "###getLinkCount" );

  int i;
  int ena_mask, lock_mask, output_mask = 0x00, dac;
  for( i=0; i<linkcount*2; ++i )
    {
      cout << endl;
	
      //output_mask = (1 << (i%linkcount)); // 1 link at a time
      if( i >= linkcount )
	{
	  if( i == linkcount ) output_mask = 0x00;
	  output_mask |= (1 << (i-linkcount)); // Additional link each time
	}
      else
	{
	  output_mask = (1 << i); // 1 link at a time
	}
      if( speed == 640 ) output_mask |= 1;
      if( !spidrctrl.setOutputMask(device_nr, output_mask) )
	error_out( "###setOutputMask" );
      else
	cout << "OutputMask=> " << output_mask << endl;

      if( !spidrctrl.getLinkStatus( device_nr, &ena_mask, &lock_mask ) )
	error_out( "###getLinkStatus" );
      else
	cout << "LinkStatus = " << ena_mask << ", " << lock_mask << endl;

      for( int k=0; k<2; ++k )
	{
	  if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
	    cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
	  else
	    cout << "OutConfig=" << config << endl;

	  if( !spidrctrl.getPllConfig( device_nr, &config ) )
	    cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
	  else
	    cout << "PllConfig=" << config << endl;

	  if( !spidrctrl.getDac( device_nr, i+1, &dac ) )
	    cout << "###getDac: " << spidrctrl.errorString() << endl;
	  else
	    cout << "DAC = " << dac << endl;
	}

      if( i == 9 )
	{
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

	  if( !spidrctrl.getOutBlockConfig( device_nr, &config ) )
	    cout << "###getOutBlockConfig: " << spidrctrl.errorString() << endl;
	  else
	    cout << "OutConfig=" << config << endl;

	  if( !spidrctrl.getPllConfig( device_nr, &config ) )
	    cout << "###getPllConfig: " << spidrctrl.errorString() << endl;
	  else
	    cout << "PllConfig=" << config << endl;
	}
      Sleep( 800 );
    }

  return 0;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrtest-outputmask <ipaddr>[:<portnr>]\n";
}

// ----------------------------------------------------------------------------
