/* ----------------------------------------------------------------------------
File   : mpx3dacsdescr.h

Descr  : Descriptions of Medipix3 DACs.

History:
01FEB2013; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef MPX3DACSDESCR_H_
#define MPX3DACSDESCR_H_

#ifndef DAC_T_DEFINED
typedef struct dac_s
{
  int         code;
  const char *name;
  int         bits;   // Number of bits
  int         dflt;
} dac_t;
#define DAC_T_DEFINED
#endif

#include "mpx3defs.h"

// Tables with descriptions of DACs in the DACs register
// according to device type

static const dac_t MPX3RX_DAC_TABLE[MPX3RX_DAC_COUNT] =
{
  { MPX3RX_DAC_THRESH_0,    "Threshold[0]", 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_1,    "Threshold[1]", 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_2,    "Threshold[2]", 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_3,    "Threshold[3]", 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_4,    "Threshold[4]", 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_5,    "Threshold[5]", 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_6,    "Threshold[6]", 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_7,    "Threshold[7]", 9, (1<<9)/2 },
  { MPX3RX_DAC_PREAMP,      "Preamp",       8, (1<<8)/2 },
  { MPX3RX_DAC_IKRUM,       "Ikrum",        8, (1<<8)/2 },
  { MPX3RX_DAC_SHAPER,      "Shaper",       8, (1<<8)/2 },
  { MPX3RX_DAC_DISC,        "Disc",         8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_LS,     "Disc_LS",      8, (1<<8)/2 },
  { MPX3RX_DAC_SHAPER_TEST, "Shaper_Test",  8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_L,      "DAC_DiscL",    8, (1<<8)/2 },
  { MPX3RX_DAC_TEST,        "DAC_test",     8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_H,      "DAC_DiscH",    8, (1<<8)/2 },
  { MPX3RX_DAC_DELAY,       "Delay",        8, (1<<8)/2 },
  { MPX3RX_DAC_TP_BUF_IN,   "TP_BufferIn",  8, (1<<8)/2 },
  { MPX3RX_DAC_TP_BUF_OUT,  "TP_BufferOut", 8, (1<<8)/2 },
  { MPX3RX_DAC_RPZ,         "RPZ",          8, (1<<8)/2 },
  { MPX3RX_DAC_GND,         "GND",          8, (1<<8)/2 },
  { MPX3RX_DAC_TP_REF,      "TP_REF",       8, (1<<8)/2 },
  { MPX3RX_DAC_FBK,         "FBK",          8, (1<<8)/2 },
  { MPX3RX_DAC_CAS,         "Cas",          8, (1<<8)/2 },
  { MPX3RX_DAC_TP_REF_A,    "TP_REFA",      9, (1<<9)/2 },
  { MPX3RX_DAC_TP_REF_B,    "TP_REFB",      9, (1<<9)/2 }
};

// ----------------------------------------------------------------------------
#endif // MPX3DACSDESCR_H_
