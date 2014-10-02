#include <QTcpSocket>
#include <iomanip>
using namespace std;

#ifdef WIN32
#include <winsock2.h>  // For htonl() and ntohl()
#else
#include <arpa/inet.h> // For htonl() and ntohl()
#endif

#include "SpidrController.h"
#include "spidrtpx3cmds.h"
#include "tpx3defs.h"

#include "dacsdescr.h" // Depends on tpx3defs.h to be included first

// Version identifier: year, month, day, release number
const int   VERSION_ID = 0x14100100;
//const int VERSION_ID = 0x14093000;
//const int VERSION_ID = 0x14092900;
//const int VERSION_ID = 0x14072100;
//const int VERSION_ID = 0x14071600;
//const int VERSION_ID = 0x14070800;
//const int VERSION_ID = 0x14032400;
//const int VERSION_ID = 0x14021400;
//const int VERSION_ID = 0x14011600;
//const int VERSION_ID = 0x13112700;

// SPIDR register addresses (some of them)
#define SPIDR_CPU2TPX_WR_I           0x01C8
#define SPIDR_DEVICES_AND_PORTS_I    0x02C0
#define SPIDR_TDC_TRIGGERCOUNTER_I   0x02F8
#define SPIDR_FE_GTX_CTRL_STAT_I     0x0300
#define SPIDR_IPMUX_CONFIG_I         0x0380
#define SPIDR_UDP_PKTCOUNTER_I       0x0384
#define SPIDR_UDPMON_PKTCOUNTER_I    0x0388
#define SPIDR_UDPPAUSE_PKTCOUNTER_I  0x038C
#define SPIDR_PIXEL_PKTCOUNTER_I     0x0390
#define SPIDR_PIXEL_FILTER_I         0x0394

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

  // Initialize the local CTPR to all zeroes
  memset( static_cast<void *> (_ctpr), 0, sizeof(_ctpr) );
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
  int dummy = 0;
  *version = 0;
  return this->requestGetInt( CMD_GET_SOFTWVERSION, dummy, version );
}

// ----------------------------------------------------------------------------

