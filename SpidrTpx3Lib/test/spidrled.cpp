/* ----------------------------------------------------------------------------
File   : spidrled.cpp

Descr  : Commandline tool to blink an LED on a SPIDR module.

Usage  :
spidrled <ipaddr>[:<portnr>] [lednr]
   Blink an LED on the SPIDR module.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.
     <portnr> : current SPIDR controller IP port number, default 50000.
     <lednr>  : LED index.

History:
04MAR2015; HenkB; Created.
---------------------------------------------------------------------------- */

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

#include <QHostAddress>
#include <QString>

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr_and_port( const char *str, int *portnr );
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;
  int     lednr;

  // Check argument count
  if( !(argc == 3) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

  bool ok;
  lednr = QString(argv[2]).toUInt( &ok );
  if( !ok )
    {
      cout << "### Invalid LED-number: " << string(argv[2]) << endl;
      usage();
      return 0;
    }
  else if( lednr >= 12 || lednr < 0 )
    {
      cout << "### LED-number out-of-range <0-11>" << endl;
      return 0;
    }

  // Open a control connection to SPIDR-TPX3 module
  // with the given address and default port
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

  int reg;
  if( lednr >= 8 )
    {
      lednr -= 8;

      if( !spidrctrl.getSpidrReg( 0x2D0, &reg ) )
	{
	  cout << "### getReg" << endl;
	  return 0;
	}
      while( 1 )
	{
	  reg ^= (1 << (lednr + 3)); // Location in the register...
	  if( !spidrctrl.setSpidrReg( 0x2D0, reg ) )
	    {
	      cout << "### setReg" << endl;
	      return 0;
	    }
	  Sleep( 150 );
	}
    }
  else
    {
      if( !spidrctrl.getSpidrReg( 0x2C4, &reg ) )
	{
	  cout << "### getReg" << endl;
	  return 0;
	}
      while(1)
	{
	  reg ^= (1 << lednr);
	  if( !spidrctrl.setSpidrReg( 0x2C4, reg ) )
	    {
	      cout << "### setReg" << endl;
	      return 0;
	    }
	  Sleep( 150 );
	}
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
       << "spidrled <ipaddr>[:<portnr>] [lednr]"
       << endl
       << "   Blink an LED on the SPIDR module."
       << endl
       << "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10."
       << endl
       << "     <portnr> : current SPIDR controller IP port number, "
       << "default 50000."
       << "     <lednr>  : LED index."
       << endl;
}

// ----------------------------------------------------------------------------
