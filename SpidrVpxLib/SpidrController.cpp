#include <QTcpSocket>
#include <iomanip>
using namespace std;

#ifdef WIN32
#include <winsock2.h>  // For htonl() and ntohl()
#else
#include <arpa/inet.h> // For htonl() and ntohl()
#endif

#include "SpidrController.h"
#include "spidrvpxcmds.h"
#include "vpxdefs.h"

#include "vpxdacsdescr.h" // Depends on vpxdefs.h to be included first

// Version identifier: year, month, day, release number
const int VERSION_ID = 0x16033000;

// SPIDR register addresses (some of them)
#define SPIDR_SHUTTERTRIG_CTRL_I     0x0290
#define SPIDR_SHUTTERTRIG_CNT_I      0x0294
#define SPIDR_SHUTTERTRIG_FREQ_I     0x0298
#define SPIDR_SHUTTERTRIG_LENGTH_I   0x029C
#define SPIDR_SHUTTERTRIG_DELAY_I    0x02AC
#define SPIDR_DEVICES_AND_PORTS_I    0x02C0
#define SPIDR_TDC_TRIGGERCOUNTER_I   0x02F8
#define SPIDR_FE_GTX_CTRL_STAT_I     0x0300
#define SPIDR_PIXEL_PKTCOUNTER_I     0x0340
#define SPIDR_IPMUX_CONFIG_I         0x0380
#define SPIDR_UDP_PKTCOUNTER_I       0x0384
#define SPIDR_UDPMON_PKTCOUNTER_I    0x0388
#define SPIDR_UDPPAUSE_PKTCOUNTER_I  0x038C

// ----------------------------------------------------------------------------
// Constructor / destructor
// ----------------------------------------------------------------------------

SpidrController::SpidrController( int ipaddr3,
				  int ipaddr2,
				  int ipaddr1,
				  int ipaddr0,
				  int port )
  : _sock( 0 ),
    _pixelConfigIndex( 0 ),
    _pixelConfig( _pixelConfigData ),
    _errId( 0 ),
    _busyRequests( 0 )
{
  _sock = new QTcpSocket;

  ostringstream oss;
  oss << (ipaddr3 & 0xFF) << "." << (ipaddr2 & 0xFF) << "."
      << (ipaddr1 & 0xFF) << "." << (ipaddr0 & 0xFF);

  _sock->connectToHost( QString::fromStdString( oss.str() ), port );

  _sock->waitForConnected( 5000 );

  // Initialize the local pixel configuration data array to all zeroes
  this->resetPixelConfig( ALL_PIXELS );
}

// ----------------------------------------------------------------------------

SpidrController::~SpidrController()
{
  if( _sock )
    {
      _sock->close();
      delete _sock;
    }
}

// ----------------------------------------------------------------------------
// Version information
// ----------------------------------------------------------------------------

int SpidrController::classVersion()
{
  return VERSION_ID;
}

// ----------------------------------------------------------------------------

bool SpidrController::getSoftwVersion( int *version )
{
  return this->requestGetInt( CMD_GET_SOFTWVERSION, 0, version );
}

// ----------------------------------------------------------------------------

bool SpidrController::getFirmwVersion( int *version )
{
  return this->requestGetInt( CMD_GET_FIRMWVERSION, 0, version );
}

// ----------------------------------------------------------------------------

std::string SpidrController::versionToString( int version )
{
  ostringstream oss;
  oss << hex << uppercase << setfill('0') << setw(8) << version;
  return oss.str();
}

// ----------------------------------------------------------------------------
// General module configuration
// ----------------------------------------------------------------------------

bool SpidrController::isConnected()
{
  return( _sock->state() == QAbstractSocket::ConnectedState );
}

// ----------------------------------------------------------------------------

std::string SpidrController::connectionStateString()
{
  QAbstractSocket::SocketState state = _sock->state();
  if( state == QAbstractSocket::UnconnectedState )
    return string( "unconnected" );
  else if( state == QAbstractSocket::HostLookupState )
    return string( "hostlookup" );
  else if( state == QAbstractSocket::ConnectingState )
    return string( "connecting" );
  else if( state == QAbstractSocket::ConnectedState )
    return string( "connected" );
  else if( state == QAbstractSocket::BoundState )
    return string( "bound" );
  else if( state == QAbstractSocket::ClosingState )
    return string( "closing" );
  else
    return string( "???" );
}

// ----------------------------------------------------------------------------

std::string SpidrController::connectionErrString()
{
  return _sock->errorString().toStdString();
}

// ----------------------------------------------------------------------------

std::string SpidrController::ipAddressString()
{
  // Return a string like: "192.168.100.10:50000"
  QString qs = _sock->peerName();
  qs += ':';
  qs += QString::number( _sock->peerPort() );
  return qs.toStdString();
}

// ----------------------------------------------------------------------------

std::string SpidrController::errorString()
{
  return _errString.str();
}

// ----------------------------------------------------------------------------

void SpidrController::clearErrorString()
{
  _errString.str( "" );
}

// ----------------------------------------------------------------------------

int SpidrController::errorId()
{
  return _errId;
}

// ----------------------------------------------------------------------------

bool SpidrController::reset( int *errorstat, int readout_speed )
{
  if( readout_speed == 1 )
    *errorstat = 0x89ABCDEF; // Magic number forcing high-speed Timepix3 readout
  else if( readout_speed == -1 )
    *errorstat = 0x12345678; // Magic number forcing low-speed Timepix3 readout
  else
    *errorstat = 0; // Use default SPIDR<->Timepix3 readout speed
  return this->requestGetInt( CMD_RESET_MODULE, 0, errorstat );
}

// ----------------------------------------------------------------------------

