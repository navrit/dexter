#include "SpidrController.h"
#include "SpidrDaq.h"
#include "ReceiverThread.h"
#include "DatasamplerThread.h"

#include "tpx3defs.h"
#include "dacsdescr.h"

// Version identifier: year, month, day, release number
const int VERSION_ID = 0x14043000;

// ----------------------------------------------------------------------------
// Constructor / destructor / info
// ----------------------------------------------------------------------------

SpidrDaq::SpidrDaq( int ipaddr3,
		    int ipaddr2,
		    int ipaddr1,
		    int ipaddr0,
		    int port )
  : _packetReceiver( 0 ),
    _fileWriter( 0 ),
    _spidrCtrl( 0 ),
    _ipAddr( 0 ),
    _ipPort( 0 ),
    _deviceNr( 0 )
{
  // Start data-acquisition on the interface
  // with the given IP address and port number
  int ipaddr[4] = { ipaddr0, ipaddr1, ipaddr2, ipaddr3 };

  _ipAddr   = (((ipaddr[0] & 0xFF) << 0) | ((ipaddr[1] & 0xFF) << 8) |
	       ((ipaddr[2] & 0xFF) << 16) | ((ipaddr[3] & 0xFF) << 24));
  _ipPort   = port;

  _packetReceiver = new ReceiverThread( ipaddr, port );

  // Create the sampler/file-writer thread, providing it with a link to
  // the packet reader thread
  _fileWriter = new DatasamplerThread( _packetReceiver );
}

// ----------------------------------------------------------------------------

