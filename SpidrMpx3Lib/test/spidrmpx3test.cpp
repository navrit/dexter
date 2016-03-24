#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(ms*1000)
#endif
#include <iostream>
#include <iomanip>
using namespace std;

#include <QString>

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "mpx3defs.h"

quint32 get_addr_and_port(const char *str, int *portnr);
void usage();

// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
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

  SpidrDaq spidrdaq( &spidrcontrol );
  cout << "SpidrDaq: ";
  for( int i=0; i<4; ++i ) cout << spidrdaq.ipAddressString( i ) << " ";
  cout << endl;
  Sleep( 1000 );
  cout << spidrdaq.errorString() << endl;

  spidrdaq.setDecodeFrames( true );

  spidrcontrol.setPixelDepth( 0, 12 );
  spidrdaq.setPixelDepth( 12 );

  spidrcontrol.setMaxPacketSize( 1024 );

  int trig_mode      = SHUTTERMODE_AUTO; // Auto-trigger mode
  int trig_length_us = 100000; // 100 ms
  int trig_freq_hz   = 5;
  int nr_of_triggers = 2;
  int trig_pulse_count;
  spidrcontrol.setShutterTriggerConfig( trig_mode, trig_length_us,
                                        trig_freq_hz, nr_of_triggers );

  int dac_index = 0;
  for( int dev=0; dev<4; ++dev )
    {
      spidrcontrol.setDac( dev, dac_index, 0 );
    }

  int i;
  for( i=0; i<10; ++i )
    //for( i=0; i<1; ++i )
    {
      cout << "Auto-trig " << i << endl;
      if( i==4 )
	{
	  spidrcontrol.setDac( 3, dac_index, i*51 );
	}
      spidrcontrol.startAutoTrigger();
      while( spidrdaq.hasFrame( 1000 ) )
	{
	  double t = spidrdaq.frameTimestampDouble();
	  unsigned int secs = (unsigned int) t;
	  unsigned int msecs = (unsigned int) ((t - secs)*1000.0+0.5);
	  //cout << "T=" << fixed << setprecision(9)
	  // << spidrdaq.frameTimestampDouble() << endl;
	  cout << "T=" << secs << "."
	       << setfill('0') << setw(3) << msecs
	       << ", " << spidrdaq.framesProcessedCount()
	       << ", Ts=" << hex << spidrdaq.frameTimestampSpidr()
	       << dec << endl;
	  spidrdaq.releaseFrame();
	}
      cout << "DAQ frames: " << spidrdaq.framesCount() << ", lost "
	   << spidrdaq.framesLostCount() << ", lost pkts "
	   << spidrdaq.lostCount() << ", exp seqnr (dev 0) "
	   << spidrdaq.expSequenceNr( 0 ) << endl;
    }

  cout << "DAQ frames: " << spidrdaq.framesCount() << " (file: "
       << spidrdaq.framesWrittenCount() << "), proc'd "
       << spidrdaq.framesProcessedCount() << "), lost "
       << spidrdaq.framesLostCount() << ", lost pkts "
       << spidrdaq.lostCount() << " (file: "
       << spidrdaq.lostCountFile() << "), pkt size "
       << spidrdaq.packetSize( 0 ) << endl;
  cout << "Lost/frame: ";
  for( i=0; i<8; ++i )
    cout << i << "=" << spidrdaq.packetsLostCountFrame( 0, i ) << ", ";
  cout << endl;
  for( i=8; i<16; ++i )
    cout << i << "=" << spidrdaq.packetsLostCountFrame( 0, i ) << ", ";
  cout << endl;

  cout << "Timestamps: ";
  for( i=0; i<4; ++i )
    cout << i << "=" << spidrdaq.frameTimestamp( i ) << ", ";
  cout << endl;
  for( i=4; i<8; ++i )
    cout << i << "=" << spidrdaq.frameTimestamp( i ) << ", ";
  cout << endl;
  for( i=8; i<12; ++i )
    cout << i << "=" << spidrdaq.frameTimestamp( i ) << ", ";
  cout << endl;
  for( i=12; i<16; ++i )
    cout << i << "=" << spidrdaq.frameTimestamp( i ) << ", ";
  cout << endl;

  spidrdaq.stop();

  return 0;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage  :\n"
    "spidrmpx3test <ipaddr>[:<portnr>]\n";
}

// ----------------------------------------------------------------------------
