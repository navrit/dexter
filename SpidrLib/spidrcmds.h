#ifndef SPIDRCMDS_H
#define SPIDRCMDS_H

// Command identifiers in messages to the SPIDR module

// General: module
#define CMD_GET_SOFTWVERSION 0x1001
#define CMD_GET_FIRMWVERSION 0x1002
#define CMD_GET_IPADDR_DEST  0x1003
#define CMD_SET_IPADDR_DEST  0x1004
#define CMD_GET_UDPPACKET_SZ 0x1005
#define CMD_SET_UDPPACKET_SZ 0x1006
#define CMD_RESET_MODULE     0x1111
#define CMD_SET_BUSY         0x2222
#define CMD_CLEAR_BUSY       0x3333

// Configuration: devices
#define CMD_GET_DEVICEID     0x101
#define CMD_GET_DEVICEIDS    0x102
#define CMD_GET_DEVICETYPE   0x103
#define CMD_SET_DEVICETYPE   0x104
#define CMD_GET_DEVICEPORT   0x105
#define CMD_GET_DEVICEPORTS  0x106
#define CMD_SET_DEVICEPORT   0x107
#define CMD_GET_SERVERPORT   0x108
#define CMD_GET_SERVERPORTS  0x109
#define CMD_SET_SERVERPORT   0x110
#define CMD_GET_DAC          0x111
#define CMD_SET_DAC          0x112
#define CMD_SET_DACS         0x113
#define CMD_READ_DACS        0x114
#define CMD_WRITE_DACS       0x115
#define CMD_WRITE_DACS_DFLT  0x116
#define CMD_SET_CTPR         0x117
#define CMD_WRITE_CTPR       0x118
#define CMD_GET_ACQENABLE    0x119
#define CMD_SET_ACQENABLE    0x120
#define CMD_RESET_DEVICE     0x121
#define CMD_RESET_DEVICES    0x122
#define CMD_SET_READY        0x123

// Configuration: pixels
// Medipix3.1
#define CMD_PIXCONF_MPX3_0   0x201
#define CMD_PIXCONF_MPX3_1   0x202
// Medipix3RX
#define CMD_PIXCONF_MPX3RX   0x203

// Configuration: OMR
#define CMD_SET_CRW          0x301
#define CMD_SET_POLARITY     0x302
#define CMD_SET_DISCCSMSPM   0x303
#define CMD_SET_INTERNALTP   0x304
#define CMD_SET_COUNTERDEPTH 0x305
#define CMD_SET_EQTHRESHH    0x306
#define CMD_SET_COLOURMODE   0x307
#define CMD_SET_CSMSPM       0x308
#define CMD_SET_SENSEDAC     0x309
#define CMD_SET_SENSEDACCODE 0x310
#define CMD_SET_EXTDAC       0x311
#define CMD_WRITE_OMR        0x312

// Trigger
#define CMD_GET_TRIGCONFIG   0x401
#define CMD_SET_TRIGCONFIG   0x402
#define CMD_AUTOTRIG_START   0x403
#define CMD_AUTOTRIG_STOP    0x404
#define CMD_TRIGGER_READOUT  0x405

// Monitoring
#define CMD_GET_ADC          0x501
#define CMD_GET_REMOTETEMP   0x502
#define CMD_GET_LOCALTEMP    0x503
#define CMD_GET_AVDD         0x504
#define CMD_GET_DVDD         0x505
#define CMD_GET_VDD          0x506

// Reply 'bit': in the reply message set in addition to the command identifier
#define CMD_REPLY            0x00010000

// No-reply 'bit': in the command message set in addition
// to the command identifier to indicate that no reply is expected
// (to speed up certain operations)
#define CMD_NOREPLY          0x00080000

#define CMD_MASK             0x0000FFFF

// Error identifiers in replies from the SPIDR module
#define ERR_UNKNOWN_CMD      0x80000001
#define ERR_HARDWARE         0x80000002
#define ERR_MSG_LENGTH       0x80000003
#define ERR_SEQUENCE         0x80000004
#define ERR_ILLEGAL_PAR      0x80000005

#endif // SPIDRCMDS_H
