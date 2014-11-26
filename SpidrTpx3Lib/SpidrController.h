#ifndef SPIDRCONTROLLER_H
#define SPIDRCONTROLLER_H

#ifdef WIN32
 // Differentiate between building the DLL or using it
 #ifdef MY_LIB_EXPORT
 #define MY_LIB_API __declspec(dllexport)
 #else
 #define MY_LIB_API __declspec(dllimport)
 #endif
 //#include "stdint.h"
#else
 // For Linux
 #define MY_LIB_API
 //#include <stdint.h>
#endif // WIN32
#include <string>
#include <sstream>

#define ALL_PIXELS  256

// Shutter trigger modes
#define SHUTTERMODE_POS_EXT       0
#define SHUTTERMODE_NEG_EXT       1
#define SHUTTERMODE_POS_EXT_TIMER 2
#define SHUTTERMODE_NEG_EXT_TIMER 3
#define SHUTTERMODE_AUTO          4

class QTcpSocket;

class MY_LIB_API SpidrController
{
 public:
  // C'tor, d'tor
  SpidrController            ( int ipaddr3, int ipaddr2,
                               int ipaddr1, int ipaddr0,
                               int port = 50000 );
  ~SpidrController           ( );

  // Version information
  int         classVersion   ( );              // Version of this class
  bool        getSoftwVersion( int *version ); // SPIDR LEON3 software version
  bool        getFirmwVersion( int *version ); // SPIDR FPGA firmware version
  std::string versionToString( int  version ); // Utility function

  // General module configuration
  bool        isConnected          ( );
  std::string connectionStateString( );
  std::string connectionErrString  ( );
  std::string ipAddressString      ( );
  std::string errorString          ( );
  void        clearErrorString     ( );
  int         errorId              ( );
  bool        reset                ( int *errorstat,
                                     int  readout_speed = 0 );
  bool        setBusy              ( );
  bool        clearBusy            ( );
  void        setBusyRequest       ( ); // For internal use
  void        clearBusyRequest     ( ); // For internal use
  bool        setLogLevel          ( int level );
  bool        displayInfo          ( ); // In the currently open telnet window
                                        // or (USB/UART) console
  bool        getPortCount         ( int *ports );
  bool        getDeviceCount       ( int *devices );
  bool        getLinkCount         ( int *links );
  bool        getSpidrId           ( int *id );
  bool        setSpidrId           ( int  id );

  // ###TODO:
  bool        setTimeOfDay         ( ); // Set the SPIDR processor clock time

  // Configuration: module/device interface
  bool        getIpAddrSrc     ( int  index,  int *ipaddr );
  bool        setIpAddrSrc     ( int  index,  int  ipaddr );
  bool        getIpAddrDest    ( int  index,  int *ipaddr );
  bool        setIpAddrDest    ( int  index,  int  ipaddr );
  bool        getDevicePort    ( int  index,  int *port_nr );
  bool        getServerPort    ( int  index,  int *port_nr );
  bool        setServerPort    ( int  index,  int  port_nr );
  bool        getHeaderFilter  ( int  dev_nr, int *eth_mask, int *cpu_mask );
  bool        setHeaderFilter  ( int  dev_nr, int  eth_mask, int  cpu_mask );

  // Configuration: device
  bool        resetDevice      ( int  dev_nr );
  bool        resetDevices     ();
  bool        reinitDevice     ( int  dev_nr );
  bool        reinitDevices    ();
  bool        getDeviceId      ( int  dev_nr, int *id );
  bool        getDeviceIds     ( int *ids );
  bool        setSenseDac      ( int  dev_nr, int  dac_code );
  bool        setExtDac        ( int  dev_nr, int  dac_code, int  dac_val );
  bool        getDac           ( int  dev_nr, int  dac_code, int *dac_val );
  bool        setDac           ( int  dev_nr, int  dac_code, int  dac_val );
  bool        setDacsDflt      ( int  dev_nr );
  bool        getGenConfig     ( int  dev_nr, int *config );
  bool        setGenConfig     ( int  dev_nr, int  config );
  bool        getPllConfig     ( int  dev_nr, int *config );
  bool        setPllConfig     ( int  dev_nr, int  config );
  bool        getOutBlockConfig( int  dev_nr, int *config );
  bool        setOutBlockConfig( int  dev_nr, int  config );
  bool        setOutputMask    ( int  dev_nr, int  mask );
  bool        setReadoutSpeed  ( int  dev_nr, int  mbits_per_sec );
  bool        getReadoutSpeed  ( int  dev_nr, int *mbits_per_sec );
  bool        getLinkStatus    ( int  dev_nr, int *status );
  bool        getLinkStatus    ( int  dev_nr,
                                 int *enabled_mask, int *locked_mask );
  bool        getSlvsConfig    ( int  dev_nr, int *config );
  bool        setSlvsConfig    ( int  dev_nr, int  config );
  bool        getPwrPulseConfig( int  dev_nr, int *config );
  bool        setPwrPulseConfig( int  dev_nr, int  config );
  bool        setPwrPulseEna   ( bool enable );
  bool        setTpxPowerEna   ( bool enable );
  bool        setBiasSupplyEna ( bool enable );
  bool        setBiasVoltage   ( int  volts );
  bool        setDecodersEna   ( bool enable );
  bool        setPeriphClk80Mhz( bool set );
  bool        setExtRefClk     ( bool set );
  std::string dacName          ( int  dac_code );
  int         dacMax           ( int  dac_code );
  bool        uploadPacket     ( int  dev_nr, unsigned char *packet, int size );
  bool        readEfuses       ( int  dev_nr, int *efuses );
#ifdef CERN_PROBESTATION
  bool        burnEfuse        ( int  dev_nr, int prog_width, int selection );
#endif

