#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <ctime>
using namespace std;
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"

int main(int argc, char *argv[])
{
    int device_nr = 0;

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


    return 0;

}

