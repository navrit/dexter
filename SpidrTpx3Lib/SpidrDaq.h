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
class FilewriterThread;
//class FramebuilderThread;

class MY_LIB_API SpidrDaq
{
 public:
  // C'tor, d'tor
  SpidrDaq( int ipaddr3, int ipaddr2, int ipaddr1, int ipaddr0, int port );
  SpidrDaq( SpidrController *spidrctrl );
  ~SpidrDaq();

  void stop();

  // General
  int         classVersion   (); // Version of this class
  std::string ipAddressString();
  std::string errorString    ();

  // Configuration
  bool openFile                 ( std::string filename,
				  bool overwrite = false );
  bool closeFile                ();
  void setFlush                 ( bool flush );
  //void setDecodeFrames        ( bool decode );
  //void setAcqMode             ( int mode );

  // Acquisition
  long long bufferSize          ();
  bool      setBufferSize       ( long long size );
  long long maxBufferSize       ();
  bool      bufferEmpty         ();
  bool      bufferFull          ();
  bool      bufferFullOccurred  ();
  void      resetBufferFullOccurred();
  char     *dataBuffer          ();

  // Frame building (### TODO)
  int      *frameData           ( int *size_in_bytes );
  void      resetFrame          ();

  // Statistics and info
  int       packetsReceivedCount();
  int       packetsLostCount    ();
  int       lastPacketSize      ();
  long long bytesReceivedCount  ();
  long long bytesLostCount      ();
  long long bytesWrittenCount   ();
  long long bytesFlushedCount   ();
  int       bufferWrapCount     ();

 private:
  ReceiverThread     *_packetReceiver;
  FilewriterThread   *_fileWriter;
  //FramebuilderThread *_frameBuilder;

  // Init function for use in c'tors
  void init        ( int             *ipaddr,
		     int              port,
		     SpidrController *spidrctrl );
  void getIdAndPort( SpidrController *spidrctrl,
		     int             *id,
		     int             *port );
};

#endif // SPIDRDAQ_H
