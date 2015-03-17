/* ----------------------------------------------------------------------------
File   : spidripmonconfig.cpp

Descr  : Commandline tool to configure monitor-stream IP on a SPIDR module.

Usage  :
spidripmonconfig <ipaddr>[:<portnr>] show
   Display devices' IP addresses and ports for the SPIDR module
   with the given (Controller) address/port.

spidripmonconfig <ipaddr>[:<portnr>] dest [<ipaddr_dst>]
   Display or set SPIDR devices' monitor-data destination IP address(es).
     <ipaddr>    : SPIDR Controller IP address, e.g. 192.168.100.10
     <ipaddr_dst>: new devices' IP destination, e.g. 192.168.2.10

spidripmonconfig <ipaddr>[:<portnr>] port [<portnr_dst>]
   Display or set SPIDR devices' monitor-data server IP port number(s).
     <ipaddr>    : SPIDR Controller IP address, e.g. 192.168.100.10
     <portnr_dst>: new devices' IP destination port (incrementing), e.g. 8192

History:
05JUN2014; HenkB; Created, modified copy of spidripconfig.cpp.
---------------------------------------------------------------------------- */

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"

#include <QHostAddress>
#include <QString>

#define SPIDR_IPMON_OFFS  16

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr_and_port( const char *str, int *portnr );
quint32 get_addr( const char *str );
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  bool         dst       = false; // Set UDP port destination address(es)
  bool         prt       = false; // Set UDP port port number(s)
  bool         show_only = false; // Show curr destination IP or port addresses
  quint32      addr_cmd    = 0;
  quint32      addr_new    = 0;
  int         *paddr_new_i = (int *) &addr_new;
  int          portnr_cmd  = 50000;
  int          portnr_new  = 0;
  QHostAddress qaddr;

  // Check argument count
  if( !(argc == 3 || argc == 4) )
    {
      usage();
    }
  else
    {
      if( QString(argv[2]) == QString("show") && argc > 3 )
	{
	  cout << "### Argument count" << endl;
	  usage();
	}
      else if( (QString(argv[2]) == QString("dest") ||
		QString(argv[2]) == QString("port")) && argc > 4 )
	{
	  cout << "### Argument count" << endl;
	  usage();
	}
    }

  // Check arguments
  if( QString(argv[2]) == QString("show") )
    {
      addr_cmd = get_addr_and_port( argv[1], &portnr_cmd );
      show_only = true;
    }
  else if( QString(argv[2]) == QString("dest") )
    {
      dst = true;
      addr_cmd = get_addr_and_port( argv[1], &portnr_cmd );

      if( argc == 4 )
	addr_new = get_addr( argv[3] );
      else
	show_only = true;
    }
  else if( QString(argv[2]) == QString("port") )
    {
      prt = true;
      addr_cmd = get_addr_and_port( argv[1], &portnr_cmd );

      if( argc == 4 )
	{
	  bool ok;
	  portnr_new = QString(argv[3]).toUInt( &ok );
	  if( !ok )
	    {
	      cout << "### Invalid IP port: " << string(argv[3]) << endl;
	      usage();
	    }
	}
      else
	{
	  show_only = true;
	}
    }
  else
    {
      cout << "### Invalid 2nd argument: " << argv[2] << endl;
      usage();
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

  int devices;
  if( !spidrctrl.getDeviceCount( &devices ) )
    {
      cout << "### Error reading device count, aborting..." << endl;
      return 0;
    }

  if( dst )
    {
      // Display all the Medipix3/Timepix3 device destination IP addresses
      // and optionally set to a new value...
      int      i, ipaddr;
      quint32 *qi = (quint32 *) &ipaddr;
      cout << "UDP port IP destination addresses (monitor stream): ";
      for( i=0; i<devices; ++i )
	{
	  if( !spidrctrl.getIpAddrDest( i + SPIDR_IPMON_OFFS, &ipaddr ) )
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
	      cout << qaddr.toString().toLatin1().constData();
	    }
	}
      cout << endl;

      if( !show_only )
	{
	  cout << "change to: ";
	  qaddr.setAddress( addr_new );
	  cout << qaddr.toString().toLatin1().constData() << endl;

	  for( i=0; i<devices; ++i )
	    if( !spidrctrl.setIpAddrDest( i + SPIDR_IPMON_OFFS,
					  *paddr_new_i ) )
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
	    cout << "==> New settings stored" << endl;
	  else
	    error_out( "### Error storing new settings" );
	}
    }
  else if( prt )
    {
      // Display all the Medipix3/Timepix3 device server/destination IP ports
      // and optionally set to a new value...
      int      i, ipport;
      cout << "UDP port IP server ports (monitor stream): ";
      for( i=0; i<devices; ++i )
	{
	  if( !spidrctrl.getServerPort( i + SPIDR_IPMON_OFFS, &ipport ) )
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
	  for( i=0; i<devices; ++i )
	    if( !spidrctrl.setServerPort( i + SPIDR_IPMON_OFFS,
					  portnr_new + i ) )
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
	    cout << "==> New settings stored" << endl;
	  else
	    error_out( "### Error storing new settings" );
	}
    }
  else
    {
      if( show_only )
	{
	  int      i, ipaddr, portnr;
	  quint32 *qi = (quint32 *) &ipaddr;
	  for( i=0; i<devices; ++i )
	    {
	      cout << "UDP monitor port " << i << ":" << endl;

	      // IP addresses
	      if( !spidrctrl.getIpAddrSrc( i + SPIDR_IPMON_OFFS, &ipaddr ) )
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
		  cout << qaddr.toString().toStdString();
		}
	      if( !spidrctrl.getIpAddrDest( i + SPIDR_IPMON_OFFS, &ipaddr ) )
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
		  cout << qaddr.toString().toStdString();
		}

	      // IP ports
	      if( !spidrctrl.getServerPort( i + SPIDR_IPMON_OFFS, &portnr ) )
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
	      if( !spidrctrl.getDevicePort( i + SPIDR_IPMON_OFFS, &portnr ) )
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
	  for( i=0; i<devices; ++i )
	    {
	      if( !spidrctrl.getIpAddrSrc( i + SPIDR_IPMON_OFFS, &ipaddr ) )
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
		  cout << qaddr.toString().toStdString();
		}
	    }
	  cout << endl << "change to: ";
	  qaddr.setAddress( addr_new );
	  cout << qaddr.toString().toStdString() << endl;

	  for( i=0; i<devices; ++i )
	    if( !spidrctrl.setIpAddrSrc( i + SPIDR_IPMON_OFFS, *paddr_new_i ) )
	      {
		cout << endl
		     << "### Error setting IP src addr " << i
		     << ", aborting..." << endl;
		return 0;
	      }

	  // And now store it all
	  if( spidrctrl.storeAddrAndPorts() )
	    cout << "==> New settings stored: "
		 << "reset/powercycle SPIDR." << endl;
	  else
	    error_out( "### Error storing new settings" );
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
    }
  return qaddr.toIPv4Address();
}

