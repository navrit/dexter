#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(ms*1000)
#endif

#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include "mpx3defs.h"
#include "mpx3dacsdescr.h"

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
	cout << "### DeviceID " << dev_nr << ": " << spidr.errorString() << endl;
      else
	cout << "DeviceID " << dev_nr << ": " << id << endl;
    }

  // Get the current acquisition-enable mask
  int mask;
  if( !spidr.getAcqEnable( &mask ) )
    cout << "### getAcqEnable: " << spidr.errorString() << endl;
  else
    cout << "getAcqEnable: " << mask << endl;

  dev_nr = 2;

  int i, dac_value;
  // Get the DAC values from Medipix device 0 and display them
  for( i=0; i<MPX3_DAC_COUNT; ++i )
    {
      if( !spidr.getDac( dev_nr, i+1, &dac_value ) )
	cout << "### DAC " << i << ": " << spidr.errorString() << endl;
      else
	cout << "DAC " << i << " ("<< spidr.dacNameMpx3rx( i )
	     << "): " << dac_value << endl;
    }

#define DO_DAC_SCAN
#ifdef DO_DAC_SCAN
  // Perform a DAC scan (on a single DAC..), displaying the ADC values
  int adc_value;
  //dev_nr = 0;
  int dac_code = MPX3_DAC_DISC;
  if( !spidr.setSenseDac( dev_nr, dac_code ) )
    cout << "### SenseDAC " << dac_code << ": " << spidr.errorString() << endl;
  else
    cout << "SenseDAC " << dac_code << endl;
  for( dac_value=0; dac_value<=spidr.dacMaxMpx3(dac_code); ++dac_value )
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
      if( !spidr.setPixelConfigMpx3rx( dev_nr ) )
	cout << "### Pixel config: " << spidr.errorString() << endl;
      else
	cout << "Pixel config uploaded" << endl;
    }
  else
    {
      cout << "### No device type, no pixel configuration upload" << endl;
    }

  // Configure the trigger, then generate some triggers
  // (there is no SpidrDaq object, so look at the frames e.g. with SpidrTV
  int trig_mode      = 4;
  int trig_period_us = 100000; // 100 ms
  int trig_freq_hz   = 3;
  int nr_of_triggers = 2;
  spidr.setShutterTriggerConfig( trig_mode, trig_period_us,
			  trig_freq_hz, nr_of_triggers );
  spidr.clearBusy();
  for( i=0; i<5; ++i )
    {
      cout << "Auto-trig " << i << endl;
      spidr.startAutoTrigger();
      Sleep( 2000 );
    }

  return 0;
}
