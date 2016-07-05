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
const int VERSION_ID = 0x16061400;

// SPIDR register addresses (some of them)
#define SPIDR_SHUTTERTRIG_CTRL_I     0x0290
#define SPIDR_SHUTTERTRIG_CNT_I      0x0294
#define SPIDR_SHUTTERTRIG_FREQ_I     0x0298
#define SPIDR_SHUTTERTRIG_LENGTH_I   0x029C
#define SPIDR_SHUTTERTRIG_DELAY_I    0x02AC
#define SPIDR_TDC_TRIGGERCOUNTER_I   0x02F8
#define SPIDR_PIXEL_PKTCOUNTER_I     0x0340
#define SPIDR_IPMUX_CONFIG_I         0x0380
#define SPIDR_UDP_PKTCOUNTER_I       0x0384
#define SPIDR_UDPMON_PKTCOUNTER_I    0x0388
#define SPIDR_UDPPAUSE_PKTCOUNTER_I  0x038C

// ----------------------------------------------------------------------------
// Velopix register and register fields descriptions

// Structure describing a Velopix register
typedef struct vpxreg_s
{
  int addr;     // Velopix register address
  const char *name;
  int nbytes;   // Register size in bytes
  int count;    // Number of registers
  int nbits;    // Size of item group in bits if its items have count > 1
} vpxreg_t;

// Structure describing a Velopix register field item
typedef struct vpxreg_item_s
{
  int id;       // Item identifier
  const char *name;
  int addr;     // Address of register containing item
  int bitindex; // Index of first bit of item
  int nbits;    // Size of item in bits
  int count;    // Number of items per register
} vpxreg_item_t;

const vpxreg_t VPX_REG[] = {
  { REG_SP,              "REG_SP",               256/8,   128, 4 },
  { REG_PIXEL,           "REG_PIXEL",            1536/8,  256, 6 },
  { REG_BX_ID,           "REG_BX_ID",            2,       1,   0 },
  { REG_EOCDP,           "REG_EOCDP",            2,       1,   0 },
  { REG_MATRIX_RST_CONF, "REG_MATRIX_RST_CONF",  2,       1,   0 },
  { REG_ROUTER,          "REG_ROUTER",           2,       1,   0 },
  { REG_GENDIGCONF,      "REG_GENDIGCONF",       2,       1,   0 },
  { REG_SLVS,            "REG_SLVS",             2,       1,   0 }
};

const vpxreg_item_t VPX_REG_ITEM[] = {
  { SP_TOT_THRESHOLD, "SP_TOT_THRESHOLD",             REG_SP,                1,  2,  64 },
  { SP_MASK_BIT,      "SP_MASK_BIT",                  REG_SP,                0,  1,  64 },

  { PIXEL_TEST_BIT, "PIXEL_TEST_BIT",                 REG_PIXEL,             5,  1, 256 },
  { PIXEL_MASK_BIT, "PIXEL_MASK_BIT",                 REG_PIXEL,             4,  1, 256 },
  { PIXEL_THR_BIT,  "PIXEL_THR_BIT",                  REG_PIXEL,             0,  4, 256 },

  { BXID_GRAY_OR_BIN,      "BXID_GRAY_OR_BIN",        REG_BX_ID,            14,  1,   1 },
  { BXID_ENABLE,           "BXID_ENABLE",             REG_BX_ID,            13,  1,   1 },
  { BXID_OVERFLOW_CONTROL, "BXID_OVERFLOW_CONTROL",   REG_BX_ID,            12,  1,   1 },
  { BXID_PRESET_VAL,       "BXID_PRESET_VAL",         REG_BX_ID,             0, 12,   1 },

  { EOCDP_BX2COL_GRAY,      "EOCDP_BX2COL_GRAY",      REG_EOCDP,             5,  1,   1 },
  { EOCDP_EN_PACKET_FILTER, "EOCDP_EN_PACKET_FILTER", REG_EOCDP,             3,  2,   1 },
  { EOCDP_BX_ID_EDGE,       "EOCDP_BX_ID_EDGE",       REG_EOCDP,             2,  1,   1 },
  { EOCDP_SHIFT_PRIOR,      "EOCDP_SHIFT_PRIOR",      REG_EOCDP,             0,  2,   1 },

  { MATRIXRSTCONF_RST_MATRIX_ON_FE_RST, "MATRIXRSTCONF_RST_MATRIX_ON_FE_RST", REG_MATRIX_RST_CONF,   9,  1,   1 },
  { MATRIXRSTCONF_RESET_DURATION,       "MATRIXRSTCONF_RESET_DURATION",       REG_MATRIX_RST_CONF,   6,  3,   1 },
  { MATRIXRSTCONF_NUM_OF_BANKS,         "MATRIXRSTCONF_NUM_OF_BANKS",         REG_MATRIX_RST_CONF,   3,  3,   1 },
  { MATRIXRSTCONF_COLS_PER_BANK,        "MATRIXRSTCONF_COLS_PER_BANK",        REG_MATRIX_RST_CONF,   0,  3,   1 },

  { ROUTER_CHAN_ENABLE, "ROUTER_CHAN_ENABLE",         REG_ROUTER,            0,  4,   1 },
};

