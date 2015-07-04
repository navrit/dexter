#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#define Sleep(ms) usleep(ms*1000)
#endif

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include "mpx3defs.h"
#include "mpx3dacsdescr.h"

static string time_str();

int main( int argc, char *argv[] )
{
  // Open a control connection to SPIDR module with address 192.168.1.10,
  // port 50000 (default)
  SpidrController spidr( 192, 168, 1, 10 );

  // Check the connection to the SPIDR module
  if( !spidr.isConnected() )
    {
      // No ?
      cout << spidr.connectionStateString() << ": "
           << spidr.connectionErrString() << endl;
      return 1;
    }
  else
    {
      cout << "Connected to SPIDR: " << spidr.ipAddressString() <<  endl;
    }

  // Get version numbers
  cout << "SpidrController class: "
       << spidr.versionToString( spidr.classVersion() ) << endl;
  int version;
  if( spidr.getFirmwVersion( &version ) )
    cout << "SPIDR firmware  : " << spidr.versionToString( version ) << endl;
  if( spidr.getSoftwVersion( &version ) )
    cout << "SPIDR software  : " << spidr.versionToString( version ) << endl;

  // Get the Medipix device chip-identifiers from the module and display them
  int dev_nr, id;
  for( dev_nr=0; dev_nr<4; ++dev_nr )
    {
      if( !spidr.getDeviceId( dev_nr, &id ) )
	cout << "### DeviceID " << dev_nr << ": "
	     << spidr.errorString() << endl;
      else
	cout << "DeviceID " << dev_nr << hex << ": 0x" << id << dec << endl;
    }

  // Get the current acquisition-enable mask
  int mask;
  if( !spidr.getAcqEnable( &mask ) )
    cout << "### getAcqEnable: " << spidr.errorString() << endl;
  else
    cout << "getAcqEnable: " << mask << endl;

  dev_nr = 2;

  int i, dac_code, dac_value;
  // Get the DAC values from Medipix3RX device 'dev_nr' and display them
  for( i=0; i<MPX3RX_DAC_COUNT; ++i )
    {
      dac_code = MPX3RX_DAC_TABLE[i].code;
      if( !spidr.getDac( dev_nr, dac_code, &dac_value ) )
	cout << "### DAC " << i << ": " << spidr.errorString() << endl;
      else
	cout << "DAC " << i << " (" << spidr.dacNameMpx3rx( dac_code )
	     << "): " << dac_value << endl;
    }

//#define DO_DAC_SCAN
#ifdef DO_DAC_SCAN
  // Perform a DAC scan (on a single DAC..), displaying the ADC values
  int adc_value;
  //dev_nr = 0;
  int dac_code = MPX3RX_DAC_DISC;
  if( !spidr.setSenseDac( dev_nr, dac_code ) )
    cout << "### SenseDAC " << dac_code << ": " << spidr.errorString() << endl;
  else
    cout << "SenseDAC " << dac_code << endl;
  for( dac_value=0; dac_value<=spidr.dacMaxMpx3rx(dac_code); ++dac_value )
    {
      if( !spidr.setDac( dev_nr, dac_code, dac_value ) )
	cout << "### setDac: " << spidr.errorString() << endl;

      if( !spidr.getAdc( dev_nr, &adc_value ) )
	cout << "### getAdc: " << spidr.errorString() << endl;
      else
	cout << setw(3) << dac_value << ": " << adc_value << endl;
    }
  return 0;
#endif

  int depth = 6;
  if( !spidr.setPixelDepth( dev_nr, depth, true ) )
  //if( !spidr.setPixelDepth( dev_nr, depth, false ) )
    cout << "### setPixelDepth: " << spidr.errorString() << endl;
  else
    cout << "setPixelDepth: " << depth << endl;

  // Create a (new) pixel configuration (for a Medipix3 device)
  //dev_nr = 0;
  int devtype = MPX_TYPE_NC;
  spidr.getDeviceType( dev_nr, &devtype );
  spidr.resetPixelConfig();
  int col;
  // Mask a number of pixel columns...
  if( devtype == MPX_TYPE_MPX31 )
    {
      cout << "MPX31 pixel config" << endl;
      // Mask a number of pixel columns...
      for( col=128; col<192; ++col )
	if( !spidr.setPixelMaskMpx3( col, ALL_PIXELS ) )
	  cout << "### Pixel mask " << col << endl;
      // Upload the pixel configuration
      if( !spidr.setPixelConfigMpx3( dev_nr ) )
	cout << "### Pixel config: " << spidr.errorString() << endl;
      else
	cout << "Pixel config uploaded" << endl;
    }
  else if( devtype == MPX_TYPE_MPX3RX )
    {
      cout << "MPX3RX pixel config" << endl;
      // Mask a number of pixel columns...
      for( col=64; col<92; ++col )
	if( !spidr.setPixelMaskMpx3rx( col, ALL_PIXELS ) )
	  cout << "### Pixel mask " << col << endl;
      // Upload the pixel configuration
      cout << "setPixCfg start:" << time_str() << endl;
      if( !spidr.setPixelConfigMpx3rx( dev_nr ) )
	cout << "### Pixel config: " << spidr.errorString() << endl;
      else
	cout << "Pixel config uploaded" << endl;
      cout << "setPixCfg stop :" << time_str() << endl;
    }
  else
    {
      cout << "### No device type, no pixel configuration upload" << endl;
    }

  // Configure the trigger, then generate some triggers
  // (there is no SpidrDaq object here, so look at the frames e.g. with SpidrTV
  int trig_mode      = SHUTTERMODE_AUTO;
  int trig_period_us = 100000; // 100 ms
  int trig_freq_hz   = 3;
  int nr_of_triggers = 1;
  spidr.setShutterTriggerConfig( trig_mode, trig_period_us,
				 trig_freq_hz, nr_of_triggers );
  spidr.clearBusy();
  for( i=0; i<5; ++i )
    {
      cout << "Auto-trig " << i << endl;
      spidr.startAutoTrigger();
      Sleep( 500 );
    }

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
