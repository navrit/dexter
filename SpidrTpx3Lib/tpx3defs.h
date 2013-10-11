/* ----------------------------------------------------------------------------
File   : tpx3defs.h

Descr  : Definitions for Timepix3 devices.

History:
21JUL2013; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef TPX3DEFS_H_
#define TPX3DEFS_H_

// ----------------------------------------------------------------------------
// Timepix3 header definitions
// (located in upper byte of 48- or 64-bit data packet)

#define TPX3_HDR_ANALOGPERIPHERY    (0x0 << 4)
#define TPX3_HDR_OUTBLOCKCONFIG     (0x1 << 4)
#define TPX3_HDR_PLLCONFIG          (0x2 << 4)
#define TPX3_HDR_GENERALCONFIG      (0x3 << 4)
#define TPX3_HDR_TIMER              (0x4 << 4)
#define TPX3_HDR_CONTROLOP          (0x7 << 4)
#define TPX3_HDR_LOADCONFIGMATRIX   (0x8 << 4)
#define TPX3_HDR_READCONFIGMATRIX   (0x9 << 4)
#define TPX3_HDR_READMATRIXSEQ      (0xA << 4)
#define TPX3_HDR_READMATRIXDATADRIV (0xB << 4)
#define TPX3_HDR_LOADCTPR           (0xC << 4)
#define TPX3_HDR_READCTPR           (0xD << 4)
#define TPX3_HDR_RESETSEQ           (0xE << 4)
#define TPX3_HDR_STOPMATRIXREADOUT  (0xF << 4)

// Analog periphery
#define TPX3_HDR_SENSEDACSEL        (TPX3_HDR_ANALOGPERIPHERY | 0x0)
#define TPX3_HDR_EXTDACSEL          (TPX3_HDR_ANALOGPERIPHERY | 0x1)
#define TPX3_HDR_SETDAC             (TPX3_HDR_ANALOGPERIPHERY | 0x2)
#define TPX3_HDR_READDAC            (TPX3_HDR_ANALOGPERIPHERY | 0x3)
#define TPX3_HDR_EFUSEREAD          (TPX3_HDR_ANALOGPERIPHERY | 0x9)
#define TPX3_HDR_TP_PERIOD_PHASE    (TPX3_HDR_ANALOGPERIPHERY | 0xC)
#define TPX3_HDR_TP_PULSENUMBER     (TPX3_HDR_ANALOGPERIPHERY | 0xD)
#define TPX3_HDR_TP_CONFIG_READ     (TPX3_HDR_ANALOGPERIPHERY | 0xE)
#define TPX3_HDR_TP_INTERN_FINISHED (TPX3_HDR_ANALOGPERIPHERY | 0xF)

// Output block configuration
#define TPX3_HDR_OUTBLCONFIG_WRITE  (TPX3_HDR_OUTBLOCKCONFIG  | 0x0)
#define TPX3_HDR_OUTBLCONFIG_READ   (TPX3_HDR_OUTBLOCKCONFIG  | 0x1)

// PLL configuration
#define TPX3_HDR_PLLCONFIG_WRITE    (TPX3_HDR_PLLCONFIG       | 0x0)
#define TPX3_HDR_PLLCONFIG_READ     (TPX3_HDR_PLLCONFIG       | 0x1)

// General configuration
#define TPX3_HDR_GENCONFIG_WRITE    (TPX3_HDR_GENERALCONFIG   | 0x0)
#define TPX3_HDR_GENCONFIG_READ     (TPX3_HDR_GENERALCONFIG   | 0x1)
#define TPX3_HDR_SLVSCONFIG_WRITE   (TPX3_HDR_GENERALCONFIG   | 0x4)
#define TPX3_HDR_SLVSCONFIG_READ    (TPX3_HDR_GENERALCONFIG   | 0x5)
// ....more

// Timer
#define TPX3_HDR_RESETTIMER         (TPX3_HDR_TIMER           | 0x0)
#define TPX3_HDR_SETTIMER_15_0      (TPX3_HDR_TIMER           | 0x1)
#define TPX3_HDR_SETTIMER_31_16     (TPX3_HDR_TIMER           | 0x2)
#define TPX3_HDR_SETTIMER_47_32     (TPX3_HDR_TIMER           | 0x3)
#define TPX3_HDR_TIMERVAL_LO        (TPX3_HDR_TIMER           | 0x4)
#define TPX3_HDR_TIMERVAL_HI        (TPX3_HDR_TIMER           | 0x5)
#define TPX3_HDR_SHUTTERSTART_LO    (TPX3_HDR_TIMER           | 0x6)
#define TPX3_HDR_SHUTTERSTART_HI    (TPX3_HDR_TIMER           | 0x7)
#define TPX3_HDR_SHUTTEREND_LO      (TPX3_HDR_TIMER           | 0x8)
#define TPX3_HDR_SHUTTEREND_HI      (TPX3_HDR_TIMER           | 0x9)
#define TPX3_HDR_T0SYNC_CMD         (TPX3_HDR_TIMER           | 0xA)

// Control operation
#define TPX3_HDR_ACKNOWLEDGE        (TPX3_HDR_CONTROLOP       | 0x0)
#define TPX3_HDR_ENDOFCOMMAND       (TPX3_HDR_CONTROLOP       | 0x1)
#define TPX3_HDR_OTHERCHIP_CMD      (TPX3_HDR_CONTROLOP       | 0x2)
#define TPX3_HDR_WRONGCOMMAND       (TPX3_HDR_CONTROLOP       | 0x3)

// ----------------------------------------------------------------------------
// Timepix3 pixel configuration definitions
// (Note that the 'threshold' (4 bits) is stored inverted)

#define TPX3_PIXCFG_BITS            6
#define TPX3_PIXCFG_MASKBIT         (1 << 0)
#define TPX3_PIXCFG_THRESHBIT3      (1 << 1)
#define TPX3_PIXCFG_THRESHBIT2      (1 << 2)
#define TPX3_PIXCFG_THRESHBIT1      (1 << 3)
#define TPX3_PIXCFG_THRESHBIT0      (1 << 4)
#define TPX3_PIXCFG_TESTBIT         (1 << 5)

// ----------------------------------------------------------------------------
// Timepix3 pixel address (16 bits), expressed in x,y
// with x and y in the range [0,255]:
// x = doublecol*2 + (pix/4)
// y = superpix*4 + (pix & 0x3)

#define TPX3_ADDR_DOUBLECOL_MASK    0xFE00
#define TPX3_ADDR_SUPERPIX_MASK     0x01F8
#define TPX3_ADDR_PIX_MASK          0x0007

// ----------------------------------------------------------------------------
// Timepix3 DACs

// Number of DACs
#define TPX3_DAC_COUNT              23
#define TPX3_DAC_COUNT_TO_SENSE     17

// Timepix3 DAC codes
#define TPX3_IBIAS_PREAMP_ON        1
#define TPX3_IBIAS_PREAMP_OFF       2
#define TPX3_VPREAMP_NCAS           3
#define TPX3_IBIAS_IKRUM            4
#define TPX3_VFBK                   5
#define TPX3_VTHRESH_FINE           6
#define TPX3_VTHRESH_COARSE         7
#define TPX3_IBIAS_DISCS1_ON        8
#define TPX3_IBIAS_DISCS1_OFF       9
#define TPX3_IBIAS_DISCS2_ON       10
#define TPX3_IBIAS_DISCS2_OFF      11
#define TPX3_IBIAS_PIXELDAC        12
#define TPX3_IBIAS_TPBUFIN         13
#define TPX3_IBIAS_TPBUFOUT        14
#define TPX3_VTP_COARSE            15
#define TPX3_VTP_FINE              16
#define TPX3_IBIAS_CP_PLL          17

#define TPX3_PLL_VCNTRL            18
#define TPX3_BANDGAP_OUTPUT        28
#define TPX3_BANDGAP_TEMP          29
#define TPX3_IBIAS_DAC             30
#define TPX3_IBIAS_DAC_CAS         31

#define TPX3_SENSEOFF              0

// ----------------------------------------------------------------------------

// Timepix3 General Configuration register
#define TPX3_POLARITY_EMIN         0x001
#define TPX3_POLARITY_HPLUS        0x000
#define TPX3_ACQMODE_TOA_TOT       0x000
#define TPX3_ACQMODE_TOA           0x002
#define TPX3_ACQMODE_EVT_ITOT      0x004
#define TPX3_GRAYCOUNT_ENA         0x008
#define TPX3_ACKCOMMAND_ENA        0x010
#define TPX3_TESTPULSE_ENA         0x020
#define TPX3_FASTLO_ENA            0x040
#define TPX3_TIMER_OVERFL_CTRL     0x080
#define TPX3_SELECTTP_DIG_ANALOG   0x100
#define TPX3_SELECTTP_EXT_INT      0x200
#define TPX3_SELECT_TOA_CLK        0x400

// Timepix3 Output Block Configuration register
#define TPX3_OUTPORT_MASK          0x00FF
#define TPX3_CLK_SRC_160_320       0x0100
#define TPX3_CLK_SRC_80_160        0x0200
#define TPX3_CLK_SRC_40_80         0x0300
#define TPX3_CLK_SRC_20_40         0x0400
#define TPX3_CLK_SRC_EXT           0x0500
#define TPX3_8B10B_ENA             0x0800
#define TPX3_CLK_FAST_OUT_ENA      0x1000
#define TPX3_CLKOUT_SRC_160_320    0x2000
#define TPX3_CLKOUT_SRC_80_160     0x4000
#define TPX3_CLKOUT_SRC_40_80      0x6000
#define TPX3_CLKOUT_SRC_20_40      0x8000
#define TPX3_CLKOUT_SRC_EXT        0xA000

// Timepix3 PLL Configuration register
#define TPX3_PLL_BYPASSED          0x0001
#define TPX3_PLL_RUN               0x0002
#define TPX3_VCNTRL_PLL            0x0004
#define TPX3_DUALEDGE_CLK          0x0008
#define TPX3_PHASESHIFT_DIV_16     0x0000
#define TPX3_PHASESHIFT_DIV_8      0x0010
#define TPX3_PHASESHIFT_DIV_4      0x0020
#define TPX3_PHASESHIFT_DIV_2      0x0030
#define TPX3_PHASESHIFT_NR_1       0x0000
#define TPX3_PHASESHIFT_NR_2       0x0040
#define TPX3_PHASESHIFT_NR_4       0x0080
#define TPX3_PHASESHIFT_NR_8       0x00C0
#define TPX3_PHASESHIFT_NR_16      0x0100
#define TPX3_PLLOUT_CONFIG_MASK    0x3E00
#define TPX3_PLLOUT_CONFIG_SHIFT   9

// ----------------------------------------------------------------------------
#endif // TPX3DEFS_H_
