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
  int         classVersion   (); // Version of this class
  std::string ipAddressString( int index = 0 );
  std::string errString      ();

  // Configuration
  void setAcqMode            ( int mode );
  bool openFile              ( std::string filename, bool overwrite = false );
  bool closeFile             ();

  // Frame building
  int *frameData             ( int *size_in_bytes );
  void resetFrame            ();

  // Statistics and info
  int  packetsWrittenCount   ();
  int  packetsProcessedCount ();
  int  packetsReceivedCount  ();

 private:
  ReceiverThread     *_packetReceiver;
  FilewriterThread   *_fileWriter;
  FramebuilderThread *_frameBuilder;

  // Init function for use in c'tors
  void init       ( int             *ipaddr,
                    int             *devport,
                    int             *devid,
                    SpidrController *spidrctrl );
  void getIdsPorts( SpidrController *spidrctrl,
                    int             *ids,
                    int             *ports );
};

#endif // SPIDRDAQ_H
