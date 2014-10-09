/* ----------------------------------------------------------------------------
File   : dacsdescr.h

Descr  : Lists of DAC descriptions for various types of Medipix devices.

History:
01FEB2013; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef DACSDESCR_H_
#define DACSDESCR_H_

typedef struct dac_s
{
  int         code;
  const char *name;
  int         offset;
  int         size;
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
  {  1, "Threshold[0]",     46, 9, 150 },
  {  2, "Threshold[1]",     55, 9, 150 },
  {  3, "Threshold[2]",     64, 9, (1<<9)/2 },
  {  4, "Threshold[3]",     73, 9, (1<<9)/2 },
  {  5, "Threshold[4]",     82, 9, (1<<9)/2 },
  {  6, "Threshold[5]",     91, 9, (1<<9)/2 },
  {  7, "Threshold[6]",    100, 9, (1<<9)/2 },
  {  8, "Threshold[7]",    109, 9, (1<<9)/2 },
  {  9, "Preamp",          118, 8, 120 },
  { 10, "Ikrum",           126, 8, 20 },
  { 11, "Shaper",          134, 8, 173 },
  { 12, "Disc",            142, 8, 255 },
  { 13, "Disc_LS",         150, 8, 170 },
  { 14, "ThresholdN",      158, 8, 20 },
  { 15, "DAC_pixel",       166, 8, 0x76 },
  { 16, "Delay",           174, 8, (1<<8)/2 },
  { 17, "TP_BufferIn",     182, 8, (1<<8)/2 },
  { 18, "TP_BufferOut",    190, 8, 0x32 },
  { 19, "RPZ",             198, 8, (1<<8)-1 },
  { 20, "GND",             206, 8, 0x6E },
  { 21, "TP_REF",          214, 8, (1<<8)/2 },
  { 22, "FBK",             222, 8, 0x8F },
  { 23, "Cas",             230, 8, 191 },
  { 24, "TP_REFA",         238, 9, (1<<9)-1 },
  { 25, "TP_REFB",         247, 9, (1<<9)-1 }
};

static const dac_t MPX3RX_DAC_TABLE[MPX3RX_DAC_COUNT] =
{
  {  1, "Threshold[0]",     30, 9, (1<<9)/2 },
  {  2, "Threshold[1]",     39, 9, (1<<9)/2 },
  {  3, "Threshold[2]",     48, 9, (1<<9)/2 },
  {  4, "Threshold[3]",     57, 9, (1<<9)/2 },
  {  5, "Threshold[4]",     66, 9, (1<<9)/2 },
  {  6, "Threshold[5]",     75, 9, (1<<9)/2 },
  {  7, "Threshold[6]",     84, 9, (1<<9)/2 },
  {  8, "Threshold[7]",     93, 9, (1<<9)/2 },
  {  9, "Preamp",          102, 8, (1<<8)/2 },
  { 10, "Ikrum",           110, 8, (1<<8)/2 },
  { 11, "Shaper",          118, 8, (1<<8)/2 },
  { 12, "Disc",            126, 8, (1<<8)/2 },
  { 13, "Disc_LS",         134, 8, (1<<8)/2 },
  { 14, "Shaper_Test",     142, 8, (1<<8)/2 },
  { 15, "DAC_DiscL",       150, 8, (1<<8)/2 },
  { 30, "DAC_test",        158, 8, (1<<8)/2 },
  { 31, "DAC_DiscH",       166, 8, (1<<8)/2 },
  { 16, "Delay",           174, 8, (1<<8)/2 },
  { 17, "TP_BufferIn",     182, 8, (1<<8)/2 },
  { 18, "TP_BufferOut",    190, 8, (1<<8)/2 },
  { 19, "RPZ",             198, 8, (1<<8)/2 },
  { 20, "GND",             206, 8, (1<<8)/2 },
  { 21, "TP_REF",          214, 8, (1<<8)/2 },
  { 22, "FBK",             222, 8, (1<<8)/2 },
  { 23, "Cas",             230, 8, (1<<8)/2 },
  { 24, "TP_REFA",         238, 9, (1<<9)/2 },
  { 25, "TP_REFB",         247, 9, (1<<9)/2 }
};

// ----------------------------------------------------------------------------
#endif // DACSDESCR_H_