/*
Register: General digital configuration register
{ GENDIGCONF_RESPONSE_TO_BROADCAST, "GENDIGCONF_RESPONSE_TO_BROADCAST", REG_GENDIGCONF
{ GENDIGCONF_PERIODIC_SHUTTER, "GENDIGCONF_PERIODIC_SHUTTER", REG_GENDIGCONF
{ GENDIGCONF_LINK_SHUTTER_AND_TP, "GENDIGCONF_LINK_SHUTTER_AND_TP", REG_GENDIGCONF
{ GENDIGCONF_SHUTTER_MODE, "GENDIGCONF_SHUTTER_MODE", REG_GENDIGCONF
{ GENDIGCONF_SELECT_PC_TOT, "GENDIGCONF_SELECT_PC_TOT",  REG_GENDIGCONF
{ GENDIGCONF_SELECTOVERFLOW_FULLRANGE_TOTOVERFLOW, "GENDIGCONF_SELECTOVERFLOW_FULLRANGE_TOTOVERFLOW", REG_GENDIGCONF
{ GENDIGCONF_SELECT_1HITTOT_ITOT,  "GENDIGCONF_SELECT_1HITTOT_ITOT", REG_GENDIGCONF
{ GENDIGCONF_READ_SEU,       "GENDIGCONF_READ_SEU", REG_GENDIGCONF
{ GENDIGCONF_SP_MON_MODE_EN, "GENDIGCONF_SP_MON_MODE_EN", REG_GENDIGCONF
{ GENDIGCONF_PIXEL_CONFIG_OR_LFSR, "GENDIGCONF_PIXEL_CONFIG_OR_LFSR", REG_GENDIGCONF
{ GENDIGCONF_SELECT_TP_ANALOG_DIG, "GENDIGCONF_SELECT_TP_ANALOG_DIG", REG_GENDIGCONF

Register: SLVS configuration register
{ SLVS_TX_CURRENT, SLVS_TX_CURRENT, REG_SLVS

Register: Shutter enabled register
{ SHUTTERON_SHIFT_DURATION, "SHUTTERON_SHIFT_DURATION", REG_SHUTTER_ON
{ SHUTTERON_DURATION, "SHUTTERON_DURATION", REG_SHUTTER_ON

Register: Shutter disabled register
{ SHUTTEROFF_SHIFT_DURATION, "SHUTTEROFF_SHIFT_DURATION", REG_SHUTTER_OFF
{ SHUTTEROFF_DURATION, "SHUTTEROFF_DURATION", REG_SHUTTER_OFF

Register: Bunch counter max value register
{ BXIDMAX_AUTO_RST_ON_MAX_VAL,
{ BXIDMAX_MAX_VAL,

Register: Output mode register
TFC_ALIGN_MODE
GWT_MODE
LF_MODE

Register: PacketProcessor config register
ENABLE_PROC
DISCARD_PACKETS
MAX_HITS_PER_PACKET
MAX_LATENCY

Register: Test output select register
SELECT

Register: TFC FlagCounterNeg register
FLAGCOUNTERNEG

Register: TFC FlagCounterPos register
FLAGCOUNTERPOS

Register: TFC Configuration register 1
RESETSEED
FLAGCOUNTENABLE
FLAGCOUNTRESET
RESYNC
EDGESET

Register: Fuse program config register
FUSESELECTION
FUSEPROGRAMWIDTH

Register: PLL configuration register
RESETDURATIONLL
EXTRESETVCO
BYPASSPLL

Register: DACOut register
ENABLEDACOUT
SELECTDACOUT

Register: Clock enable register
EN_CLK_320
EN_CLK_160
EN_CLK_80
EN_CLK_40
EN_CLK_40_tp

Register: Test pulse number register
TP_NUMBER

Register: Test pulse enable register
TP_PERIOD_ON

Register: Test pulse disable register
TP_PERIOD_OFF

Register: Test pulse config register
TP_CLOSE_SHUTTER
TP_PHASE
SHUTTER2TP_LAT

Register: Monitoring Encoding register
GRAY_OR_BIN

Register: Monitoring Enable register
ENABLE

Register: Readout channel config register
ECS_SEL
PAT_SEL
SCRAMBLERENABLE

Register: GWT config register 0
DLLCONFIRMCOUNTSELECT
GWT_EN
CP_CUR_8UA
CP_CUR_4UA
CP_CUR_2UA
START_UP_EN
DLLRESETDURATION

Register: GWT config register 1
THRESHOLD_FOR_DLL_LOCK

>>>>************ NOT NECESSARY *************
Register: PLL config register 0
EXTMODE
extDataRate
extReferenceFreq
extReadROMConfig
extControlOverride
extCODivideBy6
extCOEnableFD
extCOEnablePD
extCOEnablePFD
extCOInternalCalibration
extCOExternalCalibration

Register: PLL config register 1
extCOEnablePhase
extTrimResWienBridge
extResetControlBlock
extResetLockFilter

Register: PLL config register 2
extSelectClockPhaseCoarse
extSelectClockPhaseFine
extIcpFD
extIcpPD

Register: PLL config register 3
extIcpPFD
extTrimCDRCap
extTrimPLLCap
extTrimCDRRes
extTrimPLLRes

Register: PLL config register BIST0
extBISTtestCycles
extResetBIST
extBISTstartTest
extBISTmakeOneMeasurement

Register: PLL config register BIST1
extBISTfreqCounterGatingCounter

Register: PLL monitoring register 0
monDivideBy6A
monEnablePhaseB
monTrimRes
monInternalCalibrationC

Register: PLL monitoring register 1
statusSmStateB
monCapA
monResB
monEnableFDC
monEnablePDA
monIcpFDC

Register: PLL monitoring register 2
monIcpPD_PFDA
monExternalCalibrationB
statusInstantLockFilterLockedA
statusLockFilterLockedB
statusLockFilterlossOfLockCountC
statusLockFilterResetA

Register: PLL monitoring register 3
monEnablePFDB
statusSmUnlockedCounterC
monBISTsMuxEnableTestA
monBISTsMuxRampdnB
monBISTsMuxHoldC
statusBISTsmstateB

Register: PLL monitoring register BIST 0
statusBISTCounter0

Register: PLL monitoring register BIST 1
statusBISTCounter1

Register: PLL monitoring register BIST 2
statusBISTCounter2

Register: PLL monitoring register BIST 3
statusBISTCounter3

Register: PLL monitoring register BIST 4
statusBISTCounter4
<<<<************ NOT NECESSARY *************

Register: Monitoring counter config
ecs_error_mask
gwt_dll_mon_mask
enc_seu_mon
enable_seu_mon

Register: Column Test Pulse Register
tp_enable

RegisterBank: DAC Register
DAC_IPREAMP
DAC_IKRUM
DAC_IDISC
DAC_IPIXELDAC
DAC_VTPCOARSE
DAC_VTPFINE
DAC_VPREAMP_CAS
DAC_VFBK
DAC_VTHR
DAC_VCASDISC
DAC_VINCAS
DAC_VREFSLVS
DAC_IBIASSLVS
DAC_RES_BIAS

Register: Monitoring counters
COUNTER_VALUE

Register: SEU Monitoring counter
COUNTER_VALUE

Register: ECS Pattern Register2
ECS2

Register: Chip ID register
FUSE_31_15
CHIP_ID

Register: Packet header select register
INV_PACKET_HEADER
PACKET3_HEADER
PACKET2_HEADER
PACKET1_HEADER
PACKET0_HEADER

Register: Invalid packet register
inv_packet_data

Register: TFC Sync packet register
tfc_sync_packet_data

Register: TFC Latency register
DisableNoSyncBlock
ShutterDelay
SyncDelay
SnapshotDelay
CalibrationDelay
FEResetDelay
BxID_ResetDelay

Register: TFC Snapshot register
count_value

Register: ECS Snapshot register
count_value

Register: PLL Snapshot register
count_value

Register: DLL Snapshot register
count_value

Register: SEU Snapshot register
count_value

Register: Periphery command register
pcmd_op_code

Register: Pixel matrix control register
matrix_op_code
shutter_mode_ecs_tfc
tp_mode_ecs_tfc
fe_reset_mode_ecs_tfc
matrix_shutter
matrix_tp_enable
MATRIX_READOUT_ENABLE
MATRIX_RST_ENABLE

Register: Bunch ID snapshot register
BX_ID

Register: Bunch ID diff snapshot register
BX_ID_DIFF
*/
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
    _busyRequests( 0 ),
    _vpxRegStatus( 0 )
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

