#include <windows.h>
#include <iostream>
#include <iomanip>
using namespace std;

#include <QString>

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "mpx3defs.h"

//#define USE_SPIDRDAQ

quint32 get_addr_and_port(const char *str, int *portnr);
void usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;

  // Check argument count
  if( !(argc == 2) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port(argv[1], &portnr);

  // ----------------------------------------------------------
  // Open a control connection to the SPIDR module
  // with the given address and port, or -if the latter was not provided-
  // the default port number 50000
  SpidrController spidrcontrol((ipaddr >> 24) & 0xFF,
    (ipaddr >> 16) & 0xFF,
    (ipaddr >> 8) & 0xFF,
    (ipaddr >> 0) & 0xFF, portnr);

  // Are we connected ?
  if( !spidrcontrol.isConnected() ) {
    cout << spidrcontrol.ipAddressString() << ": "
      << spidrcontrol.connectionStateString() << ", "
      << spidrcontrol.connectionErrString() << endl;
    return 1;
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

  // Get current read-out mask
  int mask = 0;
  cout << "(acq mask 0x" << hex;
  if( spidrcontrol.getAcqEnable( &mask ) )
    cout << mask;
  else
    cout << "###";
  /*
  // Adjust read-out mask, if necessary
  spidrcontrol.setAcqEnable( 1<<devnr );
  cout << " set to 0x";
  if( spidrcontrol.getAcqEnable( &mask ) )
    cout << mask;
  else
    cout << "###";
  */
  cout << ")" << dec << endl;

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

  int  pixdepth = 12;
  bool two_counter_readout = true;
  spidrcontrol.setPixelDepth( devnr, pixdepth, two_counter_readout );
#ifdef USE_SPIDRDAQ
  spidrdaq.setPixelDepth( pixdepth );
#endif

#define USE_PIXELCONFIG
#ifdef USE_PIXELCONFIG
  // Create a (new) pixel configuration (for a Medipix3 device)
  int devtype = MPX_TYPE_NC;
  spidrcontrol.getDeviceType( devnr, &devtype );
  spidrcontrol.resetPixelConfig();
  int col, row;
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
      bool testbit = true;
      //for( col=63; col<64; ++col )
      //for( col=2; col<3; ++col )
      for( col=64; col<128; ++col )
	{
	  spidrcontrol.configPixelMpx3rx( col, ALL_PIXELS, 0, 0, testbit );
	  if( testbit )
	    spidrcontrol.configCtpr( devnr, col, 1 );
	}

      /*
      for( col=63; col<64; ++col )
	for( row=0; row<MPX_PIXEL_ROWS; ++row )
	  if( row != 0x10 )
	    spidrcontrol.configPixelMpx3rx( col, row, 0, (row&31) );
      */
      //spidrcontrol.configPixelMpx3rx( 4, 5, 31, 6, false );
      //spidrcontrol.setPixelMaskMpx3rx( 4, 5 );
      //spidrcontrol.setPixelMaskMpx3rx( 5, 6 );

#ifdef USE_SPIDRDAQ
      //bool read_it_back = true;
      bool read_it_back = false;
      if( read_it_back ) spidrdaq.disableLut( true );
#else
      bool read_it_back = false;
#endif

      // Upload the test pulse configuration
      if( !spidrcontrol.setCtpr( devnr ) )
	cout << "### CTPR config: " << spidrcontrol.errorString() << endl;

      // Upload the pixel configuration; optionally read it back
      if( !spidrcontrol.setPixelConfigMpx3rx( devnr, read_it_back ) )
	cout << "### Pixel config: " << spidrcontrol.errorString() << endl;

#ifdef USE_SPIDRDAQ
      if( read_it_back )
	{
	  // Expect to see the pixelconfiguration returned in a frame..
	  if( spidrdaq.hasFrame( 100 ) )
	    {
	      int size, lost_count;
	      int *data = spidrdaq.frameData( 0, &size, &lost_count );
	      cout << "Received pixconf frame, size=" << size
		   << " lost=" << lost_count << endl;
	      cout << hex;
	      // Display pixels with configuration != 0
	      int row, col;
	      for( row=0; row<MPX_PIXEL_ROWS; ++row )
		for( col=0; col<MPX_PIXEL_ROWS; ++col )
		  {
		    if( *data != 0 )
		      cout << setw(2) << col << ","
			   << setw(2) << row << ": " << *data << endl;
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
#endif // USE_PIXELCONFIG

  spidrcontrol.setPixelDepth( devnr, pixdepth, two_counter_readout );

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
  int trig_period_us = 1000;
  //int trig_freq_hz = 30000;
  int trig_freq_hz   = 10;
  //int nr_of_triggers = 500;
  int nr_of_triggers = 1;
  int trig_pulse_count;
  spidrcontrol.setShutterTriggerConfig( trig_mode, trig_period_us,
					trig_freq_hz, nr_of_triggers );
  //spidrcontrol.clearBusy();

  char ch;
  int i, frame_cnt = 0;
  for( i=0; i<100; ++i )
  //for( i=0; i<1; ++i )
    {
      cout << "Auto-trig " << i << endl;
      //cin >> ch;

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
	       << ", lost pkts/pixels " << spidrdaq.lostCountFrame();
	  if( two_counter_readout && (frame_cnt & 1) == 0 )
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
      Sleep( 500 );
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
       << ", lost pkts/pix " << spidrdaq.lostCount()
       << " (file: "  << spidrdaq.lostCountFile()
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

  //cin >> ch;
  spidrdaq.stop();
#endif    

  return 0;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrmpx3test-daq <ipaddr>[:<portnr>]\n";
}

// ----------------------------------------------------------------------------
