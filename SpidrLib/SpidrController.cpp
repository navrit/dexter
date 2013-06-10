#include <QTcpSocket>
#include <iomanip>
using namespace std;

#ifdef WIN32
#include <winsock2.h>  // For htonl() and ntohl()
#else
#include <arpa/inet.h> // For htonl() and ntohl()
#endif

#include "SpidrController.h"

#include "dacsdefs.h"
#include "dacsdescr.h"
#include "mpx3conf.h"
#include "spidrcmds.h"

// Version identifier: year, month, day, release number
const int VERSION_ID = 0x13060500;

// ----------------------------------------------------------------------------
// Constructor / destructor / info
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
// General
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
  // Return a string like: "192.168.1.10:50000"
  QString qs = _sock->peerName();
  qs += ':';
  qs += QString::number( _sock->peerPort() );
  return qs.toStdString();
}

// ----------------------------------------------------------------------------

std::string SpidrController::errString()
{
  return _errString.str();
}

// ----------------------------------------------------------------------------

void SpidrController::clearErrString()
{
  _errString.str( "" );
}

// ----------------------------------------------------------------------------

bool SpidrController::getIpAddrDest( int *ipaddr )
{
  return this->requestGetInt( CMD_GET_IPADDR_DEST, 0, ipaddr );
}

// ----------------------------------------------------------------------------

