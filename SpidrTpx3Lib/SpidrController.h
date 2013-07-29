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
  std::string errString            ();
  void        clearErrString       ();
  bool        reset                ();
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
  bool        getIpAddrDest   ( int port_index, int *ipaddr );
  bool        setIpAddrDest   ( int port_index, int  ipaddr );
  bool        getServerPort   ( int  dev_nr, int *port_nr );
  bool        getServerPorts  ( int *port_nrs );
  bool        setServerPort   ( int  dev_nr, int  port_nr );
  bool        getDevicePort   ( int  dev_nr, int *port_nr );
  bool        getDevicePorts  ( int *port_nrs );
  bool        setDevicePort   ( int  dev_nr, int  port_nr );

  // Configuration: device
  bool        getDeviceId     ( int  dev_nr, int *id );
  bool        getDeviceIds    ( int *ids );
  bool        setSenseDac     ( int  dev_nr, int  dac_code );
  bool        setExtDac       ( int  dev_nr, int  dac_code );
  bool        getDac          ( int  dev_nr, int  dac_code, int *dac_val );
  bool        setDac          ( int  dev_nr, int  dac_code, int  dac_val );
  bool        setDacsDflt     ( int  dev_nr );
  bool        getGenConfig    ( int  dev_nr, int *config );
  bool        setGenConfig    ( int  dev_nr, int  config );
  bool        getPllConfig    ( int  dev_nr, int *config );
  bool        setPllConfig    ( int  dev_nr, int  config );
  bool        resetDevice     ( int  dev_nr );
  bool        resetDevices    ();
  std::string dacName         ( int  dac_code );
  int         dacMax          ( int  dac_code );

  // Configuration: device test pulses
  bool        getTpPeriodPhase( int  dev_nr, int *period, int *phase );
  bool        setTpPeriodPhase( int  dev_nr, int  period, int  phase );
  bool        getTpNumber     ( int  dev_nr, int *number );
  bool        setTpNumber     ( int  dev_nr, int  number );
  bool        configCtpr      ( int  dev_nr, int  column, int val );
  bool        setCtpr         ( int  dev_nr );

  // Configuration: device pixels
  void resetPixelConfig       ();
  bool configPixel            ( int  x,
                                int  y,
                                int  threshold,
                                bool testbit = false );
  bool maskPixel              ( int x = ALL_PIXELS, int y = ALL_PIXELS );
  bool setPixelConfig         ( int dev_nr );

  // Configuration: onboard storage
  bool storeAddrAndPorts      ( int  ipaddr_src,          // ###TODO
                                int  ipaddr_dst,
                                int *srvports,
                                int *devports,
                                int  controlport = 50000 );
  bool storeDacs              ( int dev_nr );             // ###TODO
  bool storePixelConfig       ( int dev_nr );             // ###TODO

  // Trigger
  bool setTriggerConfig       ( int trig_mode,
                                int trig_period_us,
                                int trig_freq_hz,
                                int nr_of_triggers );
  bool getTriggerConfig       ( int *trig_mode,
                                int *trig_period_us,
                                int *trig_freq_hz,
                                int *nr_of_triggers );
  bool startAutoTrigger       ();
  bool stopAutoTrigger        ();
  bool triggerOneReadout      ();

  // Data-acquisition
  bool sequentialReadout      ( int dev_nr );
  bool datadrivenReadout      ( int dev_nr );
  bool pauseReadout           ( int dev_nr );

  // Monitoring
  bool getAdc                 ( int dev_nr, int *adc_val );
  bool getRemoteTemp          ( int *mdegrees );
  bool getLocalTemp           ( int *mdegrees );
  bool getAvdd                ( int *mvolt, int *mamp, int *mwatt );
  bool getDvdd                ( int *mvolt, int *mamp, int *mwatt );

  // Other
 private:
  bool get3Ints               ( int cmd, int *data0, int *data1, int *data2 );
  bool validXandY             ( int x,       int y,
                                int *xstart, int *xend,
                                int *ystart, int *yend );
  bool requestGetInt          ( int cmd, int dev_nr, int *dataword );
  bool requestGetInts         ( int cmd, int dev_nr,
                                int expected_ints, int *datawords );
  bool requestSetInt          ( int cmd, int dev_nr, int dataword );
  bool requestSetInts         ( int cmd, int dev_nr,
                                int nwords, int *datawords );
  bool requestSetIntAndBytes  ( int cmd, int dev_nr, int dataword,
                                int nbytes, unsigned char *bytes );
  bool request                ( int cmd, int dev_nr,
                                int req_len, int exp_reply_len );
  int  dacIndex               ( int dac_code );

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
  // (unlike the CTPR register, which is maintained
  //  onboard the SPIDR module processor by the LEON3 software)
  // NB: here the dimensions are y and x, or columns and rows resp.:
  unsigned int _pixelConfig[256][256];

  // String providing a description of the last error that occurred
  std::ostringstream _errString;
};

#endif // SPIDRCONTROLLER_H
