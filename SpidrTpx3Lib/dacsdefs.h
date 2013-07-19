/* ----------------------------------------------------------------------------
File   : dacsdefs.h

Descr  : Definitions for Timepix3 DACs.

History:
04JUL2013; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef DACSDEFS_H_
#define DACSDEFS_H_

// Number of DACs
#define TPX3_DAC_COUNT          23
#define TPX3_DAC_COUNT_TO_SENSE 18

// Timepix3 DAC codes
#define TPX3_IBIAS_PREAMP_ON    1
#define TPX3_IBIAS_PREAMP_OFF   2
#define TPX3_VPREAMP_NCAS       3
#define TPX3_IBIAS_IKRUM        4
#define TPX3_VFBK               5
#define TPX3_VTHRESH_FINE       6
#define TPX3_VTHRESH_COARSE     7
#define TPX3_IBIAS_DISCS1_ON    8
#define TPX3_IBIAS_DISCS1_OFF   9
#define TPX3_IBIAS_DISCS2_ON    10
#define TPX3_IBIAS_DISCS2_OFF   11
#define TPX3_IBIAS_PIXELDAC     12
#define TPX3_IBIAS_TPBUFIN      13
#define TPX3_IBIAS_TPBUFOUT     14
#define TPX3_VTP_COARSE         15
#define TPX3_VTP_FINE           16
#define TPX3_IBIAS_CP_PLL       17

#define TPX3_SENSEOFF           0

// Structure containing info about a DAC
typedef struct dac_s
{
  int         code;
  const char *name;
  int         bits;
  int         dflt;
} dac_t;

// Table with info about the DACs
static const dac_t TPX3_DAC_TABLE[TPX3_DAC_COUNT] =
{
  { TPX3_IBIAS_PREAMP_ON,  "Ibias_Preamp_ON",   8, 128 },
  { TPX3_IBIAS_PREAMP_OFF, "Ibias_Preamp_OFF",  4, 8   },
  { TPX3_VPREAMP_NCAS,     "VPreamp_NCAS",      8, 128 },
  { TPX3_IBIAS_IKRUM,      "Ibias_Ikrum",       8, 128 },
  { TPX3_VFBK,             "Vfbk",              8, 128 },
  { TPX3_VTHRESH_FINE,     "Vthreshold_fine",   9, 256 },
  { TPX3_VTHRESH_COARSE,   "Vthreshold_coarse", 4, 8   },
  { TPX3_IBIAS_DISCS1_ON,  "Ibias_DiscS1_ON",   8, 128 },
  { TPX3_IBIAS_DISCS1_OFF, "Ibias_DiscS1_OFF",  4, 8   },
  { TPX3_IBIAS_DISCS2_ON,  "Ibias_DiscS2_ON",   8, 128 },
  { TPX3_IBIAS_DISCS2_OFF, "Ibias_DiscS2_OFF",  4, 8   },
  { TPX3_IBIAS_PIXELDAC,   "Ibias_PixelDAC",    8, 128 },
  { TPX3_IBIAS_TPBUFIN,    "Ibias_TPbufferIn",  8, 128 },
  { TPX3_IBIAS_TPBUFOUT,   "Ibias_TPbufferOut", 8, 128 },
  { TPX3_VTP_COARSE,       "Vtp_coarse",        8, 128 },
  { TPX3_VTP_FINE,         "Vtp_fine",          9, 256 },
  { TPX3_IBIAS_CP_PLL,     "Ibias_CP_PLL",      8, 128 },

  { 18, "Pll_Vcntrl",        0, 0   },
  { 28, "BandGap_output",    0, 0   },
  { 29, "BandGap_Temp",      0, 0   },
  { 30, "Ibias_dac",         0, 0   },
  { 31, "Ibias_dac_cas",     0, 0   },
  { TPX3_SENSEOFF,         "SenseOFF",          0, 0   }
};

// ----------------------------------------------------------------------------
#endif // DACSDEFS_H_
