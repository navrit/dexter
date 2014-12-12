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
  int         classVersion      (); // Version of this class
  std::string ipAddressString   ( int index );
  std::string errorString       ( );

  // Configuration
  void setPixelDepth            ( int nbits );
  void setDecodeFrames          ( bool decode );
  void setCompressFrames        ( bool compress );
  bool openFile                 ( std::string filename,
                                  bool overwrite = false );
  bool closeFile                ( );

  // Acquisition
  bool      hasFrame            ( );
  int      *frameData           ( int  index,
                                  int *size_in_bytes );
  long long frameTimestamp      ( );
  long long frameTimestamp      ( int buf_i );        // For debugging
  long long frameTimestampSpidr ( );
  double    frameTimestampDouble( );                  // For Pixelman
  void      releaseFrame        ( );
  void      setCallbackId       ( int id );           // For Pixelman
  void      setCallback         ( CallbackFunc cbf ); // For Pixelman

  // Statistics and info
  int  framesWrittenCount       ( );
  int  framesProcessedCount     ( );
  int  framesCount              ( int index );
  int  framesCount              ( );
  int  framesLostCount          ( int index );
  int  framesLostCount          ( );
  int  packetsReceivedCount     ( int index );
  int  packetsReceivedCount     ( );
  int  packetsLostCount         ( int index );
  int  packetsLostCount         ( );
  int  packetsLostCountFile     ( );
  int  packetsLostCountFrame    ( int index, int buf_i ); // For debugging
  int  packetSize               ( int index );            // For debugging
  int  expSequenceNr            ( int index );            // For debugging

 private:
  std::vector<ReceiverThread *> _frameReceivers;
  FramebuilderThread *_frameBuilder;

  //static QCoreApplication *App;

  // Functions used in c'tors
  void getIdsPortsTypes( SpidrController *spidrctrl,
                         int             *ids,
                         int             *ports,
                         int             *types );
  void init( int             *ipaddr,
             int             *ids,
             int             *ports,
             int             *types,
             SpidrController *spidrctrl );
};

#endif // SPIDRDAQ_H
