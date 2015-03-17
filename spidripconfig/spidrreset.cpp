/* ----------------------------------------------------------------------------
File   : spidrreset.cpp

Descr  : Commandline tool to reset a SPIDR module.

Usage  :
spidrreset <ipaddr>[:<port>]

History:
05JUN2014; HenkB; Created.
09MAR2015; HenkB; Use :<port> syntax for the optional port number.
---------------------------------------------------------------------------- */

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"

#include <QHostAddress>
#include <QString>

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr_and_port( const char *str, int *portnr );
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr   = 0;
  int     portnr = 50000;

  // Check argument count
  if( argc != 2 )
    {
      usage();
      return 0;
    }

  // Get arguments
  ipaddr = get_addr_and_port( argv[1], &portnr );

  // Open a control connection to SPIDR-TPX3 module
  // with the given address and port, or -if the latter was not provided-
  // the default port number 50000
  SpidrController spidrctrl( (ipaddr>>24) & 0xFF,
			     (ipaddr>>16) & 0xFF,
			     (ipaddr>> 8) & 0xFF,
			     (ipaddr>> 0) & 0xFF, portnr );

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

quint32 get_addr_and_port( const char *str, int *portnr )
{
  QString qstr( str );
  if( qstr.contains( QChar(':') ) )
    {
      // A port number is provided: extract it
      bool ok;
      int p = qstr.section( ':', 1, 1).toInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid port number: "
	       << qstr.section( ':', 1, 1 ).toStdString() << endl;
	  usage();
	  exit( 0 );
	}
      else
	{
	  *portnr = p;
	}
      // Remove the port number from the string
      qstr = qstr.section( ':', 0, 0 );
    }
  QHostAddress qaddr;
  if( !qaddr.setAddress( qstr ) )
    {
      cout << "### Invalid IP address: " << qstr.toStdString() << endl;
      usage();
      exit( 0 );
    }
  return qaddr.toIPv4Address();
}

// ----------------------------------------------------------------------------

void usage()
{
  cout << endl << "Usage:" << endl
       << "spidrreset <ipaddr>[:<portnr_cmd>]" << endl
       << "   Reset the SPIDR module with the given address and port "
       << "(default 50000)" << endl;
}

// ----------------------------------------------------------------------------
