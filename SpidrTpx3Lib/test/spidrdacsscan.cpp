/* ----------------------------------------------------------------------------
File   : spidrdacsscan.cpp

Descr  : Commandline tool to scan Timepix3 (or Medipix3) DACs.

Usage  :
spidrdacsscan <ipaddr>[:<portnr>] [devnr]
   Do a (Timepix3) DACs scan on a SPIDR module.
     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10.
     <portnr> : current SPIDR controller IP port number, default 50000.
     <devnr>  : device index.

History:
21APR2015; HenkB; Created.
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

#include "../SpidrTpx3Lib/tpx3defs.h"
#include "../SpidrMpx3Lib/mpx3defs.h"
#include "../SpidrTpx3Lib/tpx3dacsdescr.h"
#include "../SpidrMpx3Lib/mpx3dacsdescr.h"
#include "SpidrController.h"

#include <QHostAddress>
#include <QString>

#define error_out(str) cout<<str<<": "<<_spidrController->errorString()<<endl

quint32 get_addr_and_port( const char *str, int *portnr );
void    usage();

// ----------------------------------------------------------------------------

SpidrController *_spidrController;

// Scan parameters
int  _deviceIndex;
int  _dacCount;
int  _dacIndex;
int  _dacCode;
int  _dacMax;
int  _dacVal;
int  _dacStep;
int  _samples;
const struct dac_s *_dacTable;

// SPIDR ADC parameters
double _adcFullScale;
double _adcRange;

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;
  int     devnr;

  // Check argument count
  if( !(argc == 3) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

  bool ok;
  devnr = QString(argv[2]).toUInt( &ok );
  if( !ok )
    {
      cout << "### Invalid device-number: " << string(argv[2]) << endl;
      usage();
      return 0;
    }
  else if( devnr >= 3 || devnr < 0 )
    {
      cout << "### Device-number out-of-range <0-3>" << endl;
      return 0;
    }

  // Open a control connection to SPIDR-TPX3 module
  // with the given address and default port
  _spidrController = new SpidrController( (ipaddr>>24) & 0xFF,
					  (ipaddr>>16) & 0xFF,
					  (ipaddr>> 8) & 0xFF,
					  (ipaddr>> 0) & 0xFF, portnr );

  // Are we connected to the SPIDR-TPX3 module?
  if( !_spidrController->isConnected() ) {
    cout << _spidrController->ipAddressString() << ": "
         << _spidrController->connectionStateString() << ", "
         << _spidrController->connectionErrString() << endl;
    return 1;
  }

  _deviceIndex  = devnr;
  _dacCount     = TPX3_DAC_COUNT_TO_SET;
  _dacIndex     = 0;
  _dacCode      = TPX3_SENSEOFF;
  _dacMax       = 0;
  _dacVal       = 0;
  _dacStep      = 1;
  _samples      = 1;
  _dacTable     = &TPX3_DAC_TABLE[0];
  _adcFullScale = 1.5;
  _adcRange     = 4096.0; // 12-bit DAC
  _spidrController->setLogLevel( 2 ); // Limit amount of SPIDR console output

  cout << "DACs Scan: step=" << _dacStep
       << ", samples/step=" << _samples << endl;

  cout << fixed << setprecision( 2 );

  // For every DAC...
  int adc_val, dflt_val;
  int cnt;
  for( _dacIndex=0; _dacIndex<_dacCount; ++_dacIndex )
    {
      // Next DAC index
      _dacCode = _dacTable[_dacIndex].code;
      _dacMax  = (1 << _dacTable[_dacIndex].bits) - 1;
      cout << endl << "DAC " << _dacCode
	   << " " << _dacTable[_dacIndex].name
	   << ", max=" << _dacMax << endl;

      // Configure Timepix/Medipix device to output the output of this DAC
      if( !_spidrController->setSenseDac( _deviceIndex, _dacCode ) )
	{
	  error_out( "setSenseDac()" );
	  break;
	}

      // Scan the DAC's range while taking output readings
      cnt = 0;
      for( _dacVal=0; _dacVal<=_dacMax; _dacVal+=_dacStep, ++cnt )
	{
	  if( !_spidrController->setDac( _deviceIndex, _dacCode, _dacVal ) )
	    {
	      error_out( "setDac()" );
	      break;
	    }

	  if( !_spidrController->getDacOut( _deviceIndex, &adc_val, _samples ) )
	    {
	      error_out( "getDacOut()" );
	      break;
	    }
	  else
	    {
	      adc_val /= _samples;
	      double adc_volt = ((double) adc_val * _adcFullScale) / _adcRange;

	      cout << setw(5) << adc_volt << " ";

	      if( (cnt & 0xF) == 0xF ) cout << endl;
	    }
	}

      // Set this DAC to its default setting...
      dflt_val = _dacTable[_dacIndex].dflt;
      if( !_spidrController->setDac( _deviceIndex, _dacCode, dflt_val ) )
	{
	  error_out( "setDac(dflt)" );
	  break;
	}
    }

  if( !_spidrController->setSenseDac( _deviceIndex, TPX3_SENSEOFF ) )
    error_out( "setSenseDac(SENSEOFF)" );

  _spidrController->setLogLevel( 0 ); // Restore SPIDR console output mode

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
       << "spidrdacsscan <ipaddr>[:<portnr>] [devnr]"
       << endl
       << "   Do a (Timepix3) DACs scan on a SPIDR module."
       << endl
       << "     <ipaddr> : current SPIDR IP address, e.g. 192.168.100.10."
       << endl
       << "     <portnr> : current SPIDR controller IP port number, "
       << "default 50000."
       << endl
       << "     <devnr>  : device index."
       << endl;
}

// ----------------------------------------------------------------------------
