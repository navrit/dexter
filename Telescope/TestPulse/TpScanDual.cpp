// 
// Testpulse scan for two chips
//
//


#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

using namespace std;
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"

#define CFG_PATH "/mnt/CONFIGS"
#define DATA_PATH "/localstore/TPX3/DATA"

bool configure( SpidrController *spidrctrl );
bool start_run( SpidrController *spidrctrl, SpidrDaq **spidrdaq, char *prefix, int run_nr, char *description);
bool stop_run( SpidrController *spidrctrl, SpidrDaq **spidrdaq );
bool timestamp( SpidrController *spidrctrl );
void *timestamp_per_sec( void *rctrl );  // to run in a separate thread
bool devId_tostring ( int devId, char *devIdstring);

int  ndev;          // number of devices, supported by this software/firmware
int  ndev_active = 0;   // number of connected devices, determined by activity on links
bool dev_ena[16];  // tells if device is connected.  max 16 devices on a SPIDR, currently max 2
int  devIds[16];
char devIdstrings[16][16];
int spidr_pixel_packets_sent[2] = {0,0};     // before and after
int spidr_data_packets_sent[2] = {0,0};     // before and after
int spidr_mon_packets_sent[2] = {0,0};      // before and after
int spidr_pause_packets_rec[2] = {0,0};          // before and after
int daq_packets_rec[2][2] = {{0,0},{0,0}};  // 2 devices before and after
int daq_packets_lost[2][2]= {{0,0},{0,0}};  // 2 devices before and after

