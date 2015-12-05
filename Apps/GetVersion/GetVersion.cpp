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

bool devId_tostring ( int devId, char *devIdstring);

// Convenience macros
#define cout_spidr_err(str) cout<<str<<": "<<spidrctrl.errorString()<<endl
#define cout_spidrdev_err(dev,str) cout<<"Dev "<<dev<<" "<<str<<": "<<spidrctrl.errorString()<<endl
#define cout_daqdev_err(dev,str) cout<<"Dev "<<dev<<" "<<str<<": "<<spidrdaq[dev].errorString()<<endl

int main(int argc, char *argv[])
{
    int device_nr = 0;
    bool dev_ena[16];     // tells if device is connected.  max 16 devices on a SPIDR, currently max 2
    int  devIds[16];
    char devIdstrings[16][16];


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

    sleep(1);   // required for compact SPIDR

    int version;
    
    cout << hex << "class version:         " << spidrctrl.classVersion() << endl;
    spidrctrl.getSoftwVersion( &version ); // SPIDR LEON3 software version, first call after reset fails...
    spidrctrl.getSoftwVersion( &version ); // SPIDR LEON3 software version, so call twice
    cout << hex << "leon SW version:       " << version << endl;
    spidrctrl.getSoftwVersion( &version ); // SPIDR LEON3 software version
    cout << hex << "leon SW version:       " << version << endl;
    spidrctrl.getFirmwVersion( &version ); // SPIDR FPGA firmware version
    cout << hex << "FPGA firmware version: " << version << endl;


    // determine number of devices, does not check if devices are active
    int ndev;
    int ndev_active;
    if ( !spidrctrl.getDeviceCount( &ndev ) )
      cout_spidr_err( "###getDeviceCount" );
    cout << "[Note] number of devices supported by firmware: " << ndev << endl;

    // check link status
    int linkstatus;
    for (int dev=0; dev<ndev; dev++) {
        if( !spidrctrl.getLinkStatus( dev, &linkstatus ) ) {
          cout_spidr_err( "###getLinkStatus()" );
        }
        // Link status: bits 0-7: 0=link enabled; bits 16-23: 1=link locked
        int links_enabled_mask = (~linkstatus) & 0xFF;
        int links_locked_mask  = (linkstatus & 0xFF0000) >> 16;
        if ( links_enabled_mask != 0 &&
             links_locked_mask == links_enabled_mask ) {
            // At least one link (of 8) is enabled, and all links enabled are locked
            dev_ena[dev] = true;
            ndev_active++;
            cout << "[Note] enabling device " << dev << endl; 
            cout << "[Note] dev " << dev << " linkstatus: " << hex << linkstatus << dec << endl;
            // get device IDs in readable form
            if ( !spidrctrl.getDeviceIds( devIds ) )
              cout_spidr_err( "###getDevidIds()" );
            //cout << hex << devIds[dev] << dec << endl;
            devId_tostring( devIds[dev], devIdstrings[dev] );
            cout << "[Note] dev " << dev << ": " << devIdstrings[dev] << endl;
           }
        else {
          cout << "###linkstatus: " << hex << linkstatus << dec << endl;
        }
    }
  
    return 0;

}


//=============================================================
bool devId_tostring ( int devId, char *devIdstring)
//=============================================================
{
    int waferno = (devId >> 8) & 0xFFF;
    int id_y = (devId >> 4) & 0xF;
    int id_x = (devId >> 0) & 0xF;
    sprintf(devIdstring,"W%04d_%c%02d", waferno, (char)(id_x-1) + 'A', id_y);  // make readable device identifier
    return true;
}
  
