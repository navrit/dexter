#include <windows.h>
#include <iostream>
#include <iomanip>
using namespace std;

#include <QString>

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "mpx3defs.h"

//#define USE_SPIDRDAQ
#define USE_PIXELCONFIG
//#define DEBUGGING_IT

quint32 get_addr_and_port(const char *str, int *portnr);
int     my_atoi(const char *c);
void    usage();

void displaySpidrRegs( SpidrController *sc );

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  quint32 ipaddr  = 0;
  int     portnr  = 50000;
#ifndef DEBUGGING_IT
  int     freq_hz = 10;
  int     ntrigs  = 100;
  // Check argument count
  if( !(argc >= 2) )
    {
      usage();
      return 0;
    }
  ipaddr = get_addr_and_port(argv[1], &portnr);
#else
  int     freq_hz = 1;
  int     ntrigs  = 1;
  ipaddr = 0xC0A8010A;
#endif

  if( argc >= 3 )
    freq_hz = my_atoi(argv[2]);

  if( argc >= 4 )
    ntrigs = my_atoi(argv[3]);

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
  SpidrDaq spidrdaq( &spidrcontrol, 0x7 );
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

  /*
  if( spidrdaq.openFile( "test.dat", true ) )
    cout << "Opened file" << endl;
  else
    cout << "Failed to open file: " << spidrdaq.errorString() << endl;
  */
#endif

  // ----------------------------------------------------------
  //spidrcontrol.setMaxPacketSize( 9000 ); // Not available on Compact-SPIDR

  int  pixdepth = 12;
  bool two_counter_readout = true;

  // DO AFTER PIXEL CONFIGURATION! :
  //spidrcontrol.setPixelDepth(devnr, pixdepth, two_counter_readout);
#ifdef USE_SPIDRDAQ
  spidrdaq.setPixelDepth( pixdepth );
#endif

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

      // Mask a number of pixel rows (columns)...
      for( col=32; col<40; ++col )
        if( !spidrcontrol.setPixelMaskMpx3rx(col,ALL_PIXELS) )
          cout << "### Pixel mask col " << col << endl;
      for( row = 160; row<180; ++row )
        if( !spidrcontrol.setPixelMaskMpx3rx(ALL_PIXELS, row) )
          cout << "### Pixel mask row " << row << endl;

      /*for( row = 160; row<170; ++row )
        if( !spidrcontrol.setPixelMaskMpx3rx(row-20, row) )
          cout << "### Pixel mask row " << row << endl;*/

      for( row = 0; row < 256; ++row ) {
        if( row > 1 ) spidrcontrol.setPixelMaskMpx3rx(row - 2, row);
        if( row > 0 ) spidrcontrol.setPixelMaskMpx3rx(row - 1, row);
        spidrcontrol.setPixelMaskMpx3rx(row, row);
        if( row < 255 ) spidrcontrol.setPixelMaskMpx3rx(row + 1, row);
        if( row < 254 ) spidrcontrol.setPixelMaskMpx3rx(row + 2, row);
      }
        
      // Set test-bit on a number of pixels
      bool testbit = true;
      for( col=128; col<130; ++col )
      //for( col=2; col<3; ++col )
      //for( col=64; col<128; ++col )
        {
          spidrcontrol.configPixelMpx3rx( col, col+2/*ALL_PIXELS*/,
                                          0, 0, testbit );
          if( testbit )
            spidrcontrol.configCtpr( devnr, col, 1 );
        }
      //for( col = 0; col<255; ++col )
        //spidrcontrol.configCtpr( devnr, col, 1 );

      /*for( col=63; col<64; ++col )
        for( row=0; row<MPX_PIXEL_ROWS; ++row )
          if( row != 0x10 )
            //spidrcontrol.configPixelMpx3rx(col, row, 0, (row & 31));
            spidrcontrol.configPixelMpx3rx(col, row, (row & 31), 0 );*/

      spidrcontrol.configPixelMpx3rx( 4, 3, 31, 6, false );
      //spidrcontrol.setPixelMaskMpx3rx( 4, 5 );
      //spidrcontrol.setPixelMaskMpx3rx( 5, 6 );

#ifdef USE_SPIDRDAQ
      bool read_back_pixconf = true;
      //bool read_back_pixconf = false;
      if( read_back_pixconf )
        spidrdaq.setLutEnable( false );
      //spidrcontrol.setLutEnable( true );
#else
      bool read_back_pixconf = false;
#endif

      // Upload the test pulse configuration
      //if( !spidrcontrol.setCtpr( devnr ) )
        //cout << "### CTPR config: " << spidrcontrol.errorString() << endl;

#ifdef USE_SPIDRDAQ
      // Upload the pixel configuration; optionally read it back
      if( !spidrcontrol.setPixelConfigMpx3rx(devnr, read_back_pixconf) )
        cout << "### Pixel config: " << spidrcontrol.errorString() << endl;

      if( read_back_pixconf )
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
              cout << "Pixels with config !=0:" << endl;
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
          spidrdaq.setLutEnable( true );
          spidrcontrol.setLutEnable( false );
	}