// ----------------------------------------------------------------------------

quint32 get_addr( const char *str )
{
  QHostAddress qaddr;
  if( !qaddr.setAddress( QString(str) ) )
    {
      cout << "### Invalid IP address: " << string(str) << endl;
      usage();
    }
  return qaddr.toIPv4Address();
}

// ----------------------------------------------------------------------------

void usage()
{
  cout << endl << "Usage:" << endl
       << "spidripmonconfig <ipaddr>[:<portnr>] show"
       << endl
       << "   Display devices' IP addresses and ports for the SPIDR module "
       << endl
       << "   with the given address/port."
       << endl << endl
       << "spidripmonconfig <ipaddr>[:<portnr>] dest [<ipaddr_dst>]"
       << endl
       << "   Display or set SPIDR devices' destination IP address(es)."
       << endl
       << "     <ipaddr>    : SPIDR controller IP address, e.g. 192.168.100.10"
       << endl
       << "     <ipaddr_dst>: new devices' IP destination, e.g. 192.168.2.1"
       << endl << endl
       << "spidripmonconfig <ipaddr>[:<portnr>] port [<portnr_dst>]"
       << endl
       << "   Display or set SPIDR devices' server IP port number(s)."
       << endl
       << "     <ipaddr>    : SPIDR controller IP address, e.g. 192.168.100.10"
       << endl
       << "     <portnr_dst>: new devices' IP destination port "
       << "(incrementing), e.g. 8192"
       << endl;
  exit( 0 );
}

// ----------------------------------------------------------------------------