bool SpidrController::getServerPort( int index, int *port_nr )
{
  return this->requestGetInt( CMD_GET_SERVERPORT, index, port_nr );
}

// ----------------------------------------------------------------------------

bool SpidrController::setServerPort( int index, int port_nr )
{
  return this->requestSetInt( CMD_SET_SERVERPORT, index, port_nr );
}

// ----------------------------------------------------------------------------
// Configuration: device
// ----------------------------------------------------------------------------

bool SpidrController::getVpxItem( int item, int *value )
{
  return this->getVpxItem( item, 0, 0, value );
}

// ----------------------------------------------------------------------------

bool SpidrController::setVpxItem( int item, int value )
{
  return this->setVpxItem( item, 0, 0, value );
}

// ----------------------------------------------------------------------------

bool SpidrController::getVpxItem( int id, int item_i, int reg_i,
				  int *value )
{
  // Find the item in our item list
  int  index;
  if( !this->findVpxItem( id, item_i, &index ) )
    return false;

  // Item info
  int addr     = VPX_REG_ITEM[index].addr;
  int bitindex = VPX_REG_ITEM[index].bitindex;
  int nbits    = VPX_REG_ITEM[index].nbits;

  // Find the register containing the item in our register list
  if( !this->findVpxReg( addr, reg_i, &index ) )
    return false;

  // Read the register containing the item
  int nbytes;
  unsigned char bytes[1536/8];
  if( !this->getVpxReg( addr + reg_i, &nbytes, bytes ) )
    return false;

  // Extract the item from the register data and return in 'value',
  // i.e. return nbits starting from (offset + bitindex)-th bit
  int offset = VPX_REG[index].nbits * item_i + bitindex;
  int byte_i = offset/8;
  int bit_i  = offset - byte_i*8;
  int val    = 0;
  while( nbits > 0 )
    {
      val <<= 1;
      // Bit equal to 1?
      if( (bytes[byte_i] & (1<<bit_i)) != 0 )
	val |= 1;
      // Next bit
      ++bit_i;
      if( bit_i == 8 )
	{
	  // Next byte
	  ++byte_i;
	  bit_i = 0;
	}
      --nbits;
    }
  *value = val;

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setVpxItem( int id, int item_i, int reg_i,
				  int value )
{
  // Find the item in our item list
  int  index;
  if( !this->findVpxItem( id, item_i, &index ) )
    return false;

  // Item info
  int addr     = VPX_REG_ITEM[index].addr;
  int bitindex = VPX_REG_ITEM[index].bitindex;
  int nbits    = VPX_REG_ITEM[index].nbits;

  // Find the register containing the item in our register list
  if( !this->findVpxReg( addr, reg_i, &index ) )
    return false;

  // Read the register containing the item
  unsigned char bytes[1536/8];
  int nbytes;
  if( !this->getVpxReg( addr + reg_i, &nbytes, bytes ) )
    return false;

  // Replace the item in the register data by 'value',
  // i.e. writing nbits starting from (offset + bitindex)-th bit
  int offset = VPX_REG[index].nbits * item_i + bitindex;
  int byte_i = offset/8;
  int bit_i  = offset - byte_i*8;
  int val    = value;
  while( nbits > 0 )
    {
      if( val & 1 )
	bytes[byte_i] |= (1 << bit_i);
      else
	bytes[byte_i] &= ~(1 << bit_i);
      val >>= 1;
      // Next bit
      ++bit_i;
      if( bit_i == 8 )
	{
	  // Next byte
	  ++byte_i;
	  bit_i = 0;
	}
      --nbits;
    }

  // Now write the new value to the register
  if( !this->setVpxReg( addr + reg_i, nbytes, bytes ) )
    return false;

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getVpxReg( int address, int *size, unsigned char *bytes )
{
  // The number of bytes in this register
  int nbytes = SpidrController::vpxRegBytes( address );

  int parameter = (address & 0x0000FFFF) | (nbytes << 16);
  *size = 0;
  if( !this->requestGetIntAndBytes( CMD_GET_VPXREG, 0,
				    &parameter, nbytes, bytes ) )
    return false;
  *size = nbytes;

  // Returned address should match
  if( (parameter & 0xFFFF) != (address & 0xFFFF) )
    return false;

  // Upper 16 bits of 'parameter' contains Velopix status word
  _vpxRegStatus = (parameter >> 16) & 0xFFFF;

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::setVpxReg( int address, int size, unsigned char *bytes )
{
  int parameter = (address & 0x0000FFFF) | (size << 16);
  if( !this->requestSetIntAndBytes( CMD_SET_VPXREG, 0,
				   &parameter, size, bytes ) )
    return false;

  // Returned address should match
  if( (parameter & 0xFFFF) != (address & 0xFFFF) )
    return false;

  // Upper 16 bits of 'parameter' contains Velopix status word
  _vpxRegStatus = (parameter >> 16) & 0xFFFF;

  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::getVpxReg32( int address, int *val )
{
  // Assume here a 4-byte register
  *val = 0; 
  int nbytes;
  unsigned char bytes[1536/8]; // Enough space for any register size
  if( !this->getVpxReg( address, &nbytes, bytes ) )
    return false;

  if( nbytes != 4 )
    {
      // Not what we expected...
      this->clearErrorString();
      _errString << "Register has size " << nbytes << " bytes, not 4";
      return false;
    }

  // Map bytes to the integer value
  for( int i=0; i<4; ++i )
    *val |= (int) ((unsigned int) bytes[i] << (i*8));
  return true;
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

bool SpidrController::getVpxReg16( int address, int *val )
{
  // Assume here a 2-byte register
  *val = 0; 
  int nbytes;
  unsigned char bytes[1536/8]; // Enough space for any register size
  if( !this->getVpxReg( address, &nbytes, bytes ) )
    return false;

  if( nbytes != 2 )
    {
      // Not what we expected...
      this->clearErrorString();
      _errString << "Register has size " << nbytes << " bytes, not 2";
      return false;
    }

  // Map bytes to the integer value
  for( int i=0; i<2; ++i )
    *val |= (int) ((unsigned int) bytes[i] << (i*8));
  return true;
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
  // Index indicates fan speed to read (Velopix chipboard or SPIDR/VC707 resp.)
  if( index == 0 ) index = 2;
  *rpm = index;
  return this->requestGetInt( CMD_GET_FANSPEED, 0, rpm );
}

// ----------------------------------------------------------------------------

bool SpidrController::setFanSpeed( int index, int percentage )
{
  // Index indicates fan speed to set (Velopix chipboard or SPIDR/VC707 resp.)
  if( index == 0 ) index = 2;
  return this->requestSetInt( CMD_SET_FANSPEED, 0, (index << 16) | percentage );
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

int SpidrController::itemId( const char *item_name, std::string &info )
{
  // Search for the item name in the list and return its identifier;
  // the name should be exact or should be a unique substring;
  // return -1 if not found
  bool found = false;
  int  i, id = -1;
  for( i=0; i<sizeof(VPX_REG_ITEM)/sizeof(vpxreg_item_t); ++i )
    {
      if( strcmp(VPX_REG_ITEM[i].name, item_name) == 0 )
	{
	  found = true;
	  break;
	}
    }

  if( !found )
    {
      // Check if the name is at least unique
      int len = strlen( item_name );
      int cnt = 0;
      for( i=0; i<sizeof(VPX_REG_ITEM)/sizeof(vpxreg_item_t); ++i )
	{
	  if( strncmp(VPX_REG_ITEM[i].name, item_name, len) == 0 )
	    {
	      ++cnt;
	      id = VPX_REG_ITEM[i].id;
	      break;
	    }
	}
      if( cnt == 1 )
	found = true;
      else
	id = -1; // Not found, or found more than one option
    }

  if( found )
    {
      const vpxreg_item_t *pitem = &VPX_REG_ITEM[i];
      id = pitem->id;

      ostringstream oss;
      oss << setfill( '0' )
	  << "addr=0x" << hex << setw(4) << pitem->addr << dec
	  << " bit[" << pitem->bitindex << ".."
	  << pitem->bitindex + pitem->nbits << "]";
      if( pitem->count > 1 )
	{
	  found = false;
	  for( i=0; i<sizeof(VPX_REG)/sizeof(vpxreg_t); ++i )
	    {
	      if( VPX_REG[i].addr == pitem->addr )
		{
		  found = true;
		  break;
		}
	    }
	  if( found )
	    oss << " (" << pitem->count << " items/reg in "
		<< VPX_REG[i].count << " regs)";
	  else
	    oss << ", ###reg not found!?";
	}
      info = oss.str();
    }

  return id;
}

// ----------------------------------------------------------------------------

int SpidrController::regAddr( const char *reg_name, std::string &info )
{
  // Search for the register name in the list and return its address;
  // the name should be exact or should be a unique substring;
  // return -1 if not found
  bool found = false;
  int  i, addr = -1;
  for( i=0; i<sizeof(VPX_REG)/sizeof(vpxreg_t); ++i )
    {
      if( strcmp(VPX_REG[i].name, reg_name) == 0 )
	{
	  found = true;
	  break;
	}
    }

  if( !found )
    {
      // Check if the name is at least unique
      int len = strlen( reg_name );
      int cnt = 0;
      for( i=0; i<sizeof(VPX_REG)/sizeof(vpxreg_t); ++i )
	{
	  if( strncmp(VPX_REG[i].name, reg_name, len) == 0 )
	    {
	      ++cnt;
	      addr = VPX_REG[i].addr;
	      break;
	    }
	}
      if( cnt == 1 )
	found = true;
      else
	addr = -1; // Not found, or found more than one option
    }

  if( found )
    {
      const vpxreg_t *preg = &VPX_REG[i];
      addr = preg->addr;

      ostringstream oss;
      oss << setfill( '0' )
	  << "addr=0x" << hex << setw(4) << preg->addr << dec
	  << " bits=" << preg->nbytes * 8;
      info = oss.str();
    }

  return addr;
}

// ----------------------------------------------------------------------------

std::string SpidrController::regName( int address )
{
  // Search for the register address in the list and return its name
  std::string name( "<unknown>" );
  bool found = false;
  int  i;
  for( i=0; i<sizeof(VPX_REG)/sizeof(vpxreg_t); ++i )
    {
      if( address >= VPX_REG[i].addr &&
	  address < VPX_REG[i].addr + VPX_REG[i].count )
	{
	  found = true;
	  break;
	}
    }
  if( found )
    name = std::string( VPX_REG[i].name );
  return name;
}

// ----------------------------------------------------------------------------

int SpidrController::vpxRegBytes( int addr )
{
  // Find the register and its size in the register list
  bool found = false;
  int  i;
  for( i=0; i<sizeof(VPX_REG)/sizeof(vpxreg_t); ++i )
    {
      if( VPX_REG[i].addr == addr )
	{
	  found = true;
	  break;
	}
    }
  int nbytes = 0;
  if( found )
    nbytes = VPX_REG[i].nbytes;
  return nbytes;
}

// ----------------------------------------------------------------------------
// Private functions
// ----------------------------------------------------------------------------

bool SpidrController::findVpxItem( int id, int item_i, int *index )
{
  bool found = false;
  int  i;
  for( i=0; i<sizeof(VPX_REG_ITEM)/sizeof(vpxreg_item_t); ++i )
    {
      if( VPX_REG_ITEM[i].id == id )
	{
	  found = true;
	  break;
	}
    }
  if( !found )
    {
      this->clearErrorString();
      _errString << "Undefined item identifier: " << id;
      return false;
    }
  int item_count = VPX_REG_ITEM[i].count;
  if( item_i >= item_count || item_i < 0 )
    {
      this->clearErrorString();
      _errString << "Item index " << item_i << "out-of-range: 0.."
		 << item_count-1;
      return false;
    }
  *index = i;
  return true;
}

// ----------------------------------------------------------------------------

bool SpidrController::findVpxReg( int addr, int reg_i, int *index )
{
  // Find the register and its size in the register list
  bool found = false;
  int  i;
  for( i=0; i<sizeof(VPX_REG)/sizeof(vpxreg_t); ++i )
    {
      if( VPX_REG[i].addr == addr )
	{
	  found = true;
	  break;
	}
    }
  if( !found )
    {
      this->clearErrorString();
      _errString << "Undefined register: " << hex << addr << dec;
      return false;
    }
  int reg_count = VPX_REG[i].count;
  if( reg_i >= reg_count || reg_i < 0 )
    {
      this->clearErrorString();
      _errString << "Register index " << reg_i << "out-of-range: 0.."
		 << reg_count-1;
      return false;
    }
  *index = i;
  return true;
}

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
					     int *dataword,
					     int nbytes,
					     unsigned char *bytes )
{
  int req_len = (4+1)*4 + nbytes;
  _reqMsg[4] = htonl( *dataword );
  memcpy( static_cast<void *> (&_reqMsg[5]),
	  static_cast<void *> (bytes), nbytes );
  int expected_len = (4+1)*4;
  if( this->request( cmd, dev_nr, req_len, expected_len ) )
    {
      *dataword = ntohl( _replyMsg[4] ); // Reply may contain status
      return true;
    }
  return false;
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
    "VPX_ERR_PARAMETER",
    "VPX_ERR_RX_TIMEOUT",
    "VPX_ERR_TX_TIMEOUT",
    "VPX_ERR_EMPTY",
    "VPX_ERR_FULL",
    "VPX_ERR_NOTEMPTY",
    "VPX_ERR_UNEXP_REPLY",
    "VPX_ERR_REPLY"
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
      // Error identifier is a bitmask
      for( int bit=0; bit<8; ++bit )
	if( errid & (1<<bit) )
	  {
	    errstr += VPX_ERR_STR[bit];
	    errstr += " ";
	  }
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
