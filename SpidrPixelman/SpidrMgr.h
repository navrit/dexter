#ifndef SPIDRMGR_H
#define SPIDRMGR_H

#include <iomanip>
#include <fstream>
#include <vector>
using namespace std;

#include "SpidrController.h"
#include "SpidrDaq.h"

typedef struct _SpidrInfo
{
  SpidrController *spidrController;
  SpidrDaq        *spidrDaq;
  int              chipCount;
  int              chipType;
  int              chipIds[4];
  int              chipMap[4];// Allows loops using chipCount independent
                              // on actual chip positions (in case of 'holes')
  int              softwVersion;
  int              firmwVersion;
  string           ipAddrString;
  INTPTR           callbackData;
} SpidrInfo;

// ----------------------------------------------------------------------------

class SpidrMgr
{
 public:
  virtual ~SpidrMgr();

  static SpidrMgr *instance();

  int    getFirst  ( int *id );
  int    getNext   ( int *id );

  SpidrController *controller( int id );
  SpidrDaq        *daq( int id );
  SpidrInfo       *info( int id );

  ofstream& log() { return _fLog; }

 protected:
  void init();

 private:
  // This is a singleton class, so c'tors are private
  SpidrMgr();
  SpidrMgr( SpidrMgr &mgr );
  SpidrMgr &operator=( SpidrMgr &mgr );

  // The one and only instance is also private, access through instance()
  static SpidrMgr *_inst;

  vector<SpidrInfo> _spidrList;
  int      _currId;
  ofstream _fLog; // Output stream for log messages
};

// ----------------------------------------------------------------------------
#endif // SPIDRMGR_H
