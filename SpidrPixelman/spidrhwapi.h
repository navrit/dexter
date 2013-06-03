#ifndef SPIDRHWAPI_H
#define SPIDRHWAPI_H

#include "mpx3hw.h"
#include "mpxerrors.h"

#ifdef MY_LIB_EXPORT
#define MY_LIB_API __declspec(dllexport)
#else
#define MY_LIB_API __declspec(dllimport)
#endif

// ----------------------------------------------------------------------------
// MPX2 Interface
/*
extern "C"
{
  MY_LIB_API Mpx2Interface *getMpx2Interface();
}

int         mpx2FindDevices     ( int ids[], int *count );
int         mpx2Init            ( int id );
int         mpx2CloseDevice     ( int id );
int         mpx2SetCallback     ( HwCallback cb );
int         mpx2SetCallbackData ( int id, INTPTR data );
int         mpx2GetHwInfoCount  ();
int         mpx2GetHwInfoFlags  ( int id, int index, u32 *flags );
int         mpx2GetHwInfo       ( int id, int index,
                                  HwInfoItem *hwInfo, int *sz );
int         mpx2SetHwInfo       ( int id, int index,
                                  void *data, int sz );
int         mpx2GetDevInfo      ( int id, DevInfo *dev_info );
int         mpx2Reset           ( int id );
int         mpx2SetDacs         ( int id, DACTYPE dac_vals[], int sz,
				  byte sense_chip[], byte ext_dac_chip[],
                                  int codes[], u32 tp_reg );
int         mpx2GetMpxDacVal    ( int id, int chip_nr, double *val );
int         mpx2SetExtDacVal    ( int id, double val );
int         mpx2SetPixelsCfg    ( int id, byte cfgs[], u32 sz );
int         mpx2SetAcqPars      ( int id, AcqParams *pars );
int         mpx2StartAcquisition( int id );
int         mpx2StopAcquisition ( int id );
int         mpx2GetAcqTime      ( int id, double *time );
int         mpx2ResetMatrix     ( int id );
int         mpx2ReadMatrix      ( int id, i16 *data, u32 sz );
int         mpx2WriteMatrix     ( int id, i16 *data, u32 sz );
int         mpx2SendTestPulses  ( int id, double pulse_height,
                                  double period, u32 pulse_count );
int         mpx2IsBusy          ( int id, BOOL *busy );
const char *mpx2GetLastError    ();
const char *mpx2GetLastDevError ( int id );
int         mpx2GetChipIDs      ( int id, char *chip_ids, u32 *sz );
int         mpx2LoadCfgData     ( int id, byte *data, u32 *sz );
int         mpx2SaveCfgData     (int  id, byte *data, u32 sz );
*/
// ----------------------------------------------------------------------------
// SPIDR MPX3 Interface

extern "C"
{
  MY_LIB_API Mpx3Interface *getMpx3Interface();
}

int         spidrFindDevices     ( int ids[], int *count );
int         spidrInit            ( int id );
int         spidrCloseDevice     ( int id );
int         spidrSetCallback     ( HwCallback cb );
int         spidrSetCallbackData ( int id, INTPTR data );
int         spidrGetHwInfoCount  ();
int         spidrGetHwInfoFlags  ( int id, int index, u32 *flags );
int         spidrGetHwInfo       ( int id, int index,
				   HwInfoItem *hw_info, int *sz );
int         spidrSetHwInfo       ( int id, int index,
				   void *data, int sz );
int         spidrGetDevInfo      ( int id, DevInfo *dev_info );
int         spidrReset           ( int id );
int         spidrSetDacs         ( int id, DACTYPE dac_vals[], int sz );
int         spidrSenseSignal     ( int id, int chip_nr,
				   int code, double *val );
int         spidrSetExtDac       ( int id, int code, double val );
int         spidrSetPixelsCfg    ( int id, byte cfgs[], u32 sz );
int         spidrSetAcqPars      ( int id, Mpx3AcqParams *pars );
int         spidrStartAcquisition( int id );
int         spidrStopAcquisition ( int id );
int         spidrGetAcqTime      ( int id, double *time );
int         spidrResetMatrix     ( int id );
int         spidrReadMatrix      ( int id, u32 *data, u32 sz );
int         spidrWriteMatrix     ( int id, u32 *data, u32 sz );
int         spidrSendTestPulses  ( int id, double charge[2], double period,
				   u32 pulse_count, u32 ctpr[8] );
int         spidrIsBusy          ( int id, BOOL *busy );
const char *spidrGetLastError    ();
const char *spidrGetLastDevError ( int id );
int         spidrGetChipIds      ( int id, char *chip_ids, u32 *sz );
int         spidrLoadCfgData     ( int id, byte *data, u32 *sz );
int         spidrSaveCfgData     ( int id, byte *data, u32 sz );

// ----------------------------------------------------------------------------
#endif // SPIDRHWAPI_H
