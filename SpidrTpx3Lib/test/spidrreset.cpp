/* ----------------------------------------------------------------------------
File   : spidrreset.cpp

Descr  : Commandline tool to (soft)reset a SPIDR module.

Usage  :
spidrreset <ipaddr>[:<portnr>] [<speed>]
   Command a SPIDR module to execute a (soft) reset.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.
     <portnr> : current SPIDR controller IP port number, default 50000.
     <speed>  : Timepix3 readout speed; 1 -> force high, 0 = force low.

History:
14DEC2015; HenkB; Created.
---------------------------------------------------------------------------- */

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"

#include <QHostAddress>
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
  int     readout_speed = 0; // Means: don't change current setting

  // Check argument count
  if( !(argc == 2 || argc == 3) )
    {
      usage();
      return 1;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

  if( argc == 3 )
    {
      readout_speed = my_atoi( argv[2] );
      if( readout_speed < 0 || readout_speed > 1 )
	{
	  cout << "### Illegal readout-speed " << readout_speed << endl;
	  usage();
	  return 1;
	}
      if( readout_speed == 0 ) readout_speed = -1; // Force low speed
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

  int errstat;
  if( spidrctrl.reset( &errstat, readout_speed ) ) {
    cout << "SPIDR reset";
    if( readout_speed == -1 )
      cout << " (force low readout-speed)";
    else if( readout_speed == 1 )
      cout << " (force high readout-speed)";
    cout << endl;
    cout << "errstat " << hex << uppercase << errstat << dec
	 << endl;
  } else {
    error_out( "reset()" );
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
  cout << endl << "Usage:" << endl
       << "spidrreset <ipaddr>[:<portnr>] [<speed>]"
       << endl
       << "   Command a SPIDR module to execute a (soft) reset."
       << endl
       << "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10."
       << endl
       << "     <portnr> : current SPIDR controller IP port number, "
       << "default 50000."
       << endl
       << "     <speed>  : Timepix3 readout speed; 1 -> force high, "
       << "0 = force low."
       << endl;
}

// ----------------------------------------------------------------------------