int main(int argc, char *argv[])
{
    int run_nr;
    int prev_run_nr = -1;
    char hostname[64];
    char fileprefix[256];
    char description[128];
    //unsigned int timer_lo1, timer_hi1;
    //clock_t start_time = clock();
    //clock_t cur_time = clock();
    //double sec_elapsed = 0, prev_sec_elapsed = 0;
    bool run_control;
    extern char *optarg; 
    int portnumber = 51000; // for run control

    int my_socket, new_socket;
    int bufsize = 1024;
    int status;
    char *buffer = (char *)malloc(bufsize);
    char cmd[32];
    struct sockaddr_in address;
    socklen_t addrlen;
    pthread_t ts_thread = NULL;   // thread for 1 sec timestamps

    if (argc != 2) { 
        cout << "usage: TpScanDual <file-prefix>" << endl;
    }

    sscanf( argv[1], "%s", fileprefix );
 
    //----------------------------------------------------------
    //Open a control connection to SPIDR - TPX3 module
    // with address 192.168.100.10, default port number 50000
    //----------------------------------------------------------
    SpidrController *spidrctrl = new SpidrController(192, 168, 100, 10);

    //Are we connected to the SPIDR - TPX3 module ?
    if (!spidrctrl->isConnected()) {
        cout << spidrctrl->ipAddressString() << ": "
	     << spidrctrl->connectionStateString() << ", "
             << spidrctrl->connectionErrString() << endl;
	return 1;
    }
    
    int addr;
    if ( !spidrctrl->getIpAddrDest( 0, &addr ) )
        cout << "###getIpAddrDest: " << spidrctrl->errorString() << endl;

    //cout << (int) ((addr >> 24) & 0xFF) << "  " << 
    //        (int) ((addr >> 16) & 0xFF) << "  " << 
    //        (int) ((addr >>  8) & 0xFF) << "  " << 
    //        (int) ((addr >>  0) & 0xFF) << endl;
 
    // first select internal or external clock
    //if (!spidrctrl->setExtRefClk(true) ) 
    if (!spidrctrl->setExtRefClk(false) ) 
        cout << "###setExtRefClk: " << spidrctrl->errorString() << endl;

    int errstat;
    if( !spidrctrl->reset( &errstat ) ) {
        cout << "###reset errorstat: " << hex << errstat << dec << endl;
    }


    // determine number of devices, does not check if devices are active
    if ( !spidrctrl->getDeviceCount( &ndev ) )
            cout << "###getDeviceCount: " << spidrctrl->errorString() << endl;
    cout << "[Note] number of devices supported by firmware: " << ndev << endl;

    ndev = 1;

    // check link status
    int linkstatus;
    for (int dev=0; dev<ndev; dev++) {
        if( !spidrctrl->getLinkStatus( dev, &linkstatus ) ) {
            cout << "#getLinkStatus(): " << hex << errstat << dec << endl;
        }
        if ( linkstatus ) {
            dev_ena[dev] = true;
            ndev_active++;
            cout << "[Note] enabling device " << dev << endl; 
            cout << "[Note] dev " << dev << " enabled links: " << hex << linkstatus << dec << endl;
            // get device IDs in readable form
            if ( !spidrctrl->getDeviceIds( devIds ) )
                cout << "#getDevidIds(): " << hex << errstat << dec << endl;
            //cout << hex << devIds[dev] << dec << endl;
            devId_tostring( devIds[dev], devIdstrings[dev] );
            cout << "[Note] dev " << dev << ": " << devIdstrings[dev] << endl;
           }
        cout << "linkstatus: " << linkstatus << endl;
    }
    
    

    // ----------------------------------------------------------
    // SPIDR configuration
    // ----------------------------------------------------------
    // set packet filter: ethernet no filter
    int eth_mask, cpu_mask;
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            if ( !spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask ) )
                cout << "Dev "<< dev << " ###getHeaderFilter: " << spidrctrl->errorString() << endl;
            eth_mask = 0xFFFF;
            //cpu_mask = 0x0080;
            if ( !spidrctrl->setHeaderFilter  ( dev, eth_mask,   cpu_mask ) )
                cout << "Dev "<< dev << " ###setHeaderFilter: " << spidrctrl->errorString() << endl;
            if ( !spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask ) )
                cout << "Dev "<< dev << " ###getHeaderFilter: " << spidrctrl->errorString() << endl;
            cout << "[Note] dev " << dev << hex << " eth_mask = " << eth_mask << "  cpu_mask = " << cpu_mask << dec << endl;
        }
    }

     

    // ----------------------------------------------------------
    // Interface to Timepix3 pixel data acquisition
    // ----------------------------------------------------------

    SpidrDaq *spidrdaq[2];
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            spidrdaq[dev] = new SpidrDaq( spidrctrl, 0x10000000, dev);
            string errstr = spidrdaq[dev]->errorString();
            if (!errstr.empty()) 
                cout << "Dev "<< dev << " ### SpidrDaq: " << errstr << endl; 
            
            spidrdaq[dev]->setFlush( false ); // Don't flush when no file is open
            //spidrdaq[dev]->setFlush( true ); // Don't flush when no file is open
            
            // Sample 'frames' as well as write pixel data to file
            //spidrdaq[dev]->setSampling( true ); // no sampling
        }
    }

    // select whether or not the let the FPGA do the ToT/ToA decoding
    // gray decoding (for ToA only) has priority over LFSR decoding
    if ( !spidrctrl->setDecodersEna(true) )
        cout << "###setDecodersEna: " << spidrctrl->errorString() << endl;

    // ----------------------------------------------------------
    // SPIDR-TPX3 and Timepix3 timers
    // ----------------------------------------------------------
    // TODO: remove?, it will be replaced by external T0-sync
    //
    //if( !spidrctrl->restartTimers() )
    //    cout << "###restartTimers: " << spidrctrl->errorString() << endl;

    
    //----------------------------------------------------------
    // stand-alone
    //----------------------------------------------------------

    bool retval;

    status = configure( spidrctrl ); 

    char prefix[256];
    sprintf( prefix, "TP/%s", fileprefix );
    sprintf(description, "test of %s", prefix);

    int cfg;
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            spidrctrl->getPllConfig(dev, &cfg);
            cout << hex << "2: PLL config " << cfg << endl;
            cfg = 0x1E;
            cout << hex << "3: PLL config " << cfg << endl;
            //cfg &= 0xFFFE;
            //cfg |= 0x100;
            spidrctrl->setPllConfig(dev, cfg);
            spidrctrl->getPllConfig(dev, &cfg);
            cout << hex << "4: PLL config " << cfg << endl;
        }
    }



    // ----------------------------------------------------------
    // open data files
    // ----------------------------------------------------------
    char filename[256];
    char path[256];
    int row;
    int col;
    int selrow;
    int selcol;
    int spacing=8;


    // Mask all but selected pixels
    for( selrow=0; selrow<spacing; ++selrow ) {
        for( selcol=0; selcol<spacing; ++selcol ) {

            // ----------------------------------------------------------
            // TPX3 Pixel configuration + thresholds
            // ----------------------------------------------------------
            //int row, col;
            int thr, mask, tp_ena;
            char trimfile[256];
        
            for (int dev=0; dev<ndev; dev++) {
                if ( dev_ena[dev] ) {
                    // Clear pixel configuration in chip
                    retval = spidrctrl->resetPixels( dev );
                    if( !retval ) {
                        cout << "Dev "<< dev << " ###resetPixels: " << spidrctrl->errorString() << endl;
                        status &= retval;
                    }
                }
            }

            for (int dev=0; dev<ndev; dev++) {
                if ( dev_ena[dev] ) {
                    // Clear local pixel config, no return value
                    spidrctrl->resetPixelConfig() ;
                    // Disable all testpulse bits
                    retval = spidrctrl->setPixelTestEna( ALL_PIXELS, ALL_PIXELS, false) ;
                    if( !retval ) {
                        cout << "Dev "<< dev << " ###setPixelTestEna: " << spidrctrl->errorString() << endl;
                        status &= retval;
                    }
                    // Enable all pixels 
                    retval = spidrctrl->setPixelMask( ALL_PIXELS, ALL_PIXELS, false );
                    if( !retval ) {
                        cout << "Dev "<< dev << " ###setPixelMask: " << spidrctrl->errorString() << endl;
                        status &= retval;
                    }
        
                    for( row=0; row<256; ++row ) {
                        for( col=0; col<256; ++col ) {
                            if ( (row%spacing == selrow) && (col%spacing==selcol) ) {
                                spidrctrl->setPixelMask( col, row, false ); // enable
                                spidrctrl->setPixelTestEna( col, row, true);
                            }
                            else {
                                spidrctrl->setPixelMask( col, row, true ); // else mask
                                spidrctrl->setPixelTestEna( col, row, false);
                            }
                        }
                        if ( col%spacing==selcol ) {
                            spidrctrl->setCtprBit( col, 1 );
                        }
                    }
        
                    // write column testpulse register
                    spidrctrl->setCtprBits( 1 );
                    retval = spidrctrl->setCtpr( dev );
                    if( !retval ) {
                        cout << "Dev "<< dev << " ###setCtpr: " << spidrctrl->errorString() << endl;
                        status &= retval;
       	            }
        
                    // read thresholds from file
                    char *charptr; 
                    char line[256];
        
                    sprintf(trimfile, "%s/%s_trimdacs.txt", CFG_PATH, devIdstrings[dev]);
                    FILE *fp = fopen(trimfile, "r");
                    if (fp == NULL) { 
                        cout << "[Warning] can not open trimdac file: " << trimfile << endl;
                        sprintf(trimfile, "%s/default_trimdacs.txt", CFG_PATH);
                        cout << "[Warning] trying default trimdac file" << endl;
                        fp = fopen(trimfile, "r");
                        if (fp == NULL) {
                             cout << "[Error] can not open trimdac file: " << trimfile << endl;
                             return false;
                        }
                    }
                    cout << "[Note] reading " << trimfile << endl; 
                    while ( !feof(fp) ) {
                        charptr = fgets(line, 256, fp);   
                        //if ( charptr == NULL) 
                        //    cout << "fgets returned NULL" << endl;
                        if (line[0] != '#') {
                            sscanf(line, "%d %d %d %d %d", &col, &row, &thr, &mask, &tp_ena);
                            retval =  spidrctrl->setPixelThreshold( col, row, thr);
                            if ( !retval ) {
                                cout << "Dev "<< dev << " ###setPixelThreshold: " << spidrctrl->errorString() << endl;
                                status &= retval;
                            }
        
                            if (mask) {
                                retval = spidrctrl->setPixelMask( col, row, true );
                                if ( !retval ) {
                                    cout << "Dev "<< dev << " ###setPixelMask: " << spidrctrl->errorString() << endl;
                                    status &= retval;
                                }
                            }
                        }
                    }
                    fclose(fp);
        
                    // Write pixel config to chip
                    if( !spidrctrl->setPixelConfig( dev ) )
                        cout << "###setPixelConfig: " << spidrctrl->errorString() << endl;
                    
        
                }
            }
        
            // ----------------------------------------------------------
            // TPX3 Testpulse configuration
            // ----------------------------------------------------------
        
            //int trig_mode = 4; // SPIDR_TRIG_AUTO;
            int trig_mode = 4; // 0 = external shutter
            int trig_length_us = 200000; // 100 ms
            int trig_freq_hz = 10; // Hz
            int nr_of_trigs = 1; // 1 triggers
                    
            retval = spidrctrl->setShutterTriggerConfig( trig_mode, trig_length_us,
                                             trig_freq_hz, nr_of_trigs );
            if ( !retval ) {
                cout << "###setShutterTriggerConfig: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        
            for (int dev=0; dev<ndev; dev++) {
               if ( dev_ena[dev] ) {
                    if( !spidrctrl->setGenConfig( dev,
                                                 //TPX3_POLARITY_EMIN |
                                                 TPX3_POLARITY_HPLUS |
                                                 TPX3_ACQMODE_TOA_TOT |
                                                 TPX3_GRAYCOUNT_ENA |
                                                 TPX3_FASTLO_ENA |
                                                 TPX3_TESTPULSE_ENA  // | 
                                                 //TPX3_SELECTTP_EXT_INT |
                                                 //TPX3_SELECTTP_DIGITAL
                                               ) )
                    cout << "###setGenCfg: " << spidrctrl->errorString() << endl;
                    // read back configuration
                    int gen_cfg;
                    if ( !spidrctrl->getGenConfig( dev, &gen_cfg) )
                         cout << "###getGenCfg: " << spidrctrl->errorString() << endl;
                    cout << "gen config " << hex << gen_cfg << endl;
        
                    spidrdaq[dev]->setSampling( true );
                }
            }
        
        
            // Set Timepix3 into acquisition mode
            retval = spidrctrl->datadrivenReadout();
            if( !retval ) {
                cout << "###ddrivenReadout: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
                
            // reapply ethernet mask
            for (int dev=0; dev<ndev; dev++) {
                if ( dev_ena[dev] ) {
                    retval = spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask );
                    if( !retval ) {
                        cout << "Dev "<< dev << " ###getHeaderFilter: " << spidrctrl->errorString() << endl;
                        status &= retval;
                    }
                    //cpu_mask = 0x0080;
                    eth_mask = 0xFFFF;
                    retval = spidrctrl->setHeaderFilter  ( dev, eth_mask,   cpu_mask );
                    if( !retval ) {
                        cout << "Dev "<< dev << " ###setHeaderFilter: " << spidrctrl->errorString() << endl;
                        status &= retval;
                    }
                }
            }
                            
            if( !spidrctrl->restartTimers() )
                cout << "###restartTimers: " << spidrctrl->errorString() << endl;
        
        
            int thrstep_c = 16;
            int thrstep_f = 2;
            //for ( int thr_c = 0; thr_c<256; thr_c+=thrstep_c) {
            for ( int thr_c = 0; thr_c<33; thr_c+=thrstep_c) {
              for ( int thr_f = 0; thr_f<200; thr_f+=thrstep_f) {
                for (int dev=0; dev<ndev; dev++) {
                    if ( dev_ena[dev] ) {
        
                        if ( !spidrctrl->setDac( dev, TPX3_VTP_FINE, thr_f ) )
                            cout << "###setDac: " << spidrctrl->errorString() << endl;
                        // coarse testpulse voltage 
                        if ( !spidrctrl->setDac( dev, TPX3_VTP_COARSE, thr_c ) )
                            cout << "###setDac: " << spidrctrl->errorString() << endl;
                        // configure testpulse generator 
                        if( !spidrctrl->setTpNumber( dev, 100 ) )
                            cout << "###setTpNumber: " << spidrctrl->errorString() << endl;
                        if( !spidrctrl->setTpPeriodPhase( dev, 50, 0 ) )
                            cout << "###setTpPeriodPhase: " << spidrctrl->errorString() << endl;
        
        
                        //sprintf(filename, "%sCHIP%d/TestPulse/%s_thr%d.dat", path, dev, prefix, thr);
                        sprintf(description, "testpulse data of chip %d thr_c = %d thr_f = %d", dev, thr_c, thr_f);
                        sprintf( filename, "%s/CHIP%d/%s%s_sc%d_sr%d_thr_c%d_f%d.dat", DATA_PATH, dev, prefix, devIdstrings[dev], selcol, selrow, thr_c, thr_f );
                        cout << "[Note] opening data file" << filename << endl;
                        retval = spidrdaq[dev]->startRecording( filename, 0 , description );
                        if ( !retval ) {
                            cout << "Dev "<< dev << " ###startRecording: " << spidrctrl->errorString() << endl;
                            status &= retval;
                        }
        
        
                    }
                }
        
                // Wait for the DAC to stabilise    
                usleep(100);
                    
                // Start triggers
                cout << "[Note] starting auto trigger " << endl;
                if( !spidrctrl->startAutoTrigger() )
                    cout << "###startAutoTrigger: " << spidrctrl->errorString() << endl;
                
                int shutter_length_us = 50000;
                usleep(shutter_length_us);
                
                for (int dev=0; dev<2; dev++) 
                {
                    if ( dev_ena[dev] ) {
                        retval = spidrdaq[dev]->stopRecording();
               
                    }
                } 
        
                int extcntr, intcntr;
                if ( !spidrctrl->getExtShutterCounter( &extcntr) )
                    cout << "###getExtShutterCounter: " << spidrctrl->errorString() << endl;
                cout << "[Note] External shutter counter: " << extcntr << endl;
                if ( !spidrctrl->getShutterCounter( &intcntr) )
                    cout << "###getShutterCounter: " << spidrctrl->errorString() << endl;
                cout << "[Note] Number of shutters given " << intcntr << endl;
              
              }
        
            }
             
        }
    }
            
    //----------------------------------------------------------
    // common closing / cleaning
    //----------------------------------------------------------
    
    for (int dev=0; dev<ndev; dev++) 
        if ( dev_ena[dev] ) 
            spidrdaq[dev]->stop();

    for (int dev=0; dev<ndev; dev++) 
        if ( dev_ena[dev] ) 
            delete spidrdaq[dev];
    delete spidrctrl;

    return 0;

}


