#include "SpidrController.h"
#include "SpidrDaq.h"
#include "ReceiverThread.h"
#include "DatasamplerThread.h"

#include "tpx3defs.h"
#include "tpx3dacsdescr.h"

// Version identifier: year, month, day, release number
const   int VERSION_ID = 0x15031300;
//const int VERSION_ID = 0x14102800;
//const int VERSION_ID = 0x14101300;
//const int VERSION_ID = 0x14092900;
//const int VERSION_ID = 0x14072400;

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

  // Allocate default buffer size
  _packetReceiver = new ReceiverThread( ipaddr, port );

  // Create the sampler/file-writer thread, providing it with a link to
  // the packet reader thread
  _fileWriter = new DatasamplerThread( _packetReceiver );
}

// ----------------------------------------------------------------------------

SpidrDaq::SpidrDaq( SpidrController *spidrctrl,
		    long long        bufsize,
		    int              device_nr )
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

  _packetReceiver = new ReceiverThread( ipaddr, port, bufsize );

  // Create the sampler/file-writer thread, providing it with a link to
  // the packet receiver thread
  _fileWriter = new DatasamplerThread( _packetReceiver );

  if( spidrctrl )
    {
      // Provide the receiver thread with the capability to control the module,
      // i.e. to set and clear a busy/inhibit/throttle signal
      _packetReceiver->setController( spidrctrl );
      // The controller will be used to assemble the file header
      // (see startRecording())
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
			       std::string descr,
			       bool        include_pixelcfg,
			       int         pixelcfg_index )
{
  // Fill in the file and device header with the current settings
  SpidrTpx3Header_t *fhdr = _fileWriter->fileHdr();
  unsigned char *pixelconfig = 0;

  // Initialize the (SPIDR-TPX3) file header
  memset( static_cast<void *> (fhdr), 0, SPIDRTPX3_HEADER_SIZE );
  memset( static_cast<void *> (&fhdr->unused), HEADER_FILLER,
	  sizeof(fhdr->unused) );
  fhdr->headerId        = SPIDR_HEADER_ID;
  // To be adjusted if the device pixel configuration is added:
  fhdr->headerSizeTotal = SPIDRTPX3_HEADER_SIZE;// Increase if pixconf added
  fhdr->headerSize      = SPIDRTPX3_HEADER_SIZE - TPX3_HEADER_SIZE;
  fhdr->format          = SPIDR_HEADER_VERSION;
  Tpx3Header_t *thdr = &fhdr->devHeader;
  memset( static_cast<void *> (&thdr->unused), HEADER_FILLER,
	  sizeof(thdr->unused) );
  thdr->headerId           = TPX3_HEADER_ID;
  thdr->headerSizeTotal    = TPX3_HEADER_SIZE; // Increased when pixconf added
  thdr->headerSize         = TPX3_HEADER_SIZE;
  thdr->format             = TPX3_HEADER_VERSION | TPX3_FAMILY_TYPE;

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
  int eth_filter, cpu_filter;
  if( _spidrCtrl )
    {
      if( include_pixelcfg )
	{
	  // By default use the pixelconfiguration index matching
	  // the device number given in the constructor
	  if( pixelcfg_index == -1 )
	    pixelcfg_index = _deviceNr;
	  pixelconfig = _spidrCtrl->pixelConfig( pixelcfg_index );
	  // Adjust the (total) header sizes when appropriate
	  if( pixelconfig )
	    {
	      fhdr->headerSizeTotal += 256*256;
	      fhdr->devHeader.headerSizeTotal += 256*256;
	    }
	}

      int val;
      if( _spidrCtrl->getChipboardId( &val ) )
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

      // Header filter (two 16-bit significant values)
      if( _spidrCtrl->getHeaderFilter( _deviceNr, &eth_filter, &cpu_filter ) )
	fhdr->spidrFilter = eth_filter | (cpu_filter << 16);
      else
	fhdr->spidrFilter = INVALID_VAL;

      // Bias voltage; if necessary select proper chipboard I2C-bus
      if( _deviceNr == 1 ) _spidrCtrl->selectChipBoard( 2 );
      if( _spidrCtrl->getBiasVoltage( &val ) )
	fhdr->spidrBiasVoltage = val;
      else
	fhdr->spidrBiasVoltage = INVALID_VAL;
      // If necessary deselect chipboard I2C-bus
      if( _deviceNr == 1 ) _spidrCtrl->selectChipBoard( 1 );
    }

  // Fill in the rest of the device header
  if( _spidrCtrl )
    {
      // If set, temporarily disable Timepix3
      // periphery reply packets in UDP stream
      if( eth_filter & 0xF3FF )
	_spidrCtrl->setHeaderFilter( _deviceNr,
				     eth_filter & ~0xF3FF, cpu_filter );

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

      // If set, reenable Timepix3 periphery reply packets in UDP stream
      // (was disabled further up)
      if( eth_filter & 0xF3FF )
	_spidrCtrl->setHeaderFilter( _deviceNr, eth_filter, cpu_filter );
    }

  return _fileWriter->startRecording( filename, runnr, pixelconfig );
}

// ----------------------------------------------------------------------------

bool SpidrDaq::stopRecording()
{
  return _fileWriter->stopRecording();
}

// ----------------------------------------------------------------------------

std::string SpidrDaq::fileName()
{
  return _fileWriter->fileName();
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

unsigned long long SpidrDaq::nextPacket()
{
  return _fileWriter->nextPacket();
}

// ----------------------------------------------------------------------------

void SpidrDaq::setBigEndian( bool b )
{
  _fileWriter->setBigEndian( b );
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

long long SpidrDaq::bytesRecordedCount()
{
  return _fileWriter->bytesRecorded();
}

// ----------------------------------------------------------------------------

long long SpidrDaq::bytesRecordedInRunCount()
{
  return _fileWriter->bytesRecordedInRun();
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
// Added by Martin v B
bool SpidrDaq::setFileCntr( int cntr )
{
  // May be used to control the file 'sequence' numbering when writing data
  // to files, but only works for the 2nd and next files created during
  // the same run number; the 1st file always has sequence number 1.
  if( _fileWriter && cntr > 0 ) // Check if _fileWriter object exists
    {
      _fileWriter->setFileCntr( cntr );
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------
