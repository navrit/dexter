#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdint.h>
#include "typeconversion.h"  // if compiled without Qt installed
#include "spidrtpx3data.h"


using namespace std;

int main( int argc, char *argv[]) 
{
    SpidrTpx3Header_t hdr;
    int ip[4];
    int date[3];
    int time[4];
    int devid[3];
    char hdrid[5], devtypeid[5];
    

    if (argc!=2)  { cout << "Usage: DumpRawFile <filename>" << endl; return -1; }

    FILE *fp = fopen(argv[1],"rb");
    if ( fp == NULL ) {
        cout << "can not open file: " << argv[1] << endl;
        return -1;
    }

    int retval = fread( &hdr, sizeof(hdr), 1, fp);
    if ( retval != 1 ) { cout << "Could not read complete header" << endl; return -2; }

    if (hdr.format != 0x1) { cout << " File format: 0x" << hdr.format << "   This decoder is for format: 0x1" << endl;  return -2; }
  
    hdrid[3] = ( hdr.headerId >> 24 ) & 0xFF;
    hdrid[2] = ( hdr.headerId >> 16 ) & 0xFF;
    hdrid[1] = ( hdr.headerId >>  8 ) & 0xFF;
    hdrid[0] = ( hdr.headerId >>  0 ) & 0xFF;
    hdrid[4] = '\0';

    devtypeid[3] = ( hdr.devHeader.headerId >> 24 ) & 0xFF;
    devtypeid[2] = ( hdr.devHeader.headerId >> 16 ) & 0xFF;
    devtypeid[1] = ( hdr.devHeader.headerId >>  8 ) & 0xFF;
    devtypeid[0] = ( hdr.devHeader.headerId >>  0 ) & 0xFF;
    devtypeid[4] = '\0';

    devid[0] = ( hdr.devHeader.deviceId >> 8 ) & 0xFFF;
    devid[1] = ( hdr.devHeader.deviceId >> 4 ) & 0xF;
    devid[2] = ( hdr.devHeader.deviceId >> 0 ) & 0xF;
    char devidchar = (char) (devid[2] + 64); // 1 -> 'A', 2 -> 'B'

    ip[0] = ( hdr.ipAddress >> 24 ) & 0xFF;
    ip[1] = ( hdr.ipAddress >> 16 ) & 0xFF;
    ip[2] = ( hdr.ipAddress >>  8 ) & 0xFF;
    ip[3] = ( hdr.ipAddress >>  0 ) & 0xFF;

    date[0] = ( hdr.yyyyMmDd >> 16 ) & 0xFFFF;
    date[1] = ( hdr.yyyyMmDd >>  8 ) & 0xFF;
    date[2] = ( hdr.yyyyMmDd >>  0 ) & 0xFF;

    time[0] = ( hdr.hhMmSsMs >> 24 ) & 0xFF;
    time[1] = ( hdr.hhMmSsMs >> 16 ) & 0xFF;
    time[2] = ( hdr.hhMmSsMs >>  8 ) & 0xFF;
    time[3] = ( hdr.hhMmSsMs >>  0 ) & 0xFF;


    cout << "===================================================================== " << endl;
    cout << "   File Header " << endl;
    cout << "===================================================================== " << endl;
    cout << "   ID                           0x" << hex << hdr.headerId << dec << " (" << hdrid << ")" << endl;
    cout << "   Total size                     " << hdr.headerSizeTotal << endl;
    cout << "   Size                           " << hdr.headerSize << endl;
    cout << "   Format                       0x" << hex << std::setw(8) << std::setfill('0') << hdr.format << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   SPIDR ID                     0x" << hdr.spidrId << endl;
    cout << "   Library version                " << hex << hdr.libVersion << dec << endl;
    cout << "   Software version               " << hex << hdr.softwVersion << dec << endl;
    cout << "   Firmware version               " << hex << hdr.firmwVersion << dec << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   IP address                     " << dec << ip[0] << "." << ip[1] << "." << ip[2] << "." << ip[3] << endl;
    cout << "   IP port                        " << hdr.ipPort << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   Date                           " << hex << date[0] << "-" << std::setw(2) << date[1] << "-" << std::setw(2) << date[2]  << dec << endl;
    cout << "   Time                           " << hex << time[0] << ":" << time[1] << ":" << time[2] << "." << time[3] << dec << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   Run number                     " << hdr.runNr << endl;
    cout << "   Sequence number                " << hdr.seqNr << endl;
    cout << "   Run Info                       " << hdr.descr << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   Spidr Configuration          0x" << hex << std::setw(8) << std::setfill('0') << hdr.spidrConfig << dec << endl;
    cout << "   Data filter                  0x" << hex << std::setw(8) << hdr.spidrFilter <<  dec << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   Bias Voltage                   " << hdr.spidrBiasVoltage << " (unit ?)" <<  dec << endl;
    cout << "===================================================================== " << endl;
    cout << endl << endl;
    cout << "===================================================================== " << endl;
    cout << "   Device Header " << endl;
    cout << "===================================================================== " << endl;
    cout << "   type ID                      0x" << hex << hdr.devHeader.headerId << dec << " (" << devtypeid << ")" << endl;
    cout << "   Total size                     " << hdr.devHeader.headerSizeTotal << endl;
    cout << "   Size                           " << hdr.devHeader.headerSize << endl;
    cout << "   Format                       0x" << hex << std::setw(8) << std::setfill('0') << hdr.devHeader.format << dec << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   Device ID                    0x" << hex << std::setw(5) << hdr.devHeader.deviceId << dec << 
                                                   "  ( W" << std::setw(4) << devid[0] << "_" << devidchar << std::setw(2) << devid[1] << " ) " << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   General config               0x" << hex << std::setw(8) << std::setfill('0') << hdr.devHeader.genConfig << dec << endl;
    cout << "   Output block config          0x" << hex << std::setw(8) << std::setfill('0') << hdr.devHeader.outblockConfig << dec << endl;
    cout << "   PLL config                   0x" << hex << std::setw(8) << std::setfill('0') << hdr.devHeader.pllConfig << dec << endl;
    cout << "   Testpulse config             0x" << hex << std::setw(8) << std::setfill('0') << hdr.devHeader.testPulseConfig << dec << endl;
    cout << "   SLVS config                  0x" << hex << std::setw(8) << std::setfill('0') << hdr.devHeader.slvsConfig << dec << endl;
    cout << "   Power pulsing config         0x" << hex << std::setw(8) << std::setfill('0') << hdr.devHeader.pwrPulseConfig << dec << endl;
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   DAC code (0..3)                " << std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[0] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[1] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[2] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[3] << endl;
    cout << "   DAC code (4..7)                " << std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[4] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[5] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[6] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[7] << endl;
    cout << "   DAC code (8..11)               " << std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[8] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[9] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[10] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[11] << endl;
    cout << "   DAC code (12..15)              " << std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[12] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[13] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[14] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[15] << endl;
    cout << "   DAC code (16..19)              " << std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[16] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[17] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[18] << "  " << 
                                                    std::setw(3) << std::setfill(' ') << hdr.devHeader.dac[19] << endl; 
    cout << "--------------------------------------------------------------------- " << endl;
    cout << "   Column testpulse register    0x " << std::setfill('0') ; 
    for (int i=0; i<16; i++) {
        cout << hex << (int)hdr.devHeader.ctpr[i] << " ";
    }
    cout << endl;   
    cout << "===================================================================== " << endl;

        


}

/*
  u8  ctpr[256/8];
  u32 unused[64-11-32-(256/8)/4];
} Tpx3Header_t;
#define SPIDRTPX3_HEADER_SIZE  sizeof(SpidrTpx3Header_t)
#define TPX3_HEADER_SIZE       sizeof(Tpx3Header_t)

#define SPIDRTPX3_HEADER_ID    0x33545053 // Represents "SPT3"
#define TPX3_HEADER_ID         0x33585054 // Represents "TPX3"
#define HEADER_FILLER          0xDD       // For unused space in headers

#endif // SPIDRTPX3DATA_H
*/
