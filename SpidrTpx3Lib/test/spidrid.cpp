/* ----------------------------------------------------------------------------
File   : spidrid.cpp

Descr  : Commandline tool to display and/or set a SPIDR module's identifier
         (serial number).

Usage  :
spidrid <ipaddr>[:<portnr] [id_new]
   Display or set a new identifier on the SPIDR module.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10
     <portnr> : current SPIDR controller IP port number, default 50000
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

quint32 get_addr_and_port( const char *str, int *portnr );
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  bool    modify = false;
  quint32 ipaddr = 0;
  int     portnr = 50000;
  int     id_curr, id_new;

  // Check argument count
  if( !(argc == 2 || argc == 3) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

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
  cout << "Connected to SPIDR: " << spidrctrl.ipAddressString();
  cout <<  endl;

  if( !spidrctrl.getChipboardId( &id_curr ) )
    {
      cout << "### Error reading ID" << endl;
      return 0;
    }
  cout << "Current chipboard ID: "
       << hex << setw(8) << setfill('0') << id_curr << dec << endl;

  if( modify )
    { 
      if( !spidrctrl.setChipboardId( id_new ) )
	{
	  cout << "### Error writing ID" << endl;
	  return 0;
	}

      if( !spidrctrl.getChipboardId( &id_curr ) )
	{
	  cout << "### Error reading ID" << endl;
	  return 0;
	}
     cout << "New chipboard ID: "
	   << hex << setw(8) << setfill('0') << id_curr << dec << endl;

     if( id_new != id_curr )
       cout << "### New ID unequal to value set!?" << endl;
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
       << "spidrid <ipaddr>[:<portnr>] [id_new]"
       << endl
       << "   Display or set a new identifier on the SPIDR module."
       << endl
       << "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10."
       << endl
       << "     <portnr> : current SPIDR controller IP port number, "
       << "default 50000."
       << endl
       << "     <id_new> : new identifier (32-bit, decimal or hexadecimal)."
       << endl;
}

// ----------------------------------------------------------------------------
