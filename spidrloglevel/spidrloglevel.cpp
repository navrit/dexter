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
  // Check argument count
  if( argc != 3 )
    {
      usage();
      return 0;
    }

  // Get arguments
  quint32 ipaddr = 0;
  int     portnr = 50000;
  int     loglvl = 0;
  ipaddr = get_addr_and_port( argv[1], &portnr );
  loglvl = my_atoi( argv[2] );

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

  if( spidrctrl.setLogLevel( loglvl ) )
    {
      cout << "SPIDR loglevel set to " << loglvl << endl;
    }
  else
    {
      cout << "### Failed to set SPIDR loglevel to " << loglvl << endl;
    }
  spidrctrl.displayInfo();

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
  int value = 0;
  while( *c >= '0' && *c <= '9' )
    {
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
       << "spidrloglevel <ipaddr>[:<portnr>] <level>" << endl
       << "   Set the SPIDR module's log level to <level>" << endl;
}

// ----------------------------------------------------------------------------
