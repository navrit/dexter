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

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

// Test of pixel configuration functions
// ======================================

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

  int errstat;
  if( spidrctrl.reset( &errstat ) ) {
    cout << "errorstat " << hex << errstat << dec << endl;
  }

  int x, y, cnf;
  for( cnf=0; cnf<3; ++cnf )
    {
      spidrctrl.selectPixelConfig( cnf );
      for( y=10; y<250; ++y )
	for( x=0; x<255; ++x )
	  {
	    spidrctrl.setPixelTestEna( x, y );
	    if( x == 34 || x == 37 ) spidrctrl.setPixelThreshold( x, y, 5 );
	  }
    }
  spidrctrl.setPixelMask( 130, 131 );
  spidrctrl.setPixelMask( 213, 222 );
  cout << "0+1: " << spidrctrl.comparePixelConfig( 0, 1 ) << endl;
  cout << "1+2: " << spidrctrl.comparePixelConfig( 1, 2 ) << endl;
  cout << "2+3: " << spidrctrl.comparePixelConfig( 2, 3 ) << endl;

  spidrctrl.selectPixelConfig( 0 );
  spidrctrl.setPixelTestEna( 0, 0 );
  cout << "0+1: " << spidrctrl.comparePixelConfig( 0, 1 ) << endl;
  cout << "1+2: " << spidrctrl.comparePixelConfig( 1, 2 ) << endl;
  cout << "2+3: " << spidrctrl.comparePixelConfig( 2, 3 ) << endl;

  int device_nr = 0;
  spidrctrl.resetPixels( device_nr ); // Essential ! (or nothing can be read)

  // Upload pixel configuration 0
  spidrctrl.selectPixelConfig( 0 );
  if( !spidrctrl.setPixelConfig( device_nr ) )
    error_out( "###setPixelConfig" );

  // Download pixel configuration into config 1
  spidrctrl.selectPixelConfig( 1 );
  if( !spidrctrl.getPixelConfig( device_nr ) )
    error_out( "###getPixelConfig" );
  cout << "0+1: " << spidrctrl.comparePixelConfig( 0, 1 ) << endl;
  cout << "1+2: " << spidrctrl.comparePixelConfig( 1, 2 ) << endl;
  cout << "2+3: " << spidrctrl.comparePixelConfig( 2, 3 ) << endl;

  return 0;
}
