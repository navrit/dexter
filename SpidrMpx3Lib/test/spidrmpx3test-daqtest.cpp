#include <windows.h>
#include <iostream>
#include <iomanip>
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

  int devcnt = 0;
  int devids[4];
  spidrcontrol.getDeviceCount( &devcnt );
  spidrcontrol.getDeviceIds( devids );
  cout << "Medipix3 IDs: " << hex << setfill('0');
  for( int i=0; i<devcnt; ++i )
    cout << setw(8) << devids[i] << " ";
  cout << dec << setfill(' ') << endl;

  // Find first available device
  int devnr = 0;
  for( int i=0; i<devcnt; ++i )
    if( devids[i] != 0 )
      {
	devnr = i;
	break;
      }
  cout << "==> Using device number " << devnr << endl;

  // Adjust readout mask
  /*
  int mask = 0;
  cout << "(acq mask 0x" << hex;
  if( spidrcontrol.getAcqEnable( &mask ) )
    cout << mask;
  else
    cout << "###";
  spidrcontrol.setAcqEnable( 1<<devnr );
  cout << " set to 0x";
  if( spidrcontrol.getAcqEnable( &mask ) )
    cout << mask;
  else
    cout << "###";
  cout << ")" << dec << endl;
  */

#ifdef USE_SPIDRDAQ
  SpidrDaq spidrdaq( &spidrcontrol );
  cout << "SpidrDaq: ";
  for( int i=0; i<4; ++i )
    cout << spidrdaq.ipAddressString( i ) << " ";
  cout << "#chips: " << spidrdaq.numberOfDevices();
  cout << endl;
  Sleep( 1000 );
  cout << spidrdaq.errorString() << endl;
  spidrdaq.setDecodeFrames( true );

  /*
  if( spidrdaq.openFile( "test.dat", true ) )
    cout << "Opened file" << endl;
  else
    cout << "Failed to open file: " << spidrdaq.errorString() << endl;
  */
#endif

  spidrcontrol.setMaxPacketSize( 9000 ); // Not available on Compact-SPIDR

  int pixdepth = 12;
  spidrcontrol.setPixelDepth( devnr, pixdepth, false );
#ifdef USE_SPIDRDAQ
  spidrdaq.setPixelDepth( pixdepth );
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
      for( col=64; col<66; ++col )
	if( !spidrcontrol.setPixelMaskMpx3rx( col, ALL_PIXELS ) )
	  cout << "### Pixel mask " << col << endl;

      // Set test-bit on a number of pixels
      for( col=63; col<64; ++col )
      //for( col=64; col<128; ++col )
	spidrcontrol.configPixelMpx3rx( col, ALL_PIXELS, 0, 0, true );

#ifdef USE_SPIDRDAQ
      bool read_it_back = true;
      if( read_it_back ) spidrdaq.disableLut( true );
#else
      bool read_it_back = false;
#endif

      // Upload the pixel configuration; read/don't read it back
      if( !spidrcontrol.setPixelConfigMpx3rx( devnr, read_it_back ) )
	cout << "### Pixel config: " << spidrcontrol.errorString() << endl;

#ifdef USE_SPIDRDAQ
      if( read_it_back )
	{
	  // Expect to see the pixelconfiguration returned in a frame..
	  if( spidrdaq.hasFrame( 100 ) )
	    {
	      int size;
	      int *data = spidrdaq.frameData( 0, &size );
	      cout << "Received pixconf frame, size=" << size << endl;
	      cout << hex;
	      // Display pixels with configuration != 0
	      int row, col;
	      for( row=0; row<MPX_PIXEL_ROWS; ++row )
		for( col=0; col<MPX_PIXEL_ROWS; ++col )
		  {
		    if( *data != 0 )
		      cout << col << "," << row << ": " << *data << endl;
		    ++data;
		  }
	      cout << dec;
	      spidrdaq.releaseFrame();
	    }
	  else
	    {
	      cout << "### Pixconf frame not received" << endl;
	    }
	  spidrdaq.disableLut( false );
	}
#endif // USE_SPIDRDAQ
    }
  else
    {
      cout << "### No device type, no pixel configuration upload" << endl;
    }
