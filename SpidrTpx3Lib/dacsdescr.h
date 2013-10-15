/* ----------------------------------------------------------------------------
File   : dacsdescr.h

Descr  : Descriptions of Timepix3 DACs.

History:
22JUL2013; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef DACSDESCR_H_
#define DACSDESCR_H_

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
  { TPX3_PLL_VCNTRL,       "Pll_Vcntrl",        8, 128 },

  { TPX3_BANDGAP_OUTPUT,   "BandGap_output",    0, 0   },
  { TPX3_BANDGAP_TEMP,     "BandGap_Temp",      0, 0   },
  { TPX3_IBIAS_DAC,        "Ibias_dac",         0, 0   },
  { TPX3_IBIAS_DAC_CAS,    "Ibias_dac_cas",     0, 0   },
  { TPX3_SENSEOFF,         "SenseOFF",          0, 0   }
};

// ----------------------------------------------------------------------------
#endif // DACSDESCR_H_
