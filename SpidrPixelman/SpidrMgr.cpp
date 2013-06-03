#include "common.h"
#include "mpxerrors.h"

#include "SpidrMgr.h"

// Static member: the one and only instance of this manager
SpidrMgr *SpidrMgr::_inst = 0;

static const char *VERSION_STR = "SpidrPixelman v1.0.0, 11 Apr 2013";

// ----------------------------------------------------------------------------

SpidrMgr::SpidrMgr()
  : _currId( -1 )
{
  _fLog.open( "logs/SpidrPixelman.log" );
  if( !_fLog.is_open() ) _fLog.open( "SpidrPixelman.log" );

  _fLog << VERSION_STR << endl;
  _fLog << fixed << setprecision(3); // Configuration for float/double output
}

// ----------------------------------------------------------------------------

SpidrMgr::~SpidrMgr()
{
  for ( u32 i = 0; i < _spidrList.size(); ++i )
    {
      delete _spidrList[i].spidrController;
      delete _spidrList[i].spidrDaq;
    }
  _spidrList.clear();

  if ( _inst ) delete _inst;
}

// ----------------------------------------------------------------------------

SpidrMgr *SpidrMgr::instance()
{
  if( _inst == 0 )
    {
      _inst = new SpidrMgr();
      _inst->init();
    }
  return _inst;
}

// ----------------------------------------------------------------------------

void SpidrMgr::init()
{
  // Get the module configuration from this file: (### TO BE IMPLEMENTED)
  std::string filename = "hwlibs/SpidrHw.ini";

  // Instantiate the requested SpidrController objects
  // ### FOR NOW JUST THE 'STANDARD' MODULE
  SpidrController *spidrctrl = new SpidrController( 192,168,1,10 );
  if( spidrctrl->isConnected() )
    {
      _fLog << "SPIDR " << spidrctrl->ipAddressString() << " "
	    << spidrctrl->connectionStateString() << endl;
    }
  else
    {
      _fLog << "### Failed to connect to SPIDR "
	    << spidrctrl->ipAddressString() << ": "
	    << spidrctrl->connectionErrString() << endl;
      return;
    }
  SpidrDaq *spidrdaq = new SpidrDaq( spidrctrl );
  spidrdaq->setDecodeFrames( true );

  _fLog << "SpidrController v"
	<< spidrctrl->versionToString( spidrctrl->classVersion() )
	<< ", SpidrDaq v"
	<< spidrctrl->versionToString( spidrdaq->classVersion() ) << endl;

  // Create a 'SPIDR info' struct for this module and fill it
  // with information and parameters concerning this module
  // (for easy access and also to prevent repeated requests
  //  to the module for the same information...)
  SpidrInfo info;
  info.spidrController = spidrctrl;
  info.spidrDaq        = spidrdaq;

  // Get device IDs and type
  if( !spidrctrl->getDeviceIds( info.chipIds ) )
    {
      _fLog << "### Mgr.getDeviceIds(): " << spidrctrl->errString() << endl;
      return;
    }
  info.chipCount    = 0;
  info.chipType     = 0;
  info.softwVersion = 0;
  info.firmwVersion = 0;
  int i;
  for( i=0; i<4; ++i )
    info.chipMap[i] = i; // Default mapping
  for( i=0; i<4; ++i )
    if( info.chipIds[i] != 0 )
      {
	// Compile a device/chip mapping to take 'holes' into account
	// (for example: only a chip in position 2 is present,
	//  or two chips in positions 0 and 2, etc)
	info.chipMap[i] = info.chipCount; // Move the hole (if any)
	info.chipMap[info.chipCount] = i;
	++info.chipCount;
	if( !spidrctrl->getDeviceType( i, &info.chipType ) )
	  {
	    _fLog << "### Mgr.getDeviceType(): "
		  << spidrctrl->errString() << endl;
	    return;
	  }
      }
  // Get software and firmware version numbers
  if( !spidrctrl->getSoftwVersion( &info.softwVersion ) )
    _fLog << "### Mgr.getSoftwVersion(): " << spidrctrl->errString() << endl;
  if( !spidrctrl->getFirmwVersion( &info.firmwVersion ) )
    _fLog << "### Mgr.getFirmwVersion(): " << spidrctrl->errString() << endl;

  // Get the module's IP address as a string
  info.ipAddrString = spidrctrl->ipAddressString();

  _fLog << info.ipAddrString << ": IDs " << hex;
  for( i=0; i<4; ++i ) _fLog << "0x" << info.chipIds[i] << " ";
  _fLog << dec << "Mapping: ";
  for( i=0; i<4; ++i ) _fLog << info.chipMap[i] << " ";
  _fLog << endl;

  // Add it to our list of module descriptions
  _spidrList.push_back( info );
}

// ----------------------------------------------------------------------------

int SpidrMgr::getFirst( int *id )
{
  _currId = 0;
  if( _spidrList.size() > 0 )
    {
      _fLog << "Mgr::getFirst() OKAY" << endl;
      *id = 0;
      return 0;
    }
  _fLog << "### Mgr::getFirst() ERROR" << endl;
  return 1;
}

// ----------------------------------------------------------------------------

int SpidrMgr::getNext( int *id )
{
  ++_currId;
  if( (u32) _currId < _spidrList.size() )
    {
      *id = _currId;
      _fLog << "Mgr::getNext() OKAY: " << _currId << endl;
      return 0;
    }
  _fLog << "### Mgr::getNext() ERROR: " << _currId << endl;
  return 1;
}

// ----------------------------------------------------------------------------

SpidrController *SpidrMgr::controller( int id )
{
  if( (u32) id < _spidrList.size() ) return _spidrList[id].spidrController;
  _fLog << "### Mgr::controller(): Invalid Hardware ID: " << id << endl;
  return 0;
}

// ----------------------------------------------------------------------------

SpidrDaq *SpidrMgr::daq( int id )
{
  if( (u32) id < _spidrList.size() ) return _spidrList[id].spidrDaq;
  _fLog << "### Mgr::daq(): Invalid Hardware ID: " << id << endl;
  return 0;
}

// ----------------------------------------------------------------------------

SpidrInfo *SpidrMgr::info( int id )
{
  if( (u32) id < _spidrList.size() ) return &_spidrList[id];
  _fLog << "### Mgr::info(): Invalid Hardware ID: " << id << endl;
  return 0;
}

// ----------------------------------------------------------------------------
