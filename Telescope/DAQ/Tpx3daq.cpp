// DAQ program for the Timepix3 telescope, July 2014
//

// TOOD
// - 1 sec heartbeat, needs non-blocking tcp/ip socket
// - check number of packets transmitted vs. packets received
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
void* doSomeThing( void *arg);

int  ndev;          // number of devices, supported by this software/firmware
bool dev_ena[16];  // tells if device is connected.  max 16 devices on a SPIDR, currently max 2
int  devIds[16];
char devIdstrings[16][16];

int main(int argc, char *argv[])
{
    int run_nr;
    char hostname[64];
    char fileprefix[256];
    char description[128];
    //unsigned int timer_lo1, timer_hi1;
    clock_t start_time = clock();
    clock_t cur_time = clock();
    double sec_elapsed = 0, prev_sec_elapsed = 0;
    bool run_control;
    extern char *optarg; 
    int portnumber = 51000; // for run control

    int my_socket,new_socket;
    int bufsize = 1024;
    int status;
    char *buffer = (char *)malloc(bufsize);
    char cmd[32];
    struct sockaddr_in address;
    socklen_t addrlen;
    pthread_t ts_thread;   // thread for 1 sec timestamps

    if (argc != 3) { 
        cout << "usage: Tpx3daq <-r> <port number>" << endl;
        cout << "       Tpx3daq <-s> <filename prefix>" << endl;
        cout << "       -r starts TCP/IP client on port <port number>" << endl;  
        cout << "       -s starts stand-alone DAQ and writes to file with the given filename " << endl;  
        return -1; 
    } 

    gethostname ( hostname, 64 );

    char c = getopt(argc, argv, "r:s:");
    switch (c) { 
        case 'r': run_control = true;
                  sscanf( optarg, "%d", &portnumber);
                  cout << "[Note] starting server for run control on " << hostname << ", listening on port " << portnumber << endl;
                  break; 
        case 's': run_control = false; 
                  sscanf( optarg, "%s", fileprefix);
                  cout << "[Note] starting stand-alone DAQ on " << hostname << ", with file prefix: " << fileprefix << endl;
                  break;
        default : run_control = false;
                  break;
    }

 
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
    if (!spidrctrl->setExtRefClk(true) ) 
    //if (!spidrctrl->setExtRefClk(false) ) 
        cout << "###setExtRefClk: " << spidrctrl->errorString() << endl;

    int errstat;
    if( !spidrctrl->reset( &errstat ) ) {
        cout << "###reset errorstat: " << hex << errstat << dec << endl;
    }


    // determine number of devices, does not check if devices are active
    if ( !spidrctrl->getDeviceCount( &ndev ) )
            cout << "###getDeviceCount: " << spidrctrl->errorString() << endl;
    cout << "[Note] number of devices supported by firmware: " << ndev << endl;


    // check link status
    int linkstatus;
    for (int dev=0; dev<ndev; dev++) {
        if( !spidrctrl->getLinkStatus( dev, &linkstatus ) ) {
            cout << "#getLinkStatus(): " << hex << errstat << dec << endl;
        }
        if ( linkstatus ) {
            dev_ena[dev] = true;
            cout << "[Note] enabling device " << dev << endl; 
            cout << "[Note] dev " << dev << " enabled links: " << hex << linkstatus << dec << endl;
            // get device IDs in readable form
            spidrctrl->getDeviceIds( devIds );
            //cout << hex << devIds[dev] << dec << endl;
            devId_tostring( devIds[dev], devIdstrings[dev] );
            cout << "[Note] dev " << dev << ": " << devIdstrings[dev] << endl;
           }
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

     

    // select whether or not the let the FPGA do the ToT/ToA decoding
    // gray decoding (for ToA only) has priority over LFSR decoding
    if ( !spidrctrl->setDecodersEna(true) )
    //if ( !spidrctrl->setDecodersEna(false) )
        cout << "###setDecodersEna: " << spidrctrl->errorString() << endl;


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
            
            //spidrdaq[dev]->setBufferSize( 0x00001000 ); // 16 MByte
            spidrdaq[dev]->setFlush( false ); // Don't flush when no file is open
            
            // Sample 'frames' as well as write pixel data to file
            spidrdaq[dev]->setSampling( false ); // no sampling
        }
    }

    // ----------------------------------------------------------
    // SPIDR-TPX3 and Timepix3 timers
    // ----------------------------------------------------------
    // TODO: remove?, it will be replaced by external T0-sync
    //
    if( !spidrctrl->restartTimers() )
        cout << "###restartTimers: " << spidrctrl->errorString() << endl;

    
    if ( run_control ) {

        //----------------------------------------------------------
        // run control via TCP/IP
        //----------------------------------------------------------
    
        // create socket and start listening
        if ((my_socket = socket( AF_INET, SOCK_STREAM, 0 ) ) > 0)
        //if ((my_socket = socket( AF_INET, SOCK_STREAM, O_NONBLOCK ) ) > 0)
            cout << "The socket is created" << endl;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(portnumber);
        if ( bind(my_socket,(struct sockaddr *)&address,sizeof(address)) == 0)
            cout << "Binding Socket" << endl;
        else 
            cout << "Socket binding failed" << endl;
        listen(my_socket,3);
     
        addrlen = sizeof(struct sockaddr_in);
        // wait for client to connect (blocking !)
        new_socket = accept(my_socket,(struct sockaddr *)&address,&addrlen);
        if (new_socket > 0) 
            cout << "Client " << inet_ntoa(address.sin_addr) << " is connected" << endl;
        // make socket non blocking
        //int s;
        //int flags = fcntl( my_socket,F_GETFL,0);
        //assert(flags != -1);
        //fcntl( my_socket, F_SETFL, flags | O_NONBLOCK);
     
    
        //----------------------------------------------------------
        // start command loop via run_control
        //----------------------------------------------------------
        bool cmd_recognised = false;
        int cmd_length = 0;
        char dummy[32];
        run_nr = -1;
        bool run_started = false;
        do {   

            // adds timestamp every second
            //if (run_started) {
            //    cur_time = clock();
            //    sec_elapsed = (cur_time - start_time) / (double) CLOCKS_PER_SEC;
            //    if (sec_elapsed > prev_sec_elapsed) {
            //        timestamp( spidrctrl );
            //        prev_sec_elapsed = sec_elapsed;    
            //    }
            //}

            cmd_recognised = false;
            cmd_length = recv(new_socket, buffer, bufsize, 0);
            if (cmd_length > 0) { 
                cout << "message received: " << buffer << " length " << cmd_length <<  endl;
                buffer[cmd_length] = '\0';
                sscanf(buffer, "%s", cmd);
            }
            else {
                sprintf( cmd, "no_cmd" );
            }
    
            if ( strcmp(cmd,"configure")==0 ) {
                cmd_recognised = true;
                cout << "configuring " << endl;
                status = configure( spidrctrl );
                if (status == true) {
                    sprintf(buffer,"OK done configuring");
                    cout << buffer << endl;
                }
                else {
                    sprintf(buffer,"FAILED to configure; status %d", status);
                    cout << buffer << endl;
                }
            }
    
            if ( strcmp(cmd,"start_run")==0 ) {
                cmd_recognised = true;
                sscanf(buffer, "%s %d %s", dummy, &run_nr, description);
                cout << "[Note] starting run " << run_nr << endl;

                start_time = clock();
  
                char rundir[256];
                sprintf( rundir, "Run%d/Run%d", run_nr, run_nr );
                status = start_run( spidrctrl, spidrdaq, rundir, run_nr, description);
                status = true;
                if (status == true) {
                    run_started = true;
                    sprintf(buffer,"OK run %d started", run_nr);
                    cout << "[Note] " << buffer << endl;
                    //spidrctrl->openShutter();
                }
                else {
                    status = stop_run( spidrctrl, spidrdaq);   // stop DAQ threads
                    sprintf(buffer,"FAILED start run %d status %d", run_nr, status);
                    cout << "[Error] " << buffer << endl;
                }

                // start thread for 1 sec heartbeat
                
                int cntr;
                spidrctrl->getShutterCounter( &cntr);
                cout << "shutter " << cntr << endl;
                spidrctrl->getExtShutterCounter( &cntr);
                cout << "ext shutter " << cntr << endl;
 


                
                int err = pthread_create( &ts_thread, NULL, &timestamp_per_sec, (void *) spidrctrl );
                if (err != 0)
                    cout << "[Error] Can not create timestamp thread, error: " << strerror(err) << endl;
                else
                    cout << "[Note] Timestamp thread created successfully" << endl;

                
                
            }
    
            if (strcmp(cmd,"stop_run")==0) {
                cmd_recognised = true;
                cout << "stopping run " << run_nr << endl;
                pthread_cancel( ts_thread );
                spidrctrl->closeShutter();
                int cntr;
                spidrctrl->getShutterCounter( &cntr);
                cout << "shutter " << cntr << endl;
                spidrctrl->getExtShutterCounter( &cntr);
                cout << "ext shutter " << cntr << endl;
                status = stop_run( spidrctrl, spidrdaq);

                // put back shutter mode to 0 (closeShutter sets it to 4)
                int trig_mode = 0; // external shutter
                int trig_length_us = 10000; //  us
                int trig_freq_hz = 5; // Hz
                int nr_of_trigs = 1; // 1 triggers
            
                if( !spidrctrl->setShutterTriggerConfig( trig_mode, trig_length_us,
                                                 trig_freq_hz, nr_of_trigs ) )
                    cout << "###setTriggerConfig: " << spidrctrl->errorString() << endl;


                if (status == 0) {
                    run_started = false;
                    sprintf(buffer,"OK run %d stopped", run_nr);
                    cout << "[Note] " << buffer << endl;
                }
                else {
                    sprintf(buffer,"FAILED stop run %d status %d", run_nr, status);
                    cout << "[Error] " << buffer << endl;
                }
            }
            
            if ( !cmd_recognised && (cmd_length>0) ) {
                sprintf(buffer,"FAILED unknown command");
                cout << "[Warning] " << buffer << endl;
            }
    
            if (cmd_length > 0) { // send a reply
                cout << "[Note] sending reply to client" << endl;
                send( new_socket, buffer, strlen(buffer), 0 );
            }
                //send(new_socket,buffer,bufsize,0);

            //usleep(500000); // slow down loop?
    
        } while( strcmp(buffer,"/q") ); // use /q to quit the loop
    
        close(new_socket);
        close(my_socket); 
    
    }


    //----------------------------------------------------------
    // stand-alone
    //----------------------------------------------------------
    if ( !run_control ) {

        bool status; 

        status = configure( spidrctrl ); 
 
        char prefix[256];
        sprintf( prefix, "Test/%s", fileprefix );
        sprintf(description, "test of %s", prefix);

        start_run( spidrctrl, spidrdaq, prefix, 0, description);
        int retval = spidrctrl->openShutter();
        //if ( !retval )
        //{
        //    cout << "###openShutter: " << spidrctrl->errorString() << endl;
        //    status &= retval;
        //}

         
        int maxtime = 10;
        for (int j=0 ; j<maxtime; j++) {
            sleep(1);
            timestamp( spidrctrl );
            //for (int dev=0; dev<ndev; dev++) {
            //    if ( dev_ena[dev] ) {
            //          spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
            //    }
            //}
            cout << "second " << j << " of " << maxtime << endl;
        }
    
        spidrctrl->closeShutter(); 
        stop_run ( spidrctrl, spidrdaq); 


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
            // if ( ( (devIds[dev] & 0xFFFFF) == 0x00000 ) || ( (devIds[dev] & 0xFFFFF) == 0xFFFFF ) ) // no chip ID
            //      sprintf(dacfile, "%s/default_dacs.txt", CFG_PATH);
            //else
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
                    if (numpar == 2) spidrctrl->setDac( dev, dac_nr, dac_val);
                    //cout << dac_nr <<  "  " << dac_val << endl;
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
            if( !retval )
            {
                cout << "Dev "<< dev << " ###resetPixels: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        }
    }
            
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            // Clear local pixel config
            spidrctrl->resetPixelConfig();
            // Disable all testpulse bits
            spidrctrl->setPixelTestEna( ALL_PIXELS, ALL_PIXELS, false); 
            // Enable all pixels 
            spidrctrl->setPixelMask( ALL_PIXELS, ALL_PIXELS, false ); 
        
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
                    spidrctrl->setPixelThreshold( col, row, thr);
                    if (mask) spidrctrl->setPixelMask( col, row, true ); 
                }
            }
            fclose(fp);

            spidrctrl->setPixelTestEna( 127, 127, true);
            // Write pixel config to chip
            retval = spidrctrl->setPixelConfig( dev );
            if( !retval )
            {
                cout << "Dev "<< dev << " ###setPixelConfig: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            // read back configuration
            int gen_cfg;
            retval = spidrctrl->getGenConfig( dev, &gen_cfg);
        
        }
    }
    
    
    // ----------------------------------------------------------
    // TPX3 Testpulse configuration
    // ----------------------------------------------------------

    // Test pulse and CTPR configuration
    // Timepix3 test pulse configuration
     for (int dev=0; dev<ndev; dev++) {
         if ( dev_ena[dev] ) {
             if( !spidrctrl->setTpPeriodPhase( dev, 10, 0 ) )
                 cout << "Dev "<< dev << " ###setTpPeriodPhase: " << spidrctrl->errorString() << endl;
             if( !spidrctrl->setTpNumber( dev, 1 ) )
                 cout << "Dev "<< dev << " ###setTpNumber: " << spidrctrl->errorString() << endl;
         }
     }

    // Enable test-pulses for some columns
     //int col;
     for (int dev=0; dev<ndev; dev++) {
         if ( dev_ena[dev] ) {
             for( col=0; col<256; ++col ) {
                 if( col == 127 )
                     spidrctrl->setCtprBit( col, 1 );
                 if( !spidrctrl->setCtpr( dev ) )
                    cout << "Dev "<< dev << " ###setCtpr: " << spidrctrl->errorString() << endl;
             }
         }
     }


    // ----------------------------------------------------------
    // Trigger configuration, parameter not relevant if shutter is opened/close 'manually'
    // ----------------------------------------------------------

    // Configure the shutter trigger
    //int trig_mode = 4; // SPIDR_TRIG_AUTO;
    int trig_mode = 0; // external shutter
    int trig_length_us = 10000; //  us
    int trig_freq_hz = 5; // Hz
    int nr_of_trigs = 1; // 1 triggers
    
    if( !spidrctrl->setShutterTriggerConfig( trig_mode, trig_length_us,
                                     trig_freq_hz, nr_of_trigs ) )
        cout << "###setTriggerConfig: " << spidrctrl->errorString() << endl;

    int i1, i2, i3, i4;
    if( !spidrctrl->getShutterTriggerConfig( &i1, &i2, &i3, &i4 ) )
        cout << "###setTriggerConfig: " << spidrctrl->errorString() << endl;
    cout << "read trigger config " << hex << i1 << " " << i2 << " " << i3 << " " << i4 << dec  << endl;


    // ----------------------------------------------------------
    // Acquisition mode
    // ----------------------------------------------------------
    // Set Timepix3 acquisition mode

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->setGenConfig( dev,
                                     TPX3_POLARITY_HPLUS |
                                     TPX3_ACQMODE_TOA_TOT |
                                     TPX3_GRAYCOUNT_ENA |
                                   TPX3_TESTPULSE_ENA |
                                   TPX3_SELECTTP_EXT_INT |
                                   TPX3_SELECTTP_DIGITAL |
                                     TPX3_FASTLO_ENA
                                   ); 
    
            if( !retval )
            {
                cout << "Dev "<< dev << " ###setGenCfg: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
        
            // read back configuration
            int gen_cfg;
            retval = spidrctrl->getGenConfig( dev, &gen_cfg);
            if( !retval )
            {
                cout << "Dev "<< dev << " ###getGenCfg: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            cout << "[Note] dev " << dev << " gen config " << hex << gen_cfg << dec <<  endl;
        
            // PLL configuration: 40 MHz on pixel matrix
            int pll_cfg = 0x01E;
            retval = spidrctrl->setPllConfig(dev, pll_cfg);
            if( !retval )
            {
                cout << "Dev "<< dev << " ###setPllCfg: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            retval = spidrctrl->getPllConfig(dev, &pll_cfg);
            if( !retval )
            {
                cout << "Dev "<< dev << " ###setPllCfg: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            cout << "[Note] dev " << dev << " PLL config " << hex << pll_cfg <<  dec << endl;
        }
    } 


    // Set Timepix3 into acquisition mode
    retval = spidrctrl->datadrivenReadout();
    if( !retval )
    {
        cout << "###ddrivenReadout: " << spidrctrl->errorString() << endl;
        status &= retval;
    }

    // reapply ethernet mask
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask );
            if( !retval )
            {
                cout << "Dev "<< dev << " ###getHeaderFilter: " << spidrctrl->errorString() << endl;
                status &= retval;
            }
            //cpu_mask = 0x0080;
            eth_mask = 0xFFFF;
            retval = spidrctrl->setHeaderFilter  ( dev, eth_mask,   cpu_mask );
            if( !retval )
            {
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
    int spidr_packets_sent = 0;
    int daq_packets_rec[2] = {0,0};
    int daq_packets_lost[2]= {0,0};

    cout << "[Note] starting run " << endl;
    // resetting counter for statistic of packets
    //spidrctrl->selectChipBoard(1);
    
    retval = spidrctrl->resetPacketCounters();
    if( !retval )
    {
        cout <<  "###resetPacketCounters: " << spidrctrl->errorString() << endl;
        status &= retval;
    }
  
    retval = spidrctrl->getDataPacketCounter( &spidr_packets_sent );
    if( !retval )
    {
        cout << "###getDatatPacketCounter: " << spidrctrl->errorString() << endl;
        status &= retval;
    }

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            daq_packets_rec[dev] = spidrdaq[dev]->packetsReceivedCount();
            daq_packets_lost[dev] = spidrdaq[dev]->packetsLostCount();
        }
    }
    //cout << "check rec0 " <<  spidrdaq[0]->packetsReceivedCount() << endl;
    //cout << "check rec1 " <<  spidrdaq[1]->packetsReceivedCount() << endl;
    //cout << "check lost0 " <<  spidrdaq[0]->packetsLostCount() << endl;
    //cout << "check lost1 " <<  spidrdaq[1]->packetsLostCount() << endl;

    cout << "Spidr sent ethernet packet count before run: " << spidr_packets_sent << endl;
    cout << "DAQ thread 0 receive packet count before run:   " << daq_packets_rec[0] <<  "  ( and lost " << daq_packets_lost[0] << " )" << endl;
    cout << "DAQ thread 1 receive packet count before run:   " << daq_packets_rec[1] <<  "  ( and lost " << daq_packets_lost[1] << " )" << endl;
    cout << "Missing ethernet packets before run: " << spidr_packets_sent - daq_packets_rec[0] - daq_packets_rec[1] << endl;

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            sprintf(filename, "%s/CHIP%d/%s.dat", DATA_PATH, dev, prefix);
            cout << "opening file " << filename << endl;
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

   
    return status;

}
        

