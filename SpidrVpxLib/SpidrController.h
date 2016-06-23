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
  bool        reset                ( int *errorstat );
  bool        setBusy              ( );
  bool        clearBusy            ( );
  void        setBusyRequest       ( ); // For internal use
  void        clearBusyRequest     ( ); // For internal use
  bool        setLogLevel          ( int level );
  bool        displayInfo          ( ); // In the currently open telnet window
                                        // or (USB/UART) console
  bool        getDeviceCount       ( int *devices );
  bool        getChipboardId       ( int *id );
  bool        setChipboardId       ( int  id );

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

  // Configuration: device
  bool        getVpxItem       ( int  item,    int *value );
  bool        getVpxItem       ( int  item,    int  item_i, int reg_i,
                                 int *value );
  bool        getVpxReg        ( int  address, int  size,
                                 unsigned char *bytes );
  bool        getVpxReg32      ( int  address, int *val );
  bool        getVpxReg16      ( int  address, int *val );
  bool        setVpxItem       ( int  item,    int value );
  bool        setVpxItem       ( int  item,    int  item_i, int reg_i,
                                 int  value );
  bool        setVpxReg        ( int  address, int  size,
                                 unsigned char *bytes );
  bool        setVpxReg32      ( int  address, int  val );
  bool        setVpxReg16      ( int  address, int  val );
  int         vpxRegStatus     ( ) { return _vpxRegStatus; }
  bool        resetDevice      ( int  dev_nr );
  bool        resetDevices     ();
  bool        getDeviceId      ( int  dev_nr, int *id );
  bool        getDac           ( int  dev_nr, int  dac_code, int *dac_val );
  bool        setDac           ( int  dev_nr, int  dac_code, int  dac_val );
  bool        setDacsDflt      ( int  dev_nr );
  std::string dacName          ( int  dac_code );
  int         dacMax           ( int  dac_code );

  // Configuration: device pixels
  int  pixelConfigCount        ( );
  int  selectPixelConfig       ( int  index );
  int  selectedPixelConfig     ( );
  unsigned char *pixelConfig   ( int  index = -1 );
  void resetPixelConfig        ( int  index = -1 );
  int  comparePixelConfig      ( int  index1, int index2 );

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
  // (used through provided tools only:)
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
  bool setMonitorStreamEna     ( bool enable );

  // Timers
  // ....

  // Monitoring
  bool getAdc                  ( int *adc_val, int chan, int nr_of_samples );
  bool getAdc                  ( int *adc_val, int nr_of_samples = 1 );
  bool getDacOut               ( int  dev_nr,
                                 int *adc_val, int nr_of_samples = 1 );
  bool getRemoteTemp           ( int *mdegrees ); // Device temperature
  bool getLocalTemp            ( int *mdegrees ); // SPIDR board temperature
  bool getFpgaTemp             ( int *mdegrees ); // SPIDR FPGA temperature
  bool getAvdd                 ( int *mvolts, int *mamps, int *mwatts );
  bool getDvdd                 ( int *mvolts, int *mamps, int *mwatts );
  bool getAvddNow              ( int *mvolts, int *mamps, int *mwatts );
  bool getDvddNow              ( int *mvolts, int *mamps, int *mwatts );
  bool getBiasVoltage          ( int *volts );
  bool getVdda                 ( int *mvolts );
  bool getFanSpeed             ( int  index, int *rpm );
  bool setFanSpeed             ( int  index, int percentage );
  bool getDataPacketCounter    ( int *cntr );
  bool getMonPacketCounter     ( int *cntr );
  bool getPausePacketCounter   ( int *cntr );
  bool getPixelPacketCounter   ( int dev_nr, int *cntr );
  bool getPixelPacketCounter   ( int *cntr );  // ### OBSOLETE
  bool resetPacketCounters     ( );
  bool getTdcTriggerCounter    ( int *cntr );

  // Other
  bool getSpidrReg             ( int  address, int *val );
  bool setSpidrReg             ( int  address, int  val );
  bool setSpidrRegBit          ( int  address, int  bitnr, bool set = true );

 private:
  bool findVpxItem             ( int item, int item_i, int *index );
  bool findVpxReg              ( int addr, int reg_i, int *index );
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
  bool requestSetIntAndBytes   ( int cmd, int dev_nr, int *dataword,
                                 int nbytes, unsigned char *bytes );
  bool request                 ( int cmd, int dev_nr,
                                 int req_len, int exp_reply_len );
  std::string spidrErrString   ( int err );
  int  dacIndex                ( int dac_code );

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

  // String providing a description of the last error that occurred
  std::ostringstream _errString;

  // Error identifier from the SPIDR-TPX3 module, from the last operation
  int _errId;

  // Busy request counter
  // (busy is set and is only removed when the counter goes to zero)
  int _busyRequests;

  int _vpxRegStatus;
};

#endif // SPIDRCONTROLLER_H
