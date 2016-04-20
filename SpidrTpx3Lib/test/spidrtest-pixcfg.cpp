#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <iomanip>
using namespace std;

#include <QString>

#include "SpidrController.h"
#include "tpx3defs.h"

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

static string time_str();

quint32 get_addr_and_port(const char *str, int *portnr);
void usage();

// Test of pixel configuration functions
// ======================================

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;
  int     device_nr = 0;

  // Check argument count
  if( !(argc == 2 || argc == 3) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port(argv[1], &portnr);

  if( argc == 3 )
    {
      bool ok;
      device_nr = QString(argv[2]).toUInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid device-number: " << string(argv[2]) << endl;
	  usage();
	  return 0;
	}
      else if( device_nr > 3 || device_nr < 0 )
	{
	  cout << "### Device-number out-of-range <0-3>" << endl;
	  return 0;
	}
    }

  // ----------------------------------------------------------
  // Open a control connection to the SPIDR module
  // with the given address and port, or -if the latter was not provided-
  // the default port number 50000
  SpidrController spidrctrl((ipaddr >> 24) & 0xFF,
    (ipaddr >> 16) & 0xFF,
    (ipaddr >> 8) & 0xFF,
    (ipaddr >> 0) & 0xFF, portnr);

  // Are we connected ?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
      << spidrctrl.connectionStateString() << ", "
      << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  //int errstat;
  //if( spidrctrl.reset( &errstat ) ) {
  //  cout << "errorstat " << hex << errstat << dec << endl;
  //}

  // Set a configuration in all available configurations
  cout << "pixelconfig count=" << spidrctrl.pixelConfigCount() << endl << endl;
  int x, y, cnf;
  for( cnf=0; cnf<spidrctrl.pixelConfigCount(); ++cnf )
    {
      spidrctrl.selectPixelConfig( cnf );
      for( y=10; y<250; ++y )
	for( x=0; x<255; ++x )
	  {
	    spidrctrl.setPixelTestEna( x, y );
	    if( x == 34 || x == 37 ) spidrctrl.setPixelThreshold( x, y, 5 );
	  }
    }
  // Change configuration #3
  spidrctrl.selectPixelConfig( 3 );
  spidrctrl.setPixelMask( 130, 131 );
  spidrctrl.setPixelMask( 213, 222 );

  // Compare configurations
  cout << "Modified pixconf #3:" << endl;
  cout << "0+1: " << spidrctrl.comparePixelConfig( 0, 1 ) << endl;
  cout << "1+2: " << spidrctrl.comparePixelConfig( 1, 2 ) << endl;
  cout << "2+3: " << spidrctrl.comparePixelConfig( 2, 3 ) << endl << endl;

  // Change configuration #0
  spidrctrl.selectPixelConfig( 0 );
  spidrctrl.setPixelMask( 130, 131 );
  spidrctrl.setPixelMask( 213, 222 );
  spidrctrl.setPixelTestEna( 0, 0 );

  // Compare configurations
  cout << "Modified pixconf #0:" << endl;
  cout << "0+1: " << spidrctrl.comparePixelConfig( 0, 1 ) << endl;
  cout << "1+2: " << spidrctrl.comparePixelConfig( 1, 2 ) << endl;
  cout << "2+3: " << spidrctrl.comparePixelConfig( 2, 3 ) << endl << endl;

  spidrctrl.resetPixels( device_nr ); // Essential ! (or nothing can be read)

  // Upload pixel configuration #0 to selected chip
  cout << "setPixCfg #0 start:" << time_str() << endl;
  spidrctrl.selectPixelConfig( 0 );
  //if( !spidrctrl.setPixelConfig( device_nr, true ) )// Preformatted 6-bit cfg
  if( !spidrctrl.setPixelConfig( device_nr, false ) ) // 8-bit cfg 
    error_out( "###setPixelConfig" );
  cout << "setPixCfg    stop :" << time_str() << endl;
  //return 0;

  cout << "Download pixel configuration from chip into configuration #1,"
       << endl << "so now config #0 should be equal to #1, "
       << "and #1 no longer equal to #2" << endl;
  spidrctrl.selectPixelConfig( 1 );
  if( !spidrctrl.getPixelConfig( device_nr ) )
    error_out( "###getPixelConfig" );
  cout << "0+1: " << spidrctrl.comparePixelConfig( 0, 1 ) << endl;
  cout << "1+2: " << spidrctrl.comparePixelConfig( 1, 2 ) << endl;
  cout << "2+3: " << spidrctrl.comparePixelConfig( 2, 3 ) << endl;

  return 0;
}

// ----------------------------------------------------------------------------

static string time_str()
{
  ostringstream oss;
#ifdef WIN32
  // Using Windows-specific 'GetLocalTime()' function for time
  SYSTEMTIME st;
  GetLocalTime( &st );
  oss << setfill('0')
      << setw(2) << st.wHour << ":"
      << setw(2) << st.wMinute << ":"
      << setw(2) << st.wSecond << ":"
      << setw(3) << st.wMilliseconds << " ";
#else
  struct timeval tv;
  gettimeofday( &tv, 0 );
  struct tm tim;
  localtime_r( &tv.tv_sec, &tim );
  oss << setfill('0')
      << setw(2) << tim.tm_hour << ":"
      << setw(2) << tim.tm_min << ":"
      << setw(2) << tim.tm_sec << ":"
      << setw(3) << tv.tv_usec/1000 << " ";
#endif // WIN32
  return oss.str();
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrtest-pixcfg <ipaddr>[:<portnr>] [devnr]\n";
}

// ----------------------------------------------------------------------------
