// Definitions for the SPIDR-TPX3 data headers

#ifndef SPIDRTPX3DATA_H
#define SPIDRTPX3DATA_H

// Format versioning
// -----------------
// Common header
#define SPIDR_HEADER_VERSION      0x00000001
// Device header
#define TPX3_HEADER_VERSION       0x00000001
// Chip family type identifiers for device header
#define TPX_FAMILY_TYPE           0x00000000
#define MPX2_FAMILY_TYPE          0x01000000
#define MPX3_FAMILY_TYPE          0x02000000
#define TPX3_FAMILY_TYPE          0x03000000

typedef struct Tpx3Header
{
  u32 headerId;
  u32 headerSizeTotal;
  u32 headerSize;
  u32 format;
  u32 deviceId;
  u32 genConfig;
  u32 outblockConfig;
  u32 pllConfig;
  u32 testPulseConfig;
  u32 slvsConfig;
  u32 pwrPulseConfig;
  u32 dac[32];
  u8  ctpr[256/8];
  u32 unused[64-11-32-(256/8)/4];
} Tpx3Header_t;

typedef struct SpidrTpx3Header
{
  u32 headerId;
  u32 headerSizeTotal;
  u32 headerSize;
  u32 format;
  u32 spidrId;
  u32 libVersion;
  u32 softwVersion;
  u32 firmwVersion;
  u32 ipAddress;
  u32 ipPort;
  u32 yyyyMmDd;
  u32 hhMmSsMs;
  u32 runNr;
  u32 seqNr;
  u32 spidrConfig; // Trigger mode, decoder on/off
  u32 spidrFilter;
  u32 spidrBiasVoltage;
  // Spare
  u32 unused[128-17-128/4];
  // Description string
  char descr[128];
  // Device header
  Tpx3Header_t devHeader;
} SpidrTpx3Header_t;

#define SPIDRTPX3_HEADER_SIZE  sizeof(SpidrTpx3Header_t)
#define TPX3_HEADER_SIZE       sizeof(Tpx3Header_t)

#define SPIDR_HEADER_ID        0x52445053 // Represents "SPDR"
#define TPX3_HEADER_ID         0x33585054 // Represents "TPX3"
#define HEADER_FILLER          0xDD       // For unused space in headers

#endif // SPIDRTPX3DATA_H
