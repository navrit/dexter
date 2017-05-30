#include <windows.h>
#include <iostream>
#include <iomanip>
using namespace std;

#include <QString>

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "mpx3defs.h"

//#define USE_SPIDRDAQ
//#define USE_PIXELCONFIG
//#define DEBUGGING_IT

quint32 get_addr_and_port(const char *str, int *portnr);
int     my_atoi(const char *c);
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr = 0;
  int     portnr = 50000;
  int     freq_hz= 5;

  // Check argument count
  if( !(argc >= 2) )
    {
      usage();
      return 0;
    }

  ipaddr = get_addr_and_port( argv[1], &portnr );

  if( argc >= 3 )
    freq_hz = my_atoi( argv[2] );

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

  // ----------------------------------------------------------
  // Get current read-out mask
  int mask = 0;
  cout << "(acq mask 0x" << hex;
  if( spidrcontrol.getAcqEnable(&mask) )
    cout << mask;
  else
    cout << "###";
  cout << ")" << dec << endl;

  // Find first available device
  int devnr = 0;
  for( int i=0; i<devcnt; ++i )
    if( devids[i] != 0 && (mask & (1<<i)) )
      {
	devnr = i;
	break;
      }
  cout << "==> Using device number " << devnr << endl;

  // ----------------------------------------------------------
#ifdef USE_SPIDRDAQ
  // Added readout mask to constructor (7 Apr 2016)
  SpidrDaq spidrdaq( &spidrcontrol );
  cout << "SpidrDaq: ";
  for( int i=0; i<4; ++i )
    cout << spidrdaq.ipAddressString( i ) << " ";
  cout << "#chips: " << spidrdaq.numberOfDevices() << " acq mask: ";
  if( spidrcontrol.getAcqEnable(&mask) )
    cout << mask;
  else
    cout << "###";
  cout << endl;
  Sleep( 1000 );
  cout << spidrdaq.errorString() << endl;
  spidrdaq.setDecodeFrames( true );
#endif

  // ----------------------------------------------------------
  //spidrcontrol.setMaxPacketSize( 9000 ); // Not available on Compact-SPIDR

  int  pixdepth = 12;
  bool two_counter_readout = false;

  // DO AFTER PIXEL CONFIGURATION! :
  //spidrcontrol.setPixelDepth(devnr, pixdepth, two_counter_readout);
#ifdef USE_SPIDRDAQ
  spidrdaq.setPixelDepth( pixdepth );
#endif

#ifdef USE_PIXELCONFIG
  // Create a (new) pixel configuration (for a Medipix3 device)
  int devtype = MPX_TYPE_NC;
  spidrcontrol.getDeviceType( devnr, &devtype );
  if( devtype != MPX_TYPE_MPX3RX )
    {
      cout << "###Dev-type " << devtype << ", not supported" << endl;
      return 1;
    }
  cout << "MPX3RX pixel config" << endl;

  spidrcontrol.resetPixelConfig();

  // Mask a number of pixel rows (columns)...
  int col, row;
  for( col=32; col<40; ++col )
    if( !spidrcontrol.setPixelMaskMpx3rx(col,ALL_PIXELS) )
      cout << "### Pixel mask col " << col << endl;
  for( row = 160; row<180; ++row )
    if( !spidrcontrol.setPixelMaskMpx3rx(ALL_PIXELS, row) )
      cout << "### Pixel mask row " << row << endl;

  for( row = 0; row < 256; ++row ) {
    if( row > 1 ) spidrcontrol.setPixelMaskMpx3rx(row - 2, row);
    if( row > 0 ) spidrcontrol.setPixelMaskMpx3rx(row - 1, row);
    spidrcontrol.setPixelMaskMpx3rx(row, row);
    if( row < 255 ) spidrcontrol.setPixelMaskMpx3rx(row + 1, row);
    if( row < 254 ) spidrcontrol.setPixelMaskMpx3rx(row + 2, row);
  }
	
  /*// Set test-bit on a number of pixels
  bool testbit = true;
  //for( col=63; col<64; ++col )
  //for( col=2; col<3; ++col )
  for( col=64; col<128; ++col )
    {
      spidrcontrol.configPixelMpx3rx( col, ALL_PIXELS, 0, 0, testbit );
      if( testbit )
	spidrcontrol.configCtpr( devnr, 256-col, 1 );
    }*/
  //for( col = 0; col<255; ++col )
  //spidrcontrol.configCtpr( devnr, col, 1 );

  // Upload the test pulse configuration
  //if( !spidrcontrol.setCtpr( devnr ) )
    //cout << "### CTPR config: " << spidrcontrol.errorString() << endl;

  // Upload the pixel configuration
  if( !spidrcontrol.setPixelConfigMpx3rx( devnr ) )
    cout << "### Pixel config: " << spidrcontrol.errorString() << endl;
#endif // USE_PIXELCONFIG

  spidrcontrol.setPixelDepth( devnr, pixdepth, two_counter_readout );
  spidrcontrol.setColourMode( devnr, true );
  spidrcontrol.setGainMode( devnr, 1 );
  spidrcontrol.setPolarity( devnr, 1 );

  //int trig_mode = SHUTTERMODE_POS_EXT;
  //int trig_period_us = 10000;
  //int trig_freq_hz   = 10;
  //int nr_of_triggers = 1;
  //int trig_pulse_count;
  //spidrcontrol.setShutterTriggerConfig( trig_mode, trig_period_us,
  //					trig_freq_hz, nr_of_triggers );
  //spidrcontrol.clearBusy();

  spidrcontrol.startContReadout( freq_hz );

  Sleep( 5000 );

  spidrcontrol.stopContReadout();

  return 0;

  //char ch;
  int i, frame_cnt = 0;
  //for( i=0; i<20; ++i )
  for( i=0; i<1; ++i )
    {
      //cin >> ch;
      cout << "Auto-trig " << i << endl;

      spidrcontrol.startAutoTrigger();

#ifdef USE_SPIDRDAQ

      while( spidrdaq.hasFrame( 1000 ) )
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
		  //return 0;
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
	}
#else
      Sleep( 1500 );
#endif
    }

#ifdef USE_SPIDRDAQ
  // DAQ summary:
  cout << endl << "DAQ frames: " << spidrdaq.framesCount() << " (";
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

int my_atoi(const char *c)
{
  int value = 0;
  while( *c >= '0' && *c <= '9' )
  {
    value *= 10;
    value += (int)(*c - '0');
    ++c;
  }
  return value;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrmpx3test-crwdaq <ipaddr>[:<portnr>] [<freq_hz>]\n"
    "  <freq_hz> : read-out frequency [Hz] (default: 5)\n";
}

// ----------------------------------------------------------------------------
