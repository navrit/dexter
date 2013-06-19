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
class FramebuilderThread;
//class QCoreApplication;

typedef void (*CallbackFunc)( int id );

class MY_LIB_API SpidrDaq
{
 public:
  // C'tor, d'tor
  SpidrDaq( int ipaddr3, int ipaddr2, int ipaddr1, int ipaddr0,
            SpidrController *spidrctrl = 0 );
  SpidrDaq( SpidrController *spidrctrl );
  ~SpidrDaq();

  void stop();

  // General
  int         classVersion(); // Version of this class
  std::string ipAddressString( int index = 0 );
  std::string errString();

  // Configuration
  void setPixelDepth( int nbits );
  void setDecodeFrames( bool decode );
  void setCompressFrames( bool compress );
  bool openFile( std::string filename, bool overwrite = false );
  bool closeFile();

  // Acquisition
  bool      hasFrame();
  int      *frameData( int dev_nr, int *size_in_bytes );
  long long frameTimestamp();
  long long frameTimestamp( int buf_i );              // For debugging
  long long frameTimestampSpidr();
  double    frameTimestampDouble();                   // For Pixelman
  void      releaseFrame();
  void      setCallbackId( int id );                  // For Pixelman
  void      setCallback( CallbackFunc cbf );          // For Pixelman

  // Statistics and info
  int  framesWrittenCount();
  int  framesProcessedCount();
  int  framesCount( int index = 0 );
  int  framesLostCount( int index = 0 );
  int  packetsLostCount( int index = 0 );
  int  packetsLostCountFile();
  int  packetsLostCountFrame( int index, int buf_i ); // For debugging
  int  packetSize( int index = 0 );                   // For debugging
  int  expSequenceNr( int index = 0 );                // For debugging

 private:
  std::vector<ReceiverThread *> _frameReceivers;
  FramebuilderThread *_frameBuilder;

  //static QCoreApplication *App;

  // Init function for use in c'tors
  void init( int *ipaddr,
             int *devport,
             int *devid,
             int *devtype,
             SpidrController *spidrctrl );
  void getIdsPortsTypes( SpidrController *spidrctrl,
                         int *id, int *port, int *type );
};

#endif // SPIDRDAQ_H
