/* ----------------------------------------------------------------------------
File   : mpx3conf.h

Descr  : Definitions for various Medipix devices concerning pixel configuration
         and more.

History:
04FEB2013; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef MPX3CONF_H_
#define MPX3CONF_H_

// ----------------------------------------------------------------------------
// General

// Medipix device types (from mpx3.h in LEON3 project):
//   0: NC ('Not Connected')
//   1: Medipix3.1
//   2: Medipix3RX
#define MPX_TYPE_NC            0
#define MPX_TYPE_MPX31         1
#define MPX_TYPE_MPX3RX        2

#define MPX_PIXELS            (256*256)
#define MPX_PIXEL_ROWS         256
#define MPX_PIXEL_COLUMNS      256

// ----------------------------------------------------------------------------
// Pixel configuration (from mpx3.c in LEON3 project)

// MPX3 pixel configuration bits in Counter0: B0 to B11
#define MPX3_CFG_CONFIGTHB_4   (1<<4)
#define MPX3_CFG_CONFIGTHA_1   (1<<5)
#define MPX3_CFG_CONFIGTHA_2   (1<<6)
#define MPX3_CFG_CONFIGTHA_4   (1<<7)
#define MPX3_CFG_GAINMODE      (1<<8)

// MPX3 pixel configuration bits in Counter1: B12 to B23
#define MPX3_CFG_TESTBIT       (1<<15)
#define MPX3_CFG_CONFIGTHB_3   (1<<16)
#define MPX3_CFG_CONFIGTHA_0   (1<<17)
#define MPX3_CFG_CONFIGTHB_0   (1<<18)
#define MPX3_CFG_CONFIGTHA_3   (1<<19)
#define MPX3_CFG_CONFIGTHB_2   (1<<20)
#define MPX3_CFG_MASKBIT       (1<<21)
#define MPX3_CFG_CONFIGTHB_1   (1<<22)

// MPX3-RX pixel configuration bits are all in Counter1: B0 to B11
#define MPX3RX_CFG_MASKBIT     0x001
#define MPX3RX_CFG_CFGDISC_L   0x03E
#define MPX3RX_CFG_CFGDISC_H   0x7C0
#define MPX3RX_CFG_TESTBIT     0x800

// ----------------------------------------------------------------------------
// Other

// SPIDR Medipix trigger modes (taken from spidr.h in LEON3 project)
#define SPIDR_TRIG_POS_EXT         0x000
#define SPIDR_TRIG_NEG_EXT         0x001
#define SPIDR_TRIG_POS_EXT_TIMER   0x002
#define SPIDR_TRIG_NEG_EXT_TIMER   0x003
#define SPIDR_TRIG_AUTO            0x004
#define SPIDR_TRIG_POS_EXT_CNTR    0x005

// ----------------------------------------------------------------------------
#endif // MPX3CONF_H_
