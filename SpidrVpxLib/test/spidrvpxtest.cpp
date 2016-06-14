/* ----------------------------------------------------------------------------
File   : spidrvpxtest.cpp

Descr  : Some tests of the SpidrVpxLib library.

Usage  :
spidrvpxtest <ipaddr>[:<portnr>]
   Command a SPIDR module to execute a (soft) reset.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.
     <portnr> : current SPIDR controller IP port number, default 50000.

History:
14JUN2016; HenkB; Created.
---------------------------------------------------------------------------- */

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include "vpxdefs.h"
#include <QString>

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr_and_port( const char *str, int *portnr );
int     my_atoi( const char *c );
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;

  // Check argument count
  if( !(argc == 2) )
    {
      usage();
      return 1;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

  // Open a control connection to the SPIDR module
  // with the given address and port, or -if the latter was not provided-
  // the default port number 50000
  SpidrController spidrctrl( (ipaddr>>24) & 0xFF,
			     (ipaddr>>16) & 0xFF,
			     (ipaddr>> 8) & 0xFF,
			     (ipaddr>> 0) & 0xFF, portnr );

  // Are we connected ?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
         << spidrctrl.connectionStateString() << ", "
         << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  int errstat;
  if( spidrctrl.reset( &errstat ) )
    cout << "SPIDR-VPX reset, errstat " << hex << uppercase << errstat
	 << dec << endl;
  else
    error_out( "reset()" );

  // Read a Velopix register
  int val;
  if( spidrctrl.getVpxReg32( 0x0530, &val ) ) // Chip ID
    cout << "VPX reg 0x400: " << hex << val << dec << endl;
  else
    error_out( "getVpxReg()" );

  return 0;
}

// ----------------------------------------------------------------------------

int my_atoi( const char *c )
{
  int value = -1;
  while( *c >= '0' && *c <= '9' )
    {
      if( value == -1 ) value = 0;
      value *= 10;
      value += (int) (*c-'0');
      ++c;
    }
  return value;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout << "Usage:" << endl
       << "spidrvpxtest <ipaddr>[:<portnr>]"
       << endl
       << "   Test of the SpidrVpxLib library."
       << endl
       << "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10."
       << endl
       << "     <portnr> : current SPIDR controller IP port number, "
       << "default 50000."
       << endl;
}

// ----------------------------------------------------------------------------
