/* ----------------------------------------------------------------------------
File   : dacsdescr.h

Descr  : Descriptions of Medipix3 DACs.

History:
01FEB2013; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef DACSDESCR_H_
#define DACSDESCR_H_

typedef struct dac_s
{
  int         code;
  const char *name;
  int         offset; // Within DACs register
  int         bits;   // Number of bits
  int         dflt;
} dac_t;

// Tables with descriptions of DACs in the DACs register
// according to device type

// NB: with the 'offset' values given here (taken from the Medipix3.1 manual),
// a bit array of length 256 (32 bytes) should be used !
// (which is the correct length of the MPX3 DACs register;
//  existing code had 'Threshold[0]' offset at 0, etc, requiring an array
//  length value of 210, when filling it using 'bitarray' functions,
//  which is a bit strange...)
static const dac_t MPX3_DAC_TABLE[MPX3_DAC_COUNT] =
{
  { MPX3_DAC_THRESH_0,   "Threshold[0]",     46, 9, 150 },
  { MPX3_DAC_THRESH_1,   "Threshold[1]",     55, 9, 150 },
  { MPX3_DAC_THRESH_2,   "Threshold[2]",     64, 9, (1<<9)/2 },
  { MPX3_DAC_THRESH_3,   "Threshold[3]",     73, 9, (1<<9)/2 },
  { MPX3_DAC_THRESH_4,   "Threshold[4]",     82, 9, (1<<9)/2 },
  { MPX3_DAC_THRESH_5,   "Threshold[5]",     91, 9, (1<<9)/2 },
  { MPX3_DAC_THRESH_6,   "Threshold[6]",    100, 9, (1<<9)/2 },
  { MPX3_DAC_THRESH_7,   "Threshold[7]",    109, 9, (1<<9)/2 },
  { MPX3_DAC_PREAMP,     "Preamp",          118, 8, 120 },
  { MPX3_DAC_IKRUM,      "Ikrum",           126, 8, 20 },
  { MPX3_DAC_SHAPER,     "Shaper",          134, 8, 173 },
  { MPX3_DAC_DISC,       "Disc",            142, 8, 255 },
  { MPX3_DAC_DISC_LS,    "Disc_LS",         150, 8, 170 },
  { MPX3_DAC_THRESH_N,   "ThresholdN",      158, 8, 20 },
  { MPX3_DAC_PIXEL,      "DAC_pixel",       166, 8, 0x76 },
  { MPX3_DAC_DELAY,      "Delay",           174, 8, (1<<8)/2 },
  { MPX3_DAC_TP_BUF_IN,  "TP_BufferIn",     182, 8, (1<<8)/2 },
  { MPX3_DAC_TP_BUF_OUT, "TP_BufferOut",    190, 8, 0x32 },
  { MPX3_DAC_RPZ,        "RPZ",             198, 8, (1<<8)-1 },
  { MPX3_DAC_GND,        "GND",             206, 8, 0x6E },
  { MPX3_DAC_TP_REF,     "TP_REF",          214, 8, (1<<8)/2 },
  { MPX3_DAC_FBK,        "FBK",             222, 8, 0x8F },
  { MPX3_DAC_CAS,        "Cas",             230, 8, 191 },
  { MPX3_DAC_TP_REF_A,   "TP_REFA",         238, 9, (1<<9)-1 },
  { MPX3_DAC_TP_REF_B,   "TP_REFB",         247, 9, (1<<9)-1 }
};

static const dac_t MPX3RX_DAC_TABLE[MPX3RX_DAC_COUNT] =
{
  { MPX3RX_DAC_THRESH_0,    "Threshold[0]",     30, 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_1,    "Threshold[1]",     39, 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_2,    "Threshold[2]",     48, 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_3,    "Threshold[3]",     57, 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_4,    "Threshold[4]",     66, 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_5,    "Threshold[5]",     75, 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_6,    "Threshold[6]",     84, 9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_7,    "Threshold[7]",     93, 9, (1<<9)/2 },
  { MPX3RX_DAC_PREAMP,      "Preamp",          102, 8, (1<<8)/2 },
  { MPX3RX_DAC_IKRUM,       "Ikrum",           110, 8, (1<<8)/2 },
  { MPX3RX_DAC_SHAPER,      "Shaper",          118, 8, (1<<8)/2 },
  { MPX3RX_DAC_DISC,        "Disc",            126, 8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_LS,     "Disc_LS",         134, 8, (1<<8)/2 },
  { MPX3RX_DAC_SHAPER_TEST, "Shaper_Test",     142, 8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_L,      "DAC_DiscL",       150, 8, (1<<8)/2 },
  { MPX3RX_DAC_TEST,        "DAC_test",        158, 8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_H,      "DAC_DiscH",       166, 8, (1<<8)/2 },
  { MPX3RX_DAC_DELAY,       "Delay",           174, 8, (1<<8)/2 },
  { MPX3RX_DAC_TP_BUF_IN,   "TP_BufferIn",     182, 8, (1<<8)/2 },
  { MPX3RX_DAC_TP_BUF_OUT,  "TP_BufferOut",    190, 8, (1<<8)/2 },
  { MPX3RX_DAC_RPZ,         "RPZ",             198, 8, (1<<8)/2 },
  { MPX3RX_DAC_GND,         "GND",             206, 8, (1<<8)/2 },
  { MPX3RX_DAC_TP_REF,      "TP_REF",          214, 8, (1<<8)/2 },
  { MPX3RX_DAC_FBK,         "FBK",             222, 8, (1<<8)/2 },
  { MPX3RX_DAC_CAS,         "Cas",             230, 8, (1<<8)/2 },
  { MPX3RX_DAC_TP_REF_A,    "TP_REFA",         238, 9, (1<<9)/2 },
  { MPX3RX_DAC_TP_REF_B,    "TP_REFB",         247, 9, (1<<9)/2 }
};

// ----------------------------------------------------------------------------
#endif // DACSDESCR_H_