#endif // USE_THIS

  //spidrcontrol.setPixelDepth( devnr, pixdepth, true );
  spidrcontrol.setPixelDepth( devnr, pixdepth, false );

  /*
  // DACs
  cout << "Before" << endl;
  int dacnr, dacval;
  for( dacnr=0; dacnr<30; ++dacnr )
    {
      spidrcontrol.getDac( devnr, dacnr, &dacval );
      cout << dacnr << ": " << dacval << endl;
    }
  spidrcontrol.setDac( devnr, 10, 10 );
  //spidrcontrol.setDacsDflt( devnr );
  cout << "After" << endl;
  for( dacnr=0; dacnr<30; ++dacnr )
    {
      spidrcontrol.getDac( devnr, dacnr, &dacval );
      cout << dacnr << ": " << dacval << endl;
    }
  */

  int trig_mode = SHUTTERMODE_AUTO; // Auto-trigger mode
  int trig_period_us = 10;
  //int trig_freq_hz = 30000;
  int trig_freq_hz   = 1;
  //int nr_of_triggers = 500;
  int nr_of_triggers = 1;
  int trig_pulse_count;
  spidrcontrol.setShutterTriggerConfig( trig_mode, trig_period_us,
					trig_freq_hz, nr_of_triggers );
  //spidrcontrol.clearBusy();

  char ch;
  int i, frame_cnt = 0;
  //for( i=0; i<10; ++i )
  for( i=0; i<1; ++i )
    {
      cout << "Auto-trig " << i << endl;
      cin >> ch;

      //if( 1 )//i==4 ) spidrcontrol.setDac( devnr, 0, i*51 );
      //spidrcontrol.setMaxPacketSize( 512+i*64 );

      spidrcontrol.startAutoTrigger();
#ifdef USE_SPIDRDAQ

      while( spidrdaq.hasFrame( 500 ) )
	{
	  ++frame_cnt;
	  cout << "counterh=" << spidrdaq.isCounterhFrame() << ", cntr="
	       << spidrdaq.frameShutterCounter() << endl;
	  cout << "DAQ frames: " << setw(3) << frame_cnt
	       << " (" << spidrdaq.framesCount() << ")"
	    //<< ", lost " << spidrdaq.framesLostCount()
	       << ", lost pkts " << spidrdaq.packetsLostCountFrame();
	  if( (frame_cnt & 1) == 0 )
	    {
	      if( !spidrdaq.isCounterhFrame() )
		{
		  cout << " ###";
		  /*return 0;*/
		}
	    }
	  else
	    {
	      if( spidrdaq.isCounterhFrame() )
		{
		  cout << " ***";
		}
	    }
	  cout << endl;
	  spidrdaq.releaseFrame();
	  //if( frame_cnt >= 25 ) spidrcontrol.stopAutoTrigger(); // TEST
	}
#else
      Sleep( 1000 );
#endif
    }
  goto summary;

  cout << "Go for single(0)" << endl;
  cin >> ch;
  spidrcontrol.triggerSingleReadout( 0 );
  cout << "Go for single(1)" << endl;
  cin >> ch;
  spidrcontrol.triggerSingleReadout( 1 );

  cout << "Go for single(0)" << endl;
  cin >> ch;
  spidrcontrol.triggerSingleReadout( 0 );
  cout << "Go for single(1)" << endl;
  cin >> ch;
  spidrcontrol.triggerSingleReadout( 1 );

 summary:

#ifdef USE_SPIDRDAQ
  // DAQ summary:
  cout << "DAQ frames: " << spidrdaq.framesCount() << " (";
  for( i=0; i<4; ++i ) cout << spidrdaq.framesCount(i) << " ";
  cout << ")" << endl;
  cout << " (file: " << spidrdaq.framesWrittenCount()
       << "), lost " << spidrdaq.framesLostCount()
       << ", recvd packets " << spidrdaq.packetsReceivedCount()
       << ", lost pkts " << spidrdaq.packetsLostCount()
       << " (file: "  << spidrdaq.packetsLostCountFile()
       << "), pkt size " << spidrdaq.packetSize( 0 ) << endl;
  cout << "Packets lost/frame (of first device present): ";
  for( i=0; i<8; ++i )
    cout << i << "=" << spidrdaq.packetsLostCountFrame( 0, i ) << ", ";
  cout << endl;

  cout << "DAQ pixels: " << spidrdaq.pixelsReceivedCount()
       << ", lost " << spidrdaq.pixelsLostCount()
       << endl;
  cout << "Pixels lost/frame (of first device present): ";
  for( i=0; i<8; ++i )
    cout << i << "=" << spidrdaq.pixelsLostCountFrame( 0, i ) << ", ";
  cout << endl;

  cin >> ch;
  spidrdaq.stop();
#endif    

  return 0;
}
