#include "spidrhwapi.h"
#include "SpidrMgr.h"

#include "mpx3defs.h"

#ifndef WIN32
#include <string.h>
#include <time.h>
#include <sys/time.h>
#endif

const char *LIB_VERSION_STR = "SpidrPixelman v1.0.0, 19 Jun 2013";

// Some convenient macros
#define GETCONTROLLER( id ) \
    SpidrController *spidrctrl = SpidrMgr::instance()->controller( id ); \
    if( !spidrctrl ) return 1;
#define GETDAQ( id ) \
    SpidrDaq *spidrdaq = SpidrMgr::instance()->daq( id ); \
    if( !spidrdaq ) return 1;
#define LOGGER()        SpidrMgr::instance()->log()
#define LOGFUNCNAME()   LOGGER() << time_str() << __FUNCTION__ << "()" << endl
#define LOGFUNCPAR(par) LOGGER() << time_str() << __FUNCTION__ \
                                 << "(" << par << ")" << endl

// Local function prototypes
static string time_str();
static void   spidrCallback( int id );

static HwCallback PixmanCallbackFunc = 0;

static int SpidrTriggerMode = SHUTTERMODE_AUTO;

// ----------------------------------------------------------------------------
// MPX3 Interface

Mpx3Interface spidrInterface =
  {
    spidrFindDevices,
    spidrInit,
    spidrCloseDevice,
    spidrSetCallback,
    spidrSetCallbackData,
    spidrGetHwInfoCount,
    spidrGetHwInfoFlags,
    spidrGetHwInfo,
    spidrSetHwInfo,
    spidrGetDevInfo,
    spidrReset,
    spidrSetDacs,
    spidrSenseSignal,
    spidrSetExtDac,
    spidrSetPixelsCfg,
    spidrSetAcqPars,
    spidrStartAcquisition,
    spidrStopAcquisition,
    spidrGetAcqTime,
    spidrResetMatrix,
    spidrReadMatrix,
    spidrWriteMatrix,
    spidrSendTestPulses,
    spidrIsBusy,
    spidrGetLastError,
    spidrGetLastDevError,
    spidrGetChipIds,
    spidrLoadCfgData,
    spidrSaveCfgData
  };

// ----------------------------------------------------------------------------
// SPIDR MPX3 Interface
// ----------------------------------------------------------------------------

MY_LIB_API Mpx3Interface *getMpx3Interface()
{
  return &spidrInterface;
}

// ----------------------------------------------------------------------------

int spidrFindDevices( int ids[], int *count )
{
  LOGFUNCNAME();
  int id, cnt = 0;
  if( SpidrMgr::instance()->getFirst( &id ) == 0 )
    {
      ids[cnt] = id;
      ++cnt;
      while( SpidrMgr::instance()->getNext( &id ) == 0 )
        {
	  ids[cnt] = id;
	  ++cnt;
        }
    }
  *count = cnt;
  return 0;
}

// ----------------------------------------------------------------------------

int spidrInit( int id )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  int errorstat = 0;
  if( !spidrctrl->reset( &errorstat ) )
    {
      LOGGER() << "### reset(): " << spidrctrl->errorString() << endl;
    }
  return errorstat;
}

// ----------------------------------------------------------------------------

int spidrCloseDevice( int id )
{
  LOGFUNCNAME();
  GETDAQ( id );
  spidrdaq->stop();
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSetCallback( HwCallback callback )
{
  LOGFUNCNAME();
  PixmanCallbackFunc = callback;
  int id;
  SpidrDaq *spidrdaq;
  if( SpidrMgr::instance()->getFirst( &id ) == 0 )
    {
      do
        {
	  spidrdaq = SpidrMgr::instance()->daq( id );
	  spidrdaq->setCallbackId( id );
	  // Function spidrCallback() (see further down) calls
	  // the Pixelman callback function(s)
	  spidrdaq->setCallback( spidrCallback );
        }
      while( SpidrMgr::instance()->getNext( &id ) == 0 );
    }
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSetCallbackData( int id, INTPTR data )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );
  spidrinfo->callbackData = data;
  return 0;
}

// ----------------------------------------------------------------------------

int spidrGetHwInfoCount()
{
  LOGFUNCNAME();
  return 5;
}

// ----------------------------------------------------------------------------

