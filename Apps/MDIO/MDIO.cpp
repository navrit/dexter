#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string.h>
#include <stdlib.h>
using namespace std;
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"


#define MDIO_ADDR 0x1L
unsigned long read_mdio(SpidrController& spidrctrl, unsigned long device, unsigned long addr)
{
    unsigned long opc = 0x0;
    unsigned long start = 1;
    
    int reg;
    spidrctrl.setSpidrReg( 0x800, start<<28 | opc<<26 | MDIO_ADDR<<21 | device<<16 | addr);
    
    spidrctrl.getSpidrReg( 0x800, &reg);
	printf("reg: %X \n", reg);
	spidrctrl.getSpidrReg( 0x804, &reg);
	
	opc = 3; //read
	start = 0; //toggle
	spidrctrl.setSpidrReg( 0x800, start<<28 | opc<<26 | MDIO_ADDR<<21 | device<<16 );
    spidrctrl.getSpidrReg( 0x800, &reg);
	
    spidrctrl.getSpidrReg( 0x804, &reg);
    return reg&0xFFFF;
}

void write_mdio(SpidrController& spidrctrl, unsigned long device, unsigned long addr, unsigned long val)
{
	unsigned long opc = 0x0;
    unsigned long start = 1;
    
    int reg;
    spidrctrl.setSpidrReg( 0x800, start<<28 | opc<<26 | MDIO_ADDR<<21 | device<<16 | addr);
    spidrctrl.getSpidrReg( 0x804, &reg);
    
	opc = 1; //write
	start = 0; //toggle
	spidrctrl.setSpidrReg( 0x800, start<<28 | opc<<26 | MDIO_ADDR<<21 | device<<16 | (val&0xFFFF));
    spidrctrl.getSpidrReg( 0x804, &reg);
}

unsigned long start_1g = 0;

unsigned long read_mdio_1g(SpidrController& spidrctrl, unsigned long device)
{
    unsigned long opc = 0x0;
    //unsigned long start = 1;
    
    int reg;
    
    opc = 2; //read
    spidrctrl.getSpidrReg( 0x808, &reg);
    start_1g = (reg>>28)&1;
    start_1g ^= 1;
    spidrctrl.setSpidrReg( 0x808, start_1g<<28 | opc<<26 | MDIO_ADDR<<21 | device<<16 | 0x0000);
    
    //spidrctrl.getSpidrReg( 0x808, &reg);
    
	//printf("reg: %X \n", reg);
	do{
		spidrctrl.getSpidrReg( 0x80C, &reg);
	}while(reg&0x80000);
	
	return reg&0xFFFF;
}



void write_mdio_1g(SpidrController& spidrctrl, unsigned long device, unsigned long val)
{
	unsigned long opc = 0x0;
    
    int reg;
    spidrctrl.getSpidrReg( 0x808, &reg);
    start_1g = (reg>>28)&1;
    start_1g ^= 1;
    
	opc = 1; //write
	spidrctrl.setSpidrReg( 0x808, start_1g<<28 | opc<<26 | MDIO_ADDR<<21 | device<<16 | (val&0xFFFF));
    do{
		spidrctrl.getSpidrReg( 0x80C, &reg);
	}while(reg&0x80000);
}

#define PHY_CTRL_REG           0x0
#define PHY_AUTO_NEG           0x4
#define PHY_1000BASE_T_CONTROL 0x9
#define PHY_SPEC_CONTROL       0x10
#define PHY_SPEC_STATUS        0x11
#define PHY_SPEC_CONTROL_EXT   0x14
#define PHY_SPEC_STATUS_EXT    0x1B

typedef struct 
{
	unsigned long ADDR;
	unsigned long ANDVAL;
	unsigned long ORVAL; 
}MDIOVAL;

