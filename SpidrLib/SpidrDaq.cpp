#include <QCoreApplication>

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "ReceiverThread.h"
#include "FramebuilderThread.h"

// Version identifier: year, month, day, release number
const int VERSION_ID = 0x13041200;

// At least one argument needed for QCoreApplication
//int   Argc = 1;
//char *Argv[] = { "SpidrDaq" }; 
//QCoreApplication *SpidrDaq::App = 0;
// In c'tor?: Create the single 'QCoreApplication' we need for the event loop
// in the receiver objects  ### SIGNALS STILL DO NOT WORK? Need exec() here..
//if( App == 0 ) App = new QCoreApplication( Argc, Argv );

// ----------------------------------------------------------------------------
// Constructor / destructor / info
// ----------------------------------------------------------------------------

SpidrDaq::SpidrDaq( int ipaddr3,
		    int ipaddr2,
		    int ipaddr1,
		    int ipaddr0,
		    SpidrController *spidrctrl )
{
  // If a SpidrController object is provided use it to find out the SPIDR's
  // Medipix device configuration, or else assume a single device
  // with a default port number
  int ipaddr[4] = { ipaddr0, ipaddr1, ipaddr2, ipaddr3 };
  int ids[4]    = { 0, 0, 0, 0 };
  int ports[4]  = { 8192, 0, 0, 0 };
  int types[4]  = { 0, 0, 0, 0 };
  if( spidrctrl ) this->getIdsPortsTypes( spidrctrl, ids, ports, types );
  this->init( ipaddr, ids, ports, types, spidrctrl );
}

// ----------------------------------------------------------------------------

SpidrDaq::SpidrDaq( SpidrController *spidrctrl )
{
  // If a SpidrController object is provided use it to find out the SPIDR's
  // Medipix device configuration and IP destination address, or else assume
  // a default IP address and a single device with a default port number
  int ipaddr[4] = { 1, 1, 168, 192 };
  int ids[4]    = { 0, 0, 0, 0 };
  int ports[4]  = { 8192, 0, 0, 0 };
  int types[4]  = { 0, 0, 0, 0 };
  if( spidrctrl )
    {
      // Get the IP destination address (this host network interface)
      // from the SPIDR module
      int addr = 0;
      if( spidrctrl->getIpAddrDest( &addr ) )
	{
	  ipaddr[3] = (addr >> 24) & 0xFF;
	  ipaddr[2] = (addr >> 16) & 0xFF;
	  ipaddr[1] = (addr >>  8) & 0xFF;
	  ipaddr[0] = (addr >>  0) & 0xFF;
	}

      this->getIdsPortsTypes( spidrctrl, ids, ports, types );
    }
  this->init( ipaddr, ids, ports, types, spidrctrl );
}

// ----------------------------------------------------------------------------

void SpidrDaq::getIdsPortsTypes( SpidrController *spidrctrl,
				 int             *ids,
				 int             *ports,
				 int             *types )
{
  if( !spidrctrl ) return;

  // Get the device IDs from the SPIDR module
  spidrctrl->getDeviceIds( ids );

  // Get the device port numbers from the SPIDR module
  // but only for devices whose ID could be determined
  for( u32 i=0; i<4; ++i )
    {
      if( ids[i] != 0 )
	{
	  spidrctrl->getServerPort( i, &ports[i] );
	  spidrctrl->getDeviceType( i, &types[i] );
	}
      else
	{
	  ports[i] = 0; types[i] = 0;
	}
    }
}

// ----------------------------------------------------------------------------

void SpidrDaq::init( int             *ipaddr,
		     int             *ids,
		     int             *ports,
		     int             *types,
		     SpidrController *spidrctrl )
{
  _frameBuilder = 0;

  ReceiverThread *recvr;
  // Use ports[] to determine what's there, in case we want
  // to start with default parameters (which includes ID=0)
  if( ports[0] != 0 )
    {
      recvr = new ReceiverThread( ipaddr, ports[0] );
      _frameReceivers.push_back( recvr );
    }
  if( ports[1] != 0 )
    {
      recvr = new ReceiverThread( ipaddr, ports[1] );
      _frameReceivers.push_back( recvr );
    }
  if( ports[2] != 0 )
    {
      recvr = new ReceiverThread( ipaddr, ports[2] );
      _frameReceivers.push_back( recvr );
    }
  if( ports[3] != 0 )
    {
      recvr = new ReceiverThread( ipaddr, ports[3] );
      _frameReceivers.push_back( recvr );
    }

  // Create the file writer thread, providing it with a link to
  // the readout threads
  _frameBuilder = new FramebuilderThread( _frameReceivers );

  // Let the first receiver notify the file writer about new data
  if( _frameReceivers[0] )
    _frameReceivers[0]->setFramebuilder( _frameBuilder );

  // Provide the receivers with the possibility to control the module,
  // for example to set and clear a busy/inhibit/frame-throttle signal
  if( spidrctrl )
    for( unsigned int i=0; i<_frameReceivers.size(); ++i )
      _frameReceivers[i]->setController( spidrctrl );

  // Provide the framebuilder with some info about the system
  // for inclusion in the event and device headers (when writing to file):

  // IP address and port numbers
  _frameBuilder->setAddrInfo( ipaddr, ports );

  // The device IDs and types
  _frameBuilder->setDeviceIds( ids );
  _frameBuilder->setDeviceTypes( types );
}

