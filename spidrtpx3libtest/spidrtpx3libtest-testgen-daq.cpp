#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(1000*ms)
#endif
#include <iostream>
#include <iomanip>
using namespace std;

#include "SpidrController.h"
#include "SpidrDaq.h"

int main( int argc, char *argv[] )
{
  // Open a control connection to SPIDR-TPX3 module
  // with address 192.168.100.10, default port number 50000
  /*
  SpidrController spidrctrl( 192, 168, 100, 10 );

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
	 << spidrctrl.connectionStateString() << ", "
	 << spidrctrl.connectionErrString() << endl;
    return 1;
  }

  // Get the interface IP address and port number for data-acquisition
  // from SpidrController:
  SpidrDaq spidrdaq( &spidrctrl );
  */

  // Data-acquisition on interface 192.168.1.1, port 0xABCD (43981)
  // (the SPIDR-TPX3 testpacket generator)
  SpidrDaq spidrdaq( 192, 168, 1, 1, 0xABCD );

  string errstr = spidrdaq.errorString();
  if( !errstr.empty() )
    cout << "### SpidrDaq: " << errstr << endl;
  cout << "Addr : " << spidrdaq.ipAddressString() << endl;

#define TEST_ONE_FULL_BUFFER
#ifdef TEST_ONE_FULL_BUFFER
  spidrdaq.setFlush( false );
#else
  if( !spidrdaq.openFile( "test.dat", true ) )
    cout << "### " << spidrdaq.errorString() << endl;
#endif

  int       cnt          = 0;
  long long recvd        = 0;
  long long last_recvd   = 0;
  long long written      = 0;
  long long last_written = 0;
  while( 1 )
    {
      recvd   = spidrdaq.bytesReceivedCount();
      written = spidrdaq.bytesWrittenCount();
      cout << cnt << ": " << spidrdaq.packetsReceivedCount() << " pkts, "
	   << recvd << " bytes"
	   << " (" << (recvd-last_recvd)/1000000 << " MB/s)"
	   << ", sz=" << spidrdaq.lastPacketSize()
	   << ", lost=" << spidrdaq.bytesLostCount()
	   << ", full=" << spidrdaq.bufferFullOccurred()
	   << ", wrote " << written
	   << " (" << (written-last_written)/1000000 << " MB/s)"
	   << " flushed " << spidrdaq.bytesFlushedCount()
	   << " wraps " << spidrdaq.bufferWrapCount()
	   << endl;
      last_recvd   = recvd;
      last_written = written;
      ++cnt;
      Sleep( 1000 );
      //if( spidrdaq.bufferFullOccurred() ) spidrdaq.closeFile();
#ifdef TEST_ONE_FULL_BUFFER
      /*
      if( cnt == 20 )
	{
	  if( !spidrdaq.openFile( "test.dat", true ) )
	    cout << "### " << spidrdaq.errorString() << endl;
	}
      */
      if( spidrdaq.bufferFullOccurred() )
	{
	  if( !spidrdaq.openFile( "test.dat", true ) )
	    cout << "### " << spidrdaq.errorString() << endl;
	  Sleep( 5000 );
	  spidrdaq.closeFile();

	  // Check if the counter increments properly
	  long long i, size = spidrdaq.bytesReceivedCount();
	  unsigned char *data = (unsigned char *) spidrdaq.dataBuffer();
	  unsigned int exp_count = 0, count;
	  char ch;
	  for( i=0; i<size; i+=8 )
	    {
	      count = ((unsigned int) data[i] <<8) | (unsigned int) data[i+1];
	      if( count != exp_count )
		{
		  cout << hex << setw(8) << i << dec
		       << ": ### " << count << ", exp " << exp_count
		       << " missed " << (count-exp_count)*8 << " bytes" << endl;
		  cin >> ch;
		  //Sleep( 500 );
		}
	      else
		{
		  cout << hex << setw(8) << i << endl;
		  //cout << i/8 << endl;
		}
	      exp_count = (count + 1) & 0xFFFF;
	    }
	}
#endif
    }

  spidrdaq.stop();

  return 0;
}