//=====================================================
bool configure( SpidrController *spidrctrl )
//=====================================================
{

    bool status = true;   
    bool retval;
    int numpar;
    int eth_mask, cpu_mask;

    // ----------------------------------------------------------
    // TPX3 DACs configuration
    // ----------------------------------------------------------
    int dac_nr, dac_val;
    char line[1024];
    char dacfile[256];
    char *cptr;

    // get device IDs in readable form
    //spidrctrl->getDeviceIds( devIds );
    //for (int dev=0; dev<2; dev++) {
        //cout << hex << devIds[dev] << dec << endl;
    //    devId_tostring( devIds[dev], devIdstrings[dev] );
    //    cout << devIdstrings[dev] << endl;
    //}

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            sprintf(dacfile, "%s/%s_dacs.txt", CFG_PATH, devIdstrings[dev]);
            FILE *fp = fopen(dacfile, "r");
            if (fp == NULL) { 
                cout << "[Warning] can not open dac file: " << dacfile << endl; 
                sprintf(dacfile, "%s/default_dacs.txt", CFG_PATH);
                cout << "[Warning] trying default dac file" << endl;    
                fp = fopen(dacfile, "r");
                if (fp == NULL) { 
                     cout << "[Error] can not open dac file: " << dacfile << endl; 
                     return false;
                }
            }
            cout << "[Note] reading " << dacfile << endl;
            while ( !feof(fp) ) {
                cptr = fgets(line, 256, fp);
                if (line[0] != '#') {
                    numpar = sscanf(line, "%d %d", &dac_nr, &dac_val);
                    if (numpar == 2) {
                        retval = spidrctrl->setDac( dev, dac_nr, dac_val);
                        if ( !retval ) {
                            cout << "Dev "<< dev << " ###setDac: " << spidrctrl->errorString() << endl;
                            status &= retval;
                        }
                    }
                }
            }
            fclose(fp);
        }

    }


    // ----------------------------------------------------------
    // TPX3 Pixel configuration + thresholds
    // ----------------------------------------------------------
    int row, col, thr, mask, tp_ena;
    char trimfile[256];

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            // Clear pixel configuration in chip
            retval = spidrctrl->resetPixels( dev );
            if( !retval ) {
                cout << "Dev "<< dev << " ###resetPixels: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        }
    }
            
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            // Clear local pixel config, no return value
            spidrctrl->resetPixelConfig() ;
            // Disable all testpulse bits
            retval = spidrctrl->setPixelTestEna( ALL_PIXELS, ALL_PIXELS, false) ;
            if( !retval ) {
                cout << "Dev "<< dev << " ###setPixelTestEna: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            // Enable all pixels 
            retval = spidrctrl->setPixelMask( ALL_PIXELS, ALL_PIXELS, false );
            if( !retval ) {
                cout << "Dev "<< dev << " ###setPixelMask: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        
            // read thresholds from file
            char *charptr; 
        
            //if ( ( (devIds[dev] & 0xFFFFF) == 0x00000 ) || ( (devIds[dev] & 0xFFFFF) == 0xFFFFF ) ) // no chip ID
            //     sprintf(trimfile, "%s/default_trimdacs.txt", CFG_PATH);
            //else
            sprintf(trimfile, "%s/%s_trimdacs.txt", CFG_PATH, devIdstrings[dev]);
            FILE *fp = fopen(trimfile, "r");
            if (fp == NULL) { 
                cout << "[Warning] can not open trimdac file: " << trimfile << endl;
                sprintf(trimfile, "%s/default_trimdacs.txt", CFG_PATH);
                cout << "[Warning] trying default trimdac file" << endl;
                fp = fopen(trimfile, "r");
                if (fp == NULL) {
                     cout << "[Error] can not open trimdac file: " << trimfile << endl;
                     return false;
                }
            }
            cout << "[Note] reading " << trimfile << endl; 
            while ( !feof(fp) ) {
                charptr = fgets(line, 256, fp);   
                //if ( charptr == NULL) 
                //    cout << "fgets returned NULL" << endl;
                if (line[0] != '#') {
                    sscanf(line, "%d %d %d %d %d", &col, &row, &thr, &mask, &tp_ena);
                    retval =  spidrctrl->setPixelThreshold( col, row, thr);
                    if ( !retval ) {
                        cout << "Dev "<< dev << " ###setPixelThreshold: " << spidrctrl->errorString() << endl;
                        status &= retval;
                    }

                    if (mask) {
                        retval = spidrctrl->setPixelMask( col, row, true );
                        if ( !retval ) {
                            cout << "Dev "<< dev << " ###setPixelMask: " << spidrctrl->errorString() << endl;
                            status &= retval;
                        }
                    }
                }
            }
            fclose(fp);

            //retval = spidrctrl->setPixelTestEna( 127, 127, true);
            //if( !retval ) {
            //    cout << "Dev "<< dev << " ###setPixelTestEna: " << spidrctrl->errorString() << endl;
            //    status &= retval;
            //}
            // Write pixel config to chip
            retval = spidrctrl->setPixelConfig( dev );
            if( !retval ) {
                if ( strstr(spidrctrl->errorString().c_str(), "ERR_UNEXP") ) ; // known feature, do nothing
                else {
                    cout << "Dev "<< dev << " ###setPixelConfig: " << spidrctrl->errorString() << endl;
                    status &= retval;
                }
            }
            
            // read back configuration
            int gen_cfg;
            retval = spidrctrl->getGenConfig( dev, &gen_cfg);
            if( !retval ) {
                cout << "Dev "<< dev << " ###getGenConfig: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        
        }
    }
    //cout << "masking pixels" << endl;
    //retval = spidrctrl->setSpidrReg( 0x394, 0x9c50);
    //retval = spidrctrl->setSpidrRegBit( 0x394, 16, true);
    //retval = spidrctrl->setSpidrReg( 0x398, 0x9a54 );
    //retval = spidrctrl->setSpidrRegBit( 0x398, 16, true);
    
    
    // ----------------------------------------------------------

    // Configure the shutter trigger
    //int trig_mode = 4; // SPIDR_TRIG_AUTO;
    int trig_mode = 4; // 0 = external shutter
    int trig_length_us = 500000; // 50 ms
    int trig_freq_hz = 5; // Hz
    int nr_of_trigs = 1; // 1 triggers
    
    retval = spidrctrl->setShutterTriggerConfig( trig_mode, trig_length_us,
                                     trig_freq_hz, nr_of_trigs );
    if ( !retval ) {
        cout << "###setShutterTriggerConfig: " << spidrctrl->errorString() << endl;
        status &= retval;
    }

    int i1, i2, i3, i4;
    retval = spidrctrl->getShutterTriggerConfig( &i1, &i2, &i3, &i4 );
    if ( !retval ) {
        cout << "###getShutterTriggerConfig: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
    cout << "[Note] read trigger config " << hex << i1 << " " << i2 << " " << i3 << " " << i4 << dec  << endl;


    // ----------------------------------------------------------
    // Acquisition mode
    // ----------------------------------------------------------
    // Set Timepix3 acquisition mode

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->setGenConfig( dev,
                                     TPX3_POLARITY_HPLUS |
                                     //TPX3_POLARITY_EMIN |
                                     TPX3_ACQMODE_TOA_TOT |
                                     TPX3_GRAYCOUNT_ENA |
                                     TPX3_TESTPULSE_ENA |
                                     //TPX3_SELECTTP_EXT_INT |
                                     //TPX3_SELECTTP_DIGITAL |
                                     TPX3_FASTLO_ENA
                                   ); 
    
            if( !retval ) {
                cout << "Dev "<< dev << " ###setGenCfg: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        
            // read back configuration
            int gen_cfg;
            retval = spidrctrl->getGenConfig( dev, &gen_cfg);
            if( !retval ) {
                cout << "Dev "<< dev << " ###getGenCfg: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            cout << "[Note] dev " << dev << " gen config " << hex << gen_cfg << dec <<  endl;
        
            // PLL configuration: 40 MHz on pixel matrix
            //int pll_cfg = 0x01E;
            int pll_cfg = 0x01E | 0x100;  // 40 MHz, 16 clock phases
            retval = spidrctrl->setPllConfig(dev, pll_cfg);
            if( !retval ) {
                cout << "Dev "<< dev << " ###setPllCfg: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            retval = spidrctrl->getPllConfig(dev, &pll_cfg);
            if( !retval ) {
                cout << "Dev "<< dev << " ###setPllCfg: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            cout << "[Note] dev " << dev << " PLL config " << hex << pll_cfg <<  dec << endl;
        }
    } 


    // Set Timepix3 into acquisition mode
    retval = spidrctrl->datadrivenReadout();
    if( !retval ) {
        cout << "###ddrivenReadout: " << spidrctrl->errorString() << endl;
        status &= retval;
    }

    // reapply ethernet mask
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask );
            if( !retval ) {
                cout << "Dev "<< dev << " ###getHeaderFilter: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            //cpu_mask = 0x0080;
            eth_mask = 0xFFFF;
            retval = spidrctrl->setHeaderFilter  ( dev, eth_mask,   cpu_mask );
            if( !retval ) {
                cout << "Dev "<< dev << " ###setHeaderFilter: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        }
    }

    return status;
}



//=======================================================================================
bool start_run( SpidrController *spidrctrl, SpidrDaq **spidrdaq, char *prefix, int run_nr, char *description) 
//=======================================================================================
{
    unsigned int timer_lo1, timer_hi1;
    int status = 0;
    bool retval;
    char filename[256];
    char dacfile[256];
    char dacfile_out[256];
    char line[256];

    cout << "[Note] starting run " << endl;

    // reset shutter counters a.o.
    retval = spidrctrl->resetCounters();
    if( !retval ) {
        cout <<  "###resetCounters: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
    
    // resetting counter for statistic of packets
    retval = spidrctrl->resetPacketCounters();
    if( !retval ) {
        cout <<  "###resetPacketCounters: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
  
    retval = spidrctrl->getDataPacketCounter( &spidr_data_packets_sent[0] );
    if( !retval ) {
        cout << "###getDataPacketCounter: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
    retval = spidrctrl->getMonPacketCounter( &spidr_mon_packets_sent[0] );
    if( !retval ) {
        cout << "###getMonPacketCounter: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
    retval = spidrctrl->getPixelPacketCounter( &spidr_pixel_packets_sent[0] );
    if( !retval ) {
       cout << "###getPixelPacketCounter: " << spidrctrl->errorString() << endl;
      status &= retval;
    }
    retval = spidrctrl->getPausePacketCounter( &spidr_pause_packets_rec[0] );
    if( !retval ) {
        cout << "###getPausePacketCounter: " << spidrctrl->errorString() << endl;
        status &= retval;
    }

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            daq_packets_rec[dev][0] = spidrdaq[dev]->packetsReceivedCount();
            daq_packets_lost[dev][0] = spidrdaq[dev]->packetsLostCount();
        }
    }

    cout << "[Note] Spidr ethernet pause packet count before run: " << spidr_pause_packets_rec[0] << endl;
    cout << "[Note] Spidr ethernet monitoring packet count before run: " << spidr_mon_packets_sent[0] << endl;
    cout << "[Note] Spidr ethernet data packet count before run: " << spidr_data_packets_sent[0] << endl;
    cout << "[Note] DAQ thread 0 receive packet count before run:   " << daq_packets_rec[0][0] <<  "  ( and lost " << daq_packets_lost[0][0] << " )" << endl;
    cout << "[Note] DAQ thread 1 receive packet count before run:   " << daq_packets_rec[1][0] <<  "  ( and lost " << daq_packets_lost[1][0] << " )" << endl;
    cout << "[Note] Spidr pixel packet count before run: " << spidr_pixel_packets_sent[0] << endl;

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            sprintf( filename, "%s/CHIP%d/%s%s.dat", DATA_PATH, dev, prefix, devIdstrings[dev] );
            cout << "[Note] opening data file" << endl;
            retval = spidrdaq[dev]->startRecording( filename, run_nr, description );
            if ( !retval ) {
                cout << "Dev "<< dev << " ###startRecording: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
            if ( !retval ) {
                cout << "Dev "<< dev << " ###getTimer: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        }
    }

    // copy trimdac settings to rundirectory
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            sprintf(dacfile, "%s/%s_trimdacs.txt", CFG_PATH, devIdstrings[dev]);
            FILE *fp = fopen(dacfile, "r");
            if (fp == NULL) { 
                cout << "[Warning] can not open dac file: " << dacfile << endl; 
                sprintf(dacfile, "%s/default_trimdacs.txt", CFG_PATH);
            }
            sprintf(dacfile_out, "%s/CHIP%d/Run%d/%s_trimdac.txt", DATA_PATH, dev, run_nr, devIdstrings[dev]);
            FILE *fpout = fopen(dacfile_out, "w");
            cout << "[Note] copying " << dacfile << " to run directory" << endl;
            while ( !feof(fp) ) {
                char *cptr = fgets(line, 256, fp);
                if (cptr != 0 ) 
                    fputs(line, fpout);
            }
            fclose(fp);
            fclose(fpout);
        }
    }
   
    return status;

}
        

//=============================================================
bool stop_run( SpidrController *spidrctrl, SpidrDaq **spidrdaq ) 
//=============================================================
{
    bool status = false;
    bool retval;
    unsigned int timer_lo1, timer_lo2, timer_hi1, timer_hi2;

    timestamp( spidrctrl );
    //for (int dev=0; dev<2; dev++) {
    //    retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
    //    status |= retval;
    //}

    sleep(1);  // waiting time to collect last ethernet packets
    
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->getShutterStart(dev, &timer_lo1, &timer_hi1);
            if ( !retval ) {
                cout << "Dev "<< dev << " ###getShutterStart: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            retval = spidrctrl->getShutterEnd(dev, &timer_lo2, &timer_hi2);
            if ( !retval ) {
                cout << "Dev "<< dev << " ###getShutterEnd: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            //cout << dec << "shutter timer high : " << (timer_hi2 - timer_hi1)*25E-9 << endl; 
            //cout << "shutter timer low : " << (timer_lo2 - timer_lo1)*25E-9 << endl; 
            //cout << "shutter timer_lo1 " << timer_lo1 << "  shutter timer_lo2 " << timer_lo2 << endl;
        }
    }

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrdaq[dev]->stopRecording();
            status &= retval;
        }
    }

    retval = spidrctrl->getDataPacketCounter( &spidr_data_packets_sent[1] );
    if( !retval ) {
        cout << "###getDatatPacketCounter: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
    retval = spidrctrl->getMonPacketCounter( &spidr_mon_packets_sent[1] );
    if( !retval ) {
        cout << "###getMonPacketCounter: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
    retval = spidrctrl->getPixelPacketCounter( &spidr_pixel_packets_sent[1] );
    if( !retval ) {
        cout << "###getPixelPacketCounter: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
    retval = spidrctrl->getPausePacketCounter( &spidr_pause_packets_rec[1] );
    if( !retval ) {
        cout << "###getPausePacketCounter: " << spidrctrl->errorString() << endl;
        status &= retval;
    }

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            daq_packets_rec[dev][1] = spidrdaq[dev]->packetsReceivedCount();
            daq_packets_lost[dev][1] = spidrdaq[dev]->packetsLostCount();
        }
    }

    cout << "[Note] Spidr ethernet pause packet count after run: " << spidr_pause_packets_rec[1] << endl;
    cout << "[Note] Spidr ethernet monitoring packet after run: " << spidr_mon_packets_sent[1] << endl;
    cout << "[Note] Spidr ethernet data packet count after run: " << spidr_data_packets_sent[1] << endl;
    cout << "[Note] DAQ thread 0 receive packet count after run:   " << daq_packets_rec[0][1] <<  "  ( and lost " << daq_packets_lost[0][1] << " )" << endl;
    cout << "[Note] DAQ thread 1 receive packet count after run:   " << daq_packets_rec[1][1] <<  "  ( and lost " << daq_packets_lost[1][1] << " )" << endl;
    int missing_packets = (spidr_data_packets_sent[1] - spidr_data_packets_sent[0]) - 
                          (daq_packets_rec[0][1] - daq_packets_rec[0][0]) -
                          (daq_packets_rec[1][1] - daq_packets_rec[1][0]);
    if ( missing_packets != 0 ) 
        cout << "[Warning] Missing ethernet packets after run: " << missing_packets << endl;
    else 
        cout << "[Note] No missing ethernet packets after run " << endl;

    cout << "[Note] Spidr pixel packet count after run: " << spidr_pixel_packets_sent[1] << endl;
    
    // get number bytes received and check versus packets sent
    //int missing_pixel_packets = spidr_pixel_packets_sent[1] - spidr_pixel_packets_sent[0];
    //if ( missing_pixel_packets != 0 ) 
    //    cout << "[Warning] Missing pixel packets after run: " << missing_pixel_packets << endl;
    //else 
    //    cout << "[Note] No missing pixel packets after run: " << endl;
 
    return status;
}

//=============================================================
bool timestamp( SpidrController *spidrctrl )
//=============================================================
{    
    bool status = true;
    bool retval;

    unsigned int timer_lo1, timer_hi1;
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
            if( !retval ) {
                cout << "###getTimer: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        }
    }
    cout << "adding timestamp" << endl;
    return status;
}


//=============================================================
void *timestamp_per_sec( void *ctrl )
//=============================================================
{
    unsigned int timer_lo1, timer_hi1;
    bool status = true;
    bool retval;

    int FpgaTemp, BoardTemp, DeviceTemp;
    int dac_val1, dac_val2;
    int navg = 1;
    int cnt = 0;
    int chip = 0;
    float Tpx3Temp[2]; 

    SpidrController *spidrctrl = (SpidrController *)ctrl;
    while (1) {
        for (int dev=0; dev<ndev; dev++) {
            if ( dev_ena[dev] ) {
                retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
                if( !retval ) {
                    cout << "###getTimer: " << spidrctrl->errorString() << endl;
                    status &= retval;
                }
            }
        }
        //cout << "adding timestamp" << endl;
        int trigcntr; 
        retval = spidrctrl->getTdcTriggerCounter( &trigcntr );
        if( !retval ) {
            cout << "###getTdcTriggerCounter: " << spidrctrl->errorString() << endl;
            status &= retval;
        }

        // in millidegrees
        chip = cnt%2;
        spidrctrl->selectChipBoard( chip + 1 ); // prepare for next measurement 
        usleep(200000);

        spidrctrl->getRemoteTemp ( &FpgaTemp );
        spidrctrl->getLocalTemp ( &BoardTemp );
        spidrctrl->getFpgaTemp ( &DeviceTemp );
    
        //cout << FpgaTemp << " " << BoardTemp << " " << DeviceTemp << endl;
        
        // get the temperature of the TPX3 by reading DAC values 28 and 29
        spidrctrl->setSenseDac( chip, TPX3_BANDGAP_OUTPUT);
        usleep(200000);
        spidrctrl->getAdc( &dac_val1, navg );
        usleep(200000);
        spidrctrl->setSenseDac( chip, TPX3_BANDGAP_TEMP );
        usleep(200000);
        spidrctrl->getAdc( &dac_val2, navg );
        usleep(200000);
    
        //cout << "dac_val1: " << dac_val1 << endl;
        //cout << "dac_val2: " << dac_val2 << endl;
    
        float AdcConversion = 1500.0/4095.0; // conversion factor of ADC in mv/lsb, full scale == 1500 mV
        float BandGapOutput = (AdcConversion * dac_val1) / (float) navg / 1000.0;
        float BandGapTemp = AdcConversion * dac_val2 / (float) navg / 1000.0 ;
        //cout << "BandGapOutput " << BandGapOutput << endl;
        //cout << "BandGapTemp " << BandGapTemp << endl;
        Tpx3Temp[chip] = 88.75 - 607.3 * ( BandGapTemp - BandGapOutput );   // from Timepix3 manual
    
        //cout << "the Timepix3 temperature is: " << Tpx3Temp << endl;


        cout << "triggers: " << std::setw(6) << trigcntr << "  chip0 temp: " << Tpx3Temp[0] << "  chip1 temp : " << Tpx3Temp[1] << endl;
        cnt++;
    }
    return NULL;
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