// ----------------------------------------------------------------------------

SpidrDaq::~SpidrDaq()
{
  this->stop();
}

// ----------------------------------------------------------------------------

void SpidrDaq::stop()
{
  if( _frameBuilder )
    {
      _frameBuilder->stop();
      delete _frameBuilder;
      _frameBuilder = 0;
    }
  for( unsigned int i=0; i<_frameReceivers.size(); ++i )
    {
      _frameReceivers[i]->stop();
      delete _frameReceivers[i];
    }
  _frameReceivers.clear();
}

// ----------------------------------------------------------------------------
// General
// ----------------------------------------------------------------------------

int SpidrDaq::classVersion()
{
  return VERSION_ID;
}

// ----------------------------------------------------------------------------

std::string SpidrDaq::ipAddressString( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() )
    return std::string( "" );
  return _frameReceivers[index]->ipAddressString();
}

// ----------------------------------------------------------------------------

std::string SpidrDaq::errString()
{
  std::string str;
  for( unsigned int i=0; i<_frameReceivers.size(); ++i )
    {
      if( !str.empty() && !_frameReceivers[i]->errString().empty() )
	str += std::string( ", " );
      str += _frameReceivers[i]->errString();
    }
  if( !str.empty() && !_frameBuilder->errString().empty() )
    str += std::string( ", " );
  str += _frameBuilder->errString();

  // Clear the error strings
  for( unsigned int i=0; i<_frameReceivers.size(); ++i )
    _frameReceivers[i]->clearErrString();
  _frameBuilder->clearErrString();

  return str;
}

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

void SpidrDaq::setPixelDepth( int nbits )
{
  for( unsigned int i=0; i<_frameReceivers.size(); ++i )
    _frameReceivers[i]->setPixelDepth( nbits );
  _frameBuilder->setPixelDepth( nbits );
}

// ----------------------------------------------------------------------------

void SpidrDaq::setDecodeFrames( bool decode )
{
  _frameBuilder->setDecodeFrames( decode );
}

// ----------------------------------------------------------------------------

void SpidrDaq::setCompressFrames( bool compress )
{
  _frameBuilder->setCompressFrames( compress );
}

// ----------------------------------------------------------------------------

bool SpidrDaq::openFile( std::string filename, bool overwrite )
{
  return _frameBuilder->openFile( filename, overwrite );
}

// ----------------------------------------------------------------------------

bool SpidrDaq::closeFile()
{
  return _frameBuilder->closeFile();
}

// ----------------------------------------------------------------------------
// Acquisition
// ----------------------------------------------------------------------------

bool SpidrDaq::hasFrame()
{
  return _frameBuilder->hasDecodedFrame();
}

// ----------------------------------------------------------------------------

int *SpidrDaq::frameData( int dev_nr, int *size_in_bytes )
{
  return _frameBuilder->decodedFrameData( dev_nr, size_in_bytes );
}

// ----------------------------------------------------------------------------

long long SpidrDaq::frameTimestamp()
{
  return _frameBuilder->decodedFrameTimestamp();
}

// ----------------------------------------------------------------------------

long long SpidrDaq::frameTimestamp( int buf_i )
{
  return _frameReceivers[0]->timeStampFrame( buf_i );
}

// ----------------------------------------------------------------------------

long long SpidrDaq::frameTimestampSpidr()
{
  return _frameBuilder->decodedFrameTimestampSpidr();
}

// ----------------------------------------------------------------------------

double SpidrDaq::frameTimestampDouble()
{
  return _frameBuilder->decodedFrameTimestampDouble();
}

// ----------------------------------------------------------------------------

void SpidrDaq::releaseFrame()
{
  _frameBuilder->releaseDecodedFrame();
}

// ----------------------------------------------------------------------------

void SpidrDaq::setCallbackId( int id )
{
  _frameBuilder->setCallbackId( id );
}

// ----------------------------------------------------------------------------

void SpidrDaq::setCallback( CallbackFunc cbf )
{
  _frameBuilder->setCallback( cbf );
}

// ----------------------------------------------------------------------------
// Statistics
// ----------------------------------------------------------------------------

int SpidrDaq::framesWrittenCount()
{
  return _frameBuilder->framesWritten();
}

// ----------------------------------------------------------------------------

int SpidrDaq::framesProcessedCount()
{
  return _frameBuilder->framesProcessed();
}

// ----------------------------------------------------------------------------

int SpidrDaq::framesCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->framesReceived();
}

// ----------------------------------------------------------------------------

int SpidrDaq::framesLostCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->framesLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsReceivedCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->packetsReceived();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsLostCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->packetsLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsLostCountFile()
{
  // The number of lost packets detected in the frames written
  // to the current or last closed file
  return _frameBuilder->packetsLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsLostCountFrame( int index, int buf_i )
{
  // DEBUG function:
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->packetsLostFrame( buf_i );
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetSize( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->packetSize();
}

// ----------------------------------------------------------------------------

int SpidrDaq::expSequenceNr( int index )
{
  // DEBUG function:
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->expSequenceNr();
}

// ----------------------------------------------------------------------------
