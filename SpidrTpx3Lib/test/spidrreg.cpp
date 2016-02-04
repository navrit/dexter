/* ----------------------------------------------------------------------------
File   : spidrreg.cpp

Descr  : Commandline tool to read and write SPIDR module registers.

Usage  :
spidrreg <ipaddr>[:<portnr>] <addr> [<value>] 
   Read or write the register with the given address.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.
     <portnr> : current SPIDR controller IP port number, default 50000.
     <addr>   : SPIDR register address (hex, minus offset of 0x800A0000).
     <value>  : if provided, value to write to register.

History:
04FEB2016; HenkB; Created.
---------------------------------------------------------------------------- */

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include <QString>

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr_and_port( const char *str, int *portnr );
void usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;
  int     addr, regval;
  bool    write_reg = false;

  // Check argument count
  if( !(argc == 3 || argc == 4) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

  bool ok;
  if( argc >= 3 )
    {
      addr = QString(argv[2]).toUInt( &ok, 16 );
      if( !ok )
	{
	  cout << "### Invalid register address: " << string(argv[2]) << endl;
	  usage();
	  return 0;
	}
      else if( addr < 0 )
	{
	  cout << "### Fan index out-of-range <0-1>" << endl;
	  return 0;
	}
    }

  if( argc == 4 )
    {
      regval = QString(argv[3]).toUInt( &ok );
      if( !ok )
	{
	  regval = QString(argv[3]).toUInt( &ok, 16 );
	  if( !ok )
	    {
	      cout << "### Invalid register value: "
		   << string(argv[2]) << endl;
	      usage();
	      return 0;
	    }
	}
      write_reg = true;
    }

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

  if( write_reg )
    {
      // Read the requested SPIDR register address
      if( !spidrctrl.setSpidrReg( addr, regval ) )
	cout << "### Failed to write register" << endl;
      else
	cout << "Wr Addr 0x" << hex << setfill('0')
	     << setw(4) << addr << ": 0x" << setw(8) << regval << endl;
    }
  else
    {
      // Read the requested SPIDR register address
      if( !spidrctrl.getSpidrReg( addr, &regval ) )
	cout << "### Failed to read register" << endl;
      else
	cout << "Rd Addr 0x" << hex << setfill('0')
	     << setw(4) << addr << ": 0x" << setw(8) << regval << endl;
    }

  return 0;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrreg <ipaddr>[:<portnr>] <addr> [<value>]\n"
    "   Read or set the fan speeds of a SPIDR module.\n"
    "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.\n"
    "     <portnr> : current SPIDR controller IP port number, default 50000.\n"
    "     <addr>   : SPIDR register address "
    "(hex, minus offset of 0x800A0000).\n"
    "     <value>  : if provided, value to write to register.\n";
}

// ----------------------------------------------------------------------------