#else
      // Upload the pixel configuration to all devices
      for( int i=0; i<devcnt; ++i )
	if( devids[i] != 0 && (mask & (1<<i)) )
	  if( !spidrcontrol.setPixelConfigMpx3rx(i, read_back_pixconf) )
	    cout << "### Pixel config " << i << ": "
		 << spidrcontrol.errorString() << endl;
#endif // USE_SPIDRDAQ
    }
  else
    {
      cout << "### No device type, no pixel configuration upload" << endl;
    }
#endif // USE_PIXELCONFIG

#ifdef USE_SPIDRDAQ
  //spidrdaq.stop(); return 0;
#endif

  spidrcontrol.setPixelDepth( devnr, pixdepth, two_counter_readout );
  spidrcontrol.setPs( devnr, 3 );
  spidrcontrol.setPolarity(devnr, true);
  spidrcontrol.setEqThreshH(devnr, 0);
  //spidrcontrol.setColourMode(devnr, 1);
  spidrcontrol.setCsmSpm(devnr, 0);
  spidrcontrol.setGainMode( devnr, 1 );
  spidrcontrol.setTpSwitch( 1, 1000000 );
  //return 0;

  // Choose 'deadtime' such that it is enough to read out the frame(s)
  int trig_mode        = SHUTTERMODE_AUTO; // Auto-trigger mode
  int nr_of_triggers   = ntrigs;
  int trig_freq_hz     = freq_hz;
  int trig_deadtime_us = 4000;
  int trig_width_us = 10000;
  //  (int) ((double)1000000.0/(double)trig_freq_hz - (double)trig_deadtime_us);
  if( trig_width_us <= 0 )
    cout << "### Frequency too high for current deadtime="
         << trig_deadtime_us << " us" << endl;
  spidrcontrol.setShutterTriggerConfig( trig_mode, trig_width_us,
                                        trig_freq_hz*1000, nr_of_triggers );
  //spidrcontrol.clearBusy();

  displaySpidrRegs( &spidrcontrol );

  char ch;
  int i, frame_cnt = 0;
  //for( i=0; i<20; ++i )
  for( i=0; i<1; ++i )
    {
      //cin >> ch;
      cout << "Auto-trig " << i << endl;

      //if( 1 )//i==4 ) spidrcontrol.setDac( devnr, 0, i*51 );
      //spidrcontrol.setMaxPacketSize( 512+i*64 );

      spidrcontrol.startAutoTrigger();
#ifdef USE_SPIDRDAQ

      while( spidrdaq.hasFrame( 1000 ) )
        {
          //displaySpidrRegs( &spidrcontrol );

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
          //if( frame_cnt >= 25 ) spidrcontrol.stopAutoTrigger(); // TEST
        }
#else
      //Sleep( 1000 + 1000*((nr_of_triggers+trig_freq_hz-1)/trig_freq_hz) );
      Sleep( 1500 );
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
  cout << endl << "DAQ frames: " << spidrdaq.framesCount() << " (";
  for( i=0; i<4; ++i ) cout << spidrdaq.framesCount(i) << " ";
  cout << ")" << endl;
  cout << " (file: " << spidrdaq.framesWrittenCount()
       << "), lost " << spidrdaq.framesLostCount()
       << ", recvd packets " << spidrdaq.packetsReceivedCount()
       << ", lost pkts/pix " << spidrdaq.lostCount()
       << " (file: "  << spidrdaq.lostCountFile()
       << "), pkt size " << spidrdaq.packetSize( 0 ) << endl;
  cout << "Packets lost/frame (of first device present):" << endl;
  for( i=0; i<8; ++i )
    cout << i << "=" << spidrdaq.packetsLostCountFrame( 0, i ) << ", ";
  cout << endl;

  cout << "DAQ pixels: " << spidrdaq.pixelsReceivedCount()
       << ", lost " << spidrdaq.pixelsLostCount()
       << endl;
  cout << "Pixels lost/frame (of first device present):" << endl;
  for( i=0; i<8; ++i )
    cout << i << "=" << spidrdaq.pixelsLostCountFrame( 0, i ) << ", ";
  cout << endl;

  //cin >> ch;
  spidrdaq.stop();
#endif    
  return 0;
}

// ----------------------------------------------------------------------------

void displaySpidrRegs( SpidrController *sc )
{
  int regaddr[] = { 0x1044, 0x1048, 0x1030, 0x1034,
                    0x1038, 0x1054, 0x0290, 0x0294,
                    0x0298, 0x029C, 0x02AC, 0x1008, 0x1040 };
  if( sc == 0 ) return;
  cout << hex << setfill( '0' );
  int val;
  for( int i=0; i<sizeof(regaddr)/sizeof(int); ++i )
    {
      sc->getSpidrReg( regaddr[i], &val );
      cout << "0x" << setw(4) << regaddr[i] << ": " << val << endl;
    }
  cout << endl << dec;
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
    "spidrmpx3test-daq <ipaddr>[:<portnr>] [<freq_hz>] [ntrigs]\n"
    "  <freq_hz> : Trigger frequency [Hz] (default: 10)\n"
    "  <ntrigs>  : Number of triggers (default: 100)\n";
}

// ----------------------------------------------------------------------------