  // Configuration: device test pulses
  bool        getTpPeriodPhase ( int  dev_nr, int *period, int *phase );
  bool        setTpPeriodPhase ( int  dev_nr, int  period, int  phase );
  bool        getTpNumber      ( int  dev_nr, int *number );
  bool        setTpNumber      ( int  dev_nr, int  number );
  bool        setCtprBit       ( int  column, int val = 1 );
  bool        setCtprBits      ( int  val = 1 );
  bool        setCtpr          ( int  dev_nr );
  bool        getCtpr          ( int  dev_nr, unsigned char **ctpr );
  unsigned char *ctpr          ( );

  // Configuration: device pixels
  int  pixelConfigCount        ( );
  int  selectPixelConfig       ( int  index );
  int  selectedPixelConfig     ( );
  unsigned char *pixelConfig   ( int  index = -1 );
  void resetPixelConfig        ( int  index = -1 );
  int  comparePixelConfig      ( int  index1, int index2 );
  bool setPixelThreshold       ( int  x,
                                 int  y,
                                 int  threshold );
  bool setPixelTestEna         ( int  x = ALL_PIXELS, int y = ALL_PIXELS,
                                 bool b = true );
  bool setPixelMask            ( int  x = ALL_PIXELS, int y = ALL_PIXELS,
                                 bool b = true );
  bool setPixelConfig          ( int  dev_nr, int cols_per_packet = 2 );
  bool getPixelConfig          ( int  dev_nr );
  bool resetPixels             ( int  dev_nr );
  bool setSinglePixelFilter    ( int  index, int x, int y, bool enable = true );
  bool setSinglePixelFilter    ( int  index, int pixaddr, int superpixaddr,
                                 int  doublecolumn, bool enable = true );

  // Configuration: non-volatile onboard storage
  bool storeAddrAndPorts       ( int  ipaddr = 0,
                                 int  ipport = 0 );
  bool eraseAddrAndPorts       ( );
  bool validAddrAndPorts       ( bool *valid );
  bool storeDacs               ( int  dev_nr );
  bool eraseDacs               ( int  dev_nr );
  bool validDacs               ( int  dev_nr, bool *valid );
  bool storeRegisters          ( int  dev_nr );              // ###TODO
  bool eraseRegisters          ( int  dev_nr );              // ###TODO
  bool validRegisters          ( int  dev_nr, bool *valid ); // ###TODO
  bool storePixelConfig        ( int  dev_nr );              // ###TODO
  bool erasePixelConfig        ( int  dev_nr );              // ###TODO
  bool validPixelConfig        ( int  dev_nr, bool *valid ); // ###TODO
  bool storeStartupOptions     ( int  startopts );
  bool getStartupOptions       ( int *startopts );

  // Shutter trigger
  bool setShutterTriggerConfig ( int  trigger_mode,
                                 int  trigger_length_us,
                                 int  trigger_freq_hz,
                                 int  trigger_count,
                                 int  trigger_delay_ns = 0 );
  bool getShutterTriggerConfig ( int *trigger_mode,
                                 int *trigger_length_us,
                                 int *trigger_freq_hz,
                                 int *trigger_count,
                                 int *trigger_delay_ns = 0 );
  bool setShutterTriggerCfg    ( int  trigger_mode,
                                 int  trigger_delay_ns = 0,
                                 int  trigger_length_ns = 25,
                                 int  trigger_freq_hz = 1,
                                 int  trigger_count = 1 );
  bool getShutterTriggerCfg    ( int *trigger_mode,
                                 int *trigger_delay_ns = 0,
                                 int *trigger_length_ns = 0,
                                 int *trigger_freq_hz = 0,
                                 int *trigger_count = 0 );
  bool startAutoTrigger        ( );
  bool stopAutoTrigger         ( );
  bool openShutter             ( );
  bool closeShutter            ( );
  bool getExtShutterCounter    ( int *cntr );
  bool getShutterCounter       ( int *cntr );
  bool resetCounters           ( );

  // Data-acquisition
  bool sequentialReadout       ( int tokens = 128, bool now = false );
  bool datadrivenReadout       ( );
  bool pauseReadout            ( );

