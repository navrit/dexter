#include <windows.h>
#include <iostream>
using namespace std;

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "mpx3defs.h"

#define USE_SPIDRDAQ

int main( int argc, char *argv[] )
{
  // Open a control connection to SPIDR module with address 192.168.1.10,
  // port 50000 (default)
  SpidrController spidrcontrol( 192, 168, 1, 10 );

  // Check the connection to the SPIDR module
  if( !spidrcontrol.isConnected() )
    {
      // No ?
      cout << spidrcontrol.connectionStateString() << ": "
           << spidrcontrol.connectionErrString() << endl;
      return 1;
    }
  else
    {
      cout << "Connected to SPIDR: "
	   << spidrcontrol.ipAddressString() <<  endl;
    }

  int devnr = 2;

#ifdef USE_SPIDRDAQ
  SpidrDaq spidrdaq( &spidrcontrol );
  cout << "SpidrDaq: ";
  for( int i=0; i<4; ++i ) cout << spidrdaq.ipAddressString( i ) << " ";
  cout << endl;
  Sleep( 1000 );
  cout << spidrdaq.errorString() << endl;

  if( spidrdaq.openFile( "test.dat", true ) )
    cout << "Opened file" << endl;
  else
    cout << "Failed to open file: " << spidrdaq.errorString() << endl;
#endif

#define USE_THIS
#ifdef USE_THIS
  // Create a (new) pixel configuration (for a Medipix3 device)
  int devtype = MPX_TYPE_NC;
  spidrcontrol.getDeviceType( devnr, &devtype );
  spidrcontrol.resetPixelConfig();
  int col;
  if( devtype == MPX_TYPE_MPX31 )
    {
      cout << "MPX31 pixel config" << endl;
      // Mask a number of pixel columns...
      for( col=32; col<64; ++col )
	if( !spidrcontrol.setPixelMaskMpx3( col, ALL_PIXELS ) )
	  cout << "### Pixel mask " << col << endl;
      // Upload the pixel configuration
      if( !spidrcontrol.setPixelConfigMpx3( devnr, false ) )
	cout << "### Pixel config: " << spidrcontrol.errorString() << endl;
    }
  else if( devtype == MPX_TYPE_MPX3RX )
    {
      cout << "MPX3RX pixel config" << endl;
      // Mask a number of pixel columns...
      for( col=64; col<92; ++col )
	if( !spidrcontrol.setPixelMaskMpx3rx( col, ALL_PIXELS ) )
	  cout << "### Pixel mask " << col << endl;
      // Upload the pixel configuration
      if( !spidrcontrol.setPixelConfigMpx3rx( devnr, false ) )
	cout << "### Pixel config: " << spidrcontrol.errorString() << endl;
    }
  else
    {
      cout << "### No device type, no pixel configuration upload" << endl;
    }
#endif

  int depth = 12;
  spidrcontrol.setPixelDepth( devnr, depth, false );
#ifdef USE_SPIDRDAQ
  spidrdaq.setPixelDepth( depth );
#endif
  spidrcontrol.setMaxPacketSize( 8000 );

  /*
  // DACs
  cout << "Before" << endl;
  int dacnr, dacval;
  for( dacnr=0; dacnr<30; ++dacnr )
    {
      spidrcontrol.getDac( devnr, dacnr, &dacval );
      cout << dacnr << ": " << dacval << endl;
    }
  spidrcontrol.writeDacs( devnr );
  //spidrcontrol.writeDacsDflt( devnr );
  cout << "After" << endl;
  for( dacnr=0; dacnr<30; ++dacnr )
    {
      spidrcontrol.getDac( devnr, dacnr, &dacval );
      cout << dacnr << ": " << dacval << endl;
    }
  */

  int trig_mode      = SHUTTERMODE_AUTO; // Auto-trigger mode
  int trig_period_us = 100000; // 100 ms
  int trig_freq_hz   = 5;
  int nr_of_triggers = 2;
  int trig_pulse_count;
  spidrcontrol.setShutterTriggerConfig( trig_mode, trig_period_us,
					trig_freq_hz, nr_of_triggers );
  spidrcontrol.clearBusy();
  int i;
  for( i=0; i<10; ++i )
    {
      cout << "Auto-trig " << i << endl;
      if( 1 )//i==4 )
	{
	  spidrcontrol.setDac( devnr, 0, i*51 );
	}
      spidrcontrol.startAutoTrigger();
      Sleep( 1000 );
#ifdef USE_SPIDRDAQ
      cout << "DAQ frames: " << spidrdaq.framesCount() << ", lost "
	   << spidrdaq.framesLostCount() << ", lost pkts "
	   << spidrdaq.packetsLostCount() << endl;
#endif
    }

#ifdef USE_SPIDRDAQ
  cout << "DAQ frames: " << spidrdaq.framesCount() << " (file: "
       << spidrdaq.framesWrittenCount() << "), lost "
       << spidrdaq.framesLostCount() << ", lost pkts "
       << spidrdaq.packetsLostCount() << " (file: "
       << spidrdaq.packetsLostCountFile() << "), pkt size "
       << spidrdaq.packetSize( 0 ) << endl;
  cout << "Lost/frame: ";
  for( i=0; i<8; ++i )
    cout << i << "=" << spidrdaq.packetsLostCountFrame( 0, i ) << ", ";
  cout << endl;

  spidrdaq.stop();
#endif    

  return 0;
}
