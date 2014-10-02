#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(1000*ms)
#endif
#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

//int main( int argc, char *argv[] )
int main()
{
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

  int device_nr = 0;

  // Enable test-pulses for (some or all) columns
  int col;
  for( col=0; col<256; ++col )
    if( col >= 10 && col < 16 )
      //if( (col & 1) == 1 )
      spidrctrl.setCtprBit( col );

  if( !spidrctrl.setCtpr( device_nr ) )
    error_out( "###setCtpr" );

  unsigned char *ctpr;
  if( spidrctrl.getCtpr( device_nr, &ctpr ) )
    {
      int i;
      cout << hex;
      for( i=0; i<32; ++i )
	cout << (unsigned int) ctpr[i] << " ";
      cout << endl;
    }
  else
    {
      error_out( "###getCtpr" );
    }

  return 0;
}