bool SpidrController::setIpAddrDest( int ipaddr )
{
  return this->requestSetInt( CMD_SET_IPADDR_DEST, 0, ipaddr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getMaxPacketSize( int *size )
{
  return this->requestGetInt( CMD_GET_UDPPACKET_SZ, 0, size );
}

// ----------------------------------------------------------------------------

bool SpidrController::setMaxPacketSize( int size )
{
  return this->requestSetInt( CMD_SET_UDPPACKET_SZ, 0, size );
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
// Configuration: devices
// ----------------------------------------------------------------------------

bool SpidrController::getDeviceId( int dev_nr, int *id )
{
  *id = 0;
  return this->requestGetInt( CMD_GET_DEVICEID, dev_nr, id );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDeviceIds( int *id )
{
  return this->requestGetInts( CMD_GET_DEVICEIDS, 0, 4, id );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDeviceType( int dev_nr, int *type )
{
  *type = 0;
  return this->requestGetInt( CMD_GET_DEVICETYPE, dev_nr, type );
}

// ----------------------------------------------------------------------------

bool SpidrController::setDeviceType( int dev_nr, int type )
{
  return this->requestSetInt( CMD_SET_DEVICETYPE, dev_nr, type );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDevicePort( int dev_nr, int *port_nr )
{
  *port_nr = 0;
  return this->requestGetInt( CMD_GET_DEVICEPORT, dev_nr, port_nr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDevicePorts( int *port_nr )
{
  return this->requestGetInts( CMD_GET_DEVICEPORTS, 0, 4, port_nr );
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

bool SpidrController::getServerPorts( int *port_nr )
{
  return this->requestGetInts( CMD_GET_SERVERPORTS, 0, 4, port_nr );
}

// ----------------------------------------------------------------------------

bool SpidrController::setServerPort( int dev_nr, int port_nr )
{
  return this->requestSetInt( CMD_SET_SERVERPORT, dev_nr, port_nr );
}

// ----------------------------------------------------------------------------

bool SpidrController::getDac( int dev_nr, int dac_nr, int *dac_val )
{
  int dac = dac_nr;
  if( this->requestGetInt( CMD_GET_DAC, dev_nr, &dac ) )
    {
      // Extract dac_nr and dac_val
      if( (dac >> 16) != dac_nr )
	{
	  this->clearErrString();
	  _errString << "DAC number mismatch in reply";
	  return false;
	}
      *dac_val = dac & 0xFFFF;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool SpidrController::setDac( int dev_nr, int dac_nr, int dac_val )
{
  // Combine dac_nr and dac_val into a single int
  int dac = ((dac_nr & 0xFFFF) << 16) | (dac_val & 0xFFFF);
  return this->requestSetInt( CMD_SET_DAC, dev_nr, dac );
}

// ----------------------------------------------------------------------------

bool SpidrController::setDacs( int dev_nr, int nr_of_dacs, int *dac_val )
{
  return this->requestSetInts( CMD_SET_DACS, dev_nr, nr_of_dacs, dac_val );
}

// ----------------------------------------------------------------------------

bool SpidrController::readDacs( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_READ_DACS, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::writeDacs( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_WRITE_DACS, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::writeDacsDflt( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_WRITE_DACS_DFLT, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::setCtpr( int dev_nr, int column, int val )
{
  // Combine column and val into a single int
  int ctpr = ((column & 0xFFFF) << 16) | (val & 0x0001);
  return this->requestSetInt( CMD_SET_CTPR, dev_nr, ctpr );
}

// ----------------------------------------------------------------------------

bool SpidrController::writeCtpr( int dev_nr )
{
  int dummy = 0;
  return this->requestSetInt( CMD_WRITE_CTPR, dev_nr, dummy );
}

// ----------------------------------------------------------------------------

bool SpidrController::getAcqEnable( int *mask )
{
  int dummy = 0;
  *mask = 0;
  return this->requestGetInt( CMD_GET_ACQENABLE, dummy, mask );
}

// ----------------------------------------------------------------------------

bool SpidrController::setAcqEnable( int mask )
{
  int dummy = 0;
  return this->requestSetInt( CMD_SET_ACQENABLE, dummy, mask );
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

bool SpidrController::setReady()
{
  int dummy = 0;
  return this->requestSetInt( CMD_SET_READY, dummy, dummy );
}

// ----------------------------------------------------------------------------

std::string SpidrController::dacNameMpx3( int index )
{
  if( index < 0 || index >= MPX3_DAC_COUNT ) return string( "????" ); 
  return string( MPX3_DAC_TABLE[index].name );
}

// ----------------------------------------------------------------------------

std::string SpidrController::dacNameMpx3rx( int index )
{
  if( index < 0 || index >= MPX3RX_DAC_COUNT ) return string( "????" ); 
  return string( MPX3RX_DAC_TABLE[index].name );
}

// ----------------------------------------------------------------------------

int SpidrController::dacMaxMpx3( int index )
{
  if( index < 0 || index >= MPX3_DAC_COUNT ) return 0;
  return( (1<<MPX3_DAC_TABLE[index].size)-1 );
}

// ----------------------------------------------------------------------------

int SpidrController::dacMaxMpx3rx( int index )
{
  if( index < 0 || index >= MPX3RX_DAC_COUNT ) return 0;
  return( (1<<MPX3RX_DAC_TABLE[index].size)-1 );
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

bool SpidrController::configPixelMpx3( int  x,
				       int  y,
				       int  configtha,
				       int  configthb,
				       bool configtha4,
				       bool configthb4,
				       bool gainmode,
				       bool testbit )
{
  int xstart, xend;
  int ystart, yend;
  if( !this->validXandY( x, y, &xstart, &xend, &ystart, &yend ) )
    return false;

  // Check other parameters
  bool invalid_parameter = false;
  if( configtha < 0 || configtha > 15 ) invalid_parameter = true;
  if( configthb < 0 || configthb > 15 ) invalid_parameter = true;
  if( invalid_parameter )
    {
      this->clearErrString();
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
	*pcfg &= MPX3_CFG_MASKBIT; // Set/reset elsewhere
	if( configtha & 0x1 ) *pcfg |= MPX3_CFG_CONFIGTHA_0;
	if( configtha & 0x2 ) *pcfg |= MPX3_CFG_CONFIGTHA_1;
	if( configtha & 0x4 ) *pcfg |= MPX3_CFG_CONFIGTHA_2;
	if( configtha & 0x8 ) *pcfg |= MPX3_CFG_CONFIGTHA_3;
	if( configtha4 )      *pcfg |= MPX3_CFG_CONFIGTHA_4;
	if( configthb & 0x1 ) *pcfg |= MPX3_CFG_CONFIGTHB_0;
	if( configthb & 0x2 ) *pcfg |= MPX3_CFG_CONFIGTHB_1;
	if( configthb & 0x4 ) *pcfg |= MPX3_CFG_CONFIGTHB_2;
	if( configthb & 0x8 ) *pcfg |= MPX3_CFG_CONFIGTHB_3;
	if( configthb4 )      *pcfg |= MPX3_CFG_CONFIGTHB_4;
	if( gainmode )        *pcfg |= MPX3_CFG_GAINMODE;
	if( testbit )         *pcfg |= MPX3_CFG_TESTBIT;
      }

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::maskPixelMpx3( int x, int y )
{
  return this->maskPixel( x, y, MPX3_CFG_MASKBIT );
}

// ----------------------------------------------------------------------------

bool SpidrController::writePixelConfigMpx3( int dev_nr, bool with_replies )
{
  // To be done in 2 stages:
  // 12 bits of configuration (bits 0-11) per pixel to 'Counter0', then
  // 12 bits (bits 12-23) to 'Counter1' (using variable 'counter' for that)

  // Space for one row (256 pixels) of Medipix-formatted
  // pixel configuration data
  unsigned char pixelrow[(256*12)/8];

  unsigned int   *pconfig;
  int             counter, row, column, pixelbit, pixelbitmask, bit;
  unsigned char  *prow;
  unsigned char   byte, bitmask;
  for( counter=0; counter<2; ++counter )
    {
      int hi_bit = 11 + counter*12;
      int lo_bit = counter*12;
      int cmd    = CMD_PIXCONF_MPX3_0 + counter;
      if( !with_replies ) cmd |= CMD_NOREPLY;

      // Convert the data in _pixelConfig row-by-row into the format
      // as required by the Medipix device
      for( row=0; row<256; ++row )
	{
	  // Next row
	  pconfig = &_pixelConfig[row][0];
	  // Refill pixelrow[]
	  prow = pixelrow;
	  memset( static_cast<void *> (pixelrow), 0, sizeof(pixelrow) );
	  // 12 bits of configuration data, starting from MSB
	  for( pixelbit=hi_bit; pixelbit>=lo_bit; --pixelbit )
	    {
	      pixelbitmask = (1 << pixelbit);
	      for( column=0; column<256; column+=8 )
		{
		  // Fill a byte
		  byte = 0;
		  bitmask = 0x80;
		  for( bit=0; bit<8; ++bit )
		    {
		      if( pconfig[column + bit] & pixelbitmask )
			byte |= bitmask;
		      bitmask >>= 1;
		    }
		  *prow = byte;
		  ++prow;
		}
	    }

	  // Even with 'with_replies' false, acknowledge first and last message
	  // (###NB: something goes wrong with uIP on SPIDR without it)
	  if( row == 0 || row == 255 ) cmd &= ~CMD_NOREPLY;

	  // Send this row of formatted configuration data to the SPIDR module
	  if( this->requestSetIntAndBytes( cmd, dev_nr,
					   row, // Sequence number
					   sizeof( pixelrow ),
					   pixelrow ) == false )
	    return false;
	}
    }
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::configPixelMpx3rx( int  x,
					 int  y,
					 int  discl,
					 int  disch,
					 bool testbit )
{
  int xstart, xend;
  int ystart, yend;
  if( !this->validXandY( x, y, &xstart, &xend, &ystart, &yend ) )
    return false;

  // Check other parameters
  bool invalid_parameter = false;
  if( discl < 0 || discl > 31 ) invalid_parameter = true;
  if( disch < 0 || disch > 31 ) invalid_parameter = true;
  if( invalid_parameter )
    {
      this->clearErrString();
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
	*pcfg &= MPX3RX_CFG_MASKBIT; // Set/reset elsewhere
	*pcfg |= (discl << 1);
	*pcfg |= (disch << 6);
	if( testbit ) *pcfg |= MPX3RX_CFG_TESTBIT;
      }

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::maskPixelMpx3rx( int x, int y )
{
  return this->maskPixel( x, y, MPX3RX_CFG_MASKBIT );
}

// ----------------------------------------------------------------------------

bool SpidrController::writePixelConfigMpx3rx( int dev_nr, bool with_replies )
{
  // The SPIDR software will see to it that it gets done in 2 stages:
  // the first 128 rows, then the second 128 rows...
  // (due to hardware bug in Medipix3RX device)

  // Space for one row (256 pixels) of Medipix-formatted
  // pixel configuration data
  unsigned char pixelrow[(256*12)/8];

  unsigned int   *pconfig;
  int             row, column, pixelbit, pixelbitmask, bit;
  unsigned char  *prow;
  unsigned char   byte, bitmask;
  int             cmd = CMD_PIXCONF_MPX3RX;
  if( !with_replies ) cmd |= CMD_NOREPLY;

  // Convert the data in _pixelConfig row-by-row into the format
  // as required by the Medipix device
  for( row=0; row<256; ++row )
    {
      // Next row
      pconfig = &_pixelConfig[row][0];
      // Refill pixelrow[]
      prow = pixelrow;
      memset( static_cast<void *> (pixelrow), 0, sizeof(pixelrow) );
      // 12 bits of configuration data, starting from MSB
      for( pixelbit=11; pixelbit>=0; --pixelbit )
	{
	  pixelbitmask = (1 << pixelbit);
	  for( column=0; column<256; column+=8 )
	    {
	      // Fill a byte
	      byte = 0;
	      bitmask = 0x80;
	      for( bit=0; bit<8; ++bit )
		{
		  if( pconfig[column + bit] & pixelbitmask )
		    byte |= bitmask;
		  bitmask >>= 1;
		}
	      *prow = byte;
	      ++prow;
	    }
	}

      // Even with 'with_replies' false, acknowledge first and last message
      // (###NB: something goes wrong with uIP on SPIDR without it:
      //         to be investigated... )
      if( row == 0 || row == 255 ) cmd &= ~CMD_NOREPLY;

      // Send this row of formatted configuration data to the SPIDR module
      if( this->requestSetIntAndBytes( cmd, dev_nr,
				       row, // Sequence number
				       sizeof( pixelrow ),
				       pixelrow ) == false )
	return false;
    }
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::maskPixel( int x, int y, int maskbit )
{
  int xstart, xend;
  int ystart, yend;
  if( !this->validXandY( x, y, &xstart, &xend, &ystart, &yend ) )
    return false;

  // Set the mask bit in the requested pixels
  int xi, yi;
  for( yi=ystart; yi<yend; ++yi )
    for( xi=xstart; xi<xend; ++xi )
      _pixelConfig[yi][xi] |= maskbit;

  return true;
}

// ----------------------------------------------------------------------------
// Configuration: OMR
// ----------------------------------------------------------------------------

bool SpidrController::setContRdWr( bool crw )
{
  int val = 0;
  if( crw ) val = 1;
  return this->requestSetInt( CMD_SET_CRW, 0, val );
}

// ----------------------------------------------------------------------------

bool SpidrController::setPolarity( bool polarity )
{
  int val = 0;
  if( polarity ) val = 1;
  return this->requestSetInt( CMD_SET_POLARITY, 0, val );
}

// ----------------------------------------------------------------------------

bool SpidrController::setDiscCsmSpm( int disc )
{
  // For Medipix3RX only
  return this->requestSetInt( CMD_SET_DISCCSMSPM, 0, disc );
}

// ----------------------------------------------------------------------------

bool SpidrController::setInternalTestPulse( bool internal )
{
  int val = 0;
  if( internal ) val = 1;
  return this->requestSetInt( CMD_SET_INTERNALTP, 0, val );
}

// ----------------------------------------------------------------------------

bool SpidrController::setPixelDepth( int bits )
{
  int pixelcounterdepth_id = 2; // 12-bit
  if( bits == 1 )
    pixelcounterdepth_id = 0;   // 1-bit
  else if( bits == 4 || bits == 6 )
    pixelcounterdepth_id = 1;   // 4-bit or 6-bit (RX)
  else if( bits == 24 )
    pixelcounterdepth_id = 3;   // 24-bit
  return this->requestSetInt( CMD_SET_COUNTERDEPTH, 0,
			      pixelcounterdepth_id );
}

// ----------------------------------------------------------------------------

bool SpidrController::setEqThreshH( bool equalize )
{
  // Matches 'Equalization' bit for Medipix3RX
  int val = 0;
  if( equalize ) val = 1;
  return this->requestSetInt( CMD_SET_EQTHRESHH, 0, val );
}

// ----------------------------------------------------------------------------

bool SpidrController::setColourMode( bool colour )
{
  int val = 0;
  if( colour ) val = 1;
  return this->requestSetInt( CMD_SET_COLOURMODE, 0, val );
}

// ----------------------------------------------------------------------------

bool SpidrController::setCsmSpm( int csm )
{
  // For Medipix3RX only
  return this->requestSetInt( CMD_SET_CSMSPM, 0, csm );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSenseDac( int dac_nr )
{
  return this->requestSetInt( CMD_SET_SENSEDAC, 0, dac_nr );
}

// ----------------------------------------------------------------------------

bool SpidrController::setSenseDacCode( int dac_code )
{
  return this->requestSetInt( CMD_SET_SENSEDACCODE, 0, dac_code );
}

// ----------------------------------------------------------------------------

bool SpidrController::setExtDac( int dac_nr )
{
  return this->requestSetInt( CMD_SET_EXTDAC, 0, dac_nr );
}

// ----------------------------------------------------------------------------

bool SpidrController::writeOmr( int dev_nr )
{
  return this->requestSetInt( CMD_WRITE_OMR, dev_nr, 0 );
}

// ----------------------------------------------------------------------------
// Trigger
// ----------------------------------------------------------------------------

bool SpidrController::setTriggerConfig( int trigger_mode,
					int trigger_period_us,
					int trigger_freq_hz,
					int nr_of_triggers,
					int trigger_pulse_count )
{
  int datawords[5];
  datawords[0] = trigger_mode;
  datawords[1] = trigger_period_us;
  datawords[2] = trigger_freq_hz;
  datawords[3] = nr_of_triggers;
  datawords[4] = trigger_pulse_count;
  return this->requestSetInts( CMD_SET_TRIGCONFIG, 0, 5, datawords );
}

// ----------------------------------------------------------------------------

bool SpidrController::getTriggerConfig( int *trigger_mode,
					int *trigger_period_us,
					int *trigger_freq_hz,
					int *nr_of_triggers,
					int *trigger_pulse_count )
{
  int data[5];
  int dummy = 0;
  if( !this->requestGetInts( CMD_GET_TRIGCONFIG, dummy, 5, data ) )
    return false;
  *trigger_mode        = data[0];
  *trigger_period_us   = data[1];
  *trigger_freq_hz     = data[2];
  *nr_of_triggers      = data[3];
  *trigger_pulse_count = data[4];
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

bool SpidrController::getVdd( int *mvolt, int *mamp, int *mwatt )
{
  return this->get3Ints( CMD_GET_VDD, mvolt, mamp, mwatt );
}

// ----------------------------------------------------------------------------
// Other
// ----------------------------------------------------------------------------

bool SpidrController::setLogLevel( int level )
{
  return this->requestSetInt( CMD_SET_LOGLEVEL, 0, level );
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
	  this->clearErrString();
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
	  this->clearErrString();
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
  if( !_sock->waitForBytesWritten( 1000 ) )
    {
      this->clearErrString();
      _errString << "Time-out sending command";
      return false;
    }

  // Reply expected ?
  if( cmd & CMD_NOREPLY ) return true;

  if( !_sock->waitForReadyRead( 1000 ) )
    {
      this->clearErrString();
      _errString << "Time-out receiving reply";
      return false;
    }

  int reply_len = _sock->read( (char *) _replyMsg, sizeof(_replyMsg) );
  if( reply_len < 0 )
    {
      this->clearErrString();
      _errString << "Failed to read reply";
      return false;
    }

  // Various checks on the received reply
  if( reply_len < exp_reply_len )
    {
      this->clearErrString();
      _errString << "Unexpected reply length, got "
		 << reply_len << " expected " << exp_reply_len;
      return false;
    }
  int err = ntohl( _replyMsg[2] ); // (Check 'err' before 'reply')
  if( err != 0 )
    {
      this->clearErrString();
      _errString << "Error from SPIDR: 0x" << hex << err;
      return false;
    }
  int reply = ntohl( _replyMsg[0] );
  if( reply != (cmd | CMD_REPLY) )
    {
      this->clearErrString();
      _errString << "Unexpected reply: 0x" << hex << reply;
      return false;
    }
  if( ntohl( _replyMsg[3] ) != dev_nr )
    {
      this->clearErrString();
      _errString << "Unexpected device number in reply: " << _replyMsg[3];
      return false;
    }
  return true;
}

// ----------------------------------------------------------------------------