//=============================================================
bool stop_run( SpidrController *spidrctrl, SpidrDaq **spidrdaq ) 
//=============================================================
{
    bool status = false;
    bool retval;
    unsigned int timer_lo1, timer_lo2, timer_hi1, timer_hi2;
    int spidr_packets_sent=0;
    int daq_packets_rec[2] = {0,0};
    int daq_packets_lost[2]= {0,0};


    timestamp( spidrctrl );
    //for (int dev=0; dev<2; dev++) {
    //    retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
    //    status |= retval;
    //}

    sleep(1);  // waiting time to collect last ethernet packets
    
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->getShutterStart(dev, &timer_lo1, &timer_hi1);
            status &= retval;
            retval = spidrctrl->getShutterEnd(dev, &timer_lo2, &timer_hi2);
            status &= retval;
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

    retval = spidrctrl->getDataPacketCounter( &spidr_packets_sent );
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            daq_packets_rec[dev] = spidrdaq[dev]->packetsReceivedCount();
            daq_packets_lost[dev] = spidrdaq[dev]->packetsLostCount();
        }
    }

    cout << "Spidr sent ethernet packet count after run: " << spidr_packets_sent << endl;
    cout << "DAQ thread 0 receive packet count after run:   " << daq_packets_rec[0] <<  "  ( and lost " << daq_packets_lost[0] << " )" << endl;
    cout << "DAQ thread 1 receive packet count after run:   " << daq_packets_rec[1] <<  "  ( and lost " << daq_packets_lost[1] << " )" << endl;
    cout << "Missing ethernet packets after run: " << spidr_packets_sent - daq_packets_rec[0] - daq_packets_rec[1] << endl;
 
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
            status &= retval;
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

    SpidrController *spidrctrl = (SpidrController *)ctrl;
    while (1) {
        for (int dev=0; dev<ndev; dev++) {
            if ( dev_ena[dev] ) {
                retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
                status &= retval;
            }
        }
        cout << "adding timestamp" << endl;
        sleep(1);
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

void* doSomeThing( void *arg)
{
    //unsigned long i = 0;
    //pthread_t id = pthread_self();

    while (1) {
        cout << "another timestamp" << endl;
    }

    return NULL;
}
