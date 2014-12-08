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
#include "SpidrDaq.h"
#include "mpx3defs.h"

int main( int argc, char *argv[] )
{
  SpidrController spidrcontrol( 192, 168, 1, 10 );

  // Check if we are properly connected to the SPIDR module
  if( spidrcontrol.isConnected() )
    {
      cout << "Connected to SPIDR: " << spidrcontrol.ipAddressString();
      int ipaddr;
      if( spidrcontrol.getIpAddrDest( 0, &ipaddr ) )
	cout << ", IP dest: "
	     << ((ipaddr>>24) & 0xFF) << "."
	     << ((ipaddr>>16) & 0xFF) << "."
	     << ((ipaddr>> 8) & 0xFF) << "."
	     << ((ipaddr>> 0) & 0xFF);
      cout <<  endl;
    }
  else
    {
      cout << spidrcontrol.connectionStateString() << ": "
	   << spidrcontrol.connectionErrString() << endl;
      return 1;
    }

  SpidrDaq spidrdaq( &spidrcontrol );
  cout << "SpidrDaq: ";
  for( int i=0; i<4; ++i ) cout << spidrdaq.ipAddressString( i ) << " ";
  cout << endl;
  Sleep( 1000 );
  cout << spidrdaq.errString() << endl;

  spidrdaq.setDecodeFrames( true );

  spidrcontrol.setPixelDepth( 12 );
  spidrdaq.setPixelDepth( 12 );

  spidrcontrol.setMaxPacketSize( 1024 );

  int trig_mode      = 4;      // Auto-trigger mode
  int trig_period_us = 100000; // 100 ms
  int trig_freq_hz   = 5;
  int nr_of_triggers = 2;
  int trig_pulse_count;
  spidrcontrol.setShutterTriggerConfig( trig_mode, trig_period_us,
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
      Sleep( 1000 );
      while( spidrdaq.hasFrame() )
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
	  Sleep( 50 ); // Allow time to get and decode the next frame, if any
	}
      cout << "DAQ frames: " << spidrdaq.framesCount() << ", lost "
	   << spidrdaq.framesLostCount() << ", lost pkts "
	   << spidrdaq.packetsLostCount() << ", exp seqnr (dev 0) "
	   << spidrdaq.expSequenceNr( 0 ) << endl;
    }

  cout << "DAQ frames: " << spidrdaq.framesCount() << " (file: "
       << spidrdaq.framesWrittenCount() << "), proc'd "
       << spidrdaq.framesProcessedCount() << "), lost "
       << spidrdaq.framesLostCount() << ", lost pkts "
       << spidrdaq.packetsLostCount() << " (file: "
       << spidrdaq.packetsLostCountFile() << "), pkt size "
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