bool SpidrController::getFirmwVersion( int *version )
{
  int dummy = 0;
  *version = 0;
  return this->requestGetInt( CMD_GET_FIRMWVERSION, dummy, version );
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

bool SpidrController::reset( int *errorstat )
{
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

bool SpidrController::getPortCount( int *ports )
{
  return this->requestGetInt( CMD_GET_PORTCOUNT, 0, ports );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDeviceCount( int *devices )
{
  return this->requestGetInt( CMD_GET_DEVICECOUNT, 0, devices );
}

// ----------------------------------------------------------------------------

bool SpidrController::getLinkCount( int *links )
{
  int reg;
  if( !this->getSpidrReg( SPIDR_DEVICES_AND_PORTS_I, &reg ) ) return false;
  *links = ((reg & 0xF00) >> 8) + 1;
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getSpidrId( int *id )
{
  return this->requestGetInt( CMD_GET_SPIDRID, 0, id );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSpidrId( int id )
{
  return this->requestSetInt( CMD_SET_SPIDRID, 0, id );
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
  *port_nr = 0;
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
  *port_nr = 0;
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

bool SpidrController::getHeaderFilter( int  dev_nr,
				       int *eth_mask,
				       int *cpu_mask )
{
  int mask;
  if( !this->requestGetInt( CMD_GET_HEADERFILTER, dev_nr, &mask ) )
    return false;

  *eth_mask = (mask & 0xFFFF);
  *cpu_mask = ((mask >> 16) & 0xFFFF);
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setHeaderFilter( int dev_nr,
				       int eth_mask,
				       int cpu_mask )
{
  return this->requestSetInt( CMD_SET_HEADERFILTER, dev_nr,
			      (eth_mask & 0xFFFF) |
			      ((cpu_mask & 0xFFFF) << 16) );
}

// ----------------------------------------------------------------------------
// Configuration: device
// ----------------------------------------------------------------------------

bool SpidrController::resetDevice( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_RESET_DEVICE, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::resetDevices()
{
  int dummy = 0;
  return this->requestSetInt( CMD_RESET_DEVICES, dummy, dummy );
}

// ----------------------------------------------------------------------------

bool  SpidrController::reinitDevice ( int  dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_REINIT_DEVICE, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool  SpidrController::reinitDevices()
{
  int dummy = 0;
  return this->requestSetInt( CMD_REINIT_DEVICES, dummy, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDeviceId( int dev_nr, int *id )
{
  *id = 0;
  return this->requestGetInt( CMD_GET_DEVICEID, dev_nr, id );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDeviceIds( int *ids )
{
  int nr_of_devices;
  *ids = 0;
  if( !this->getDeviceCount( &nr_of_devices ) ) return false;
  return this->requestGetInts( CMD_GET_DEVICEIDS, 0,
			       nr_of_devices, ids );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSenseDac( int dev_nr, int dac_code )
{
  return this->requestSetInt( CMD_SET_SENSEDAC, dev_nr, dac_code );
}

// ----------------------------------------------------------------------------

bool SpidrController::setExtDac( int dev_nr, int dac_code, int dac_val )
{
  // Combine dac_code and dac_val into a single int
  // (the DAC to set is the SPIDR-TPX3 DAC)
  int dac_data = ((dac_code & 0xFFFF) << 16) | (dac_val & 0xFFFF);
  return this->requestSetInt( CMD_SET_EXTDAC, dev_nr, dac_data );
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
  int dummy = 0;
  return this->requestSetInt( CMD_SET_DACS_DFLT, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::getGenConfig( int dev_nr, int *config )
{
  return this->requestGetInt( CMD_GET_GENCONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::setGenConfig( int dev_nr, int config )
{
  return this->requestSetInt( CMD_SET_GENCONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::getPllConfig( int dev_nr, int *config )
{
  return this->requestGetInt( CMD_GET_PLLCONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::setPllConfig( int dev_nr, int config )
{
  return this->requestSetInt( CMD_SET_PLLCONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::getOutBlockConfig( int dev_nr, int *config )
{
  return this->requestGetInt( CMD_GET_OUTBLOCKCONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::setOutBlockConfig( int dev_nr, int config )
{
  return this->requestSetInt( CMD_SET_OUTBLOCKCONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::setOutputMask( int dev_nr, int mask )
{
  return this->requestSetInt( CMD_SET_OUTPUTMASK, dev_nr, mask );
}

// ----------------------------------------------------------------------------

bool SpidrController::getLinkStatus( int dev_nr, int *status )
{
  return this->getSpidrReg( SPIDR_FE_GTX_CTRL_STAT_I+(dev_nr<<2), status );
}

// ----------------------------------------------------------------------------

bool SpidrController::getLinkStatus( int  dev_nr,
				     int *enabled_mask,
				     int *locked_mask )
{
  int status;
  if( !this->getLinkStatus( dev_nr, &status ) ) return false;

  // Link status: bits 0-7: 0=link enabled; bits 16-23: 1=link locked
  *enabled_mask = (~status) & 0xFF;
  *locked_mask  = (status & 0xFF0000) >> 16;
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getSlvsConfig( int dev_nr, int *config )
{
  return this->requestGetInt( CMD_GET_SLVSCONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSlvsConfig( int dev_nr, int config )
{
  return this->requestSetInt( CMD_SET_SLVSCONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::getPwrPulseConfig( int dev_nr, int *config )
{
  return this->requestGetInt( CMD_GET_PWRPULSECONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::setPwrPulseConfig( int dev_nr, int config )
{
  return this->requestSetInt( CMD_SET_PWRPULSECONFIG, dev_nr, config );
}

// ----------------------------------------------------------------------------

bool SpidrController::setPwrPulseEna( bool enable )
{
  return this->requestSetInt( CMD_PWRPULSE_ENA, 0, (int) enable );
}

// ----------------------------------------------------------------------------

bool SpidrController::setTpxPowerEna( bool enable )
{
  return this->requestSetInt( CMD_TPX_POWER_ENA, 0, (int) enable );
}

// ----------------------------------------------------------------------------

bool SpidrController::setBiasSupplyEna( bool enable )
{
  return this->requestSetInt( CMD_BIAS_SUPPLY_ENA, 0, (int) enable );
}

// ----------------------------------------------------------------------------

bool SpidrController::setBiasVoltage( int volts )
{
  // Parameter 'volts' should be between 12 and 104 Volts
  // (which is the range SPIDR-TPX3 can set)
  if( volts < 12 ) volts = 12;
  if( volts > 104 ) volts = 104;

  // Convert the volts to the appropriate DAC value
  int dac_val = ((volts - 12)*4095)/(104 - 12);

  return this->requestSetInt( CMD_SET_BIAS_ADJUST, 0, dac_val );
}

// ----------------------------------------------------------------------------

bool SpidrController::setDecodersEna( bool enable )
{
  return this->requestSetInt( CMD_DECODERS_ENA, 0, (int) enable );
}

// ----------------------------------------------------------------------------

bool SpidrController::setPeriphClk80Mhz( bool set )
{
  // Set or reset bit 24 of the CPU-to-TPX control register...
  return this->setSpidrRegBit( SPIDR_CPU2TPX_WR_I, 24, set );
}

// ----------------------------------------------------------------------------

bool SpidrController::setExtRefClk( bool set )
{
  // Set or reset bit 25 of the CPU-to-TPX control register...
  return this->setSpidrRegBit( SPIDR_CPU2TPX_WR_I, 25, set );
}

// ----------------------------------------------------------------------------

std::string SpidrController::dacName( int dac_code )
{
  int index = this->dacIndex( dac_code );
  if( index < 0 ) return string( "????" ); 
  return string( TPX3_DAC_TABLE[index].name );
}

// ----------------------------------------------------------------------------

int SpidrController::dacMax( int dac_code )
{
  int index = this->dacIndex( dac_code );
  if( index < 0 ) return 0;
  return( (1<<TPX3_DAC_TABLE[index].bits) - 1 );
}

// ----------------------------------------------------------------------------

bool SpidrController::uploadPacket( int            dev_nr,
				    unsigned char *packet,
				    int            size )
{
  return this->requestSetIntAndBytes( CMD_UPLOAD_PACKET, dev_nr,
				      size, size, packet );
}


// ----------------------------------------------------------------------------

bool SpidrController::readEfuses( int dev_nr, int *efuses )
{
  return this->requestGetInt( CMD_GET_EFUSES, dev_nr, efuses );
}
  
// ----------------------------------------------------------------------------

#ifdef CERN_PROBESTATION
bool SpidrController::burnEfuse( int dev_nr, 
                                 int prog_width, 
                                 int selection )
{
  int dword = ((selection & 0x1F) << 6) | (prog_width & 0x3F);
  return this->requestSetInt( CMD_BURN_EFUSE, dev_nr, dword );
}
#endif // CERN_PROBESTATION

// ----------------------------------------------------------------------------

#ifdef TLU
#define SPIDR_TLU_CONTROL_I  (0x02FC)
#define TLU_ENABLE_bm        1

bool SpidrController::setTluEnable( int  dev_nr, bool enable )
{
  int  tlu_reg;
  bool result = false;
  if( this->getSpidrReg( SPIDR_TLU_CONTROL_I , &tlu_reg ) )
    {
      if( enable )
        tlu_reg |= TLU_ENABLE_bm;
      else
        tlu_reg &= ~TLU_ENABLE_bm;
      if( this->setSpidrReg( SPIDR_TLU_CONTROL_I, tlu_reg ) )
	{
	  int eth_mask, cpu_mask;
	  if( this->getHeaderFilter( dev_nr, &eth_mask, &cpu_mask ) )
	    {
	      eth_mask |= 0x0020; // Let headers 0x5 pass through
	      result = this->setHeaderFilter( dev_nr, eth_mask, cpu_mask );
	    }
	}
    }
  return result;
}
#endif // TLU

// ----------------------------------------------------------------------------
// Configuration: device test pulses
// ----------------------------------------------------------------------------

bool SpidrController::getTpPeriodPhase( int dev_nr, int *period, int *phase )
{
  int tp_data;
  if( !this->requestGetInt( CMD_GET_TPPERIODPHASE, dev_nr, &tp_data ) )
    return false;

  // Extract period and phase values
  *period = (tp_data & 0xFFFF);
  *phase  = ((tp_data >> 16) & 0xFFFF);
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setTpPeriodPhase( int dev_nr, int period, int phase )
{
  // Combine period and phase into a single int
  int tp_data = ((phase & 0xFFFF) << 16) | (period & 0xFFFF);
  return this->requestSetInt( CMD_SET_TPPERIODPHASE, dev_nr, tp_data );
}

// ----------------------------------------------------------------------------

bool SpidrController::getTpNumber( int dev_nr, int *number )
{
  return this->requestGetInt( CMD_GET_TPNUMBER, dev_nr, number );
}

// ----------------------------------------------------------------------------

bool SpidrController::setTpNumber( int dev_nr, int number )
{
  return this->requestSetInt( CMD_SET_TPNUMBER, dev_nr, number );
}

// ----------------------------------------------------------------------------
/*
bool SpidrController::configCtpr( int dev_nr, int column, int val )
{
  // Combine column and val into a single int
  int ctpr = ((column & 0xFFFF) << 16) | (val & 0x0001);
  return this->requestSetInt( CMD_CONFIG_CTPR, dev_nr, ctpr );
}
*/
// ----------------------------------------------------------------------------

bool SpidrController::setCtprBit( int column, int val )
{
  if( column < 0 || column > 255 ) return false;
  if( !(val == 0 || val == 1) ) return false;
  this->setBitsBigEndianReversed( _ctpr, column, 1, val, 256 );
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setCtprBits( int val )
{
  // Set all bits in _ctpr to 0 or 1
  if( !(val == 0 || val == 1) ) return false;
  if( val == 1 ) val = 0xFFFFFFFF;
  int i;
  for( i=0; i<(256/8)/4; ++i )
    this->setBitsBigEndianReversed( _ctpr, i*32, 32, val, 256 );
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setCtpr( int dev_nr )
{
  return this->requestSetIntAndBytes( CMD_SET_CTPR, dev_nr,
				      0, // Not used for now
				      sizeof( _ctpr ), _ctpr );
}

// ----------------------------------------------------------------------------
/*
bool SpidrController::setCtprLeon( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_SET_CTPR_LEON, dev_nr, dummy );
}
*/
// ----------------------------------------------------------------------------

bool SpidrController::getCtpr( int dev_nr, unsigned char **ctpr )
{
  *ctpr = _ctpr;
  return this->requestGetBytes( CMD_GET_CTPR, dev_nr, 256/8, _ctpr );
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

const unsigned int TPX3_THRESH_CONV_TABLE[16] =
  { 0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
    0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF };

bool SpidrController::setPixelThreshold( int  x,
					 int  y,
					 int  threshold )
{
  int xstart, xend;
  int ystart, yend;
  if( !this->validXandY( x, y, &xstart, &xend, &ystart, &yend ) )
    return false;

  // Check threshold value parameter
  bool invalid_parameter = false;
  if( threshold < 0 || threshold > 15 ) invalid_parameter = true;
  if( invalid_parameter )
    {
      this->clearErrorString();
      _errString << "Invalid pixel config parameter";
      return false;
    }

  // Set or reset the 'threshold DAC tuning' bits in the requested pixels
  int xi, yi;
  unsigned char *pcfg;
  for( yi=ystart; yi<yend; ++yi )
    for( xi=xstart; xi<xend; ++xi )
      {
	pcfg = &_pixelConfig[yi*256 + xi];
	*pcfg &= ~TPX3_PIXCFG_THRESH_MASK;
	*pcfg |= (TPX3_THRESH_CONV_TABLE[threshold] << 1);
      }

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setPixelTestEna( int x, int y, bool b )
{
  return this->setPixelBit( x, y, TPX3_PIXCFG_TESTBIT, b );
}

// ----------------------------------------------------------------------------

bool SpidrController::setPixelMask( int x, int y, bool b )
{
  return this->setPixelBit( x, y, TPX3_PIXCFG_MASKBIT, b );
}

// ----------------------------------------------------------------------------

bool SpidrController::setPixelConfig( int dev_nr, int cols_per_packet )
{
  // Space for up to four columns (256 pixels each) pixel configuration data
  // in the shape of 1 byte/pixel
  // NB: under Linux the resulting packets get split into 2 parts(?), which
  //     cannot be handled by the LEON software, so use cols_per_packet=2
  unsigned char pixelcol[256*4];
  int x, y, col;

  if( cols_per_packet < 1 )
    cols_per_packet = 1;
  else if( cols_per_packet > 4 )
    cols_per_packet = 4;

  unsigned char *p;
  for( x=0; x<256; x+=cols_per_packet )
    {
      // Compile a pixel configuration column
      // from the pixel configuration data stored in _pixelConfig
      for( col=0; col<cols_per_packet; ++col )
	{
	  p = & _pixelConfig[x + col];
	  for( y=0; y<256; ++y, p+=256 ) pixelcol[col*256+y] = *p;
	  //for( y=0; y<256; ++y )
	  //  pixelcol[col*256+y] = _pixelConfig[y*256 + x+col];
	}

      // Send this column of pixel configuration data
      if( !this->requestSetIntAndBytes( CMD_SET_PIXCONF, dev_nr,
					x, // Sequence number (column)
					cols_per_packet * 256,
					pixelcol ) )
	return false;
    }
  return true;
}
/*
// SINGLE COLUMN UPLOAD:
bool SpidrController::setPixelConfig( int dev_nr )
{
  // Space for one column (256 pixels) pixel configuration data
  // in the shape of 1 byte/pixel
  unsigned char pixelcol[256];
  int x, y;
  for( x=0; x<256; ++x )
    {
      // Compile a pixel configuration column
      // from the pixel configuration data stored in _pixelConfig
      for( y=0; y<256; ++y )
	pixelcol[y] = _pixelConfig[y*256 + x];

      // Send this column of pixel configuration data
      if( !this->requestSetIntAndBytes( CMD_SET_PIXCONF, dev_nr,
					x, // Sequence number (column)
					sizeof( pixelcol ),
					pixelcol ) )
	return false;
    }
  return true;
}
*/
// ----------------------------------------------------------------------------

bool SpidrController::getPixelConfig( int dev_nr )
{
  this->resetPixelConfig();

  // Space for one column (256 pixels) pixel configuration data
  // in the shape of 1 byte/pixel
  unsigned char pixelcol[256];
  int x, y, xcopy;
  for( x=0; x<256; ++x )
    {
      // Get this column of pixel configuration data
      // (returned as 1 byte per pixel)
      xcopy = x; // The column number is required too
      if( !this->requestGetIntAndBytes( CMD_GET_PIXCONF, dev_nr,
					&xcopy,
					256, pixelcol ) )
	return false;

      // Copy the column to the pixel configuration data into _pixelConfig
      for( y=0; y<256; ++y )
	_pixelConfig[y*256 + x] = pixelcol[y];
    }
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::resetPixels( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_RESET_PIXELS, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSinglePixelFilter( int  index,
					    int  x,
					    int  y,
					    bool enable )
{
  // Convert x,y coordinates to double-column, superpixel and pixel numbers
  int dcol, spix, pix;
  dcol = x / 2;
  spix = y / 4;
  pix  = (x & 0x3) + (y & 0x1)*4;
  return this->setSinglePixelFilter( index, pix, spix, dcol, enable );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSinglePixelFilter( int  index,
					    int  pixaddr,
					    int  superpixaddr,
					    int  doublecolumn,
					    bool enable )
{
  // Enable or disable a single pixel filter
  int regval = ((pixaddr & 0x7) |
		((superpixaddr & 0x3F) << 3) |
		((doublecolumn & 0x7F) << 9));
  if( enable ) regval |= 0x10000; // Enable-filter bit
  if( index < 0 || index > 3 ) return false;
  return this->setSpidrReg( SPIDR_PIXEL_FILTER_I + (index<<2), regval );
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
  int dummy = 0;
  return this->requestSetInt( CMD_ERASE_ADDRPORTS, 0, dummy );
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
  int dummy = 0;
  return this->requestSetInt( CMD_STORE_DACS, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::eraseDacs( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_ERASE_DACS, dev_nr, dummy );
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
// Shutter Trigger
// ----------------------------------------------------------------------------

bool SpidrController::setShutterTriggerConfig( int trigger_mode,
					       int trigger_length_us,
					       int trigger_freq_hz,
					       int trigger_count )
{
  int datawords[4];
  datawords[0] = trigger_mode;
  datawords[1] = trigger_length_us;
  datawords[2] = trigger_freq_hz;
  datawords[3] = trigger_count;
  return this->requestSetInts( CMD_SET_TRIGCONFIG, 0, 4, datawords );
}

// ----------------------------------------------------------------------------

bool SpidrController::getShutterTriggerConfig( int *trigger_mode,
					       int *trigger_length_us,
					       int *trigger_freq_hz,
					       int *trigger_count )
{
  int data[4];
  if( !this->requestGetInts( CMD_GET_TRIGCONFIG, 0, 4, data ) )
    return false;
  *trigger_mode      = data[0];
  *trigger_length_us = data[1];
  *trigger_freq_hz   = data[2];
  *trigger_count     = data[3];
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::startAutoTrigger()
{
  int dummy1 = 0, dummy2 = 0;
  return this->requestSetInt( CMD_AUTOTRIG_START, dummy1, dummy2 );
}

// ----------------------------------------------------------------------------

bool SpidrController::stopAutoTrigger()
{
  int dummy1 = 0, dummy2 = 0;
  return this->requestSetInt( CMD_AUTOTRIG_STOP, dummy1, dummy2 );
}

// ----------------------------------------------------------------------------

bool SpidrController::openShutter()
{
  // Set to auto-trigger mode with number of triggers set to 0
  // and the frequency (10Hz) lower than the trigger period (200ms) allows
  //if( !this->setShutterTriggerConfig( 4, 200000, 10, 0 ) ) return false;
  // It is sufficient to set the trigger period to zero (June 2014)
  if( !this->setShutterTriggerConfig( 4, 0, 10, 1 ) ) return false;
  return this->startAutoTrigger();
}

// ----------------------------------------------------------------------------

bool SpidrController::closeShutter()
{
  if( !this->stopAutoTrigger() ) return false;
  // Set to auto-trigger mode (just in case), and to default trigger settings
  //if( !this->setShutterTriggerConfig( 4, 100000, 1, 1 ) ) return false;
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
  int dummy = 0;
  return this->requestSetInt( CMD_RESET_COUNTERS, 0, dummy );
}

// ----------------------------------------------------------------------------
// Data-acquisition
// ----------------------------------------------------------------------------

bool SpidrController::sequentialReadout( int tokens )
{
  return this->requestSetInt( CMD_SEQ_READOUT, 0, tokens );
}

// ----------------------------------------------------------------------------

bool SpidrController::datadrivenReadout()
{
  int dummy = 0;
  return this->requestSetInt( CMD_DDRIVEN_READOUT, 0, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::pauseReadout()
{
  int dummy = 0;
  return this->requestSetInt( CMD_PAUSE_READOUT, 0, dummy );
}

// ----------------------------------------------------------------------------
// Timers
// ----------------------------------------------------------------------------

bool SpidrController::restartTimers()
{
  int dummy = 0;
  return this->requestSetInt( CMD_RESTART_TIMERS, 0, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::resetTimer( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_RESET_TIMER, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::getTimer( int dev_nr,
				unsigned int *timer_lo,
				unsigned int *timer_hi )
{
  int data[2];
  if( !this->requestGetInts( CMD_GET_TIMER, dev_nr, 2, data ) )
    return false;
  unsigned int *d = (unsigned int *) data;
  *timer_lo = d[0];
  *timer_hi = d[1];
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setTimer( int dev_nr,
				unsigned int timer_lo,
				unsigned int timer_hi )
{
  int data[2];
  data[0] = timer_lo;
  data[1] = timer_hi;
  return this->requestSetInts( CMD_SET_TIMER, dev_nr, 2, data );
}

// ----------------------------------------------------------------------------

bool SpidrController::getShutterStart( int dev_nr,
				       unsigned int *timer_lo,
				       unsigned int *timer_hi )
{
  int data[2];
  if( !this->requestGetInts( CMD_GET_SHUTTERSTART, dev_nr, 2, data ) )
    return false;
  unsigned int *d = (unsigned int *) data;
  *timer_lo = d[0];
  *timer_hi = d[1];
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getShutterEnd( int dev_nr,
				     unsigned int *timer_lo,
				     unsigned int *timer_hi )
{
  int data[2];
  if( !this->requestGetInts( CMD_GET_SHUTTEREND, dev_nr, 2, data ) )
    return false;
  unsigned int *d = (unsigned int *) data;
  *timer_lo = d[0];
  *timer_hi = d[1];
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::t0Sync( int dev_nr )
{
  return this->requestSetInt( CMD_T0_SYNC, dev_nr, 0 );
}

// ----------------------------------------------------------------------------
// Monitoring
// ----------------------------------------------------------------------------

bool SpidrController::getAdc( int *adc_val, int nr_of_samples )
{
  *adc_val = nr_of_samples;
  return this->requestGetInt( CMD_GET_ADC, 0, adc_val );
}

// ----------------------------------------------------------------------------

bool SpidrController::getRemoteTemp( int *mdegrees )
{
  *mdegrees = 0;
  return this->requestGetInt( CMD_GET_REMOTETEMP, 0, mdegrees );
}

// ----------------------------------------------------------------------------

bool SpidrController::getLocalTemp( int *mdegrees )
{
  *mdegrees = 0;
  return this->requestGetInt( CMD_GET_LOCALTEMP, 0, mdegrees );
}

// ----------------------------------------------------------------------------

bool SpidrController::getFpgaTemp( int *mdegrees )
{
  *mdegrees = 0;
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

bool SpidrController::getFanSpeed( int *rpm )
{
  *rpm = 0; // Indicates which fan speed to return (SPIDR or VC707)
  return this->requestGetInt( CMD_GET_FANSPEED, 0, rpm );
}

// ----------------------------------------------------------------------------

bool SpidrController::getFanSpeedVC707( int *rpm )
{
  *rpm = 1; // Indicates which fan speed to return (SPIDR or VC707)
  return this->requestGetInt( CMD_GET_FANSPEED, 0, rpm );
}

// ----------------------------------------------------------------------------

bool SpidrController::selectChipBoard( int board_nr )
{
  return this->requestSetInt( CMD_SELECT_CHIPBOARD, 0, board_nr );
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

bool SpidrController::getPixelPacketCounter( int *cntr )
{
  return this->getSpidrReg( SPIDR_PIXEL_PKTCOUNTER_I, cntr );
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

bool SpidrController::getGpio( int *gpio_in )
{
  return this->requestGetInt( CMD_GET_GPIO, 0, gpio_in );
}

// ----------------------------------------------------------------------------

bool SpidrController::setGpio( int gpio_out )
{
  return this->requestSetInt( CMD_SET_GPIO , 0, gpio_out );
}

// ----------------------------------------------------------------------------

bool SpidrController::setGpioPin( int pin_nr, int state )
{
  int dword = ((pin_nr & 0xFFFF) << 16) | (state & 0xFFFF);
  return this->requestSetInt( CMD_SET_GPIO_PIN, 0, dword );
}

// ----------------------------------------------------------------------------

bool SpidrController::getSpidrReg( int addr, int *val )
{
  int data[2];
  data[0] = addr;
  if( !this->requestGetInts( CMD_GET_SPIDRREG, 0, 2, data ) )
    return false;
  if( data[0] != addr )
    return false;
  *val = data[1];
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setSpidrReg( int addr, int val )
{
  int data[2];
  data[0] = addr;
  data[1] = val;
  return this->requestSetInts( CMD_SET_SPIDRREG, 0, 2, data );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSpidrRegBit( int addr, int bitnr, bool set )
{
  if( bitnr < 0 || bitnr > 31 ) return false;
  int reg;
  if( !this->getSpidrReg( addr, &reg ) ) return false;
  // Set or reset bit 'bitnr' of the register...
  if( set )
    reg |= (1 << bitnr);
  else
    reg &= ~(1 << bitnr);
  return this->setSpidrReg( addr, reg );
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

  if( !_sock->waitForReadyRead( 400 ) )
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
		 << reply_len << " expected " << exp_reply_len;
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

int SpidrController::dacIndex( int dac_code )
{
  int i;
  for( i=0; i<TPX3_DAC_COUNT; ++i )
    if( TPX3_DAC_TABLE[i].code == dac_code ) return i;
  return -1;
}

// ----------------------------------------------------------------------------

static const char *TPX3_ERR_STR[] =
  {
    "no error",
    "TPX3_ERR_SC_ILLEGAL",
    "TPX3_ERR_SC_STATE",
    "TPX3_ERR_SC_ERRSTATE",
    "TPX3_ERR_SC_WORDS",
    "TPX3_ERR_TX_TIMEOUT",
    "TPX3_ERR_EMPTY",
    "TPX3_ERR_NOTEMPTY",
    "TPX3_ERR_FULL",
    "TPX3_ERR_UNEXP_REPLY",
    "TPX3_ERR_UNEXP_HEADER"
  };

static const char *MON_ERR_STR[] =
  {
    "MON_ERR_MAX6642_DAQ",
    "MON_ERR_INA219_0_DAQ",
    "ERR_INA219_1_DAQ",
    "<unknown>",
    "MON_ERR_MAX6642_INIT",
    "MON_ERR_INA219_0_INIT",
    "MON_ERR_INA219_1_INIT",
    "<unknown>"
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

std::string SpidrController::spidrErrString( int err )
{
  std::string errstr;
  unsigned int errid = err & 0xFF;
  
  if( errid >= (sizeof(ERR_STR)/sizeof(char*)) )
    errstr = "<unknown>";
  else
    errstr = ERR_STR[errid];

  if( errid == ERR_TPX3_HARDW )
    {
      errid = (err & 0xFF00) >> 8;
      errstr += ", ";
      // Error identifier is a number
      if( errid >= (sizeof(TPX3_ERR_STR)/sizeof(char*)) )
	errstr += "<unknown>";
      else
	errstr += TPX3_ERR_STR[errid];
    }
  else if( errid == ERR_MON_HARDW )
    {
      errid = (err & 0xFF00) >> 8;
      errstr += ", ";
      // Error identifier is a bitmask
      for( int bit=0; bit<8; ++bit )
	if( errid & (1<<bit) )
	  {
	    errstr += MON_ERR_STR[bit];
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
