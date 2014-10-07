/* ----------------------------------------------------------------------------
File   : spidrtpx3pixconftest.cpp

Descr  : Commandline tool to run a test on the pixel configuration matrix
         of a Timepix3 device connected to a SPIDR-TPX3 module.

Usage  :
spidrtpx3pixconftest <ipaddr> [device_nr]

History:
06OCT2014; HenkB; Created.
---------------------------------------------------------------------------- */

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"

#include <QHostAddress>
#include <QString>

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr( const char *str );
void    usage( char *appname );

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32      addr  = 0;
  int          devnr = 0;
  QHostAddress qaddr;

  // Check argument count
  if( !(argc == 2 || argc == 3) )
    {
      usage( argv[0] );
      return 0;
    }

  // Check arguments
  addr = get_addr( argv[1] );
  if( argc == 3 )
    {
      bool ok;
      devnr = QString(argv[2]).toUInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid device number: " << string(argv[3]) << endl;
	  usage( argv[0]);
	  return 0;
	}
    }

  // Open a control connection to SPIDR-TPX3 module
  // with the given address and default port number 50000
  SpidrController spidrctrl( (addr>>24) & 0xFF, (addr>>16) & 0xFF,
			     (addr>> 8) & 0xFF, (addr>> 0) & 0xFF );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
         << spidrctrl.connectionStateString() << ", "
         << spidrctrl.connectionErrString() << endl;
    // No!
    return 1;
  }

  // Switch to slower 'serial links' output (instead of GTX links
  //if( !spidrctrl.setSpidrRegBit( 0x2D0, 0 ) )
  //  cout << "###setSpidrRegBit: " << spidrctrl.errorString() << endl;

  int errstat;
  if( spidrctrl.reset( &errstat ) )
    {
      cout << "==> Reset: error=" << hex << errstat << endl;
      spidrctrl.displayInfo();
    }
  else
    {
      error_out( "### reset" );
    }

  // Essential after reset! (or nothing can be read from the pixel matrix)
  // (to be included in onboard reset procedure?)
  spidrctrl.resetPixels( devnr );

  // Test using a pattern
  const unsigned char PATTERN[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
                                    0x3E, 0x3D, 0x3B, 0x37, 0x2F, 0x1F,
				    0x3F };
  unsigned char *pixcfg;
  int index, i, sz = sizeof(PATTERN);
  cout << endl << "Test using sequential pattern:" << hex << setfill('0');
  for( index=0; index<sz; ++index )
    cout << " " << setw(2) << (unsigned int) PATTERN[index];
  cout << endl;
  for( index=0; index<sz; ++index )
    {
      spidrctrl.selectPixelConfig( 0 );
      spidrctrl.resetPixelConfig();
      spidrctrl.selectPixelConfig( 1 );
      spidrctrl.resetPixelConfig();

      // Configure pixel config matrix 0
      pixcfg = spidrctrl.pixelConfig( 0 );
      for( i=0; i<256*256; ++i, ++pixcfg )
	*pixcfg = PATTERN[(index+i) % sz];

      // Upload configuration 0
      spidrctrl.selectPixelConfig( 0 );
      if( !spidrctrl.setPixelConfig( devnr ) )
	error_out( "###setPixelConfig" );

      // Read back configuration into config matrix 1
      spidrctrl.selectPixelConfig( 1 );
      if( !spidrctrl.getPixelConfig( devnr ) )
	error_out( "###getPixelConfig" );

      // Compare matrix 0 and 1
      if( spidrctrl.comparePixelConfig( 0, 1 ) == 0 )
	{
	  cout << "patt [" << dec << index << "]: OKAY" << endl;
	}
      else
	{
	  cout << "### patt [" << dec << index << "]: ERROR" << endl;

	  // Display some of the differences found
	  unsigned char *pc0, *pc1;
	  int x, y, cnt = 0;
	  pc0 = spidrctrl.pixelConfig( 0 );
	  pc1 = spidrctrl.pixelConfig( 1 );
	  for( y=0; y<256; ++y )
	    for( x=0; x<256; ++x, ++pc0, ++pc1 )
	      {
		if( *pc0 != *pc1 )
		  {
		    if( cnt < sz+1 )
		      {
			cout << "  y,x=" << dec << y << "," << x
			     << ": got " << hex
			     << (unsigned int) *pc1 << " expected "
			     << (unsigned int) *pc0 << endl;
		      }
		    ++cnt;
		  }
	      }
	  cout << "  Found " << dec << cnt << " different bytes" << endl;
	}
    }
  cout << endl;

  // Test using a constant byte value
  const unsigned char CONST[] = { 0x15, 0x2A, 0x3F };
  sz = sizeof(CONST);
  for( index=0; index<sz; ++index )
    {
      spidrctrl.selectPixelConfig( 0 );
      spidrctrl.resetPixelConfig();
      spidrctrl.selectPixelConfig( 1 );
      spidrctrl.resetPixelConfig();

      // Configure pixel config matrix 0
      pixcfg = spidrctrl.pixelConfig( 0 );
      for( i=0; i<256*256; ++i, ++pixcfg )
	*pixcfg = CONST[index];

      // Upload configuration 0
      spidrctrl.selectPixelConfig( 0 );
      if( !spidrctrl.setPixelConfig( devnr ) )
	error_out( "###setPixelConfig" );

      // Read back configuration into config matrix 1
      spidrctrl.selectPixelConfig( 1 );
      if( !spidrctrl.getPixelConfig( devnr ) )
	error_out( "###getPixelConfig" );

      // Compare matrix 0 and 1
      if( spidrctrl.comparePixelConfig( 0, 1 ) == 0 )
	{
	  cout << "const " << hex << (unsigned int) CONST[index]
	       << ": OKAY" << endl;
	}
      else
	{
	  cout << "### const " << hex << (unsigned int) CONST[index]
	       << ": ERROR" << endl;

	  // Display some of the differences found
	  unsigned char *pc0, *pc1;
	  int x, y, cnt = 0;
	  pc0 = spidrctrl.pixelConfig( 0 );
	  pc1 = spidrctrl.pixelConfig( 1 );
	  for( y=0; y<256; ++y )
	    for( x=0; x<256; ++x, ++pc0, ++pc1 )
	      {
		if( *pc0 != *pc1 )
		  {
		    if( cnt < sz+1 )
		      {
			cout << "  y,x=" << dec << y << "," << x
			     << ": got " << hex
			     << (unsigned int) *pc1 << " expected "
			     << (unsigned int) *pc0 << endl;
		      }
		    ++cnt;
		  }
	      }
	  cout << "  Found " << dec << cnt << " different bytes" << endl;
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
      //usage();
      exit( 0 );
    }
  return qaddr.toIPv4Address();
}

// ----------------------------------------------------------------------------

void usage( char *appname )
{
  cout << endl << "Usage: "
       << appname << " <ipaddr> [<devnr>]" << endl
       << "   Run a pixel configuration matrix test on a Timepix3 device"
       << endl
       << "   with the given device index number <devnr> (default 0)"
       << endl
       << "   on the SPIDR module with the given address (port number 50000)."
       << endl
       << "   A walking-1 and walking-0 pattern is applied to each pixel's "
       << endl
       << "   configuration (6-bits significant) in the matrix."
       << endl;
}

// ----------------------------------------------------------------------------
