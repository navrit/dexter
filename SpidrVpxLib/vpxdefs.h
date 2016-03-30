/* ----------------------------------------------------------------------------
File   : vpxdefs.h

Descr  : Definitions for Velopix devices.

History:
29MAR2016; HenkB; Created.
---------------------------------------------------------------------------- */
#ifndef VPXDEFS_H_
#define VPXDEFS_H_

// ----------------------------------------------------------------------------
// Velopix register definitions

// ----------------------------------------------------------------------------
// Velopix DACs

// Number of DACs
#define VPX_DAC_COUNT               14

// Velopix DAC codes
#define VPX_DAC_IPREAMP             1
#define VPX_DAC_IKRUM               2
#define VPX_DAC_IDISC               3
#define VPX_DAC_IPIXELDAC           4
#define VPX_DAC_VTPCOARSE           5
#define VPX_DAC_VTPFINE             6
#define VPX_DAC_VPREAMP_CAS         7
#define VPX_DAC_VFBK                8
#define VPX_DAC_VTHR                9
#define VPX_DAC_VCAS_DISC           10
#define VPX_DAC_VIN_CAS             11
#define VPX_DAC_VREFSLVS            12
#define VPX_DAC_IBIASSLVS           13
#define VPX_DAC_RES_BIAS            14

// ----------------------------------------------------------------------------
#endif // VPXDEFS_H_
