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
#define SHUTTERTRIG_POS_EXT_CNTR  5

class QTcpSocket;

class MY_LIB_API SpidrController
{
 public:
  // C'tor, d'tor
  SpidrController( int ipaddr3, int ipaddr2, int ipaddr1, int ipaddr0,
                   int port = 50000 );
  ~SpidrController();

  // Version information
  int         classVersion();                  // Version of this class
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
  bool        getMaxPacketSize     ( int *size ); // Max UDP data packet size
  bool        setMaxPacketSize     ( int  size );
  bool        reset                ( int *errorstat );
  bool        setBusy              ( );
  bool        clearBusy            ( );
  void        setBusyRequest       ( ); // For internal use
  void        clearBusyRequest     ( ); // For internal use
  bool        setLogLevel          ( int level );
  bool        displayInfo          ( ); // In the currently open telnet window
                                        // or (USB/UART) console
  bool        getDeviceCount       ( int *devices );
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

  // Configuration: devices
  bool        getDeviceId      ( int  dev_nr, int *id );
  bool        getDeviceIds     ( int *ids );
  bool        getDeviceType    ( int  dev_nr, int *type );
  bool        setDeviceType    ( int  dev_nr, int  type );
  bool        getDac           ( int  dev_nr, int  dac_code, int *dac_val );
  bool        setDac           ( int  dev_nr, int  dac_code, int dac_val );
  bool        setDacs          ( int  dev_nr, int  nr_of_dacs, int *dac_val );
  bool        setDacsDflt      ( int  dev_nr );
  bool        configCtpr       ( int  dev_nr, int column, int val );
  bool        setCtpr          ( int  dev_nr );
  bool        getAcqEnable     ( int *mask );
  bool        setAcqEnable     ( int  mask );
  bool        resetDevice      ( int  dev_nr );
  bool        resetDevices     ( );
  bool        setReady         ( );
  std::string dacNameMpx3      ( int  index );
  std::string dacNameMpx3rx    ( int  index );
  int         dacMaxMpx3       ( int  index );
  int         dacMaxMpx3rx     ( int  index );

  // Configuration: pixels
  void resetPixelConfig        ( );
  // Medipix3.1
  bool configPixelMpx3         ( int  x,
				 int  y,
				 int  configtha,
				 int  configthb,
				 bool configtha4 = false,
				 bool configthb4 = false,
				 bool gainmode = false,
				 bool testbit = false );
  bool setPixelMaskMpx3        ( int  x = ALL_PIXELS, int y = ALL_PIXELS,
				 bool b = true );
  bool setPixelConfigMpx3      ( int dev_nr, bool with_replies = true );
  // Medipix3RX
  bool configPixelMpx3rx       ( int  x,
				 int  y,
				 int  discl,
				 int  disch,
				 bool testbit = false );
  bool setPixelMaskMpx3rx      ( int  x = ALL_PIXELS, int y = ALL_PIXELS,
				 bool b = true );
  bool setPixelConfigMpx3rx   ( int dev_nr, bool with_replies = true );
  unsigned int *pixelConfig   ( );

  // Configuration: OMR
  bool setContRdWr             ( int  dev_nr, bool crw );
  bool setPolarity             ( int  dev_nr, bool polarity );
  bool setDiscCsmSpm           ( int  dev_nr, int  disc );
  bool setInternalTestPulse    ( int  dev_nr, bool internal );
  bool setPixelDepth           ( int  dev_nr, int  bits );
  bool setEqThreshH            ( int  dev_nr, bool equalize );
  bool setColourMode           ( int  dev_nr, bool colour );
  bool setCsmSpm               ( int  dev_nr, int  csm );
  bool setEnablePixelCom       ( int  dev_nr, bool enable );
  bool setGainMode             ( int  dev_nr, int  mode );
  bool setSenseDac             ( int  dev_nr, int  dac_code );
  bool setExtDac               ( int  dev_nr, int  dac_code, int dac_val );

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

  // Shutter trigger
  bool setShutterTriggerConfig ( int  trig_mode,
				 int  trig_period_us,
				 int  trig_freq_hz,
				 int  nr_of_triggers,
				 int  trig_pulse_count = 0 );
  bool getShutterTriggerConfig ( int *trig_mode,
				 int *trig_period_us,
				 int *trig_freq_hz,
				 int *nr_of_triggers,
				 int *trig_pulse_count );
  bool startAutoTrigger        ( );
  bool stopAutoTrigger         ( );
  bool triggerOneReadout       ( );

  // Monitoring
  bool getAdc                  ( int *adc_val, int chan, int nr_of_samples );
  bool getAdc                  ( int  dev_nr, int *adc_val );
  bool getDacOut               ( int  dev_nr,
                                 int *dacout_val, int nr_of_samples );
  bool getRemoteTemp           ( int *mdegrees );
  bool getLocalTemp            ( int *mdegrees );
  bool getAvdd                 ( int *mvolt, int *mamp, int *mwatt );
  bool getDvdd                 ( int *mvolt, int *mamp, int *mwatt );
  bool getVdd                  ( int *mvolt, int *mamp, int *mwatt );
  bool getAvddNow              ( int *mvolt, int *mamp, int *mwatt );
  bool getDvddNow              ( int *mvolt, int *mamp, int *mwatt );
  bool getVddNow               ( int *mvolt, int *mamp, int *mwatt );

  // Other
  bool getSpidrReg             ( int  addr, int *val );
  bool setSpidrReg             ( int  addr, int  val );
  bool setSpidrRegBit          ( int  addr, int bitnr, bool set = true );

 private:
  bool loadOmr              ( int  dev_nr );
  bool setPixelBit          ( int  x, int y, unsigned int bitmask, bool b );
  bool get3Ints             ( int  cmd, int *data0, int *data1, int *data2 );
  bool validXandY           ( int  x,       int y,
                              int *xstart, int *xend,
                              int *ystart, int *yend );
  bool requestGetInt        ( int  cmd, int dev_nr, int *dataword );
  bool requestGetInts       ( int  cmd, int dev_nr,
                              int  expected_ints, int *datawords );
  bool requestSetInt        ( int  cmd, int dev_nr, int dataword );
  bool requestSetInts       ( int  cmd, int dev_nr,
                              int  nwords, int *datawords );
  bool requestSetIntAndBytes( int  cmd, int dev_nr, int dataword,
                              int  nbytes, unsigned char *bytes );
  bool request              ( int  cmd, int dev_nr,
                              int  req_len, int exp_reply_len );

 private:
  // Socket connecting to the SPIDR module
  QTcpSocket *_sock;

  // Busy request counter
  // (busy is set and is only removed when the counter goes to zero)
  int _busyRequests;

  // Message buffers for request and reply
  int _reqMsg[512];
  int _replyMsg[512];

  // A device's pixel configuration is compiled locally before uploading
  // (unlike the DACs registers bit-arrays or OMR, which are maintained
  //  onboard the SPIDR module processor/software)
  // NB: here the dimensions are y and x, or columns and rows resp.:
  unsigned int _pixelConfig[256][256];

  // String providing a description of the last error that occurred
  std::ostringstream _errString;

  // Error identifier from the SPIDR module, from the last operation
  int _errId;
};

#endif // SPIDRCONTROLLER_H
