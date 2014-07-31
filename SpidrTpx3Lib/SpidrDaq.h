#ifndef SPIDRDAQ_H
#define SPIDRDAQ_H

#ifdef WIN32
 // On Windows differentiate between building the DLL or using it
 #ifdef MY_LIB_EXPORT
 #define MY_LIB_API __declspec(dllexport)
 #else
 #define MY_LIB_API __declspec(dllimport)
 #endif
#else
 // Linux
 #define MY_LIB_API
#endif // WIN32

#include <string>
#include <vector>

class SpidrController;
class ReceiverThread;
class DatasamplerThread;

class MY_LIB_API SpidrDaq
{
 public:
  // C'tor, d'tor
  SpidrDaq( int ipaddr3, int ipaddr2, int ipaddr1, int ipaddr0, int port );
  SpidrDaq( SpidrController *spidrctrl,
	    long long bufsize = 0x20000000, //  512 MByte
	    int device_nr = 0 );
  ~SpidrDaq();

  void stop();

  // General
  int         classVersion   (); // Version of this class
  std::string ipAddressString();
  std::string errorString    ();

  // Data acquisition
  bool        startRecording      ( std::string filename,
				    int         runnr = 0,
				    std::string descr = std::string(),
				    bool        include_pixelconfig = false );
  bool        stopRecording       ();
  std::string fileName            ();
  long long   fileMaxSize         ();
  void        setFileMaxSize      ( long long size );
  void        setFlush            ( bool enable );
  void        setSampling         ( bool enable );
  void        setSampleAll        ( bool enable );
  long long   bufferSize          ();
  bool        setBufferSize       ( long long size );
  bool        bufferEmpty         ();
  bool        bufferFull          ();
  bool        bufferFullOccurred  ();
  void        resetBufferFullOccurred();
  char       *dataBuffer          ();
  bool        setFileCntr( int cntr );    // MvB


  // Data sampling
  bool        getSample           ( int max_size, int timeout_ms );
  bool        getSampleMin        ( int min_size, int max_size,
				    int timeout_ms );
  int         sampleSize          ();
  char       *sampleData          ();
  // Data sampling ('frame' mode)
  bool        getFrame            ( int timeout_ms );
  int         frameSize           ();
  char       *frameData           ();

  bool        nextPixel           ( int *x, int *y,
				    int *data = 0, int *timestamp = 0 );
  unsigned long long nextPixel  ();
  unsigned long long nextPacket  ();
  void        setBigEndian        ( bool b );

  // Statistics and info
  int         packetsReceivedCount();
  int         packetsLostCount    ();
  int         lastPacketSize      ();
  long long   bytesReceivedCount  ();
  long long   bytesLostCount      ();
  long long   bytesWrittenCount   ();
  long long   bytesSampledCount   ();
  long long   bytesFlushedCount   ();
  int         bufferWrapCount     ();

 private:
  ReceiverThread    *_packetReceiver;
  DatasamplerThread *_fileWriter;
  SpidrController   *_spidrCtrl;
  int                _ipAddr;
  int                _ipPort;
  int                _deviceNr;
};

#endif // SPIDRDAQ_H
