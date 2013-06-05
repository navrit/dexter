#ifndef SPIDRCMDS_H
#define SPIDRCMDS_H

// Command identifiers in messages to (and from) the SPIDR module

// General: module
#define CMD_GET_SOFTWVERSION 0x901
#define CMD_GET_FIRMWVERSION 0x902
#define CMD_GET_IPADDR_DEST  0x903
#define CMD_SET_IPADDR_DEST  0x904
#define CMD_GET_UDPPACKET_SZ 0x905
#define CMD_SET_UDPPACKET_SZ 0x906
#define CMD_RESET_MODULE     0x907
#define CMD_SET_BUSY         0x908
#define CMD_CLEAR_BUSY       0x909
#define CMD_SET_LOGLEVEL     0x90A

// Configuration: devices
#define CMD_GET_DEVICEID     0x110
#define CMD_GET_DEVICEIDS    0x111
#define CMD_GET_DEVICETYPE   0x112
#define CMD_SET_DEVICETYPE   0x113
#define CMD_GET_DEVICEPORT   0x114
#define CMD_GET_DEVICEPORTS  0x115
#define CMD_SET_DEVICEPORT   0x116
#define CMD_GET_SERVERPORT   0x117
#define CMD_GET_SERVERPORTS  0x118
#define CMD_SET_SERVERPORT   0x119
#define CMD_GET_DAC          0x11A
#define CMD_SET_DAC          0x11B
#define CMD_SET_DACS         0x11C
#define CMD_READ_DACS        0x11D
#define CMD_WRITE_DACS       0x11E
#define CMD_WRITE_DACS_DFLT  0x11F
#define CMD_SET_CTPR         0x120
#define CMD_WRITE_CTPR       0x121
#define CMD_GET_ACQENABLE    0x122
#define CMD_SET_ACQENABLE    0x123
#define CMD_RESET_DEVICE     0x124
#define CMD_RESET_DEVICES    0x125
#define CMD_SET_READY        0x126

// Configuration: pixels
// Medipix3.1
#define CMD_PIXCONF_MPX3_0   0x22A
#define CMD_PIXCONF_MPX3_1   0x22B
// Medipix3RX
#define CMD_PIXCONF_MPX3RX   0x22C

// Configuration: OMR
#define CMD_SET_CRW          0x330
#define CMD_SET_POLARITY     0x331
#define CMD_SET_DISCCSMSPM   0x332
#define CMD_SET_INTERNALTP   0x333
#define CMD_SET_COUNTERDEPTH 0x334
#define CMD_SET_EQTHRESHH    0x335
#define CMD_SET_COLOURMODE   0x336
#define CMD_SET_CSMSPM       0x337
#define CMD_SET_SENSEDAC     0x338
#define CMD_SET_SENSEDACCODE 0x339
#define CMD_SET_EXTDAC       0x33A
#define CMD_WRITE_OMR        0x33B

// Trigger
#define CMD_GET_TRIGCONFIG   0x440
#define CMD_SET_TRIGCONFIG   0x441
#define CMD_AUTOTRIG_START   0x442
#define CMD_AUTOTRIG_STOP    0x443
#define CMD_TRIGGER_READOUT  0x444

// Monitoring
#define CMD_GET_ADC          0x548
#define CMD_GET_REMOTETEMP   0x549
#define CMD_GET_LOCALTEMP    0x54A
#define CMD_GET_AVDD         0x54B
#define CMD_GET_DVDD         0x54C
#define CMD_GET_VDD          0x54D