int spidrGetHwInfoFlags( int id, int index, u32 *flags )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  *flags = 0;
  if( index < 0 || index >= spidrGetHwInfoCount() ) return 1;
  switch( index )
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      break;
    default:
      return 1;
    }
  return 0;
}

// ----------------------------------------------------------------------------

int spidrGetHwInfo( int id, int index, HwInfoItem *hw_info, int *sz )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );

  char data[64];
  int *data32 = (int *) data;
  string str;
  switch( index )
    {
    case 0:
      // SPIDR Pixelman library version
      hw_info->name  = "SpidrPixmanLibVersion";
      hw_info->descr = "SPIDR Pixelman lib version";
      hw_info->type  = TYPE_CHAR;
      hw_info->flags = 0;
      str = string( LIB_VERSION_STR );
      memcpy( data, str.c_str(), str.length() + 1 );
      hw_info->count = str.length() + 1;
      break;

    case 1:
      // SPIDR firmware version
      hw_info->name  = "SpidrFwVersion";
      hw_info->descr = "SPIDR firmware version";
      hw_info->type  = TYPE_CHAR;
      hw_info->flags = 0;
      str = spidrctrl->versionToString( spidrinfo->firmwVersion );
      memcpy( data, str.c_str(), str.length() + 1 );
      hw_info->count = str.length() + 1;
      break;

    case 2:
      // SPIDR software version
      hw_info->name  = "SpidrSwVersion";
      hw_info->descr = "SPIDR software version";
      hw_info->type  = TYPE_CHAR;
      hw_info->flags = 0;
      str = spidrctrl->versionToString( spidrinfo->softwVersion );
      memcpy( data, str.c_str(), str.length() + 1 );
      hw_info->count = str.length() + 1;
      break;

    case 3:
      // Chip IDs
      hw_info->name  = "Mpx3ChipIds";
      hw_info->descr = "Medipix3 chip IDs";
      hw_info->type  = TYPE_U32;
      hw_info->flags = 0;
      hw_info->count = 4;
      for( int i=0; i<4; ++i ) data32[i] = spidrinfo->chipIds[i];
      break;

    case 4:
      // Chip mapping
      hw_info->name  = "Mpx3ChipMap";
      hw_info->descr = "Medipix3 chip order";
      hw_info->type  = TYPE_U32;
      hw_info->flags = 0;
      hw_info->count = 4;
      for( int i=0; i<4; ++i ) data32[i] = spidrinfo->chipMap[i];
      break;

    default:
      LOGGER() << "### GetHwInfo(): invalid item index " << index << endl;
      hw_info->type  = TYPE_BOOL;
      hw_info->count = 0;
      return 1;
    }

  int size = sizeofType[hw_info->type] * hw_info->count;
  if( *sz == 0 )
    {
      // Inform client about required size
      *sz = size;
      LOGGER() << "GetHwInfo(" << index << "): required size = "
	       << size << endl;
    }
  else if( *sz < size )
    {
      LOGGER() << "### GetHwInfo(" << index << "): invalid item buffer size"
	       << ", need " << size << ", available " << *sz << endl;
      return 1;
    }
  else
    {
      memcpy( hw_info->data, data, size );
    }
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSetHwInfo( int id, int index, void *data, int sz )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  if( index < 0 || index >= spidrGetHwInfoCount() ) return 1;
  // ### TO BE DONE
  return 0;
}

// ----------------------------------------------------------------------------

