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
  bool        isConnected          ();
  std::string connectionStateString();
  std::string connectionErrString  ();
  std::string ipAddressString      ();
  std::string errorString          ();
  void        clearErrorString     ();
  int         errorId              ();
  bool        reset                ( int *errorstat );
  bool        setBusy              ();
  bool        clearBusy            ();
  void        setBusyRequest       (); // For internal use
  void        clearBusyRequest     (); // For internal use
  bool        setLogLevel          ( int level );
  bool        displayInfo          (); // In the currently open telnet window
                                       // or (USB/UART) console
  bool        getPortCount         ( int *ports );
  bool        getDeviceCount       ( int *devices );

  // ###TODO:
  bool        setTimeOfDay         (); // Set the SPIDR processor clock time

  // Configuration: module/device interface
  bool        getIpAddrSrc     ( int  index,  int *ipaddr );
  bool        setIpAddrSrc     ( int  index,  int  ipaddr );
  bool        getIpAddrDest    ( int  index,  int *ipaddr );
  bool        setIpAddrDest    ( int  index,  int  ipaddr );
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
  bool        getSlvsConfig    ( int  dev_nr, int *config );
  bool        setSlvsConfig    ( int  dev_nr, int  config );
  bool        getPwrPulseConfig( int  dev_nr, int *config );
  bool        setPwrPulseConfig( int  dev_nr, int  config );
  bool        setPwrPulseEna   ( bool enable );
  bool        setTpxPowerEna   ( bool enable );
  bool        setBiasSupplyEna ( bool enable );
  bool        setBiasVoltage   ( int  volts );
  bool        setDecodersEna   ( bool enable );
  std::string dacName          ( int  dac_code );
  int         dacMax           ( int  dac_code );
  bool        uploadPacket     ( int  dev_nr, unsigned char *packet, int size );
#ifdef CERN_PROBESTATION
  bool        burnEfuse        ( int device_nr, int program_width, int selection );
#endif
  bool        readEfuse        ( int device_nr, int *efuses );

  // Configuration: device test pulses
  bool        getTpPeriodPhase ( int  dev_nr, int *period, int *phase );
  bool        setTpPeriodPhase ( int  dev_nr, int  period, int  phase );
  bool        getTpNumber      ( int  dev_nr, int *number );
  bool        setTpNumber      ( int  dev_nr, int  number );
  bool        setCtprBit       ( int  column, int val = 1 );
  bool        setCtprBits      ( int  val = 1 );
  bool        setCtpr          ( int  dev_nr );
  bool        getCtpr          ( int  dev_nr, unsigned char **ctpr );

  // Configuration: device pixels
  void resetPixelConfig        ();
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
  unsigned char *pixelConfig   ();

  // Configuration: non-volatile onboard storage
  bool storeAddrAndPorts       ( int  ipaddr = 0,
                                 int  ipport = 0 );
  bool storeDacs               ( int  dev_nr );
  bool storeRegisters          ( int  dev_nr );              // ###TODO
  bool storePixelConfig        ( int  dev_nr );              // ###TODO
  bool eraseAddrAndPorts       ();
  bool eraseDacs               ( int  dev_nr );
  bool eraseRegisters          ( int  dev_nr );              // ###TODO
  bool erasePixelConfig        ( int  dev_nr );              // ###TODO
  bool validAddrAndPorts       ( bool *valid );
  bool validDacs               ( int  dev_nr, bool *valid );
  bool validRegisters          ( int  dev_nr, bool *valid ); // ###TODO
  bool validPixelConfig        ( int  dev_nr, bool *valid ); // ###TODO

  // Trigger
  bool setTriggerConfig        ( int  trigger_mode,
                                 int  trigger_length_us,
                                 int  trigger_freq_hz,
                                 int  trigger_count );
  bool getTriggerConfig        ( int *triger_mode,
                                 int *triger_length_us,
                                 int *triger_freq_hz,
                                 int *trigger_count );
  bool startAutoTrigger        ();
  bool stopAutoTrigger         ();
  bool openShutter             ();
  bool closeShutter            ();
  bool getShutterCounter       ( int *cntr );
  bool getTriggerCounter       ( int *cntr );
  bool resetCounters           ();

  // Data-acquisition
  bool sequentialReadout       ( int tokens = 128 );
  bool datadrivenReadout       ();
  bool pauseReadout            ();

  // Timers
  bool restartTimers           ();
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
  bool getAdc                  ( int  dev_nr, int *adc_val,
				 int  nr_of_samples = 1 );
  bool getRemoteTemp           ( int *mdegrees );
  bool getLocalTemp            ( int *mdegrees );
  bool getFpgaTemp             ( int *mdegrees );
  bool getAvdd                 ( int *mvolts, int *mamps, int *mwatts );
  bool getDvdd                 ( int *mvolts, int *mamps, int *mwatts );
  bool getBiasVoltage          ( int *volts );
  bool getVdda                 ( int *mvolts );
  bool getFanSpeed             ( int *rpm );
  bool getFanSpeedVC707        ( int *rpm );

  // Other
  bool setGPIO                 ( int gpio_pin, int state );
  bool getGPIO                 ( int gpio_pin, int *state );

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

  // Busy request counter
  // (busy is set and is only removed when the counter goes to zero)
  int _busyRequests;

  // Message buffers for request and reply
  int _reqMsg[512];
  int _replyMsg[512];

  // A device's pixel configuration is compiled locally before upload
  // NB: here the dimensions are y and x, or row and column number resp.:
  unsigned char _pixelConfig[256][256];

  // Storage for one 256-bit CTPR which is compiled locally before upload
  unsigned char _ctpr[256/8];

  // String providing a description of the last error that occurred
  std::ostringstream _errString;

  // Error identifier from the SPIDR-TPX3 module, from the last operation
  int _errId;
};

#endif // SPIDRCONTROLLER_H
