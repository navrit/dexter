/* ----------------------------------------------------------------------------
File   : spidrid.cpp

Descr  : Commandline tool to display and/or set a SPIDR module's identifier
         (serial number).

Usage  :
spidrid <ipaddr> [id_new]
   Display or set a new identifier on the SPIDR module.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10
     <id_new> : new identifier (32-bit, decimal or hexadecimal)

History:
28APR2014; HenkB; Created.
---------------------------------------------------------------------------- */

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"

#include <QHostAddress>
#include <QString>

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr( const char *str );
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  bool         modify = false;
  quint32      addr_curr = 0;
  QHostAddress qaddr;
  int          id_curr, id_new;

  // Check argument count
  if( !(argc == 2 || argc == 3) )
    {
      usage();
      return 0;
    }

  addr_curr = get_addr( argv[1] );

  if( argc == 3 )
    {
      bool ok;
      // Hex or decimal
      if( argv[2][0] == '0' && (argv[2][1] == 'x' || argv[2][1] == 'X') )
	id_new = QString(argv[2]).toUInt( &ok, 16 );
      else
	id_new = QString(argv[2]).toUInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid ID: " << string(argv[2]) << endl;
	  usage();
	  return 0;
	}
      modify = true;
    }

  // Open a control connection to SPIDR-TPX3 module
  // with the given address and default port
  SpidrController spidrctrl( (addr_curr>>24) & 0xFF,
			     (addr_curr>>16) & 0xFF,
			     (addr_curr>> 8) & 0xFF,
			     (addr_curr>> 0) & 0xFF );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
         << spidrctrl.connectionStateString() << ", "
         << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  if( !spidrctrl.getSpidrId( &id_curr ) )
    {
      cout << "### Error reading ID" << endl;
      return 0;
    }
  cout << "Current SPIDR ID: "
       << hex << setw(8) << setfill('0') << id_curr << dec << endl;

  if( modify )
    { 
      if( !spidrctrl.setSpidrId( id_new ) )
	{
	  cout << "### Error writing ID" << endl;
	  return 0;
	}

      if( !spidrctrl.getSpidrId( &id_curr ) )
	{
	  cout << "### Error reading ID" << endl;
	  return 0;
	}
     cout << "New SPIDR ID: "
	   << hex << setw(8) << setfill('0') << id_curr << dec << endl;

     if( id_new != id_curr )
       cout << "### New ID unequal to value set!?" << endl;
    }

  return 0;
}

// ----------------------------------------------------------------------------

quint32 get_addr( const char *str )
{
  QHostAddress qaddr;
  if( !qaddr.setAddress( QString(str) ) )
    {
      cout << "### Invalid IP address: " << string(str) << endl;
      usage();
      exit( 0 );
    }
  return qaddr.toIPv4Address();
}

// ----------------------------------------------------------------------------

void usage()
{
  cout << endl << "Usage:" << endl
       << "spidrid <ipaddr> [id_new]"
       << endl
       << "   Display or set a new identifier on the SPIDR module."
       << endl
       << "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10"
       << endl
       << "     <id_new> : new identifier (32-bit, decimal or hexadecimal)"
       << endl;
}

// ----------------------------------------------------------------------------
