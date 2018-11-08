/* ----------------------------------------------------------------------------
File   : mpx3defs.h

Descr  : Definitions for the Medipix3.1 and Medipix3RX devices.

History:
21JUL2013; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef MPX3DEFS_H_
#define MPX3DEFS_H_

#define MPX_PIXELS             (256*256)
#define MPX_PIXEL_ROWS          256
#define MPX_PIXEL_COLUMNS       256

// ----------------------------------------------------------------------------

// MPX3-RX pixel configuration bits are all in Counter1: B0 to B11
#define MPX3RX_CFG_MASKBIT      0x001
#define MPX3RX_CFG_CFGDISC_L    0x03E
#define MPX3RX_CFG_CFGDISC_H    0x7C0
#define MPX3RX_CFG_TESTBIT      0x800

// ----------------------------------------------------------------------------
// Medipix3 DACs

#define MPX3RX_DAC_COUNT        27

/* Get the DAC index with the following only, the 1 offset is taken care of there... */
/* int dacIndex = _mpx3gui->getDACs()->GetDACIndex( dac_code ); */

// Medipix3RX DAC codes
#define MPX3RX_DAC_THRESH_0     1
#define MPX3RX_DAC_THRESH_1     2
#define MPX3RX_DAC_THRESH_2     3
#define MPX3RX_DAC_THRESH_3     4
#define MPX3RX_DAC_THRESH_4     5
#define MPX3RX_DAC_THRESH_5     6
#define MPX3RX_DAC_THRESH_6     7
#define MPX3RX_DAC_THRESH_7     8
#define MPX3RX_DAC_PREAMP       9
#define MPX3RX_DAC_IKRUM        10
#define MPX3RX_DAC_SHAPER       11
#define MPX3RX_DAC_DISC         12
#define MPX3RX_DAC_DISC_LS      13
#define MPX3RX_DAC_SHAPER_TEST  14
#define MPX3RX_DAC_DISC_L       15
#define MPX3RX_DAC_TEST         30
#define MPX3RX_DAC_DISC_H       31
#define MPX3RX_DAC_DELAY        16
#define MPX3RX_DAC_TP_BUF_IN    17
#define MPX3RX_DAC_TP_BUF_OUT   18
#define MPX3RX_DAC_RPZ          19
#define MPX3RX_DAC_GND          20
#define MPX3RX_DAC_TP_REF       21
#define MPX3RX_DAC_FBK          22
#define MPX3RX_DAC_CAS          23
#define MPX3RX_DAC_TP_REF_A     24
#define MPX3RX_DAC_TP_REF_B     25

// ----------------------------------------------------------------------------
#endif // MPX3DEFS_H_
