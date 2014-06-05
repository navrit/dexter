/* ----------------------------------------------------------------------------
File   : spidrreset.cpp

Descr  : Commandline tool to reset a SPIDR module.

Usage  :
spidrreset <ipaddr> [port]

History:
05JUN2014; HenkB; Created.
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
  quint32      addr_cmd   = 0;
  int          portnr_cmd = 50000;
  QHostAddress qaddr;

  // Check argument count
  if( !(argc == 2 || argc == 3) )
    {
      usage();
      return 0;
    }

  // Check arguments
  addr_cmd = get_addr( argv[1] );
  if( argc == 3 )
    {
      bool ok;
      portnr_cmd = QString(argv[2]).toUInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid IP port: " << string(argv[3]) << endl;
	  usage();
	  return 0;
	}
    }

  // Open a control connection to SPIDR-TPX3 module
  // with the given address and port, or -if the latter was not provided-
  // the default port number 50000
  SpidrController spidrctrl( (addr_cmd>>24) & 0xFF,
			     (addr_cmd>>16) & 0xFF,
			     (addr_cmd>> 8) & 0xFF,
			     (addr_cmd>> 0) & 0xFF, portnr_cmd );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
         << spidrctrl.connectionStateString() << ", "
         << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  int errstat;
  if( spidrctrl.reset( &errstat ) )
    {
      cout << "==> Reset done, error=" << hex << errstat << endl;
      spidrctrl.displayInfo();
    }
  else
    {
      error_out( "### reset" );
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
       << "spidrreset <ipaddr> [<portnr_cmd>]" << endl
       << "   Reset the SPIDR module with the given address and port "
       << "(default 50000)" << endl;
}

// ----------------------------------------------------------------------------