// Short strings describing the commands
// (indexed by the lower byte of the command identifier)
static char *CMD_STR[] =
  {
    "-----",            // 0x900
    "GET_SOFTWVERSION", // 0x901
    "GET_FIRMWVERSION", // 0x902
    "GET_IPADDR_DEST ", // 0x903
    "SET_IPADDR_DEST ", // 0x904
    "GET_UDPPACKET_SZ", // 0x905
    "SET_UDPPACKET_SZ", // 0x906
    "RESET_MODULE    ", // 0x907
    "SET_BUSY        ", // 0x908
    "CLEAR_BUSY      ", // 0x909
    "SET_LOGLEVEL    ", // 0x90A
    "-----",            // 0x90B
    "-----",            // 0x90C
    "-----",            // 0x90D
    "-----",            // 0x90E
    "-----",            // 0x90F

    "GET_DEVICEID    ", // 0x110
    "GET_DEVICEIDS   ", // 0x111
    "GET_DEVICETYPE  ", // 0x112
    "SET_DEVICETYPE  ", // 0x113
    "GET_DEVICEPORT  ", // 0x114
    "GET_DEVICEPORTS ", // 0x115
    "SET_DEVICEPORT  ", // 0x116
    "GET_SERVERPORT  ", // 0x117
    "GET_SERVERPORTS ", // 0x118
    "SET_SERVERPORT  ", // 0x119
    "GET_DAC         ", // 0x11A
    "SET_DAC         ", // 0x11B
    "SET_DACS        ", // 0x11C
    "READ_DACS       ", // 0x11D
    "WRITE_DACS      ", // 0x11E
    "WRITE_DACS_DFLT ", // 0x11F
    "SET_CTPR        ", // 0x120
    "WRITE_CTPR      ", // 0x121
    "GET_ACQENABLE   ", // 0x122
    "SET_ACQENABLE   ", // 0x123
    "RESET_DEVICE    ", // 0x124
    "RESET_DEVICES   ", // 0x125
    "SET_READY       ", // 0x126
    "-----",            // 0x127
    "-----",            // 0x128
    "-----",            // 0x129

    "PIXCONF_MPX3_0  ", // 0x22A
    "PIXCONF_MPX3_1  ", // 0x22B
    "PIXCONF_MPX3RX  ", // 0x22C
    "-----",            // 0x22D
    "-----",            // 0x22E
    "-----",            // 0x22F

    "SET_CRW         ", // 0x330
    "SET_POLARITY    ", // 0x331
    "SET_DISCCSMSPM  ", // 0x332
    "SET_INTERNALTP  ", // 0x333
    "SET_COUNTERDEPTH", // 0x334
    "SET_EQTHRESHH   ", // 0x335
    "SET_COLOURMODE  ", // 0x336
    "SET_CSMSPM      ", // 0x337
    "SET_SENSEDAC    ", // 0x338
    "SET_SENSEDACCODE", // 0x339
    "SET_EXTDAC      ", // 0x33A
    "WRITE_OMR       ", // 0x33B
    "-----",            // 0x33C
    "-----",            // 0x33D
    "-----",            // 0x33E
    "-----",            // 0x33F

    "GET_TRIGCONFIG  ", // 0x440
    "SET_TRIGCONFIG  ", // 0x441
    "AUTOTRIG_START  ", // 0x442
    "AUTOTRIG_STOP   ", // 0x443
    "TRIGGER_READOUT ", // 0x444
    "-----",            // 0x445
    "-----",            // 0x446
    "-----",            // 0x447

    "GET_ADC         ", // 0x548
    "GET_REMOTETEMP  ", // 0x549
    "GET_LOCALTEMP   ", // 0x54A
    "GET_AVDD        ", // 0x54B
    "GET_DVDD        ", // 0x54C
    "GET_VDD         "  // 0x54E
  };

// Reply bit: set in the reply message in the command identifier
#define CMD_REPLY            0x00010000

// No-reply bit: set in the command message in the command identifier
// indicating to the SPIDR server that no reply is expected
// (to speed up certain operations, such as pixel configuration uploads)
#define CMD_NOREPLY          0x00080000

#define CMD_MASK             0x0000FFFF

// Error identifiers in replies from the SPIDR module
#define ERR_UNKNOWN_CMD      0x80000001
#define ERR_HARDWARE         0x80000002
#define ERR_MSG_LENGTH       0x80000003
#define ERR_SEQUENCE         0x80000004
#define ERR_ILLEGAL_PAR      0x80000005

#endif // SPIDRCMDS_H
