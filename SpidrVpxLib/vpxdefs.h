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
#define REG_SP                              0x0000
#define REG_PIXEL                           0x0100
#define REG_BX_ID                           0x0200
#define REG_EOCDP                           0x0201
#define REG_MATRIX_RST_CONF                 0x0202
//#define REG_EOCFAB                        0x0203
#define REG_ROUTER                          0x0204
#define REG_GENDIGCONF                      0x0205
#define REG_SLVS                            0x0206
#define REG_SHUTTER_ON                      0x0207
#define REG_SHUTTER_OFF                     0x0208
#define REG_BX_ID_MAX                       0x0209

// Register items
#define SP_TOT_THRESHOLD                    0
#define SP_MASK_BIT                         1

#define PIXEL_TEST_BIT                      10
#define PIXEL_MASK_BIT                      11
#define PIXEL_THR_BIT                       12

#define BXID_GRAY_OR_BIN                    20
#define BXID_ENABLE                         21
#define BXID_OVERFLOW_CONTROL               22
#define BXID_PRESET_VAL                     23

#define EOCDP_BX2COL_GRAY                   30
#define EOCDP_EN_PACKET_FILTER              31
#define EOCDP_BX_ID_EDGE                    32
#define EOCDP_SHIFT_PRIOR                   33

#define MATRIXRSTCONF_RST_MATRIX_ON_FE_RST  40
#define MATRIXRSTCONF_RESET_DURATION        41
#define MATRIXRSTCONF_NUM_OF_BANKS          42
#define MATRIXRSTCONF_COLS_PER_BANK         43

#define ROUTER_CHAN_ENABLE                  50

// ----------------------------------------------------------------------------
#endif // VPXDEFS_H_
