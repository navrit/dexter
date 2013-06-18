// Definitions concerning event and device headers

#ifndef SPIDRDATA_H
#define SPIDRDATA_H

// Format versioning
#define EVT_HEADER_VERSION  0x00000001
#define DEV_HEADER_VERSION  0x00000001
#define DEV_DATA_DECODED    0x10000000
#define DEV_DATA_COMPRESSED 0x20000000

typedef struct EvtHeader
{
  u32 headerId;
  u32 format;
  u32 headerSize;
  u32 dataSize;     // Including device headers but not this header
  u32 ipAddress;
  u32 nrOfDevices;
  u32 ports[4];
  u32 evtNr;
  u32 secs;         // Date-time of frame arrival (complete)
  u32 msecs;
  u32 pixelDepth;
  u32 triggerConfig[5];
  u32 unused[32-19];
} EvtHeader_t;

typedef struct DevHeader
{
  u32 headerId;
  u32 format;
  u32 headerSize;
  u32 dataSize;       // Not including this header
  u32 deviceId;
  u32 deviceType;
  u32 spidrHeader[3]; // Trigger/shutter counter, sequence number, time stamp
  u32 lostPackets;
  u32 unused[16-10];
} DevHeader_t;

typedef struct SpidrHeader
{
  // NB: copy of header produced by the SPIDR module; values are big-endian
  u16 triggerCnt;
  u16 shutterCnt;
  u16 sequenceNr;
  u16 timeLo;
  u16 timeMi;
  u16 timeHi;
} SpidrHeader_t;

#define EVT_HEADER_SIZE      sizeof(EvtHeader_t)
#define DEV_HEADER_SIZE      sizeof(DevHeader_t)
#define SPIDR_HEADER_SIZE    sizeof(SpidrHeader_t)

#define MPX3_12BIT_RAW_SIZE  ((256 * 256 * 12) / 8)
#define MPX3_24BIT_RAW_SIZE  ((256 * 256 * 24) / 8)
#define MPX3_MAX_FRAME_SIZE  MPX3_24BIT_RAW_SIZE

#define EVT_HEADER_ID        0x52445053 // Represents "SPDR"
#define DEV_HEADER_ID        0x3358504D // Represents "MPX3"

#endif // SPIDRDATA_H
