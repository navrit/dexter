#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(1000*ms)
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
  int     device_nr = 0;

  // Check argument count
  if( !(argc == 2 || argc == 3) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port(argv[1], &portnr);

  if( argc == 3 )
    {
      bool ok;
      device_nr = QString(argv[2]).toUInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid device-number: " << string(argv[2]) << endl;
	  usage();
	  return 0;
	}
      else if( device_nr > 3 || device_nr < 0 )
	{
	  cout << "### Device-number out-of-range <0-3>" << endl;
	  return 0;
	}
    }

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

  //int errstat;
  //if( spidrctrl.reset( &errstat ) ) {
  //  cout << "errorstat " << hex << errstat << dec << endl;
  //}

  int i;
  unsigned char *ctpr;

  // Enable test-pulses for (some or all) columns
  int col;
  for( col=0; col<256; ++col )
    if( col >= 10 && col < 16 )
      //if( (col & 1) == 1 )
      spidrctrl.setCtprBit( col );

  ctpr = spidrctrl.ctpr();
  cout << hex;
  for( i=0; i<32; ++i )
    cout << (unsigned int) ctpr[i] << " ";
  cout << endl;

  if( !spidrctrl.setCtpr( device_nr ) )
    error_out( "###setCtpr" );

  ctpr = 0;
  if( spidrctrl.getCtpr( device_nr, &ctpr ) )
    {
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

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrtest-ctpr <ipaddr>[:<portnr>] [devnr]\n";
}

// ----------------------------------------------------------------------------
