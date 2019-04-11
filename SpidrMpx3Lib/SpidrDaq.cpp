#include <QCoreApplication>

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "UdpReceiver.h"
#include "FrameAssembler.h"

// Version identifier: year, month, day, release number

// - Fixes for 24-bit readout in the framebuilders' mpx3RawToPixel() and
//   processFrame() functions.
// - Fix for SPIDR-LUT decoded row counter (firmware bug) in
//   FramebuilderThreadC::mpx3RawToPixel(): just count EOR pixelpackets instead.
const int   VERSION_ID = 0x18102400;

// - Fix 24-bit bug in ReceiverThread::setPixelDepth().
// - Tolerate SOF out-of-order in ReceiverThreadC::readDatagrams().
// - Use row counter in EOR/EOF pixel packets in
//   FramebuilderThreadC::mpx3RawToPixel().
//const int VERSION_ID = 0x17020200;

//const int VERSION_ID = 0x16082900; // Add frameFlags()
//const int VERSION_ID = 0x16061400; // Add info header processing
                                     // (ReceiverThreadC)
//const int VERSION_ID = 0x16040800; // Add parameter readout_mask to c'tor
//const int VERSION_ID = 0x16032400; // Renamed disableLut() to setLutEnable()
//const int VERSION_ID = 0x16030900; // Compact-SPIDR support added
//const int VERSION_ID = 0x15101500;
//const int VERSION_ID = 0x15100100;
//const int VERSION_ID = 0x15093000;
//const int VERSION_ID = 0x15092100;
//const int VERSION_ID = 0x15051900;
//const int VERSION_ID = 0x14012400;

// At least one argument needed for QCoreApplication
//int   Argc = 1;
//char *Argv[] = { "SpidrDaq" }; 
//QCoreApplication *SpidrDaq::App = 0;
// In c'tor?: Create the single 'QCoreApplication' we need for the event loop
// in the receiver objects  ### SIGNALS STILL DO NOT WORK? Need exec() here..
//if( App == 0 ) App = new QCoreApplication( Argc, Argv );

// ----------------------------------------------------------------------------
// Constructor / destructor / info
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

SpidrDaq::SpidrDaq( SpidrController *spidrctrl,
		    int              readout_mask )
{
  // If a SpidrController object is provided use it to find out the SPIDR's
  // Medipix device configuration and IP destination address, or else assume
  // a default IP address and a single device with a default port number
  int ports[4]  = { 8192, 0, 0, 0 };
  if( spidrctrl )
    {
      // Get the IP destination address (this host network interface)
      // from the SPIDR module
      int addr = 0;
      spidrctrl->getIpAddrDest( 0, &addr);

      this->getPorts( spidrctrl, ports);

      // Adjust SPIDR read-out mask if requested:
      // Reset unwanted ports/devices to 0
      for( int i=0; i<4; ++i )
        if( (readout_mask & (1<<i)) == 0 ) ports[i] = 0;
      // Read out the remaining devices
      readout_mask = 0;
      for( int i=0; i<4; ++i )
        if( ports[i] != 0 ) readout_mask |= (1<<i);

      // Set the new read-out mask if required
      int device_mask;
      if( spidrctrl->getAcqEnable(&device_mask) && device_mask != readout_mask )
        spidrctrl->setAcqEnable( readout_mask );

      chipMask = readout_mask;

      int fwVersion;
      spidrctrl->getFirmwVersion(&fwVersion);
      udpReceiver = new UdpReceiver(addr, ports[0], fwVersion < 0x18100100);
      th = udpReceiver->spawn();
      frameSetManager = udpReceiver->getFrameSetManager();
      frameSetManager->chipMask = readout_mask;
    }
}

// ----------------------------------------------------------------------------

void SpidrDaq::getPorts( SpidrController *spidrctrl,
                 int             *ports)
{
  if( !spidrctrl ) return;

  int ids[4]    = { 0, 0, 0, 0 };

  // Get the device IDs from the SPIDR module
  spidrctrl->getDeviceIds( ids );

  // Get the device port numbers from the SPIDR module
  // but only for devices whose ID could be determined (i.e. is unequal to 0)
  for( int i=0; i<4; ++i )
    {
      ports[i] = 0;
      if( ids[i] != 0 )
        {
          spidrctrl->getServerPort( i, &ports[i] );
        }
    }
}

// ----------------------------------------------------------------------------

SpidrDaq::~SpidrDaq()
{
    this->stop();
    if (th.joinable()) th.join();
    delete udpReceiver;
    udpReceiver = nullptr;
    delete frameSetManager;
    frameSetManager = nullptr;
}

// ----------------------------------------------------------------------------

void SpidrDaq::stop()
{
    if (udpReceiver != nullptr)
        udpReceiver->stop();
}

// ----------------------------------------------------------------------------
// General
// ----------------------------------------------------------------------------

int SpidrDaq::classVersion()
{
  return VERSION_ID;
}

// ----------------------------------------------------------------------------

std::string SpidrDaq::ipAddressString( int index )
{
  //if( index < 0 || index >= (int) _frameReceivers.size() )
    return std::string( "" );
  //return _frameReceivers[index]->ipAddressString();
}

// ----------------------------------------------------------------------------

std::string SpidrDaq::errorString()
{
  std::string str;
  /*
  for( unsigned int i=0; i<_frameReceivers.size(); ++i )
    {
      if( !str.empty() && !_frameReceivers[i]->errString().empty() )
	str += std::string( ", " );
      str += _frameReceivers[i]->errString();
    }
  if( !str.empty() && !_frameBuilder->errString().empty() )
    str += std::string( ", " );
  str += _frameBuilder->errString();

  // Clear the error strings
  for( unsigned int i=0; i<_frameReceivers.size(); ++i )
    _frameReceivers[i]->clearErrString();
  _frameBuilder->clearErrString();
 */
  return str;
}

// ----------------------------------------------------------------------------

bool SpidrDaq::hasError()
{ /*
  for( unsigned int i=0; i<_frameReceivers.size(); ++i )
    if( !_frameReceivers[i]->errString().empty() )
      return true;
  if( !_frameBuilder->errString().empty() )
    return true; */
  return false;
}

// ----------------------------------------------------------------------------
// Acquisition
// ----------------------------------------------------------------------------

int SpidrDaq::framesAvailable()
{
  return frameSetManager->available();
}

bool SpidrDaq::hasFrame( unsigned long timeout_ms )
{
  return frameSetManager->wait(timeout_ms);
}

// ----------------------------------------------------------------------------

FrameSet *SpidrDaq::getFrameSet()
{
  return frameSetManager->getFrameSet();
}

// ----------------------------------------------------------------------------

void SpidrDaq::releaseFrame(FrameSet *fs)
{
  frameSetManager->releaseFrameSet(fs);
}

// ----------------------------------------------------------------------------

void SpidrDaq::releaseAll()
{
  frameSetManager->clear();
}

// ----------------------------------------------------------------------------
// Statistics
// ----------------------------------------------------------------------------

int SpidrDaq::framesCount()
{
  return frameSetManager->_framesReceived;
}

// ----------------------------------------------------------------------------

int SpidrDaq::framesLostCount()
{
  return frameSetManager->_framesLost;
}

// ----------------------------------------------------------------------------

void SpidrDaq::resetLostCount()
{
  frameSetManager->_framesLost = 0;
}
