#include <QCoreApplication>

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "ReceiverThread.h"
#include "ReceiverThreadC.h"
#include "FramebuilderThread.h"
#include "FramebuilderThreadC.h"

// Version identifier: year, month, day, release number
const int VERSION_ID = 0x16021700; // Compact-SPIDR support added
//const int VERSION_ID = 0x15101500;
//const int VERSION_ID = 0x15100100;
//const int VERSION_ID = 0x15093000;
//const int VERSION_ID = 0x15092100;
//const int VERSION_ID = 0x15051900;
//const int VERSION_ID = 0x14012400;

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
      if( spidrctrl->getIpAddrDest( 0, &addr ) )
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
  // but only for devices whose ID could be determined (i.e. is unequal to 0)
  int i;
  for( i=0; i<4; ++i ) { ports[i] = 0; types[i] = 0; }
  for( i=0; i<4; ++i )
    if( ids[i] != 0 )
      {
	spidrctrl->getServerPort( i, &ports[i] );
	spidrctrl->getDeviceType( i, &types[i] );
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

  bool is_cspidr = false;
  if( spidrctrl )
    is_cspidr = spidrctrl->isCompactSpidr();
  
  // Use ports[] to determine what's there, in case we want
  // to start with default parameters (which includes ID=0)
  if( ports[0] != 0 )
    {
      if( is_cspidr )
	recvr = new ReceiverThreadC( ipaddr, ports[0] );
      else
	recvr = new ReceiverThread( ipaddr, ports[0] );
      _frameReceivers.push_back( recvr );
    }
  if( ports[1] != 0 )
    {
      if( is_cspidr )
	recvr = new ReceiverThreadC( ipaddr, ports[1] );
      else
	recvr = new ReceiverThread( ipaddr, ports[1] );
      _frameReceivers.push_back( recvr );
    }
  if( ports[2] != 0 )
    {
      if( is_cspidr )
	recvr = new ReceiverThreadC( ipaddr, ports[2] );
      else
	recvr = new ReceiverThread( ipaddr, ports[2] );
      _frameReceivers.push_back( recvr );
    }
  if( ports[3] != 0 )
    {
      if( is_cspidr )
	recvr = new ReceiverThreadC( ipaddr, ports[3] );
      else
	recvr = new ReceiverThread( ipaddr, ports[3] );
      _frameReceivers.push_back( recvr );
    }

  // Create the file writer thread, providing it with a link to
  // the readout threads
  if( is_cspidr )
    _frameBuilder = new FramebuilderThreadC( _frameReceivers );
  else
    _frameBuilder = new FramebuilderThread( _frameReceivers );

  // Let the first receiver notify the file writer about new data
  if( _frameReceivers.size() > 0 )
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
  _frameBuilder->setDeviceIdsAndTypes( ids, types );
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

std::string SpidrDaq::errorString()
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

bool SpidrDaq::hasError()
{
  for( unsigned int i=0; i<_frameReceivers.size(); ++i )
    if( !_frameReceivers[i]->errString().empty() )
      return true;
  if( !_frameBuilder->errString().empty() )
    return true;
  return false;
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

void SpidrDaq::disableLut( bool disable )
{
  _frameBuilder->disableLut( disable );
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

bool SpidrDaq::hasFrame( unsigned long timeout_ms )
{
  return _frameBuilder->hasFrame( timeout_ms );
}

// ----------------------------------------------------------------------------

int *SpidrDaq::frameData( int index, int *size_in_bytes, int *packets_lost )
{
  return _frameBuilder->frameData( index, size_in_bytes, packets_lost );
}

// ----------------------------------------------------------------------------

void SpidrDaq::releaseFrame()
{
  _frameBuilder->releaseFrame();
}

// ----------------------------------------------------------------------------

void SpidrDaq::clearFrameData( int index )
{
  // Utility function to set a frame data array to zero
  _frameBuilder->clearFrameData( index );
}

// ----------------------------------------------------------------------------

int SpidrDaq::frameShutterCounter( int index )
{
  return _frameBuilder->frameShutterCounter( index );
}

// ----------------------------------------------------------------------------

bool SpidrDaq::isCounterhFrame( int index )
{
  return _frameBuilder->isCounterhFrame( index );
}

// ----------------------------------------------------------------------------

long long SpidrDaq::frameTimestamp()
{
  return _frameBuilder->frameTimestamp();
}

// ----------------------------------------------------------------------------

long long SpidrDaq::frameTimestamp( int buf_i )
{
  // DEBUG function:
  return _frameReceivers[0]->timeStampFrame( buf_i );
}

// ----------------------------------------------------------------------------

long long SpidrDaq::frameTimestampSpidr()
{
  return _frameBuilder->frameTimestampSpidr();
}

// ----------------------------------------------------------------------------

double SpidrDaq::frameTimestampDouble()
{
  // For Pixelman
  return _frameBuilder->frameTimestampDouble();
}

// ----------------------------------------------------------------------------

void SpidrDaq::setCallbackId( int id )
{
  // For Pixelman
  _frameBuilder->setCallbackId( id );
}

// ----------------------------------------------------------------------------

void SpidrDaq::setCallback( CallbackFunc cbf )
{
  // For Pixelman
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

int SpidrDaq::framesCount()
{
  int count = 0;
  for( int i=0; i<(int) _frameReceivers.size(); ++i )
    count += _frameReceivers[i]->framesReceived();
  return count;
}

// ----------------------------------------------------------------------------

int SpidrDaq::framesLostCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->framesLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::framesLostCount()
{
  int count = 0;
  for( int i=0; i<(int) _frameReceivers.size(); ++i )
    count += _frameReceivers[i]->framesLost();
  return count;
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsReceivedCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->packetsReceived();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsReceivedCount()
{
  int count = 0;
  for( int i=0; i<(int) _frameReceivers.size(); ++i )
    count += _frameReceivers[i]->packetsReceived();
  return count;
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsLostCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->packetsLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsLostCount()
{
  int count = 0;
  for( int i=0; i<(int) _frameReceivers.size(); ++i )
    count += _frameReceivers[i]->packetsLost();
  return count;
}

// ----------------------------------------------------------------------------

void SpidrDaq::resetLostCount()
{
  for( int i=0; i<(int) _frameReceivers.size(); ++i )
    _frameReceivers[i]->resetLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsLostCountFile()
{
  // The total number of lost packets detected in the frames written
  // to the current or last closed file
  return _frameBuilder->packetsLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::packetsLostCountFrame()
{
  // The total number of lost packets detected in the current frame
  return _frameBuilder->packetsLostFrame();
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
  // DEBUG function:
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
// For Compact-SPIDR type
// ----------------------------------------------------------------------------

int SpidrDaq::pixelsReceivedCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->pixelsReceived();
}

// ----------------------------------------------------------------------------

int SpidrDaq::pixelsReceivedCount()
{
  int count = 0;
  for( int i=0; i<(int) _frameReceivers.size(); ++i )
    count += _frameReceivers[i]->pixelsReceived();
  return count;
}

// ----------------------------------------------------------------------------

int SpidrDaq::pixelsLostCount( int index )
{
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->pixelsLost();
}

// ----------------------------------------------------------------------------

int SpidrDaq::pixelsLostCount()
{
  int count = 0;
  for( int i=0; i<(int) _frameReceivers.size(); ++i )
    count += _frameReceivers[i]->pixelsLost();
  return count;
}

// ----------------------------------------------------------------------------

int SpidrDaq::pixelsLostCountFrame( int index, int buf_i )
{
  // DEBUG function:
  if( index < 0 || index >= (int) _frameReceivers.size() ) return -1;
  return _frameReceivers[index]->pixelsLostFrame( buf_i );
}

// ----------------------------------------------------------------------------
