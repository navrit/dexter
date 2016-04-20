/* ----------------------------------------------------------------------------
File   : spidrloglevel.cpp

Descr  : Commandline tool to set the SPIDR application software's 'log level'.

Usage  :
spidrloglevel <ipaddr>[:<portnr>] [<level>]
   Set the application software log level on a SPIDR module.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.
     <portnr> : current SPIDR controller IP port number, default 50000.
     <level>  : log level (0 = DEBUG, 1 = INFO, 2 = WARNING, 3 = ERROR, 4 = FAIL).

History:
25MAR2016; HenkB; Created.
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
#include <QString>

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr_and_port( const char *str, int *portnr );
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;
  int     level;

  // Check argument count
  if( !(argc == 3) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

  bool ok;
  level = QString(argv[2]).toUInt( &ok );
  if( !ok )
    {
      cout << "### Invalid level: " << string(argv[2]) << endl;
      usage();
      return 0;
    }
  //else if( level >= 4 || level < 0 )
  else if( level >= 16 || level < 0 )
  {
      cout << "### Level out-of-range <0-4>" << endl;
      return 0;
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

  //if( !spidrctrl.setLogLevel(level) )
  //  cout << "### setLogLevel" << endl;
  if( !spidrctrl.setGpio(level) )
    cout << "### setGpio" << endl;

  return 0;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage:\n"
    "spidrloglevel <ipaddr>[:<portnr>] <level>\n"
    "   Set the application software log level on a SPIDR module.\n"
    "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.\n"
    "     <portnr> : current SPIDR controller IP port number, default 50000.\n"
    "     <level>  : log level (0=DEBUG, 1=INFO, 2=WARNING, "
    "3=ERROR, 4=FAIL).\n";
}

// ----------------------------------------------------------------------------