bool SpidrController::setBusy()
{
  return this->requestSetInt( CMD_SET_BUSY, 0, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::clearBusy()
{
  return this->requestSetInt( CMD_CLEAR_BUSY, 0, 0 );
}

// ----------------------------------------------------------------------------

void SpidrController::setBusyRequest()
{
  // To be used by the receiver threads
  this->setBusy();
  ++_busyRequests;
}

// ----------------------------------------------------------------------------

void SpidrController::clearBusyRequest()
{
  // To be used by the receiver threads
  --_busyRequests;
  if( _busyRequests == 0 ) this->clearBusy();
}

// ----------------------------------------------------------------------------

bool SpidrController::setLogLevel( int level )
{
  return this->requestSetInt( CMD_SET_LOGLEVEL, 0, level );
}

// ----------------------------------------------------------------------------

bool SpidrController::displayInfo()
{
  return this->requestSetInt( CMD_DISPLAY_INFO, 0, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDeviceCount( int *devices )
{
  return this->requestGetInt( CMD_GET_DEVICECOUNT, 0, devices );
}

// ----------------------------------------------------------------------------

bool SpidrController::getChipboardId( int *id )
{
  return this->requestGetInt( CMD_GET_CHIPBOARDID, 0, id );
}

// ----------------------------------------------------------------------------

bool SpidrController::setChipboardId( int id )
{
  return this->requestSetInt( CMD_SET_CHIPBOARDID, 0, id );
}

// ----------------------------------------------------------------------------
// Configuration: module/device interface
// ----------------------------------------------------------------------------

bool SpidrController::getIpAddrSrc( int index, int *ipaddr )
{
  return this->requestGetInt( CMD_GET_IPADDR_SRC, index, ipaddr );
}

// ----------------------------------------------------------------------------

bool SpidrController::setIpAddrSrc( int index, int ipaddr )
{
  return this->requestSetInt( CMD_SET_IPADDR_SRC, index, ipaddr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getIpAddrDest( int index, int *ipaddr )
{
  return this->requestGetInt( CMD_GET_IPADDR_DEST, index, ipaddr );
}

// ----------------------------------------------------------------------------

bool SpidrController::setIpAddrDest( int index, int ipaddr )
{
  return this->requestSetInt( CMD_SET_IPADDR_DEST, index, ipaddr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDevicePort( int index, int *port_nr )
{
  return this->requestGetInt( CMD_GET_DEVICEPORT, index, port_nr );
}

// ----------------------------------------------------------------------------
/*
### Probably won't be used, so outcommented, at least for now (Jan 2014)
bool SpidrController::getDevicePorts( int *port_nrs )
{
  int nr_of_ports;
  *port_nrs = 0;
  if( this->getPortCount( &nr_of_ports ) )
    return this->requestGetInts( CMD_GET_DEVICEPORTS, 0,
				 nr_of_ports, port_nrs );
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::setDevicePort( int index, int port_nr )
{
  return this->requestSetInt( CMD_SET_DEVICEPORT, index, port_nr );
}
*/
// ----------------------------------------------------------------------------

bool SpidrController::getServerPort( int index, int *port_nr )
{
  return this->requestGetInt( CMD_GET_SERVERPORT, index, port_nr );
}

// ----------------------------------------------------------------------------
/*
bool SpidrController::getServerPorts( int *port_nrs )
{
  int nr_of_ports;
  *port_nrs = 0;
  if( this->getPortCount( &nr_of_ports ) )
    return this->requestGetInts( CMD_GET_SERVERPORTS, 0,
				 nr_of_ports, port_nrs );
  return false;
}
*/
// ----------------------------------------------------------------------------

bool SpidrController::setServerPort( int index, int port_nr )
{
  return this->requestSetInt( CMD_SET_SERVERPORT, index, port_nr );
}

// ----------------------------------------------------------------------------
// Configuration: device
// ----------------------------------------------------------------------------

bool SpidrController::resetDevice( int dev_nr )
{
  return this->requestSetInt( CMD_RESET_DEVICE, dev_nr, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::resetDevices()
{
  return this->requestSetInt( CMD_RESET_DEVICES, 0, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDeviceId( int dev_nr, int *id )
{
  return this->requestGetInt( CMD_GET_DEVICEID, dev_nr, id );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDac( int dev_nr, int dac_code, int *dac_val )
{
  int dac_data = dac_code;
  if( !this->requestGetInt( CMD_GET_DAC, dev_nr, &dac_data ) )
    return false;

  // Extract dac_nr and dac_val
  if( (dac_data >> 16) != dac_code )
    {
      this->clearErrorString();
      _errString << "DAC code mismatch in reply";
      return false;
    }
  *dac_val = dac_data & 0xFFFF;
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setDac( int dev_nr, int dac_code, int dac_val )
{
  // Combine dac_code and dac_val into a single int
  int dac_data = ((dac_code & 0xFFFF) << 16) | (dac_val & 0xFFFF);
  return this->requestSetInt( CMD_SET_DAC, dev_nr, dac_data );
}

// ----------------------------------------------------------------------------

bool SpidrController::setDacsDflt( int dev_nr )
{
  return this->requestSetInt( CMD_SET_DACS_DFLT, dev_nr, 0 );
}

// ----------------------------------------------------------------------------

std::string SpidrController::dacName( int dac_code )
{
  int index = this->dacIndex( dac_code );
  if( index < 0 ) return string( "????" ); 
  return string( VPX_DAC_TABLE[index].name );
}

// ----------------------------------------------------------------------------

int SpidrController::dacMax( int dac_code )
{
  int index = this->dacIndex( dac_code );
  if( index < 0 ) return 0;
  return( (1<<VPX_DAC_TABLE[index].bits) - 1 );
}

// ----------------------------------------------------------------------------

bool SpidrController::readEfuses( int dev_nr, int *efuses )
{
  return this->requestGetInt( CMD_GET_EFUSES, dev_nr, efuses );
}
  
// ----------------------------------------------------------------------------
// Configuration: pixels
// ----------------------------------------------------------------------------

int SpidrController::pixelConfigCount()
{
  // The total number of (locally stored) pixel configuration
  return( sizeof(_pixelConfigData)/(256*256) );
}

// ----------------------------------------------------------------------------

int SpidrController::selectPixelConfig( int index )
{
  // Select a (locally stored) pixel configuration
  if( index >= 0 && index < this->pixelConfigCount() )
    {
      _pixelConfigIndex = index;
      _pixelConfig = &_pixelConfigData[index * 256*256];
      return index;
    }
  return -1;
}

// ----------------------------------------------------------------------------

int SpidrController::selectedPixelConfig()
{
  // What is the index of the currently selected local pixel configuration
  return _pixelConfigIndex;
}

// ----------------------------------------------------------------------------

unsigned char *SpidrController::pixelConfig( int index )
{
  // Return a pointer to the start of a pixel configuration array
  if( index == -1 )
    // Return pointer to currently active/selected pixel configuration
    return _pixelConfig;
  else if( index >= 0 && index < this->pixelConfigCount() )
    // Return pointer to requested pixel configuration
    return &_pixelConfigData[index * 256*256];
  return 0;
}

// ----------------------------------------------------------------------------

void SpidrController::resetPixelConfig( int index )
{
  if( index == -1 )
    {
      // Zero the local (active/selected) pixel configuration data array
      memset( static_cast<void *> (_pixelConfig), 0, 256*256 );
    }
  else if( index == ALL_PIXELS )
    {
      // Zero all locally stored pixel configuration data arrays
      memset( static_cast<void *> (_pixelConfigData), 0,
	      sizeof(_pixelConfigData) );
    }
  else if( index >= 0 && index < this->pixelConfigCount() )
    {
      // Zero the local pixel configuration data array indicated by 'index'
      memset( static_cast<void *> (&_pixelConfigData[index * 256*256]), 0,
	      256*256 );
    }
}

// ----------------------------------------------------------------------------

int SpidrController::comparePixelConfig( int index1, int index2 )
{
  // Select (locally stored) pixel configurations to compare,
  // return -1 in case of an invalid index
  if( index1 < 0 || index1 >= this->pixelConfigCount() ) return -1;
  if( index2 < 0 || index2 >= this->pixelConfigCount() ) return -1;
  if( index1 == index2 ) return 0; // Equal

  unsigned char *cnf1 = &_pixelConfigData[index1 * 256*256];
  unsigned char *cnf2 = &_pixelConfigData[index2 * 256*256];
  if( memcmp( static_cast<void *> (cnf1),
	      static_cast<void *> (cnf2), 256*256 ) == 0 )
    return 0; // Equal
  return 1;   // Not equal
}

// ----------------------------------------------------------------------------
// Configuration: non-volatile storage
// ----------------------------------------------------------------------------

bool SpidrController::storeAddrAndPorts( int ipaddr_src,
					 int ipport )
{
  // Store SPIDR controller and devices addresses and ports
  // to onboard non-volatile memory; at the same time changes
  // the controller's IP-address and/or port if the corresponding
  // parameter values are unequal to zero, but these values become
  // current only *after* the next hard reset or power-up
  int datawords[2];
  datawords[0] = ipaddr_src;
  datawords[1] = ipport;
  return this->requestSetInts( CMD_STORE_ADDRPORTS, 0, 2, datawords );
}

// ----------------------------------------------------------------------------

bool SpidrController::eraseAddrAndPorts()
{
  return this->requestSetInt( CMD_ERASE_ADDRPORTS, 0, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::validAddrAndPorts( bool *valid )
{
  int result = 0;
  *valid = false;
  if( this->requestGetInt( CMD_VALID_ADDRPORTS, 0, &result ) )
    {
      if( result ) *valid = true;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::storeDacs( int dev_nr )
{
  return this->requestSetInt( CMD_STORE_DACS, dev_nr, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::eraseDacs( int dev_nr )
{
  return this->requestSetInt( CMD_ERASE_DACS, dev_nr, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::validDacs( int dev_nr, bool *valid )
{
  int result = 0;
  *valid = false;
  if( this->requestGetInt( CMD_VALID_DACS, dev_nr, &result ) )
    {
      if( result ) *valid = true;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::storeStartupOptions( int startopts )
{
  // Current layout of 'startopts' parameter (15 Oct 2014):
  // - bits 31-30: 01 (indicating a valid word)
  // - bits 15-8 : bias voltage [V] [12..104], bit 15 must be 0
  // - bit 7     : enable bias powersupply at powerup
  // - bit 1     : Timepix3 output links powerup to slow (0) or fast (1)
  // - bit 0     : enable Timepix3 powersupply at powerup
  return this->requestSetInt( CMD_STORE_STARTOPTS, 0, startopts );
}

// ----------------------------------------------------------------------------

bool SpidrController::getStartupOptions( int *startopts )
{
  return this->requestGetInt( CMD_GET_STARTOPTS, 0, startopts );
}

// ----------------------------------------------------------------------------

bool SpidrController::readFlash( int            flash_id,
				 int            address,
				 int           *nbytes,
				 unsigned char *databytes )
{
  int addr = (address & 0x00FFFFFF) | (flash_id << 24);
  *nbytes = 0;
  if( !this->requestGetIntAndBytes( CMD_READ_FLASH, 0,
				    &addr, 1024, databytes ) )
    return false;

  // Returned address should match
  if( addr != ((address & 0x00FFFFFF) | (flash_id << 24)) )
    return false;

  *nbytes = 1024;
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::writeFlash( int            flash_id,
				  int            address,
				  int            nbytes,
				  unsigned char *databytes )
{
  int addr = (address & 0x00FFFFFF) | (flash_id << 24);
  return this->requestSetIntAndBytes( CMD_WRITE_FLASH, 0,
				      addr, nbytes, databytes );
}

// ----------------------------------------------------------------------------
// Shutter Trigger
// ----------------------------------------------------------------------------

bool SpidrController::setShutterTriggerConfig( int trigger_mode,
					       int trigger_length_us,
					       int trigger_freq_hz,
					       int trigger_count,
					       int trigger_delay_ns )
{
  int datawords[5];
  datawords[0] = trigger_mode;
  datawords[1] = trigger_length_us;
  datawords[2] = trigger_freq_hz;
  datawords[3] = trigger_count;
  datawords[4] = trigger_delay_ns;
  int len = 5;
  if( trigger_delay_ns == 0 ) len = 4; // No need to send (for compatibility)
  return this->requestSetInts( CMD_SET_TRIGCONFIG, 0, len, datawords );
}

// ----------------------------------------------------------------------------

bool SpidrController::getShutterTriggerConfig( int *trigger_mode,
					       int *trigger_length_us,
					       int *trigger_freq_hz,
					       int *trigger_count,
					       int *trigger_delay_ns )
{
  int data[5];
  data[4] = 0;
  if( !this->requestGetInts( CMD_GET_TRIGCONFIG, 0, 5, data ) )
    // For backwards-compatibility try a length of 4 ('delay' was added later)
    if( !this->requestGetInts( CMD_GET_TRIGCONFIG, 0, 4, data ) )
      return false;
  *trigger_mode      = data[0];
  *trigger_length_us = data[1];
  *trigger_freq_hz   = data[2];
  *trigger_count     = data[3];
  if( trigger_delay_ns ) *trigger_delay_ns = data[4];
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setShutterTriggerCfg( int trigger_mode,
					    int trigger_delay_ns,
					    int trigger_length_ns,
					    int trigger_freq_hz,
					    int trigger_count )
{
  // New version of setShutterTriggerConfig() with all times in nanoseconds
  // instead of microseconds, and all but parameter 'trigger_mode' optional
  // (uses individual register write operations)
  // NB: maximum resolution is only 25ns (at 40MHz)
  // NB: parameter order changed with respect to setShutterTriggerConfig()
  if( trigger_mode < 0 || trigger_mode > SHUTTERMODE_AUTO )
    return false; // Illegal trigger mode

  // Mode
  int reg;
  if( !this->getSpidrReg( SPIDR_SHUTTERTRIG_CTRL_I, &reg ) )
    return false;
  reg &= ~0x7;         // Clear mode
  reg |= trigger_mode; // Set mode
  if( !this->setSpidrReg( SPIDR_SHUTTERTRIG_CTRL_I, reg ) )
    return false;

  if( trigger_mode == SHUTTERMODE_AUTO )
    {
      // Frequency and count
      if( !this->setSpidrReg( SPIDR_SHUTTERTRIG_FREQ_I,
			      40000000/trigger_freq_hz ) ) return false;
      if( !this->setSpidrReg( SPIDR_SHUTTERTRIG_CNT_I,
			      trigger_count ) ) return false;
    }
  else
    {
      // Delay
      if( !this->setSpidrReg( SPIDR_SHUTTERTRIG_DELAY_I,
			      trigger_delay_ns/25 ) ) return false;
    }
  if( trigger_mode == SHUTTERMODE_AUTO ||
      trigger_mode == SHUTTERMODE_POS_EXT_TIMER ||
      trigger_mode == SHUTTERMODE_NEG_EXT_TIMER )
    {
      // Duration (length)
      if( !this->setSpidrReg( SPIDR_SHUTTERTRIG_LENGTH_I,
			      trigger_length_ns/25 ) ) return false;
    }
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getShutterTriggerCfg( int *trigger_mode,
					    int *trigger_delay_ns,
					    int *trigger_length_ns,
					    int *trigger_freq_hz,
					    int *trigger_count )
{
  // New version of getShutterTriggerConfig() with all times in nanoseconds
  // instead of microseconds, including new parameter 'trigger_delay_ns'
  // NB: maximum resolution is only 25ns (at 40MHz)
  // NB: parameter order changed with respect to getShutterTriggerConfig()
  int reg;
  // Mode
  if( trigger_mode )
    {
      if( !this->getSpidrReg( SPIDR_SHUTTERTRIG_CTRL_I, &reg ) ) return false;
      *trigger_mode = (reg & 0x7);
    }
  // Delay
  if( trigger_delay_ns )
    {
      if( !this->getSpidrReg( SPIDR_SHUTTERTRIG_DELAY_I, &reg ) ) return false;
      *trigger_delay_ns = reg * 25;
    }
  // Length/width/duration
  if( trigger_length_ns )
    {
      if( !this->getSpidrReg( SPIDR_SHUTTERTRIG_LENGTH_I, &reg ) ) return false;
      *trigger_length_ns = reg * 25;
    }
  // Frequency
  if( trigger_freq_hz )
    {
      if( !this->getSpidrReg( SPIDR_SHUTTERTRIG_FREQ_I, &reg ) ) return false;
      *trigger_freq_hz = 40000000 / reg;
    }
  // Count
  if( trigger_count )
    {
      if( !this->getSpidrReg( SPIDR_SHUTTERTRIG_CNT_I, &reg ) ) return false;
      *trigger_count = reg;
    }
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::startAutoTrigger()
{
  return this->requestSetInt( CMD_AUTOTRIG_START, 0, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::stopAutoTrigger()
{
  return this->requestSetInt( CMD_AUTOTRIG_STOP, 0, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::openShutter()
{
  // Set to auto-trigger mode with number of triggers set to 0
  // and the frequency (10Hz) lower than the trigger period (200ms) allows
  //if( !this->setShutterTriggerConfig( 4, 200000, 10, 0 ) ) return false;
  // It is sufficient to set the trigger period to zero (June 2014)
  if( !this->setShutterTriggerConfig( SHUTTERMODE_AUTO, 0, 10, 1 ) )
    return false;
  return this->startAutoTrigger();
}

// ----------------------------------------------------------------------------

bool SpidrController::closeShutter()
{
  if( !this->stopAutoTrigger() ) return false;
  // Set to auto-trigger mode (just in case), and to default trigger settings
  //if( !this->setShutterTriggerConfig( SHUTTERMODE_AUTO, 100000, 1, 1 ) )
  //  return false;
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getExtShutterCounter( int *cntr )
{
  return this->requestGetInt( CMD_GET_EXTSHUTTERCNTR, 0, cntr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getShutterCounter( int *cntr )
{
  return this->requestGetInt( CMD_GET_SHUTTERCNTR, 0, cntr );
}

// ----------------------------------------------------------------------------

bool SpidrController::resetCounters()
{
  return this->requestSetInt( CMD_RESET_COUNTERS, 0, 0 );
}

// ----------------------------------------------------------------------------

bool SpidrController::setMonitorStreamEna( bool enable )
{
  int reg;
  if( !this->getSpidrReg( SPIDR_SHUTTERTRIG_CTRL_I, &reg ) )
    return false;
  if( enable )
    reg |= 0x00001000; //SPIDR_MONPACKETS_ENA;
  else
    reg &= ~0x00001000; //~SPIDR_MONPACKETS_ENA;
  if( !this->setSpidrReg( SPIDR_SHUTTERTRIG_CTRL_I, reg ) )
    return false;
  return true;
}

// ----------------------------------------------------------------------------
// Timers
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Monitoring
// ----------------------------------------------------------------------------

bool SpidrController::getAdc( int *adc_val, int chan, int nr_of_samples )
{
  // Get the sum of a number of ADC samples of the selected SPIDR ADC channel
  *adc_val = (chan & 0xFFFF) | ((nr_of_samples & 0xFFFF) << 16);
  return this->requestGetInt( CMD_GET_SPIDR_ADC, 0, adc_val );
}

// ----------------------------------------------------------------------------

bool SpidrController::getAdc( int *adc_val, int nr_of_samples )
{
  // Get an ADC sample of the Timepix3 'DACOut' output
  // (### OBSOLETE MEMBER: use getDacOut() instead)
  *adc_val = nr_of_samples;
  return this->requestGetInt( CMD_GET_ADC, 0, adc_val );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDacOut( int  dev_nr,
				 int *dacout_val,
				 int  nr_of_samples )
{
  // Get (an) ADC sample(s) of a Timepix3 device's 'DACOut' output
  int chan = dev_nr; // Assume this is how they are connected to the ADC
  return this->getAdc( dacout_val, chan, nr_of_samples );
}

// ----------------------------------------------------------------------------

bool SpidrController::getRemoteTemp( int *mdegrees )
{
  return this->requestGetInt( CMD_GET_REMOTETEMP, 0, mdegrees );
}

// ----------------------------------------------------------------------------

bool SpidrController::getLocalTemp( int *mdegrees )
{
  return this->requestGetInt( CMD_GET_LOCALTEMP, 0, mdegrees );
}

// ----------------------------------------------------------------------------

bool SpidrController::getFpgaTemp( int *mdegrees )
{
  return this->requestGetInt( CMD_GET_FPGATEMP, 0, mdegrees );
}

// ----------------------------------------------------------------------------

bool SpidrController::getAvdd( int *mvolts, int *mamps, int *mwatts )
{
  return this->get3Ints( CMD_GET_AVDD, mvolts, mamps, mwatts );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDvdd( int *mvolts, int *mamps, int *mwatts )
{
  return this->get3Ints( CMD_GET_DVDD, mvolts, mamps, mwatts );
}

// ----------------------------------------------------------------------------

bool SpidrController::getAvddNow( int *mvolts, int *mamps, int *mwatts )
{
  return this->get3Ints( CMD_GET_AVDD_NOW, mvolts, mamps, mwatts );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDvddNow( int *mvolts, int *mamps, int *mwatts )
{
  return this->get3Ints( CMD_GET_DVDD_NOW, mvolts, mamps, mwatts );
}

// ----------------------------------------------------------------------------

bool SpidrController::getBiasVoltage( int *volts )
{
  int chan = 1; // SPIDR-TPX3 ADC input
  int adc_data = chan;
  if( this->requestGetInt( CMD_GET_SPIDR_ADC, 0, &adc_data ) )
    {
      // Full-scale is 1.5V = 1500mV
      // and 0.01V represents approximately 1V bias voltage
      *volts = (((adc_data & 0xFFF)*1500 + 4095) / 4096) / 10;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::getVdda( int *mvolts )
{
  int chan = 2; // SPIDR-TPX3 ADC input
  int adc_data = chan;
  if( this->requestGetInt( CMD_GET_SPIDR_ADC, 0, &adc_data ) )
    {
      // Full-scale is 1.5V = 1500mV;
      // this channel has a 1:2 voltage-divider
      *mvolts = (((adc_data & 0xFFF)*1500 + 4095) / 4096) * 2;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::getFanSpeed( int index, int *rpm )
{
  // Index indicates fan speed to return (chipboard or SPIDR/VC707 resp.)
  *rpm = index;
  return this->requestGetInt( CMD_GET_FANSPEED, 0, rpm );
}

// ----------------------------------------------------------------------------

bool SpidrController::setFanSpeed( int index, int percentage )
{
  // Index indicates fan speed to set (chipboard or SPIDR/VC707 resp.)
  return this->requestSetInt( CMD_SET_FANSPEED, 0, (index << 16) | percentage );
}

// ----------------------------------------------------------------------------

bool SpidrController::getHumidity( int *percentage )
{
  return this->requestGetInt( CMD_GET_HUMIDITY, 0, percentage );
}

// ----------------------------------------------------------------------------

bool SpidrController::getPressure( int *mbar )
{
  return this->requestGetInt( CMD_GET_PRESSURE, 0, mbar );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDataPacketCounter( int *cntr )
{
  return this->getSpidrReg( SPIDR_UDP_PKTCOUNTER_I, cntr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getMonPacketCounter( int *cntr )
{
  return this->getSpidrReg( SPIDR_UDPMON_PKTCOUNTER_I, cntr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getPausePacketCounter( int *cntr )
{
  return this->getSpidrReg( SPIDR_UDPPAUSE_PKTCOUNTER_I, cntr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getPixelPacketCounter( int dev_nr, int *cntr )
{
  return this->getSpidrReg( SPIDR_PIXEL_PKTCOUNTER_I + dev_nr, cntr );
}

// ----------------------------------------------------------------------------

bool SpidrController::resetPacketCounters( )
{
  bool result = true;
  if( !this->setSpidrReg( SPIDR_UDP_PKTCOUNTER_I,      0 ) ) result = false;
  if( !this->setSpidrReg( SPIDR_UDPMON_PKTCOUNTER_I,   0 ) ) result = false;
  if( !this->setSpidrReg( SPIDR_UDPPAUSE_PKTCOUNTER_I, 0 ) ) result = false;
  if( !this->setSpidrReg( SPIDR_PIXEL_PKTCOUNTER_I,    0 ) ) result = false;
  return result;
}

// ----------------------------------------------------------------------------

bool SpidrController::getTdcTriggerCounter( int *cntr )
{
  return this->getSpidrReg( SPIDR_TDC_TRIGGERCOUNTER_I, cntr );
}

// ----------------------------------------------------------------------------
// Other
// ----------------------------------------------------------------------------

bool SpidrController::getSpidrReg( int address, int *val )
{
  int data[2];
  data[0] = address;
  if( !this->requestGetInts( CMD_GET_SPIDRREG, 0, 2, data ) )
    return false;
  if( data[0] != address )
    return false;
  *val = data[1];
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setSpidrReg( int address, int val )
{
  int data[2];
  data[0] = address;
  data[1] = val;
  return this->requestSetInts( CMD_SET_SPIDRREG, 0, 2, data );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSpidrRegBit( int address, int bitnr, bool set )
{
  if( bitnr < 0 || bitnr > 31 ) return false;
  int reg;
  if( !this->getSpidrReg( address, &reg ) )
    return false;
  // Set or reset bit 'bitnr' of the register...
  if( set )
    reg |= (1 << bitnr);
  else
    reg &= ~(1 << bitnr);
  return this->setSpidrReg( address, reg );
}

// ----------------------------------------------------------------------------

bool SpidrController::getVpxReg( int address, int size, unsigned char *bytes )
{
  int addr = (address & 0x0000FFFF) | (size << 16);
  if( !this->requestGetIntAndBytes( CMD_GET_VPXREG, 0,
				    &addr, size, bytes ) )
    return false;

  // Returned address should match
  if( addr != ((address & 0x0000FFFF) | (size << 16)) ) return false;

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getVpxReg32( int address, int *val )
{
  // Assume here a 4-byte register
  *val = 0; 
  unsigned char bytes[4];
  if( !this->getVpxReg( address, 4, bytes ) )
    return false;

  // Map bytes to the integer value
  for( int i=0; i<4; ++i )
    *val |= (int) ((unsigned int) bytes[i] << (i*8));
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getVpxReg16( int address, int *val )
{
  // Assume here a 2-byte register
  *val = 0; 
  unsigned char bytes[2];
  if( !this->getVpxReg( address, 2, bytes ) )
    return false;

  // Map bytes to the integer value
  for( int i=0; i<2; ++i )
    *val |= (int) ((unsigned int) bytes[i] << (i*8));
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setVpxReg( int address, int size, unsigned char *bytes )
{
  int addr = (address & 0x0000FFFF) | (size << 16);
  return this->requestSetIntAndBytes( CMD_SET_VPXREG, 0,
				      addr, size, bytes );
}

// ----------------------------------------------------------------------------

bool SpidrController::setVpxReg32( int address, int val )
{
  // Map the integer value to a byte array
  unsigned char bytes[4];
  for( int i=0; i<4; ++i )
    bytes[i] = (unsigned char) ((val>>(i*8)) & 0xFF);

  return this->setVpxReg( address, 4, bytes );
}

// ----------------------------------------------------------------------------

bool SpidrController::setVpxReg16( int address, int val )
{
  // Map the integer value to a byte array
  unsigned char bytes[2];
  for( int i=0; i<2; ++i )
    bytes[i] = (unsigned char) ((val>>(i*8)) & 0xFF);

  return this->setVpxReg( address, 2, bytes );
}

// ----------------------------------------------------------------------------
// Private functions
// ----------------------------------------------------------------------------

bool SpidrController::setPixelBit( int x, int y, unsigned char bitmask, bool b )
{
  int xstart, xend;
  int ystart, yend;
  if( !this->validXandY( x, y, &xstart, &xend, &ystart, &yend ) )
    return false;

  // Set or unset the bit(s) in the requested pixels
  int xi, yi;
  if( b )
    {
      for( yi=ystart; yi<yend; ++yi )
	for( xi=xstart; xi<xend; ++xi )
	  _pixelConfig[yi*256 + xi] |= bitmask;
    }
  else
    {
      for( yi=ystart; yi<yend; ++yi )
	for( xi=xstart; xi<xend; ++xi )
	  _pixelConfig[yi*256 + xi] &= ~bitmask;
    }

  return true;
}

// ----------------------------------------------------------------------------

void SpidrController::setBitsBigEndianReversed( unsigned char *buffer,
						int pos,
						int nbits,
						unsigned int value,
						int array_size_in_bits )
{
  // Store bits 'big-endian': highest position first
  // but reversed within the bytes, i.e. the most-significant bit
  // is stored in bit position 0 of byte 0 (for SPIDR-TPX3).
  // This function stores up to 32 bits from 'value'.
  int bytpos, bitmask;
  unsigned char *byt;

  // Position counted from the end of the bit array
  pos     = array_size_in_bits - pos - 1;
  bytpos  = pos >> 3;
  byt     = &buffer[bytpos];
  bitmask = 1 << (7-(pos & 0x7)); // Reversed within a byte...

  while( nbits > 0 )
    {
      // Clear or set the bit
      if( value & 0x1 )
	//buffer[bytpos] |= bitmask;
	*byt |= bitmask;
      else
	//buffer[bytpos] &= ~bitmask;
	*byt &= ~bitmask;

      bitmask <<= 1;
      if( (bitmask & 0xFF) == 0 )
	{
	  bitmask = 0x01;
	  //--bytpos;
	  --byt;
	}
      value >>= 1;
      --nbits;
    }
}

// ----------------------------------------------------------------------------

bool SpidrController::get3Ints( int cmd, int *data0, int *data1, int *data2 )
{
  int data[3];
  int dummy = 0;
  if( !this->requestGetInts( cmd, dummy, 3, data ) ) return false;
  *data0 = data[0];
  *data1 = data[1];
  *data2 = data[2];
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::validXandY( int x,       int y,
				  int *xstart, int *xend,
				  int *ystart, int *yend )
{
  if( x == ALL_PIXELS )
    {
      *xstart = 0;
      *xend   = 256;
    }
  else
    {
      if( x >= 0 && x <= 255 )
	{
	  *xstart = x;
	  *xend   = x+1;
	}
      else
	{
	  this->clearErrorString();
	  _errString << "Invalid x coordinate: " << x;
	  return false;
	}
    }
  if( y == ALL_PIXELS )
    {
      *ystart = 0;
      *yend   = 256;
    }
  else
    {
      if( y >= 0 && y <= 255 )
	{
	  *ystart = y;
	  *yend   = y+1;
	}
      else
	{
	  this->clearErrorString();
	  _errString << "Invalid y coordinate: " << y;
	  return false;
	}
    }
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::requestGetInt( int cmd, int dev_nr, int *dataword )
{
  int req_len = (4+1)*4;
  _reqMsg[4] = htonl( *dataword ); // May contain an additional parameter!
  int expected_len = (4+1)*4;
  if( this->request( cmd, dev_nr, req_len, expected_len ) )
    {
      *dataword = ntohl( _replyMsg[4] );
      return true;
    }
  else
    {
      *dataword = 0;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::requestGetInts( int cmd, int dev_nr,
				      int expected_ints, int *datawords )
{
  int req_len = (4+1)*4;
  _reqMsg[4] = htonl( *datawords ); // May contain an additional parameter!
  int expected_len = (4 + expected_ints) * 4;
  if( this->request( cmd, dev_nr, req_len, expected_len ) )
    {
      int i;
      for( i=0; i<expected_ints; ++i )
	datawords[i] = ntohl( _replyMsg[4+i] );
      return true;
    }
  else
    {
      int i;
      for( i=0; i<expected_ints; ++i ) datawords[i] = 0;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::requestGetBytes( int cmd, int dev_nr,
				       int expected_bytes,
				       unsigned char *databytes )
{
  int req_len = (4+1)*4;
  _reqMsg[4] = 0;
  int expected_len = (4*4) + expected_bytes;
  if( this->request( cmd, dev_nr, req_len, expected_len ) )
    {
      memcpy( static_cast<void *> (databytes),
	      static_cast<void *> (&_replyMsg[4]), expected_bytes );
      return true;
    }
  else
    {
      int i;
      for( i=0; i<expected_bytes; ++i ) databytes[i] = 0;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::requestGetIntAndBytes( int cmd, int dev_nr,
					     int *dataword,
					     int expected_bytes,
					     unsigned char *databytes )
{
  // Send a message with 1 dataword, expect a reply with a dataword
  // and a number of bytes
  int req_len = (4+1)*4;
  _reqMsg[4] = htonl( *dataword ); // May contain an additional parameter!
  int expected_len = (4+1)*4 + expected_bytes;
  if( this->request( cmd, dev_nr, req_len, expected_len ) )
    {
      *dataword = ntohl( _replyMsg[4] );
      memcpy( static_cast<void *> (databytes),
	      static_cast<void *> (&_replyMsg[5]), expected_bytes );
      return true;
    }
  else
    {
      int i;
      for( i=0; i<expected_bytes; ++i ) databytes[i] = 0;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::requestSetInt( int cmd, int dev_nr, int dataword )
{
  int req_len = (4+1)*4;
  _reqMsg[4] = htonl( dataword );
  int expected_len = (4+1)*4;
  return this->request( cmd, dev_nr, req_len, expected_len );
}

// ----------------------------------------------------------------------------

bool SpidrController::requestSetInts( int cmd, int dev_nr,
				      int nwords, int *datawords )
{
  int req_len = (4 + nwords)*4;
  for( int i=0; i<nwords; ++i )
    _reqMsg[4+i] = htonl( datawords[i] );
  int expected_len = (4+1)*4;
  return this->request( cmd, dev_nr, req_len, expected_len );
}

// ----------------------------------------------------------------------------

bool SpidrController::requestSetIntAndBytes( int cmd, int dev_nr,
					     int dataword,
					     int nbytes,
					     unsigned char *bytes )
{
  int req_len = (4+1)*4 + nbytes;
  _reqMsg[4] = htonl( dataword );
  memcpy( static_cast<void *> (&_reqMsg[5]),
	  static_cast<void *> (bytes), nbytes );
  int expected_len = (4+1)*4;
  return this->request( cmd, dev_nr, req_len, expected_len );
}

// ----------------------------------------------------------------------------

bool SpidrController::request( int cmd,     int dev_nr,
			       int req_len, int exp_reply_len )
{
  _reqMsg[0] = htonl( cmd );
  _reqMsg[1] = htonl( req_len );
  _reqMsg[2] = 0; // Dummy for now; reply uses it to return an error status
  _reqMsg[3] = htonl( dev_nr );

  _sock->write( (const char *) _reqMsg, req_len );
  if( !_sock->waitForBytesWritten( 400 ) )
    {
      this->clearErrorString();
      _errString << "Time-out sending command";
      return false;
    }

  // Reply expected ?
  if( cmd & CMD_NOREPLY ) return true;

  if( !_sock->waitForReadyRead( 2000 ) )
    {
      this->clearErrorString();
      _errString << "Time-out receiving reply";
      return false;
    }

  int reply_len = _sock->read( (char *) _replyMsg, sizeof(_replyMsg) );
  if( reply_len < 0 )
    {
      this->clearErrorString();
      _errString << "Failed to read reply";
      return false;
    }

  // Various checks on the received reply
  if( reply_len < exp_reply_len )
    {
      this->clearErrorString();
      _errString << "Unexpected reply length, got "
		 << reply_len << ", expected at least " << exp_reply_len;
      return false;
    }
  int err = ntohl( _replyMsg[2] ); // (Check 'err' before 'reply')
  _errId = err;
  if( err != 0 )
    {
      this->clearErrorString();
      _errString << "Error from SPIDR: " << this->spidrErrString( err )
		 << " (0x" << hex << err << ")";
      return false;
    }
  int reply = ntohl( _replyMsg[0] );
  if( reply != (cmd | CMD_REPLY) )
    {
      this->clearErrorString();
      _errString << "Unexpected reply: 0x" << hex << reply;
      return false;
    }
  if( ntohl( _replyMsg[3] ) != (unsigned int) dev_nr )
    {
      this->clearErrorString();
      _errString << "Unexpected device number in reply: " << _replyMsg[3];
      return false;
    }
  return true;
}

// ----------------------------------------------------------------------------

static const char *VPX_ERR_STR[] =
  {
    "no error"
  };

static const char *SPIDR_ERR_STR[] =
  {
    "SPIDR_ERR_I2C_INIT",
    "SPIDR_ERR_LINK_INIT",
    "SPIDR_ERR_MPL_INIT",
    "SPIDR_ERR_MPU_INIT",
    "SPIDR_ERR_MAX6642_INIT",
    "SPIDR_ERR_INA219_0_INIT",
    "SPIDR_ERR_INA219_1_INIT",
    "SPIDR_ERR_I2C"
  };

static const char *STORE_ERR_STR[] =
  {
    "no error",
    "STORE_ERR_TPX",
    "STORE_ERR_WRITE",
    "STORE_ERR_WRITE_CHECK",
    "STORE_ERR_READ",
    "STORE_ERR_UNMATCHED_ID",
    "STORE_ERR_NOFLASH"
  };

static const char *MONITOR_ERR_STR[] =
  {
    "MON_ERR_TEMP_DAQ",
    "MON_ERR_POWER_DAQ",
  };

std::string SpidrController::spidrErrString( int err )
{
  std::string errstr;
  unsigned int errid = err & 0xFF;
  
  if( errid >= (sizeof(ERR_STR)/sizeof(char*)) )
    errstr = "<unknown>";
  else
    errstr = ERR_STR[errid];

  if( errid == ERR_VPX_HARDW )
    {
      errid = (err & 0xFF00) >> 8;
      errstr += ", ";
      // Error identifier is a number
      if( errid >= (sizeof(VPX_ERR_STR)/sizeof(char*)) )
	errstr += "<unknown>";
      else
	errstr += VPX_ERR_STR[errid];
    }
  else if( errid == ERR_MON_HARDW )
    {
      errid = (err & 0xFF00) >> 8;
      errstr += ", ";
      // Error identifier is a bitmask
      for( int bit=0; bit<8; ++bit )
	if( errid & (1<<bit) )
	  {
	    errstr += SPIDR_ERR_STR[bit];
	    errstr += " ";
	  }
    }
  else if( errid == ERR_FLASH_STORAGE )
    {
      errid = (err & 0xFF00) >> 8;
      errstr += ", ";
      // Error identifier is a number
      if( errid >= (sizeof(STORE_ERR_STR)/sizeof(char*)) )
	errstr += "<unknown>";
      else
	errstr += STORE_ERR_STR[errid];
    }

  return errstr;
}

// ----------------------------------------------------------------------------

int SpidrController::dacIndex( int dac_code )
{
  int i;
  for( i=0; i<VPX_DAC_COUNT; ++i )
    if( VPX_DAC_TABLE[i].code == dac_code ) return i;
  return -1;
}

// ----------------------------------------------------------------------------