int spidrGetDevInfo( int id, DevInfo *dev_info )
{
  LOGFUNCNAME();
  SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );
  if( !spidrinfo ) return 1;

  // Fill in the DevInfo struct
  dev_info->pixCount = spidrinfo->chipCount * MPX_PIXELS;

  if( spidrinfo->chipCount == 4 )
    dev_info->rowLen = MPX_PIXEL_ROWS * 2;
  else
    dev_info->rowLen = MPX_PIXEL_ROWS;

  dev_info->numberOfChips = spidrinfo->chipCount;

  if( spidrinfo->chipCount == 4 )
    dev_info->numberOfRows = 2;
  else
    dev_info->numberOfRows = 1;

  if( spidrinfo->chipType == MPX_TYPE_MPX31 )
    dev_info->mpxType = MPX_3;
  else
    dev_info->mpxType = MPX_3RX;

  string name = spidrinfo->ipAddrString;
  strncpy( dev_info->chipboardID, name.c_str(), MPX_MAX_CHBID );

  dev_info->ifaceName       = "SPIDR";
  dev_info->ifaceType       = 63;     // Undefined
  dev_info->ifaceVendor     = 4095;   // Undefined
  dev_info->ifaceSerial     = 262143; // Undefined
  dev_info->chipboardSerial = 0;

  dev_info->suppAcqModes    = (ACQMODE_ACQSTART_TIMERSTOP |
			       ACQMODE_HWTRIGSTART_TIMERSTOP |
			       ACQMODE_EXTSHUTTER);
  dev_info->suppCallback    = FALSE;

  dev_info->clockReadout    = 125.0;
  dev_info->clockTimepix    = 125.0;

  dev_info->timerMinVal     = 0.000000008; // 8 ns
  dev_info->timerMaxVal     = 34.35973836; // 32-bits * 8 ns
  dev_info->timerStep       = 0.000000008; // 8 ns

  dev_info->maxPulseCount   = (u32) 0xFFFFFFFF;
  dev_info->maxPulseHeight  = 1.0;
  dev_info->maxPulsePeriod  = 34.35973836; // 32-bits * 8 ns

  dev_info->extDacMinV      = 0.0;
  dev_info->extDacMaxV      = 0.0;
  dev_info->extDacStep      = 0.0;

  return 0;
}

// ----------------------------------------------------------------------------