MDIOVAL MdioVals[12]=
{
	{PHY_CTRL_REG,           0x80BF, 0x0140},
	{PHY_CTRL_REG,           0xFFFF, 0x8000},
	{PHY_AUTO_NEG,           0xFE1F, 0x0000},
	{PHY_CTRL_REG,           0xFFFF, 0x8000},
	{PHY_1000BASE_T_CONTROL, 0xFCFF, 0x0000},
	{PHY_CTRL_REG,           0xFFFF, 0x8000},
	{PHY_SPEC_CONTROL,       0xFFFF, 0xC000},
	{PHY_CTRL_REG,           0xFFFF, 0x8000},
	{PHY_SPEC_CONTROL_EXT,   0xFFFF, 0x0000},
	{PHY_CTRL_REG,           0xFFFF, 0x8000},
	{PHY_SPEC_STATUS_EXT,    0xFFF7, 0x0007}, //SFP
	//{PHY_SPEC_STATUS_EXT,    0xFFFF, 0x000F}, //COPPER
	{PHY_CTRL_REG,           0xFFFF, 0x8000}
};
	
	
void InitMDIOSFP(SpidrController& spidrctrl)
{
	int i;
	unsigned long reg;
	printf("Initializing MDIO Values\n");
	for(i=0; i<12; i++)
	{
		reg = read_mdio_1g(spidrctrl, MdioVals[i].ADDR);
		printf("%02lX: R %04lX, ", MdioVals[i].ADDR, reg);
		reg &= MdioVals[i].ANDVAL;
		reg |= MdioVals[i].ORVAL;
		printf("W: %04lX, ", reg);
		write_mdio_1g(spidrctrl, MdioVals[i].ADDR, reg);
		reg = read_mdio_1g(spidrctrl, MdioVals[i].ADDR);
		printf("V: %04lX\n", reg);
	}
}

int main(int argc, char *argv[])
{
    //int device_nr = 0;

    //----------------------------------------------------------
    //Open a control connection to SPIDR - TPX3 module
    // with address 192.168 .100 .10, default port number 50000
    //----------------------------------------------------------
    SpidrController spidrctrl(192, 168, 100, 10);

    //Are we connected to the SPIDR - TPX3 module ?
    if (!spidrctrl.isConnected()) {
        cout << spidrctrl.ipAddressString() << ": "
	     << spidrctrl.connectionStateString() << ", "
             << spidrctrl.connectionErrString() << endl;
	return 1;
    }
    
    int addr;
    spidrctrl.getIpAddrDest( 0, &addr );

    //cout << (int) ((addr >> 24) & 0xFF) << "  " << 
    //        (int) ((addr >> 16) & 0xFF) << "  " << 
    //        (int) ((addr >>  8) & 0xFF) << "  " << 
    //        (int) ((addr >>  0) & 0xFF) << endl;
 

    int errstat;
    if( !spidrctrl.reset( &errstat ) ) {
        cout << "errorstat " << hex << errstat << dec << endl;
    }

    int version;
    
    cout << hex << "class version:         " << spidrctrl.classVersion() << endl;
    spidrctrl.getSoftwVersion( &version ); // SPIDR LEON3 software version
    cout << hex << "leon SW version:       " << version << endl;
    spidrctrl.getFirmwVersion( &version ); // SPIDR FPGA firmware version
    cout << hex << "FPGA firmware version: " << version << endl;
    
    /*
     * 10g phy read / write
     * 
    unsigned long int devad=1;
	unsigned long int regad=0x8300;
	if(argc>1)
		sscanf(argv[1], "%lX", &devad);
    
	
	if(argc>2)
		sscanf(argv[2], "%lX", &regad);
    
    if(argc>3)
    {
		unsigned long int regval;
		sscanf(argv[3], "%lX", &regval);
		printf("Write spiddr reg %lX: %lX\n",regad, regval); 
		write_mdio(spidrctrl, devad, regad, regval);
	}
	else
		printf("spiddr device %lX reg %lX: %lX\n",devad, regad, read_mdio(spidrctrl, devad,regad)); 
    */
    
    
    /*
     * 1g phy readout
     */
    unsigned long int devad=1;
	if(argc>1)
		sscanf(argv[1], "%lX", &devad);
    
	
	if(argc>2)
    {
		unsigned long int regval;
		sscanf(argv[2], "%lX", &regval);
		printf("Write spiddr reg %lX: %lX\n",devad, regval); 
		write_mdio_1g(spidrctrl, devad, regval);
	}
	else 
	{
		if(argc>1)
		{
			printf("spiddr reg %lX: %lX\n",devad, read_mdio_1g(spidrctrl, devad)); 
		}
		else
		{
			InitMDIOSFP(spidrctrl);
			int reg;
			spidrctrl.setSpidrReg( 0x380, 0);
			spidrctrl.getSpidrReg( 0x380, &reg);
			printf("IPMUX_CONFIG: %X\n" , reg);
		}
	}
	
	
	return 0;

}

