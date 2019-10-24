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
#include <thread>
#include <vector>

#include "FrameSet.h"
#include "FrameSetManager.h"

class SpidrController;
class UdpReceiver;
class FrameAssembler;

class MY_LIB_API SpidrDaq {
public:
  // C'tor, d'tor
  // SpidrDaq( int ipaddr3, int ipaddr2, int ipaddr1, int ipaddr0,
  // int port, int readout_mask = 0xF );
  SpidrDaq(SpidrController *spidrctrl, int readout_mask = 0xF);
  ~SpidrDaq();

  // General
  void stop();        // To be called before exiting/deleting
  int classVersion(); // Version of this class
  std::string ipAddressString(/*int index*/);
  std::string errorString();
  bool hasError();

  int chipMask;

  // Acquisition
  void setBothCounters(bool b) { frameSetManager->setBothCounters(b); }
  int framesAvailable();
  bool hasFrame(unsigned long timeout_ms = 0);
  FrameSet *getFrameSet();
  void releaseFrame(FrameSet *fs = nullptr);
  void releaseAll();

  int framesCount();
  int framesLostCount();
  void resetLostCount();

private:
  FrameSetManager *frameSetManager;
  UdpReceiver *udpReceiver;
  std::thread th;

  // Functions used in c'tors
  void getPorts(SpidrController *spidrctrl, int *ports);
};

#endif // SPIDRDAQ_H