int spidrReset( int id )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  if( !spidrctrl->resetDevices() )
    {
      LOGGER() << "### resetDevices(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSetDacs( int id, DACTYPE dac_vals[], int sz )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );

  int dac_cnt = 0;
  if( spidrinfo->chipType == MPX_TYPE_MPX31 )
    dac_cnt = DAC_COUNT_MPX3;
  else if( spidrinfo->chipType == MPX_TYPE_MPX3RX )
    dac_cnt = DAC_COUNT_MPX3_RX;
  else
    {
      LOGGER() << "### SetDacs(): unknown chip type "
	       << spidrinfo->chipType << endl;
      return 1;
    }

  int chips_todo = sz / dac_cnt;
  if( chips_todo != spidrinfo->chipCount )
    LOGGER() << "+++ WARN SetDacs(): DAC array size " << sz
	     << " does not match (#chips=" << spidrinfo->chipCount
	     << ",type=" << spidrinfo->chipType << ",DACs="
	     << dac_cnt << ")" << endl;

  for( int chip=0; chip<chips_todo; ++chip )
    {
#define USE_SETDACS
#ifdef USE_SETDACS
      // Using a single message to transfer all DAC values
      int dacvals32[32];
      for( int dac_nr=0; dac_nr<dac_cnt; ++dac_nr )
	dacvals32[dac_nr] = dac_vals[dac_nr];
      if( !spidrctrl->setDacs( spidrinfo->chipMap[chip],
			       dac_cnt, dacvals32 ) )
	{
	  LOGGER() << "### setDacs(): " << spidrctrl->errorString() << endl;
	  return 1;
	}
#else
      // Using a message per DAC value
      for( int dac_nr=0; dac_nr<dac_cnt; ++dac_nr, ++i )
	{
	  if( !spidrctrl->setDac( spidrinfo->chipMap[chip],
				  dac_nr, dac_vals[i] ) )
	    {
	      LOGGER() << "### setDac(): " << spidrctrl->errString() << endl;
	      return 1;
	    }
	}
#endif // USE_SETDACS
    }
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSenseSignal( int id, int chip_nr,
		      int code, double *val )
{
  LOGFUNCPAR(code);
  GETCONTROLLER( id );
  SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );
  int chip_pos = spidrinfo->chipMap[chip_nr];

  // In Pixelman version 2011_12_07 'code' appears to be a DAC number/index
  // rather than the DAC 'code' !
  if( !spidrctrl->setSenseDac( code ) )
  //if( !spidrctrl->setSenseDacCode( code ) )
    {
      LOGGER() << "### setSenseDac(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  if( !spidrctrl->writeOmr( chip_pos ) )
    {
      LOGGER() << "### writeOmr(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  int adc_val;
  if( !spidrctrl->getAdc( chip_pos, &adc_val ) )
    {
      LOGGER() << "### getAdc(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  // Convert the ADC count to Volt (SPIDR ADC ref is 1.5V)
  *val = ((double) adc_val * 1.5) / (double) 0xFFFF;
  //LOGGER() << "ADC " << adc_val << ", V = " << *val << endl;
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSetExtDac( int id, int code, double val )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  // ### TO BE DONE
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSetPixelsCfg( int id, byte cfgs[], u32 sz )
{
  // NB: 'sz' indicates the number of pixels, not bytes
  //     (from email from Daniel Turecek, 11 Jun 2013)
  LOGFUNCPAR(sz);
  GETCONTROLLER( id );
  SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );

  unsigned int pixman_pixcfg;
  int chip, i, x, y;
  int chips_todo = sz / MPX_PIXELS;
  if( chips_todo != spidrinfo->chipCount )
    {
      LOGGER() << "+++ WARN spidrSetPixelsCfg(): config size " << sz
	       << " does not match chip count "
	       << spidrinfo->chipCount << endl;
      // Adjust...
      if( chips_todo > spidrinfo->chipCount )
	chips_todo = spidrinfo->chipCount;
    }
  for( i=0; i<16; ++i )
    LOGGER() << (unsigned int) cfgs[i] << " ";
  LOGGER() << endl;

  for( chip=0; chip<chips_todo; ++chip )
    {
      spidrctrl->resetPixelConfig();
      x = 0;
      y = 0;

      // Extract the various pixel configuration parameters
      // (depending on MPX31 or MPX3RX device type)
      // and set these in the configuration
      if( spidrinfo->chipType == MPX_TYPE_MPX31 )
	{
	  // Medipix3.1 device
	  int  configtha = 0, configthb = 0;
	  bool configtha4 = false, configthb4 = false;
	  bool gainmode = false, test_it = false, mask_it = false;
	  for( i=0; i<MPX_PIXELS; ++i )
	    {
	      // Depending on what Pixelman provides...:
	      // assume 'typedef struct _Mpx3PixCfg' in common.h
	      pixman_pixcfg = ((unsigned int) cfgs[i] |
			       (((unsigned int) cfgs[i+1]) << 8));

	      mask_it    = ((pixman_pixcfg & 0x0001) == 0); // NB: 0=active
	      test_it    = ((pixman_pixcfg & 0x0002) == 0); // NB: 0=active
	      gainmode   = ((pixman_pixcfg & 0x0004) != 0);
	      configtha  = ((pixman_pixcfg & 0x0078) >> 3);
	      configtha4 = ((pixman_pixcfg & 0x0080) != 0);
	      configthb  = ((pixman_pixcfg & 0x0F00) >> 8);
	      configthb4 = ((pixman_pixcfg & 0x1000) != 0);

	      if( mask_it ) spidrctrl->setPixelMaskMpx3( x, y );

	      spidrctrl->configPixelMpx3( x, y,
					  configtha, configthb,
					  configtha4, configthb4,
					  gainmode, test_it );
	      ++x;
	      if( x == MPX_PIXEL_COLUMNS )
		{
		  ++y; x = 0;
		}
	    }
	  // Upload the configuration for this chip
	  if( !spidrctrl->setPixelConfigMpx3( spidrinfo->chipMap[chip] ) )
	    {
	      LOGGER() << "### writePixelConfigMpx3( "
		       << spidrinfo->chipMap[chip] << " ): "
		       << spidrctrl->errorString() << endl;
	      return 1;
	    }
	}
      else if( spidrinfo->chipType == MPX_TYPE_MPX3RX )
	{
	  // Medipix3RX device
	  int  discl = 0, disch = 0;
	  bool test_it = false, mask_it = false;
	  for( i=0; i<MPX_PIXELS; i+=2 )
	    {
	      // Depending on what Pixelman provides...:
	      // assume 'typedef struct _Mpx3RxPixCfg' in common.h
	      pixman_pixcfg = ((unsigned int) cfgs[i] |
			       (((unsigned int) cfgs[i+1]) << 8));

	      mask_it = ((pixman_pixcfg & 0x0001) == 0); // NB: 0=active
	      test_it = ((pixman_pixcfg & 0x0002) == 0); // NB: 0=active
	      discl   = ((pixman_pixcfg & 0x00F8) >> 3);
	      disch   = ((pixman_pixcfg & 0x1F00) >> 8);

	      if( mask_it ) spidrctrl->setPixelMaskMpx3rx( x, y );

	      spidrctrl->configPixelMpx3rx( x, y, discl, disch, test_it );

	      ++x;
	      if( x == MPX_PIXEL_COLUMNS )
		{
		  ++y; x = 0;
		}
	    }
	  // Upload the configuration for this chip
	  if( !spidrctrl->setPixelConfigMpx3rx( spidrinfo->chipMap[chip] ) )
	    {
	      LOGGER() << "### writePixelConfigMpx3rx( "
		       << spidrinfo->chipMap[chip] << " ): "
		       << spidrctrl->errorString() << endl;
	      return 1;
	    }
	}
      else
	{
	  LOGGER() << "### SetPixelsCfg(): unknown chip type" << endl;
	  return 1;
	}
    }
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSetAcqPars( int id, Mpx3AcqParams *pars )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  GETDAQ( id );
  /*
    typedef struct _Mpx3AcqParams
    {
    BOOL useHwTimer;                 // hw timer is used
    BOOL polarityPositive;           // positive polarity
    int mode;                        // acq mode (see ACQMODE_xxx in common.h)
    int acqCount;                    // acq count (for burst mode)
    double time;                     // acq time (if controlled by HW timer)
    Mpx3OpMode opMode;               // operation mode
    Mpx3CounterDepth counterDepth;   // depth of counter 1,4,12,24 bits
    Mpx3ColumnBlock columnBlock;     // column block selection
    Mpx3RowBlock rowBlock;           // row block selection
    Mpx3CounterSelect counterSelect; // counter selection
    Mpx3Gain gain;                   // gain mode
    BOOL equalizeTHH;                // bit to control if THH will be equalized
    BOOL equalization;               // bit to control if it's equalization
    BOOL discCsmSpm;                 // bit to control discCsmSpm bit in OMR
    BOOL infoHeader;                 // bit to control infoHeader bit in OMR
    } Mpx3AcqParams;
  */

  bool polarity = false;
  if( pars->polarityPositive ) polarity = true;
  if( !spidrctrl->setPolarity( polarity ) )
    {
      LOGGER() << "### setPolarity(): " << spidrctrl->errorString() << endl;
      return 1;
    }

  /* These are definitions taken from common.h:
     ACQMODE_ACQSTART_TIMERSTOP:
       acq. is started immediately (after startAcq call),
       acq. is stopped by timer (HW or PC) - "common acq"
     ACQMODE_ACQSTART_HWTRIGSTOP
       acq. is started immediately (after startAcq call),
       acq. is stopped by trigger from HW library (e.g. ext shutter monitor)
     ACQMODE_ACQSTART_SWTRIGSTOP
       acq. is started immediately (after startAcq call),
       acq. is stopped by mpxCtrlTrigger(TRIGGER_ACQSTOP) trigger to
       HW library (e.g. ext shutter monitor)
     ACQMODE_HWTRIGSTART_TIMERSTOP acq. is started by trigger
       from HW library, stopped by timer (HW or PC)
     ACQMODE_HWTRIGSTART_HWTRIGSTOP acq. is started by trigger
       from HW library, stopped by trigger from HW library
     ACQMODE_SWTRIGSTART_TIMERSTOP acq. is started by
       mpxCtrlTrigger(TRIGGER_ACQSTART), stopped by timer (HW or PC)
     ACQMODE_SWTRIGSTART_SWTRIGSTOP acq. is started by
       mpxCtrlTrigger(TRIGGER_ACQSTART), stopped by
       mpxCtrlTrigger(TRIGGER_ACQSTOP)
     ACQMODE_EXTSHUTTER
       shutter is controlled externally during acq. (e.g. independently on SW)
     ACQMODE_BURST
       burst mode enabled

     SPIDR supports:
       - Software (auto-trigger) start, timer stop,
       - Hardware (external trigger) start, timer stop,
       - Hardware start, hardware stop
     where the external trigger can be configured for positive/negative edge.
  */
  int trigger_mode;
  switch( pars->mode )
    {
    case ACQMODE_ACQSTART_TIMERSTOP:
      trigger_mode = SHUTTERMODE_AUTO;
      break;
    case ACQMODE_HWTRIGSTART_TIMERSTOP:
      trigger_mode = SHUTTERMODE_POS_EXT_TIMER;
      break;
    case ACQMODE_EXTSHUTTER:
      trigger_mode = SHUTTERMODE_POS_EXT;
      break;
    default:
      trigger_mode = SHUTTERMODE_AUTO;
      break;
    }
  SpidrTriggerMode = trigger_mode; // Remember for acquisition starts
  int trigger_length_us   = (int) (1000000.0 * pars->time);
  int trigger_freq_hz;
  int nr_of_triggers      = 1;
  int trigger_pulse_count = 0;
  if( pars->time > 0.0 )
    trigger_freq_hz = (int) (1.0 / pars->time);
  else
    trigger_freq_hz = 1;
  if( trigger_freq_hz > 1 ) --trigger_freq_hz;
  if( !spidrctrl->setShutterTriggerConfig( trigger_mode, trigger_length_us,
				    trigger_freq_hz, nr_of_triggers,
				    trigger_pulse_count ) )
    {
      LOGGER() << "### setTriggerConfig(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  LOGGER() << "SetAcqPars.setTriggerConfig(): mode=" << trigger_mode
	   << ", length_us=" << trigger_length_us
	   << ", freq=" << trigger_freq_hz
	   << ", trigs=" << nr_of_triggers << endl;

  int pixeldepth = 12;
  if( pars->counterDepth == MPX3_COUNTER_1B )
    pixeldepth = 1;
  else if( pars->counterDepth == MPX3_COUNTER_4B_6B )
    pixeldepth = 4;
  else if( pars->counterDepth == MPX3_COUNTER_12B )
    pixeldepth = 12;
  else if( pars->counterDepth == MPX3_COUNTER_24B )
    pixeldepth = 24;
  spidrdaq->setPixelDepth( pixeldepth );
  if( !spidrctrl->setPixelDepth( pixeldepth ) )
    {
      LOGGER() << "### setPixelDepth(): " << spidrctrl->errorString() << endl;
      return 1;
    }

  return 0;
}

// ----------------------------------------------------------------------------

int spidrStartAcquisition( int id )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  if( !spidrctrl->clearBusy() )
    {
      LOGGER() << "### clearBusy(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  if( !spidrctrl->startAutoTrigger() )
    {
      LOGGER() << "### startAutoTrigger(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  return 0;
}

// ----------------------------------------------------------------------------

int spidrStopAcquisition( int id )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  if( !spidrctrl->stopAutoTrigger() )
    {
      LOGGER() << "### stopAutoTrigger(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  if( !spidrctrl->setBusy() )
    {
      LOGGER() << "### setBusy(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  return 0;
}

// ----------------------------------------------------------------------------

int spidrGetAcqTime( int id, double *time )
{
  LOGFUNCNAME();
  GETDAQ( id );
  *time = spidrdaq->frameTimestampDouble();
  //LOGGER() << "T=" << *time << endl;
  return 0;
}

// ----------------------------------------------------------------------------

int spidrResetMatrix( int id )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  // Nothing needs to be done, is automatic...
  return 0;
}

// ----------------------------------------------------------------------------

int spidrReadMatrix( int id, u32 *data, u32 sz )
{
  LOGFUNCPAR(sz);
  GETDAQ( id );
  SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );

  int chips_todo = sz / MPX_PIXELS;
  if( chips_todo != spidrinfo->chipCount )
    {
      // Adjust if necessary...
      if( chips_todo > spidrinfo->chipCount )
	chips_todo = spidrinfo->chipCount;
    }

  // Copy the device frame(s)
  int chip, size_in_bytes;
  for( chip=0; chip<chips_todo; ++chip )
    {
      // NB: SpidrDaq orders devices in 'mapped' fashion,
      //     so don't use spidrinfo->chipMap[chip]
      int *frame_data = spidrdaq->frameData( chip, &size_in_bytes );
      memcpy( (void *) data, (void *) frame_data, size_in_bytes );
      data += MPX_PIXELS;
    }
  spidrdaq->releaseFrame();
  return 0;
}

// ----------------------------------------------------------------------------

int spidrWriteMatrix( int id, u32 *data, u32 sz )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  // ### Not implemented
  return 0;
}

// ----------------------------------------------------------------------------

int spidrSendTestPulses( int id, double charge[2], double period,
			 u32 pulse_count, u32 ctpr[8] )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  // For SPIDR: 'charge' is don't care,
  // 'period' in seconds, i.e. multiply by 1000000
  int trigger_mode        = 4; // SPIDR_TRIG_AUTO
  int trigger_period_us   = (int) (1000000.0 * period);
  int trigger_freq_hz     = 10;
  int nr_of_triggers      = pulse_count;
  int trigger_pulse_count = 0;
  if( !spidrctrl->setShutterTriggerConfig( trigger_mode, trigger_period_us,
				    trigger_freq_hz, nr_of_triggers,
				    trigger_pulse_count ) )
    {
      LOGGER() << "### setTriggerConfig(): " << spidrctrl->errorString() << endl;
      return 1;
    }
  LOGGER() << "SendTestPulses.setTriggerConfig(): mode=" << trigger_mode
	   << ", period_us=" << trigger_period_us
	   << ", freq=" << trigger_freq_hz
	   << ", trigs=" << nr_of_triggers << endl;
  return 0;
}

// ----------------------------------------------------------------------------

int spidrIsBusy( int id, BOOL *busy )
{
  LOGFUNCNAME();
  GETDAQ( id );
  bool b = !spidrdaq->hasFrame();
  if ( b )
    *busy = TRUE;
  else
    *busy = FALSE;
  /*
  if( !(*busy) )
    {
      // DEBUG info
      LOGGER() << "  Seqnr=" << spidrdaq->expSequenceNr() << endl;
    }
  */
  return 0;
}

// ----------------------------------------------------------------------------

const char *spidrGetLastError()
{
  LOGFUNCNAME();
  return "GetLastError(): see logfile";
}

// ----------------------------------------------------------------------------

const char *spidrGetLastDevError( int id )
{
  LOGFUNCNAME();
  SpidrController *spidrctrl = SpidrMgr::instance()->controller( id );
  if ( !spidrctrl ) return "GetLastDevError(): see logfile";
  return spidrctrl->errorString().c_str();
}

// ----------------------------------------------------------------------------

int spidrGetChipIds( int id, char *chip_ids, u32 *sz )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );
  if( *sz < 4*sizeof(u32) )
    {
      *sz = 4*sizeof(u32);
      return 1;
    }
  *sz = 4*sizeof(u32);
  // Get the chip IDs according to the chip mapping
  int device_ids[4];
  for( int chip=0; chip<4; ++chip )
    if( !spidrctrl->getDeviceId( spidrinfo->chipMap[chip],
				 &device_ids[chip] ) )
      {
	LOGGER() << "### getDeviceId(" << spidrinfo->chipMap[chip]
		 << "): " << spidrctrl->errorString() << endl;
	return 1;
      }
  memcpy( (void *) chip_ids, (void *) device_ids, *sz );
  return 0;
}

