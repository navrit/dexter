#include <windows.h>
#include <iostream>
using namespace std;

#include "SpidrControl.h"
#include "dacsdefs.h"

int main( int argc, char *argv[] )
{
  SpidrControl spidr( 192, 168, 1, 10 );

  // Check if we are properly connected to the SPIDR module
  if( spidr.isConnected() )
    {
      cout << "Connected to SPIDR: " << spidr.ipAddressString() <<  endl;
    }
  else
    {
      cout << spidr.connectionStateString() << ": "
	   << spidr.connectionErrString() << endl;
      return 1;
    }

  // Get version numbers
  cout << "SpidrControl class: "
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
	cout << "### DeviceID " << dev_nr << ": " << spidr.errString() << endl;
      else
	cout << "DeviceID " << dev_nr << ": " << id << endl;
    }

  // Get the current acquisition-enable mask
  int mask;
  if( !spidr.getAcqEnable( &mask ) )
    cout << "### getAcqEnable: " << spidr.errString() << endl;
  else
    cout << "getAcqEnable: " << mask << endl;

  int dac_nr, dac_value;
#ifdef DO_DAC_READ
  // Get the DAC values from Medipix device 0 and display them
  if( !spidr.readDacs( dev_nr ) )
    cout << "### readDacs: " << spidr.errString() << endl;
  dev_nr = 0;
  for( dac_nr=0; dac_nr<MPX3_DAC_COUNT; ++dac_nr )
    {
      if( !spidr.getDac( dev_nr, dac_nr, &dac_value ) )
	cout << "### DAC " << dac_nr << ": " << spidr.errString() << endl;
      else
	cout << "DAC " << dac_nr << " ("<< spidr.dacNameMpx3( dac_nr )
	     << "): " << dac_value << endl;
    }
#endif

#ifdef DO_DAC_SCAN
  // Perform a DAC scan (on a single DAC..), displaying the ADC values
  int adc_value;
  dev_nr = 0;
  dac_nr = MPX3_DAC_DISC;
  if( !spidr.setSenseDac( dac_nr ) )
    cout << "### SenseDAC " << dac_nr << ": " << spidr.errString() << endl;
  else
    cout << "SenseDAC " << dac_nr << endl;
  for( dac_value=0; dac_value<=spidr.dacMaxMpx3(dac_nr); ++dac_value )
    {
      if( !spidr.setDac( dev_nr, dac_nr, dac_value ) )
	cout << "### setDac: " << spidr.errString() << endl;

      if( !spidr.writeDacs( dev_nr ) )
	cout << "### writeDacs: " << spidr.errString() << endl;

      if( !spidr.getAdc( dev_nr, &adc_value ) )
	cout << "### getAdc: " << spidr.errString() << endl;
      else
	cout << dac_value << ": " << adc_value << endl;
    }
  return 0;
#endif

  //dac_nr = MPX3_DAC_THRESH_0;
  //spidr.setDac( dev_nr, dac_nr, 0 );
  //spidr.writeDacs( dev_nr );

  // Create a (new) pixel configuration (for a Medipix3 device)
  spidr.resetPixelConfig();
  dev_nr = 0;
  int col;
  // Mask a number of pixel columns...
  for( col=128; col<192; ++col )
    if( !spidr.maskPixelMpx3( col, ALL_PIXELS ) )
      cout << "### Pixel mask " << col << endl;
  // Upload the pixel configuration
  if( !spidr.writePixelConfigMpx3( dev_nr ) )
    cout << "### Pixel config: " << spidr.errString() << endl;

  int i;
  spidr.configTrigger( 4, 100000, 3, 2 );
  for( i=0; i<10; ++i )
    {
      cout << "Auto-trig " << i << endl;
      spidr.startAutoTrigger();
      Sleep( 2000 );
    }

#ifdef TEST_
  trig_mode = 0;
  trig_period_us = 0;
  spidrcontrol.getTriggerConfig( &trig_mode, &trig_period_us,
				 &trig_freq_hz, &nr_of_triggers,
				 &trig_pulse_count );
  cout << "mode=" << trig_mode << ", period=" << trig_period_us << endl;
  return 0;
#endif // TEST_

  return 0;
}