  // Timers
  bool restartTimers           ( );
  bool resetTimer              ( int dev_nr );
  bool getTimer                ( int dev_nr,
                                 unsigned int *timer_lo,
                                 unsigned int *timer_hi );
  bool setTimer                ( int dev_nr,
                                 unsigned int timer_lo,
                                 unsigned int timer_hi );
  bool getShutterStart         ( int dev_nr,
                                 unsigned int *timer_lo,
                                 unsigned int *timer_hi );
  bool getShutterEnd           ( int dev_nr,
                                 unsigned int *timer_lo,
                                 unsigned int *timer_hi );
  bool t0Sync                  ( int  dev_nr );

  // Monitoring
  bool getAdc                  ( int *adc_val, int chan, int nr_of_samples );
  bool getAdc                  ( int *adc_val, int nr_of_samples = 1 );
  bool getDacOut               ( int  dev_nr,
                                 int *adc_val, int nr_of_samples = 1 );
  bool getRemoteTemp           ( int *mdegrees );
  bool getLocalTemp            ( int *mdegrees );
  bool getFpgaTemp             ( int *mdegrees );
  bool getAvdd                 ( int *mvolts, int *mamps, int *mwatts );
  bool getDvdd                 ( int *mvolts, int *mamps, int *mwatts );
  bool getAvddNow              ( int *mvolts, int *mamps, int *mwatts );
  bool getDvddNow              ( int *mvolts, int *mamps, int *mwatts );
  bool getBiasVoltage          ( int *volts );
  bool getVdda                 ( int *mvolts );
  bool getFanSpeed             ( int *rpm );
  bool getFanSpeedVC707        ( int *rpm );
  bool selectChipBoard         ( int  board_nr );
  bool getDataPacketCounter    ( int *cntr );
  bool getMonPacketCounter     ( int *cntr );
  bool getPausePacketCounter   ( int *cntr );
  bool getPixelPacketCounter   ( int *cntr );
  bool resetPacketCounters     ( );
  bool getTdcTriggerCounter    ( int *cntr );

  // Other
  bool getGpio                 ( int *gpio_in );
  bool setGpio                 ( int  gpio_out );
  bool setGpioPin              ( int  pin_nr, int state );
  bool getSpidrReg             ( int  addr, int *val );
  bool setSpidrReg             ( int  addr, int  val );
  bool setSpidrRegBit          ( int  addr, int bitnr, bool set = true );
#ifdef TLU
  bool setTluEnable            ( int  dev_nr, bool enable );
#endif

 private:
  bool setPixelBit             ( int x, int y, unsigned char bitmask, bool b );
  void setBitsBigEndianReversed( unsigned char *buffer,
                                 int pos, int nbits, unsigned int value,
                                 int array_size_in_bits );
  bool get3Ints                ( int cmd, int *data0, int *data1, int *data2 );
  bool validXandY              ( int x,       int y,
                                 int *xstart, int *xend,
                                 int *ystart, int *yend );
  bool requestGetInt           ( int cmd, int dev_nr, int *dataword );
  bool requestGetInts          ( int cmd, int dev_nr,
                                 int expected_ints, int *datawords );
  bool requestGetBytes         ( int cmd, int dev_nr,
				 int expected_bytes, unsigned char *databytes );
  bool requestGetIntAndBytes   ( int cmd, int dev_nr, int *dataword,
                                 int expected_bytes, unsigned char *databytes );
  bool requestSetInt           ( int cmd, int dev_nr, int dataword );
  bool requestSetInts          ( int cmd, int dev_nr,
                                 int nwords, int *datawords );
  bool requestSetIntAndBytes   ( int cmd, int dev_nr, int dataword,
                                 int nbytes, unsigned char *bytes );
  bool request                 ( int cmd, int dev_nr,
                                 int req_len, int exp_reply_len );
  int  dacIndex                ( int dac_code );
  std::string spidrErrString   ( int err );

 private:
  // Socket connecting to the SPIDR module
  QTcpSocket *_sock;

  // Message buffers for request and reply
  int _reqMsg[512];
  int _replyMsg[512];

  // A device's pixel configuration is compiled locally before upload; space is
  // (currently statically) reserved for a number of pixel configurations,
  // with '_pixelConfig' pointing to the start of the currently 'active'
  // --i.e. selected by _pixelConfigIndex-- configuration array
  int            _pixelConfigIndex;
  unsigned char  _pixelConfigData[4*256*256];
  unsigned char *_pixelConfig;

  // Storage for one 256-bit CTPR which is compiled locally before upload
  unsigned char _ctpr[256/8];

  // String providing a description of the last error that occurred
  std::ostringstream _errString;

  // Error identifier from the SPIDR-TPX3 module, from the last operation
  int _errId;

  // Busy request counter
  // (busy is set and is only removed when the counter goes to zero)
  int _busyRequests;
};

#endif // SPIDRCONTROLLER_H
