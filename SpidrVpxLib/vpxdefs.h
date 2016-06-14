/* ----------------------------------------------------------------------------
File   : vpxdefs.h

Descr  : Definitions for Velopix devices.

History:
29MAR2016; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef VPXDEFS_H_
#define VPXDEFS_H_

// ----------------------------------------------------------------------------
// Velopix DACs

// Number of DACs
#define VPX_DAC_COUNT                  14

// Velopix DAC codes
#define VPX_DAC_IPREAMP                1
#define VPX_DAC_IKRUM                  2
#define VPX_DAC_IDISC                  3
#define VPX_DAC_IPIXELDAC              4
#define VPX_DAC_VTPCOARSE              5
#define VPX_DAC_VTPFINE                6
#define VPX_DAC_VPREAMP_CAS            7
#define VPX_DAC_VFBK                   8
#define VPX_DAC_VTHR                   9
#define VPX_DAC_VCAS_DISC              10
#define VPX_DAC_VIN_CAS                11
#define VPX_DAC_VREFSLVS               12
#define VPX_DAC_IBIASSLVS              13
#define VPX_DAC_RES_BIAS               14

// ----------------------------------------------------------------------------
// Velopix register definitions

// Register addresses
#define REG_SUPERPIXEL_CONFIG          0x0000
#define REG_PIXEL_CONFIG               0x0100
#define REG_BUNCHCOUNTER_CONFIG        0x0200
#define REG_EOCBLOCKGLOBAL_CONFIG      0x0201
#define REG_PIXELMATRIXRESET_CONFIG    0x0202

// Register items
#define SUPERPIXEL_MASK_BIT            0
#define SUPERPIXEL_TOT_THRESHOLD       1
#define PIXEL_MASK_BIT                 10
#define PIXEL_THR_BIT                  11
#define PIXEL_TEST_BIT                 12
#define BUNCHCOUNTER_PRESET_VAL        20
#define BUNCHCOUNTER_OVERFLOW_CONTROL  21
#define BUNCHCOUNTER_ENABLE            22
#define BUNCHCOUNTER_GRAY_OR_BIN       23

// Structure describing a Velopix register
typedef struct vpxreg_s
{
  int addr;
  int nbytes;
  int count;
} vpxreg_t;

// Structure describing a Velopix register item
typedef struct vpxreg_item_s
{
  int item;
  int addr;
  int bitindex;
  int nbits;
} vpxreg_item_t;

const vpxreg_t VPX_REG[] = {
  { REG_SUPERPIXEL_CONFIG,       256/8,  128 },
  { REG_PIXEL_CONFIG,            1536/8, 256 },
  { REG_BUNCHCOUNTER_CONFIG,     2,      1   },
  { REG_EOCBLOCKGLOBAL_CONFIG,   2,      1   },
  { REG_PIXELMATRIXRESET_CONFIG, 2,      1   }
};

const vpxreg_item_t VPX_REG_ITEM[] = {
  { SUPERPIXEL_MASK_BIT,           REG_SUPERPIXEL_CONFIG,     0,  1 },
  { SUPERPIXEL_TOT_THRESHOLD,      REG_SUPERPIXEL_CONFIG,     1,  2 },
  { PIXEL_MASK_BIT,                REG_PIXEL_CONFIG,          0,  1 },
  { PIXEL_THR_BIT,                 REG_PIXEL_CONFIG,          1,  4 },
  { PIXEL_TEST_BIT,                REG_PIXEL_CONFIG,          5,  1 },
  { BUNCHCOUNTER_PRESET_VAL,       REG_BUNCHCOUNTER_CONFIG,   0, 12 },
  { BUNCHCOUNTER_OVERFLOW_CONTROL, REG_BUNCHCOUNTER_CONFIG,  12,  1 },
  { BUNCHCOUNTER_ENABLE,           REG_BUNCHCOUNTER_CONFIG,  13,  1 },
  { BUNCHCOUNTER_GRAY_OR_BIN,      REG_BUNCHCOUNTER_CONFIG,  14,  1 }
};

// ----------------------------------------------------------------------------
#endif // VPXDEFS_H_