// ----------------------------------------------------------------------------

int spidrLoadCfgData( int id, byte *data, u32 *sz )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  // ### TO BE DONE
  return 1;
}

// ----------------------------------------------------------------------------

int spidrSaveCfgData( int id, byte *data, u32 sz )
{
  LOGFUNCNAME();
  GETCONTROLLER( id );
  // ### TO BE DONE
  return 1;
}

// ----------------------------------------------------------------------------

#include <sstream>
#include <iomanip>

static string time_str()
{
  ostringstream oss;
#ifdef WIN32
  // Using Windows-specific 'GetLocalTime()' function for time
  SYSTEMTIME st;
  GetLocalTime( &st );
  oss << setfill('0')
      << setw(2) << st.wHour << ":"
      << setw(2) << st.wMinute << ":"
      << setw(2) << st.wSecond << ":"
      << setw(3) << st.wMilliseconds << " ";
#else
  struct timeval tv;
  gettimeofday( &tv, 0 );
  struct tm tim;
  localtime_r( &tv.tv_sec, &tim );
  oss << setfill('0')
      << setw(2) << tim.tm_hour << ":"
      << setw(2) << tim.tm_min << ":"
      << setw(2) << tim.tm_sec << ":"
      << setw(3) << tv.tv_usec/1000 << " ";
#endif // WIN32
  return oss.str();
}

// ----------------------------------------------------------------------------

static void spidrCallback( int id )
{
  if( PixmanCallbackFunc )
    {
      // The 'id' provided by SPIDR allows us to find and provide the proper
      // callback data for the subsequent callbacks to Pixelman
      SpidrInfo *spidrinfo = SpidrMgr::instance()->info( id );
      INTPTR data = spidrinfo->callbackData;
      if( spidrinfo )
	{
	  PixmanCallbackFunc( data, HWCB_ACQSTARTED, 0 );
	  PixmanCallbackFunc( data, HWCB_ACQFINISHED, 0 );
	}
    }
}

// ----------------------------------------------------------------------------
