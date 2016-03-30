/* ----------------------------------------------------------------------------
File   : vpxdacsdescr.h

Descr  : Descriptions of Velopix DACs.

History:
29MAR2016; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef VPXDACSDESCR_H_
#define VPXDACSDESCR_H_

#ifndef DAC_T_DEFINED
// Structure containing info about a DAC
typedef struct dac_s
{
  int         code;
  const char *name;
  int         bits;
  int         dflt;
} dac_t;
#define DAC_T_DEFINED
#endif

// Table with info about the DACs
static const dac_t VPX_DAC_TABLE[VPX_DAC_COUNT] =
{
  { VPX_DAC_IPREAMP,      "I_Preamp",          8,  128 },
  { VPX_DAC_IKRUM,        "I_Krum",            8,  128 },
  { VPX_DAC_IDISC,        "I_Disc",            8,  128 },
  { VPX_DAC_IPIXELDAC,    "I_PixelDAC",        8,  128 },
  { VPX_DAC_VTPCOARSE,    "Vtp_coarse",        8,  128 },
  { VPX_DAC_VTPFINE,      "Vtp_fine",         10,  512 },
  { VPX_DAC_VPREAMP_CAS,  "V_Preamp_CAS",      8,  128 },
  { VPX_DAC_VFBK,         "Vfbk",              8,  128 },
  { VPX_DAC_VTHR,         "Vthr",             14, 4096 },
  { VPX_DAC_VCAS_DISC,    "Vcas_Disc",         8,  128 },
  { VPX_DAC_VIN_CAS,      "Vin_CAS",           8,  128 },
  { VPX_DAC_VREFSLVS,     "Vref_SLVS",         8,  128 },
  { VPX_DAC_IBIASSLVS,    "Ibias_SLVS",        8,  128 },
  { VPX_DAC_RES_BIAS,     "Res_bias",          4,    8 }
};

// ----------------------------------------------------------------------------
#endif // VPXDACSDESCR_H_
