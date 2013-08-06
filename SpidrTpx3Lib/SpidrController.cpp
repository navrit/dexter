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
const int VERSION_ID = 0x13080200;

// ----------------------------------------------------------------------------
// Constructor / destructor
// ----------------------------------------------------------------------------

SpidrController::SpidrController( int ipaddr3,
				  int ipaddr2,
				  int ipaddr1,
				  int ipaddr0,
				  int port )
{
  _sock = new QTcpSocket;

  ostringstream oss;
  oss << (ipaddr3 & 0xFF) << "." << (ipaddr2 & 0xFF) << "."
      << (ipaddr1 & 0xFF) << "." << (ipaddr0 & 0xFF);

  _sock->connectToHost( QString::fromStdString( oss.str() ), port );

  _sock->waitForConnected( 5000 );

  this->resetPixelConfig();

  _busyRequests = 0;
  _errId = 0;
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

bool SpidrController::reset()
{
  return this->requestSetInt( CMD_RESET_MODULE, 0, 0 );
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
// Configuration: module/device interface
// ----------------------------------------------------------------------------

bool SpidrController::getIpAddrDest( int port_index, int *ipaddr )
{
  return this->requestGetInt( CMD_GET_IPADDR_DEST, port_index, ipaddr );
}

// ----------------------------------------------------------------------------

bool SpidrController::setIpAddrDest( int port_index, int ipaddr )
{
  return this->requestSetInt( CMD_SET_IPADDR_DEST, port_index, ipaddr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDevicePort( int dev_nr, int *port_nr )
{
  *port_nr = 0;
  return this->requestGetInt( CMD_GET_DEVICEPORT, dev_nr, port_nr );
}

// ----------------------------------------------------------------------------

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

bool SpidrController::setDevicePort( int dev_nr, int port_nr )
{
  return this->requestSetInt( CMD_SET_DEVICEPORT, dev_nr, port_nr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getServerPort( int dev_nr, int *port_nr )
{
  *port_nr = 0;
  return this->requestGetInt( CMD_GET_SERVERPORT, dev_nr, port_nr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getServerPorts( int *port_nrs )
{
  int nr_of_ports;
  *port_nrs = 0;
  if( this->getPortCount( &nr_of_ports ) )
    return this->requestGetInts( CMD_GET_SERVERPORTS, 0,
				 nr_of_ports, port_nrs );
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::setServerPort( int dev_nr, int port_nr )
{
  return this->requestSetInt( CMD_SET_SERVERPORT, dev_nr, port_nr );
}

// ----------------------------------------------------------------------------
// Configuration: device
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
  if( this->getDeviceCount( &nr_of_devices ) )
    return this->requestGetInts( CMD_GET_DEVICEIDS, 0,
				 nr_of_devices, ids );
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::setSenseDac( int dev_nr, int dac_code )
{
  return this->requestSetInt( CMD_SET_SENSEDAC, dev_nr, dac_code );
}

// ----------------------------------------------------------------------------

bool SpidrController::setExtDac( int dev_nr, int dac_code )
{
  return this->requestSetInt( CMD_SET_EXTDAC, dev_nr, dac_code );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDac( int dev_nr, int dac_code, int *dac_val )
{
  int dac_data = dac_code;
  if( this->requestGetInt( CMD_GET_DAC, dev_nr, &dac_data ) )
    {
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
  return false;
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
// Configuration: device test pulses
// ----------------------------------------------------------------------------

bool SpidrController::getTpPeriodPhase( int dev_nr, int *period, int *phase )
{
  int tp_data;
  if( this->requestGetInt( CMD_GET_TPPERIODPHASE, dev_nr, &tp_data ) )
    {
      // Extract period and phase values
      *period = (tp_data & 0xFFFF);
      *phase  = ((tp_data >> 16) & 0xFFFF);
      return true;
    }
  return false;
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

bool SpidrController::configCtpr( int dev_nr, int column, int val )
{
  // Combine column and val into a single int
  int ctpr = ((column & 0xFFFF) << 16) | (val & 0x0001);
  return this->requestSetInt( CMD_CONFIG_CTPR, dev_nr, ctpr );
}

// ----------------------------------------------------------------------------

bool SpidrController::setCtpr( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_SET_CTPR, dev_nr, dummy );
}

// ----------------------------------------------------------------------------
// Configuration: pixels
// ----------------------------------------------------------------------------

void SpidrController::resetPixelConfig()
{
  // Set the local pixel configuration data array to all zeroes
  memset( static_cast<void *> (_pixelConfig), 0, sizeof(_pixelConfig) );
}

// ----------------------------------------------------------------------------

const unsigned int TPX3_THRESH_CONV_TABLE[16] =
  { 0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
    0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF };

bool SpidrController::configPixel( int  x,
				   int  y,
				   int  threshold,
				   bool testbit )
{
  int xstart, xend;
  int ystart, yend;
  if( !this->validXandY( x, y, &xstart, &xend, &ystart, &yend ) )
    return false;

  // Check other parameters
  bool invalid_parameter = false;
  if( threshold < 0 || threshold > 15 ) invalid_parameter = true;
  if( invalid_parameter )
    {
      this->clearErrorString();
      _errString << "Invalid pixel config parameter";
      return false;
    }

  // Set or reset the configuration bits in the requested pixels
  int xi, yi;
  unsigned int *pcfg;
  for( yi=ystart; yi<yend; ++yi )
    for( xi=xstart; xi<xend; ++xi )
      {
	pcfg = &_pixelConfig[yi][xi];
	*pcfg &= TPX3_PIXCFG_MASKBIT;
	*pcfg |= TPX3_THRESH_CONV_TABLE[threshold];
	if( testbit ) *pcfg |= TPX3_PIXCFG_TESTBIT;
      }

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::maskPixel( int x, int y )
{
  int xstart, xend;
  int ystart, yend;
  if( !this->validXandY( x, y, &xstart, &xend, &ystart, &yend ) )
    return false;

  // Set the mask bit in the requested pixels
  int xi, yi;
  for( yi=ystart; yi<yend; ++yi )
    for( xi=xstart; xi<xend; ++xi )
      _pixelConfig[yi][xi] |= TPX3_PIXCFG_MASKBIT;

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setPixelConfig( int dev_nr )
{
  // Space for one column (256 pixels) of Timepix3-formatted
  // pixel configuration data (6 bits/pixel)
  unsigned char pixelcol[(256*6)/8];
  int bit_i, byt_i;

  int col, y, pixcnf, bitmask, bit;
  for( col=0; col<256; ++col )
    {
      // Compile a pixel configuration column
      bit_i = 0;
      byt_i = 0;
      for( y=0; y<256; ++y )
	{
	  pixcnf = _pixelConfig[y][col];
	  bitmask = 0x01;
	  for( bit=0; bit<6; ++bit, bitmask<<=1, ++bit_i )
	    {
	      if( pixcnf & bitmask )
		pixelcol[byt_i] |= (1 << (bit_i & 0x7));
	      if( (bit_i & 0x7) == 7 ) ++byt_i;
	    }
	}

      // Send this column
      if( this->requestSetIntAndBytes( CMD_SET_PIXCONF, dev_nr,
				       col, // Sequence number
				       sizeof( pixelcol ),
				       pixelcol ) == false )
	return false;
    }

  return true;
}

// ----------------------------------------------------------------------------
// Trigger
// ----------------------------------------------------------------------------

bool SpidrController::setTriggerConfig( int trigger_mode,
					int trigger_period_us,
					int trigger_freq_hz,
					int nr_of_triggers )
{
  int datawords[4];
  datawords[0] = trigger_mode;
  datawords[1] = trigger_period_us;
  datawords[2] = trigger_freq_hz;
  datawords[3] = nr_of_triggers;
  return this->requestSetInts( CMD_SET_TRIGCONFIG, 0, 4, datawords );
}

// ----------------------------------------------------------------------------

bool SpidrController::getTriggerConfig( int *trigger_mode,
					int *trigger_period_us,
					int *trigger_freq_hz,
					int *nr_of_triggers )
{
  int data[4];
  int dummy = 0;
  if( !this->requestGetInts( CMD_GET_TRIGCONFIG, dummy, 4, data ) )
    return false;
  *trigger_mode        = data[0];
  *trigger_period_us   = data[1];
  *trigger_freq_hz     = data[2];
  *nr_of_triggers      = data[3];
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

bool SpidrController::triggerOneReadout()
{
  int dummy1 = 0, dummy2 = 0;
  return this->requestSetInt( CMD_TRIGGER_READOUT, dummy1, dummy2 );
}

// ----------------------------------------------------------------------------
// Data-acquisition
// ----------------------------------------------------------------------------

bool SpidrController::sequentialReadout( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_SEQ_READOUT, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::datadrivenReadout( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_DD_READOUT, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::pauseReadout( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_PAUSE_READOUT, dev_nr, dummy );
}

// ----------------------------------------------------------------------------
// Monitoring
// ----------------------------------------------------------------------------

bool SpidrController::getAdc( int dev_nr, int *adc_val )
{
  *adc_val = 0;
  return this->requestGetInt( CMD_GET_ADC, dev_nr, adc_val );
}

// ----------------------------------------------------------------------------

bool SpidrController::getRemoteTemp( int *mdegrees )
{
  int dummy = 0;
  *mdegrees = 0;
  return this->requestGetInt( CMD_GET_REMOTETEMP, dummy, mdegrees );
}

// ----------------------------------------------------------------------------

bool SpidrController::getLocalTemp( int *mdegrees )
{
  int dummy = 0;
  *mdegrees = 0;
  return this->requestGetInt( CMD_GET_LOCALTEMP, dummy, mdegrees );
}

// ----------------------------------------------------------------------------

bool SpidrController::getAvdd( int *mvolt, int *mamp, int *mwatt )
{
  return this->get3Ints( CMD_GET_AVDD, mvolt, mamp, mwatt );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDvdd( int *mvolt, int *mamp, int *mwatt )
{
  return this->get3Ints( CMD_GET_DVDD, mvolt, mamp, mwatt );
}

// ----------------------------------------------------------------------------
// Private functions
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
  int len = (4+1)*4;
  _reqMsg[0] = htonl( cmd );
  _reqMsg[1] = htonl( len );
  _reqMsg[2] = 0; // Dummy for now; reply uses this location for error status
  _reqMsg[3] = htonl( dev_nr );
  _reqMsg[4] = htonl( *dataword ); // May contain an additional parameter
  int expected_len = 5 * 4;
  if( this->request( cmd, dev_nr, len, expected_len ) )
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
  int len = (4+1)*4;
  _reqMsg[0] = htonl( cmd );
  _reqMsg[1] = htonl( len );
  _reqMsg[2] = 0; // Dummy for now; reply uses this location for error status
  _reqMsg[3] = htonl( dev_nr );
  _reqMsg[4] = 0;
  int expected_len = (4 + expected_ints) * 4;
  if( this->request( cmd, dev_nr, len, expected_len ) )
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

bool SpidrController::requestSetInt( int cmd, int dev_nr, int dataword )
{
  int len = (4+1)*4;
  _reqMsg[0] = htonl( cmd );
  _reqMsg[1] = htonl( len );
  _reqMsg[2] = 0; // Dummy for now; reply uses this location for error status
  _reqMsg[3] = htonl( dev_nr );
  _reqMsg[4] = htonl( dataword );
  int expected_len = 5 * 4;
  return this->request( cmd, dev_nr, len, expected_len );
}

// ----------------------------------------------------------------------------

bool SpidrController::requestSetInts( int cmd, int dev_nr,
				      int nwords, int *datawords )
{
  int len = (4 + nwords)*4;
  _reqMsg[0] = htonl( cmd );
  _reqMsg[1] = htonl( len );
  _reqMsg[2] = 0; // Dummy for now; reply uses this location for error status
  _reqMsg[3] = htonl( dev_nr );
  for( int i=0; i<nwords; ++i )
    _reqMsg[4+i] = htonl( datawords[i] );
  int expected_len = 5 * 4;
  return this->request( cmd, dev_nr, len, expected_len );
}

// ----------------------------------------------------------------------------

bool SpidrController::requestSetIntAndBytes( int cmd, int dev_nr,
					     int dataword,
					     int nbytes,
					     unsigned char *bytes )
{
  int len = (4+1)*4 + nbytes;
  _reqMsg[0] = htonl( cmd );
  _reqMsg[1] = htonl( len );
  _reqMsg[2] = 0; // Dummy for now; reply uses this location for error status
  _reqMsg[3] = htonl( dev_nr );
  _reqMsg[4] = htonl( dataword );
  memcpy( static_cast<void *> (&_reqMsg[5]),
	  static_cast<void *> (bytes), nbytes );
  int expected_len = 5 * 4;
  return this->request( cmd, dev_nr, len, expected_len );
}

// ----------------------------------------------------------------------------

bool SpidrController::request( int cmd,     int dev_nr,
			       int req_len, int exp_reply_len )
{
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
      _errString << "Error from SPIDR: 0x" << hex << err;
      return false;
    }
  int reply = ntohl( _replyMsg[0] );
  if( reply != (cmd | CMD_REPLY) )
    {
      this->clearErrorString();
      _errString << "Unexpected reply: 0x" << hex << reply;
      return false;
    }
  if( ntohl( _replyMsg[3] ) != dev_nr )
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
