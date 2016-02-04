/* ----------------------------------------------------------------------------
File   : spidrfan.cpp

Descr  : Commandline tool to control SPIDR module fan speed(s).

Usage  :
spidrfan <ipaddr>[:<portnr>] [<fannr> <speed>] 
   Read or set the fan speeds of a SPIDR module.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.
     <portnr> : current SPIDR controller IP port number, default 50000.
     <fannr>  : index of fan to control (0=SPIDR, 1=chipboard).
     <speed>  : fan speed to set as a percentage of the full-scale range.

History:
29JAN2016; HenkB; Created.
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
  int     fannr = -1, speed;

  // Check argument count
  if( !(argc == 2 || argc == 4) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

  if( argc == 4 )
    {
      bool ok;
      fannr = QString(argv[2]).toUInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid fan index: " << string(argv[2]) << endl;
	  usage();
	  return 0;
	}
      else if( fannr > 1 || fannr < 0 )
	{
	  cout << "### Fan index out-of-range <0-1>" << endl;
	  return 0;
	}

      speed = QString(argv[3]).toUInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid fan speed: " << string(argv[2]) << endl;
	  usage();
	  return 0;
	}
      else if( speed > 100 || speed < 0 )
	{
	  cout << "### Fan speed percentage to set out-of-range <0-100>"
	       << endl;
	  return 0;
	}
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

  if( fannr < 0 )
    {
      // Read the current fan speeds
      if( !spidrctrl.getFanSpeed( 0, &speed ) )
	cout << "### Failed to read fan #0" << endl;
      else
	cout << "Fan #0: " << speed << " [rpm]" << endl;

      if( !spidrctrl.getFanSpeed( 1, &speed ) )
	cout << "### Failed to read fan #1" << endl;
      else
	cout << "Fan #1: " << speed << " [rpm]" << endl;
    }
  else
    {
      // Set the requested fan speed
      if( !spidrctrl.setFanSpeed( fannr, speed ) )
	cout << "### Failed to set fan speed #" << fannr << endl;
      else
	cout << "Fan #" << fannr << " set to " << speed << "%" << endl;
    }

  return 0;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrfan <ipaddr>[:<portnr>] [<fannr> <speed>]\n"
    "   Read or set the fan speeds of a SPIDR module.\n"
    "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.\n"
    "     <portnr> : current SPIDR controller IP port number, default 50000.\n"
    "     <fannr>  : index of fan to control (0=SPIDR, 1=chipboard).\n"
    "     <speed>  : fan speed to set as a percentage of the full-scale range.\n";
}

// ----------------------------------------------------------------------------
