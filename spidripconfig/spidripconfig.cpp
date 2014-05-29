/* ----------------------------------------------------------------------------
File   : spidripconfig.cpp

Descr  : Commandline tool to configure IP on a SPIDR module.

Usage  :
spidripconfig <ipaddr> defaults
   Go back to default settings (erase stored IP addresses and ports).
     <ipaddr>    : current SPIDR IP address, e.g. 192.168.100.10

spidripconfig <ipaddr> show [<portnr_cmd>]
   Display devices' IP addresses and ports for the SPIDR module
   with the given address/port.

spidripconfig <ipaddr> <ipaddr_new> [<portnr_cmd>] [<portnr_new>]
   Set new IP source address for SPIDR *and* its devices
   and optionally a new command & control port number.
     <ipaddr>    : current SPIDR IP address, e.g. 192.168.100.10
     <ipaddr_new>: new SPIDR IP address, e.g. 192.168.101.10
     <portnr_cmd>: current IP port for SPIDR command & control, default 50000
     <portnr_new>: new IP port for SPIDR command & control

spidripconfig <ipaddr> dest [<ipaddr_dst>]
   Display or set SPIDR devices' destination IP address(es).
     <ipaddr>    : current SPIDR IP address, e.g. 192.168.100.10
     <ipaddr_dst>: new devices' IP destination, e.g. 192.168.101.1

spidripconfig <ipaddr> port [<portnr_dst>]
   Display or set SPIDR devices' server IP port number(s).
     <ipaddr>    : current SPIDR IP address, e.g. 192.168.100.10
     <portnr_dst>: new devices' IP destination port (incrementing), e.g. 8192

History:
27MAR2014; HenkB; Created.
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
  bool         dflts     = false; // Set all to default
  bool         dst       = false; // Set UDP port destination address(es)
  bool         prt       = false; // Set UDP port port number(s)
  bool         show_only = false; // Show curr destination IP or port addresses
  quint32      addr_curr   = 0;
  quint32      addr_new    = 0;
  int         *paddr_new_i = (int *) &addr_new;
  int          portnr_cmd  = 50000;
  int          portnr_new  = 0;
  QHostAddress qaddr;

  // Check argument count
  if( !(argc == 3 || argc == 4 || argc == 5) )
    {
      usage();
      return 0;
    }
  else
    {
      if( QString(argv[2]) == QString("defaults") && argc > 3 )
	{
	  cout << "### Argument count" << endl;
	  usage();
	  return 0;
	}
      else if( (QString(argv[2]) == QString("dest") ||
		QString(argv[2]) == QString("port") ||
		QString(argv[2]) == QString("show")) && argc > 4 )
	{
	  cout << "### Argument count" << endl;
	  usage();
	  return 0;
	}
    }

  // Check arguments
  if( QString(argv[2]) == QString("defaults") )
    {
      dflts = true;
      addr_curr = get_addr( argv[1] );
    }
  else if( QString(argv[2]) == QString("dest") )
    {
      dst = true;
      addr_curr = get_addr( argv[1] );

      if( argc == 4 )
	addr_new = get_addr( argv[3] );
      else
	show_only = true;
    }
  else if( QString(argv[2]) == QString("port") )
    {
      prt = true;
      addr_curr = get_addr( argv[1] );

      if( argc == 4 )
	{
	  bool ok;
	  portnr_new = QString(argv[3]).toUInt( &ok );
	  if( !ok )
	    {
	      cout << "### Invalid IP port: " << string(argv[3]) << endl;
	      usage();
	      return 0;
	    }
	}
      else
	{
	  show_only = true;
	}
    }
  else if( QString(argv[2]) == QString("show") )
    {
      addr_curr = get_addr( argv[1] );
      show_only = true;

      if( argc == 4 )
	{
	  bool ok;
	  portnr_cmd = QString(argv[3]).toUInt( &ok );
	  if( !ok )
	    {
	      cout << "### Invalid IP port: " << string(argv[3]) << endl;
	      usage();
	      return 0;
	    }
	}
    }
  else
    {
      // Set a new command & control IP address on the SPIDR
      // (e.g. when you need to move the SPIDR to a network interface
      //  with a different IP address / subnet mask).
      // Note that this action also changes the SPIDR's devices'
      // IP destination (server) addresses to the same value;
      // use "spidripconfig <ipaddr> dest [<ipaddr_dst>]" to change them
      // to another value if necessary.
      addr_curr = get_addr( argv[1] );
      addr_new  = get_addr( argv[2] );

      // An optional port number (SPIDR's current port number)
      if( argc >= 4 )
	{
	  bool ok;
	  portnr_cmd = QString(argv[3]).toUInt( &ok );
	  if( !ok )
	    {
	      cout << "### Invalid IP port: " << string(argv[3]) << endl;
	      usage();
	      return 0;
	    }

	  if( argc == 5 )
	    {
	      bool ok;
	      portnr_new = QString(argv[4]).toUInt( &ok );
	      if( !ok )
		{
		  cout << "### Invalid IP port: " << string(argv[4]) << endl;
		  usage();
		  return 0;
		}
	    }
	}
    }

  // Open a control connection to SPIDR-TPX3 module
  // with the given address and port, or -if the latter was not provided-
  // the default port number 50000
  SpidrController spidrctrl( (addr_curr>>24) & 0xFF,
			     (addr_curr>>16) & 0xFF,
			     (addr_curr>> 8) & 0xFF,
			     (addr_curr>> 0) & 0xFF, portnr_cmd );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
         << spidrctrl.connectionStateString() << ", "
         << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  if( dflts )
    {
      if( !spidrctrl.eraseAddrAndPorts() )
	error_out( "### Error erasing configuration" );
      else
	cout << "==> Default settings restored: "
	     << "reset/powercycle SPIDR." << endl;
    }
  else if( dst )
    {
      // Display all the Medipix3/Timepix3 device destination IP addresses
      // and optionally set to a new value...
      int ports;
      if( !spidrctrl.getPortCount( &ports ) )
	{
	  cout << "### Error reading port count, aborting..." << endl;
	  return 0;
	}

      int      i, ipaddr;
      quint32 *qi = (quint32 *) &ipaddr;
      cout << "UDP port IP destination addresses: ";
      for( i=0; i<ports; ++i )
	{
	  if( !spidrctrl.getIpAddrDest( i, &ipaddr ) )
	    {
	      cout << endl
		   << "### Error getting IP dest addr " << i
		   << ", aborting..." << endl;
	      return 0;
	    }
	  else
	    {
	      if( i > 0 ) cout << ", ";
	      qaddr.setAddress( *qi );
	      cout << qaddr.toString().toAscii().constData();
	    }
	}
      cout << endl;

      if( !show_only )
	{
	  cout << "change to: ";
	  qaddr.setAddress( addr_new );
	  cout << qaddr.toString().toAscii().constData() << endl;

	  for( i=0; i<ports; ++i )
	    if( !spidrctrl.setIpAddrDest( i, *paddr_new_i ) )
	      {
		cout << endl
		     << "### Error setting IP dest addr " << i
		     << ", aborting..." << endl;
		return 0;
	      }
	}

      // And now store it all
      if( !show_only )
	{
	  if( spidrctrl.storeAddrAndPorts() )
	    cout << "==> New settings stored." << endl;
	  else
	    error_out( "### Error storing new settings" );
	}
    }
  else if( prt )
    {
      // Display all the Medipix3/Timepix3 device server/destination IP ports
      // and optionally set to a new value...
      int ports;
      if( !spidrctrl.getPortCount( &ports ) )
	{
	  cout << "### Error reading port count, aborting..." << endl;
	  return 0;
	}

      int      i, ipport;
      cout << "UDP port IP server ports: ";
      for( i=0; i<ports; ++i )
	{
	  if( !spidrctrl.getServerPort( i, &ipport ) )
	    {
	      cout << endl
		   << "### Error getting IP port nr " << i
		   << ", aborting..." << endl;
	      return 0;
	    }
	  else
	    {
	      if( i > 0 ) cout << ", ";
	      cout << ipport;
	    }
	}
      cout << endl;

      if( !show_only )
	{
	  cout << "change to: " << portnr_new << " (incrementing)" << endl;

	  // Set incrementing port number
	  for( i=0; i<ports; ++i )
	    if( !spidrctrl.setServerPort( i, portnr_new + i ) )
	      {
		cout << endl
		     << "### Error setting IP port nr " << i
		     << ", aborting..." << endl;
		return 0;
	      }
	}

      // And now store it all
      if( !show_only )
	{
	  if( spidrctrl.storeAddrAndPorts() )
	    cout << "==> New settings stored." << endl;
	  else
	    error_out( "### Error storing new settings" );
	}
    }
  else
    {
      int ports;
      if( !spidrctrl.getPortCount( &ports ) )
	{
	  cout << "### Error reading port count, aborting..." << endl;
	  return 0;
	}

      if( show_only )
	{
	  int      i, ipaddr, portnr;
	  quint32 *qi = (quint32 *) &ipaddr;
	  for( i=0; i<ports; ++i )
	    {
	      cout << "UDP port " << i << ":" << endl;

	      // IP addresses
	      if( !spidrctrl.getIpAddrSrc( i, &ipaddr ) )
		{
		  cout << endl
		       << "### Error getting IP src addr"
		       << ", aborting..." << endl;
		  return 0;
		}
	      else
		{
		  cout << "addr src ";
		  qaddr.setAddress( *qi );
		  cout << qaddr.toString().toAscii().constData();
		}
	      if( !spidrctrl.getIpAddrDest( i, &ipaddr ) )
		{
		  cout << endl
		       << "### Error getting IP dst addr"
		       << ", aborting..." << endl;
		  return 0;
		}
	      else
		{
		  cout << ", addr dst ";
		  qaddr.setAddress( *qi );
		  cout << qaddr.toString().toAscii().constData();
		}

	      // IP ports
	      if( !spidrctrl.getServerPort( i, &portnr ) )
		{
		  cout << endl
		       << "### Error getting device IP dest port"
		       << ", aborting..." << endl;
		  return 0;
		}
	      else
		{
		  cout << ", server port " << portnr;
		}
	      if( !spidrctrl.getDevicePort( i, &portnr ) )
		{
		  cout << endl
		       << "### Error getting device IP src port"
		       << ", aborting..." << endl;
		  return 0;
		}
	      else
		{
		  cout << ", (device port " << portnr << ")";
		}
	      cout << endl;
	    }
	}
      else
	{
	  // Also set all the Medipix3/Timepix3 device source IP addresses
	  // to the new value...
	  int      i, ipaddr;
	  quint32 *qi = (quint32 *) &ipaddr;
	  cout << "UDP port IP source addresses: ";
	  for( i=0; i<ports; ++i )
	    {
	      if( !spidrctrl.getIpAddrSrc( i, &ipaddr ) )
		{
		  cout << endl
		       << "### Error getting IP src addr " << i
		       << ", aborting..." << endl;
		  return 0;
		}
	      else
		{
		  if( i > 0 ) cout << ", ";
		  qaddr.setAddress( *qi );
		  cout << qaddr.toString().toAscii().constData();
		}
	    }
	  cout << endl << "change to: ";
	  qaddr.setAddress( addr_new );
	  cout << qaddr.toString().toAscii().constData() << endl;

	  for( i=0; i<ports; ++i )
	    if( !spidrctrl.setIpAddrSrc( i, *paddr_new_i ) )
	      {
		cout << endl
		     << "### Error setting IP src addr " << i
		     << ", aborting..." << endl;
		return 0;
	      }

	  // And now change the SPIDR's IP (source) address,
	  // and -optionally- port number, and store it all
	  if( spidrctrl.storeAddrAndPorts( *paddr_new_i, portnr_new ) )
	    cout << "==> New settings stored: "
		 << "reset/powercycle SPIDR." << endl;
	  else
	    error_out( "### Error storing new settings" );
	}
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
       << "spidripconfig <ipaddr> defaults"
       << endl
       << "   Go back to default settings "
       << "(erase stored IP addresses and ports)."
       << endl
       << "     <ipaddr>    : current SPIDR IP address, e.g. 192.168.100.10"
       << endl << endl
       << "spidripconfig <ipaddr> show [<portnr_cmd>]"
       << endl
       << "   Display devices' IP addresses and ports for the SPIDR module "
       << endl
       << "   with the given address/port."
       << endl << endl
       << "spidripconfig <ipaddr> <ipaddr_new> [<portnr_cmd>] [portnr_new]"
       << endl
       << "   Set new IP source address for SPIDR *and* its devices"
       << endl
       << "   and optionally a new command & control port number."
       << endl
       << "     <ipaddr>    : current SPIDR IP address, e.g. 192.168.100.10"
       << endl
       << "     <ipaddr_new>: new SPIDR IP address, e.g. 192.168.101.10"
       << endl
       << "     <portnr_cmd>: current IP port for SPIDR command & control, "
       << "default 50000"
       << endl
       << "     <portnr_new>: new IP port for SPIDR command & control"
       << endl << endl
       << "spidripconfig <ipaddr> dest [<ipaddr_dst>]"
       << endl
       << "   Display or set SPIDR devices' destination IP address(es)."
       << endl
       << "     <ipaddr>    : current SPIDR IP address, e.g. 192.168.100.10"
       << endl
       << "     <ipaddr_dst>: new devices IP destination, e.g. 192.168.101.1"
       << endl << endl
       << "spidripconfig <ipaddr> port [<portnr_dst>]"
       << endl
       << "   Display or set SPIDR devices' server IP port number(s)."
       << endl
       << "     <ipaddr>    : current SPIDR IP address, e.g. 192.168.100.10"
       << endl
       << "     <portnr_dst>: new devices IP destination port "
       << "(incrementing), e.g. 8192"
       << endl;
}

// ----------------------------------------------------------------------------
