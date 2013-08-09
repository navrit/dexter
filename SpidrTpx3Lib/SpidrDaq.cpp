#include "SpidrController.h"
#include "SpidrDaq.h"
#include "ReceiverThread.h"
#include "FilewriterThread.h"

// Version identifier: year, month, day, release number
const int VERSION_ID = 0x13080800;

// ----------------------------------------------------------------------------
// Constructor / destructor / info
// ----------------------------------------------------------------------------

SpidrDaq::SpidrDaq( int ipaddr3,
		    int ipaddr2,
		    int ipaddr1,
		    int ipaddr0,
		    int port )
{
  // Start data-acquisition on the interface
  // with the given IP address and port number
  int ipaddr[4] = { ipaddr0, ipaddr1, ipaddr2, ipaddr3 };
  this->init( ipaddr, port, 0 );
}

// ----------------------------------------------------------------------------

SpidrDaq::SpidrDaq( SpidrController *spidrctrl )
{
  // If the SpidrController parameter is provided use it to find out
  // the SPIDR's Medipix/Timepix device configuration and
  // IP destination address, or else assume a default IP address and
  // a single device with a default port number
  int ipaddr[4] = { 1, 100, 168, 192 };
  int id        = 0;
  int port      = 8192;
  if( spidrctrl )
    {
      // Get the IP destination address (this host network interface)
      // from the SPIDR module
      int addr = 0;
      if( spidrctrl->getIpAddrDest( 0, &addr ) )
	{
	  ipaddr[3] = (addr >> 24) & 0xFF;
	  ipaddr[2] = (addr >> 16) & 0xFF;
	  ipaddr[1] = (addr >>  8) & 0xFF;
	  ipaddr[0] = (addr >>  0) & 0xFF;
	}

      this->getIdAndPort( spidrctrl, &id, &port );
    }
  this->init( ipaddr, port, spidrctrl );
}

// ----------------------------------------------------------------------------

void SpidrDaq::getIdAndPort( SpidrController *spidrctrl,
			     int             *id,
			     int             *port )
{
  if( !spidrctrl ) return;

  // Get the device IDs from the SPIDR module
  spidrctrl->getDeviceId( 0, id );

  // Get the device port number from the SPIDR-TPX3 module
  // provided a sensible chip-ID was obtained
  //if( *id != 0 )
  spidrctrl->getServerPort( 0, port );
  //else
  //*port = 8192;
}

// ----------------------------------------------------------------------------

void SpidrDaq::init( int             *ipaddr,
		     int              port,
		     SpidrController *spidrctrl )
{
  _packetReceiver = new ReceiverThread( ipaddr, port );

  // Create the file writer thread, providing it with a link to
  // the packet reader thread
  _fileWriter = new FilewriterThread( _packetReceiver );

  // Provide the receiver with the possibility to control the module,
  // for example to set and clear a busy/inhibit/throttle signal
  //if( spidrctrl )
  //  _packetReceiver->setController( spidrctrl );
}

// ----------------------------------------------------------------------------

SpidrDaq::~SpidrDaq()
{
  this->stop();
}

// ----------------------------------------------------------------------------

void SpidrDaq::stop()
{
  if( _fileWriter )
    {
      _fileWriter->stop();
      delete _fileWriter;
      _fileWriter = 0;
    }
  /*
  if( _frameBuilder )
    {
      _frameBuilder->stop();
      delete _frameBuilder;
      _frameBuilder = 0;
    }
  */
  if( _packetReceiver )
    {
      _packetReceiver->stop();
      delete _packetReceiver;
      _packetReceiver = 0;
    }
}

// ----------------------------------------------------------------------------
// General
// ----------------------------------------------------------------------------

int SpidrDaq::classVersion()
{
  return VERSION_ID;
}

// ----------------------------------------------------------------------------

std::string SpidrDaq::ipAddressString()
{
  return _packetReceiver->ipAddressString();
}

// ----------------------------------------------------------------------------

std::string SpidrDaq::errorString()
{
  std::string str;
  str += _packetReceiver->errorString();
  if( !str.empty() && !_fileWriter->errorString().empty() )
    str += std::string( ", " );
  str += _fileWriter->errorString();

  // Clear the error strings...
  _packetReceiver->clearErrorString();
  _fileWriter->clearErrorString();

  return str;
}

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

bool SpidrDaq::openFile( std::string filename, bool overwrite /* = false */ )
{
  return _fileWriter->openFile( filename, overwrite );
}

// ----------------------------------------------------------------------------

bool SpidrDaq::closeFile()
{
  return _fileWriter->closeFile();
}

// ----------------------------------------------------------------------------

void SpidrDaq::setFlush( bool flush )
{
  _fileWriter->setFlush( flush );
}

// ----------------------------------------------------------------------------
/*
void SpidrDaq::setDecodeFrames( bool decode )
{
  //_frameBuilder->setDecodeFrames( decode );
}
*/
// ----------------------------------------------------------------------------
// Acquisition
// ----------------------------------------------------------------------------

bool SpidrDaq::bufferFull()
{
  return _packetReceiver->full();
}

// ----------------------------------------------------------------------------

bool SpidrDaq::bufferFullOccurred()
{
  return _packetReceiver->fullOccurred();
}

// ----------------------------------------------------------------------------

void SpidrDaq::resetBufferFullOccurred()
{
  _packetReceiver->resetFullOccurred();
}

// ----------------------------------------------------------------------------

char *SpidrDaq::dataBuffer()
{
  // To allow direct access to the 'raw' data buffer
  return _packetReceiver->data();
}

// ----------------------------------------------------------------------------
// Frame building
// ----------------------------------------------------------------------------

int *SpidrDaq::frameData( int *size_in_bytes )
{
  //return _frameBuilder->frameData( size_in_bytes );
  return 0;
}

// ----------------------------------------------------------------------------

void SpidrDaq::resetFrame()
{
  //_frameBuilder->resetFrame();
}

// ----------------------------------------------------------------------------
// Statistics
// ----------------------------------------------------------------------------

int SpidrDaq::packetsReceivedCount()
{
  return _packetReceiver->packetsReceived();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsLostCount()
{
  return _packetReceiver->packetsLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::lastPacketSize()
{
  return _packetReceiver->lastPacketSize();
}

// ----------------------------------------------------------------------------

long long SpidrDaq::bytesReceivedCount()
{
  return _packetReceiver->bytesReceived();
}

// ----------------------------------------------------------------------------

long long SpidrDaq::bytesLostCount()
{
  return _packetReceiver->bytesLost();
}

// ----------------------------------------------------------------------------

long long SpidrDaq::bytesWrittenCount()
{
  return _fileWriter->bytesWritten();
}

// ----------------------------------------------------------------------------

long long SpidrDaq::bytesFlushedCount()
{
  return _fileWriter->bytesFlushed();
}

// ----------------------------------------------------------------------------

int SpidrDaq::bufferWrapCount()
{
  return _packetReceiver->bufferWraps();
}

// ----------------------------------------------------------------------------