SpidrDaq::SpidrDaq( SpidrController *spidrctrl, int device_nr )
  : _packetReceiver( 0 ),
    _fileWriter( 0 ),
    _spidrCtrl( 0 ),
    _ipAddr( 0 ),
    _ipPort( 0 ),
    _deviceNr( 0 )
{
  // If the SpidrController parameter is provided use it to find out
  // the SPIDR's Medipix/Timepix device configuration and
  // IP destination address, or else assume a default IP address and
  // a single device with a default port number
  int ipaddr[4] = { 1, 100, 168, 192 };
  int port      = 8192 + device_nr;
  if( spidrctrl )
    {
      // Get the IP destination address (this host network interface)
      // from the SPIDR module
      int addr = 0;
      if( spidrctrl->getIpAddrDest( device_nr, &addr ) )
	{
	  ipaddr[3] = (addr >> 24) & 0xFF;
	  ipaddr[2] = (addr >> 16) & 0xFF;
	  ipaddr[1] = (addr >>  8) & 0xFF;
	  ipaddr[0] = (addr >>  0) & 0xFF;
	}

      if( !spidrctrl->getServerPort( device_nr, &port ) )
	port = 8192 + device_nr;
    }

  _ipAddr   = (((ipaddr[0] & 0xFF) << 0) | ((ipaddr[1] & 0xFF) << 8) |
	       ((ipaddr[2] & 0xFF) << 16) | ((ipaddr[3] & 0xFF) << 24));
  _ipPort   = port;
  _deviceNr = device_nr;

  _packetReceiver = new ReceiverThread( ipaddr, port );

  // Create the sampler/file-writer thread, providing it with a link to
  // the packet reader thread
  _fileWriter = new DatasamplerThread( _packetReceiver );

  // Provide the receiver with the possibility to control the module,
  // i.e. to set and clear a busy/inhibit/throttle signal
  if( spidrctrl )
    {
      _packetReceiver->setController( spidrctrl );
      _spidrCtrl = spidrctrl;
    }
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
// Acquisition
// ----------------------------------------------------------------------------

#define INVALID_VAL 0xFFFFFFFF

bool SpidrDaq::startRecording( std::string filename,
			       int         runnr,
			       std::string descr )
{
  // Fill in the file and device header with the current settings
  SpidrTpx3Header_t *fhdr = _fileWriter->fileHdr();

  // Run number
  fhdr->runNr = runnr;

  // (Re)set description string
  memset( static_cast<void *>(&fhdr->descr), 0, sizeof(fhdr->descr) );
  if( !descr.empty() )
    {
      u32 sz = descr.length();
      if( sz > sizeof(fhdr->descr)-1 )
	sz = sizeof(fhdr->descr)-1;

      // Copy string into the file header
      descr.copy( fhdr->descr, sz );
    }

  // Fill in the rest of the file header
  fhdr->ipAddress  = _ipAddr;
  fhdr->ipPort     = _ipPort;
  fhdr->libVersion = this->classVersion();
  if( _spidrCtrl )
    {
      int val;
      if( _spidrCtrl->getSpidrId( &val ) )
	fhdr->spidrId = val;
      else
	fhdr->spidrId = INVALID_VAL;

      if( _spidrCtrl->getSoftwVersion( &val ) )
	fhdr->softwVersion = val;
      else
	fhdr->softwVersion = INVALID_VAL;

      if( _spidrCtrl->getFirmwVersion( &val ) )
	fhdr->firmwVersion = val;
      else
	fhdr->firmwVersion = INVALID_VAL;

      // SPIDR configuration: trigger mode, decoder on/off, ...
      // Trigger control register (lower 12 bits):
      if( _spidrCtrl->getSpidrReg( 0x290, &val ) )
	fhdr->spidrConfig |= (val & 0xFFF);
      // Decoder control register (LSB nibble shifted to bit 16):
      if( _spidrCtrl->getSpidrReg( 0x2A8, &val ) )
	fhdr->spidrConfig |= (val & 0xF) << 16;

      // Header filter (16 bits value)
      int cpu_filter;
      if( _spidrCtrl->getHeaderFilter( _deviceNr, &val, &cpu_filter ) )
	fhdr->spidrFilter = val;
      else
	fhdr->spidrFilter = INVALID_VAL;

      // Bias voltage
      if( _spidrCtrl->getBiasVoltage( &val ) )
	fhdr->spidrBiasVoltage = val;
      else
	fhdr->spidrBiasVoltage = INVALID_VAL;
    }

  // Fill in the rest of the device header
  if( _spidrCtrl )
    {
      int val;
      Tpx3Header_t *thdr = &fhdr->devHeader;
      if( _spidrCtrl->getDeviceId( _deviceNr, &val ) )
	thdr->deviceId = val;
      else
	thdr->deviceId = INVALID_VAL;

      if( _spidrCtrl->getGenConfig( _deviceNr, &val ) )
	thdr->genConfig = val;
      else
	thdr->genConfig = INVALID_VAL;

      if( _spidrCtrl->getOutBlockConfig( _deviceNr, &val ) )
	thdr->outblockConfig = val;
      else
	thdr->outblockConfig = INVALID_VAL;

      if( _spidrCtrl->getPllConfig( _deviceNr, &val ) )
	thdr->pllConfig = val;
      else
	thdr->pllConfig = INVALID_VAL;

      if( _spidrCtrl->getSlvsConfig( _deviceNr, &val ) )
	thdr->slvsConfig = val;
      else
	thdr->slvsConfig = INVALID_VAL;

      if( _spidrCtrl->getPwrPulseConfig( _deviceNr, &val ) )
	thdr->pwrPulseConfig = val;
      else
	thdr->pwrPulseConfig = INVALID_VAL;

      {
	// Test pulse configuration
	int period = INVALID_VAL, phase = INVALID_VAL, number = INVALID_VAL;
	_spidrCtrl->getTpPeriodPhase( _deviceNr, &period, &phase );
	_spidrCtrl->getTpNumber( _deviceNr, &number );

	// Recreate layout from returned Timepix3 periphery data...
	thdr->testPulseConfig = (phase << 24) | (period << 16) | number;
      }

      // (Re)set DAC values
      memset( static_cast<void *>(&thdr->dac), 0, sizeof(thdr->dac) );
      int i, code;
      for( i=0; i<TPX3_DAC_COUNT; ++i )
	{
	  code = TPX3_DAC_TABLE[i].code;
	  if( _spidrCtrl->getDac( _deviceNr, code, &val ) )
	    thdr->dac[code] = val;
	  else
	    thdr->dac[code] = INVALID_VAL;
	}

      // (Re)set CTPR values
      memset( static_cast<void *>(&thdr->ctpr), 0, sizeof(thdr->ctpr) );
      unsigned char *pctpr;
      if( _spidrCtrl->getCtpr( _deviceNr, &pctpr ) )
	memcpy( thdr->ctpr, pctpr, sizeof(thdr->ctpr) );
      else
	memset( static_cast<void *>(&thdr->ctpr), 0xAA, sizeof(thdr->ctpr) );
    }

  return _fileWriter->startRecording( filename, runnr );
}

// ----------------------------------------------------------------------------

bool SpidrDaq::stopRecording()
{
  return _fileWriter->stopRecording();
}

// ----------------------------------------------------------------------------

long long SpidrDaq::fileMaxSize()
{
  return _fileWriter->fileMaxSize();
}

// ----------------------------------------------------------------------------

void SpidrDaq::setFileMaxSize( long long size )
{
  _fileWriter->setFileMaxSize( size );
}

// ----------------------------------------------------------------------------

void SpidrDaq::setFlush( bool enable )
{
  _fileWriter->setFlush( enable );
}

// ----------------------------------------------------------------------------

void SpidrDaq::setSampling( bool enable )
{
  _fileWriter->setSampling( enable );
}

// ----------------------------------------------------------------------------

void SpidrDaq::setSampleAll( bool enable )
{
  _fileWriter->setSampleAll( enable );
}

// ----------------------------------------------------------------------------

long long SpidrDaq::bufferSize()
{
  return _packetReceiver->bufferSize();
}

// ----------------------------------------------------------------------------

bool SpidrDaq::setBufferSize( long long size )
{
  return _packetReceiver->setBufferSize( size );
}

// ----------------------------------------------------------------------------

long long SpidrDaq::maxBufferSize()
{
  return _packetReceiver->maxBufferSize();
}

// ----------------------------------------------------------------------------

bool SpidrDaq::bufferEmpty()
{
  return _packetReceiver->empty();
}

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
// Pixel data sampling
// ----------------------------------------------------------------------------

bool SpidrDaq::getSample( int max_size, int timeout_ms )
{
  return _fileWriter->getSample( 0, max_size, timeout_ms );
}

// ----------------------------------------------------------------------------

bool SpidrDaq::getSampleMin( int min_size, int max_size, int timeout_ms )
{
  return _fileWriter->getSample( min_size, max_size, timeout_ms );
}

// ----------------------------------------------------------------------------

int SpidrDaq::sampleSize()
{
  return _fileWriter->sampleSize();
}

// ----------------------------------------------------------------------------

char *SpidrDaq::sampleData()
{
  return _fileWriter->sampleData();
}

// ----------------------------------------------------------------------------

bool SpidrDaq::getFrame( int timeout_ms )
{
  return _fileWriter->getFrame( timeout_ms );
}

// ----------------------------------------------------------------------------

int SpidrDaq::frameSize()
{
  return _fileWriter->sampleSize();
}

// ----------------------------------------------------------------------------

char *SpidrDaq::frameData()
{
  return _fileWriter->sampleData();
}

// ----------------------------------------------------------------------------

bool SpidrDaq::nextPixel( int *x, int *y, int *data, int *timestamp )
{
  return _fileWriter->nextPixel( x, y, data, timestamp );
}

// ----------------------------------------------------------------------------

unsigned long long SpidrDaq::nextPixel()
{
  return _fileWriter->nextPixel();
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

long long SpidrDaq::bytesSampledCount()
{
  return _fileWriter->bytesSampled();
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
