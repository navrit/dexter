// DAQ program for the Timepix3 telescope, July 2014
//

// TOD0
// - check number of packets transmitted vs. packets received
// - clean up code
// - check all return values

#include <unistd.h>
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
// Convenience macros
#define cout_spidr_err(str) cout<<str<<": "<<spidrctrl->errorString()<<endl
#define cout_spidrdev_err(dev,str) cout<<"Dev "<<dev<<" "<<str<<": "<<spidrctrl->errorString()<<endl
#define cout_daqdev_err(dev,str) cout<<"Dev "<<dev<<" "<<str<<": "<<spidrdaq[dev]->errorString()<<endl

bool configure( SpidrController *spidrctrl );
bool start_run( SpidrController *spidrctrl, SpidrDaq **spidrdaq, char *prefix, int run_nr, char *description);
bool stop_run( SpidrController *spidrctrl, SpidrDaq **spidrdaq );
bool timestamp( SpidrController *spidrctrl );
void *timestamp_per_sec( void *rctrl );  // to run in a separate thread
bool devId_tostring ( int devId, char *devIdstring);

int  ndev;            // number of devices, supported by this software/firmware
int  ndev_active = 0; // number of connected devices, determined by activity on links
bool dev_ena[16];     // tells if device is connected.  max 16 devices on a SPIDR, currently max 2
int  devIds[16];
char devIdstrings[16][16];
bool run_control;
int  spidr_pixel_packets_sent[2] = {0,0};    // before and after
int  spidr_data_packets_sent[2] = {0,0};     // before and after
int  spidr_mon_packets_sent[2] = {0,0};      // before and after
int  spidr_pause_packets_rec[2] = {0,0};     // before and after
int  daq_packets_rec[2][2] = {{0,0},{0,0}};  // 2 devices before and after
int  daq_packets_lost[2][2]= {{0,0},{0,0}};  // 2 devices before and after
int my_socket, new_socket;
SpidrDaq *spidrdaq[2];

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
    extern char *optarg; 
    int portnumber = 51000; // for run control

    int bufsize = 1024;
    bool status;
    char *buffer = (char *)malloc(bufsize);
    char cmd[32];
    struct sockaddr_in address;
    socklen_t addrlen;
    pthread_t ts_thread = 0;   // thread for 1 sec timestamps

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
    // Open a control connection to SPIDR - TPX3 module
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
      cout_spidr_err( "###getIpAddrDest" );

    // first select internal or external clock
    if (!spidrctrl->setExtRefClk(true) ) 
    //if (!spidrctrl->setExtRefClk(false) ) 
      cout_spidr_err( "###setExtRefClk" );

    int errstat;
    if( !spidrctrl->reset( &errstat ) ) {
        cout << "###reset errorstat: " << hex << errstat << dec << endl;
    }

    // determine number of devices, does not check if devices are active
    if ( !spidrctrl->getDeviceCount( &ndev ) )
      cout_spidr_err( "###getDeviceCount" );
    cout << "[Note] number of devices supported by firmware: " << ndev << endl;

    // check link status
    int linkstatus;
    for (int dev=0; dev<ndev; dev++) {
        if( !spidrctrl->getLinkStatus( dev, &linkstatus ) ) {
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
            if ( !spidrctrl->getDeviceIds( devIds ) )
	      cout_spidr_err( "###getDevidIds()" );
            //cout << hex << devIds[dev] << dec << endl;
            devId_tostring( devIds[dev], devIdstrings[dev] );
            cout << "[Note] dev " << dev << ": " << devIdstrings[dev] << endl;
           }
	else {
	  cout << "###linkstatus: " << hex << linkstatus << dec << endl;
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
	      cout_spidrdev_err( dev, "###getHeaderFilter" );
            eth_mask = 0xFFFF;
            //cpu_mask = 0x0080;
            if ( !spidrctrl->setHeaderFilter  ( dev, eth_mask,   cpu_mask ) )
	      cout_spidrdev_err( dev, "###setHeaderFilter" );;
            if ( !spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask ) )
	      cout_spidrdev_err( dev, "###getHeaderFilter" );;
            cout << "[Note] dev " << dev << hex << " eth_mask = " << eth_mask << "  cpu_mask = " << cpu_mask << dec << endl;
        }
    }
     
    // select whether or not the let the FPGA do the ToT/ToA decoding
    // gray decoding (for ToA only) has priority over LFSR decoding
    if ( !spidrctrl->setDecodersEna(true) )
    //if ( !spidrctrl->setDecodersEna(false) )
      cout_spidr_err( "###setDecodersEna" );


    // ----------------------------------------------------------
    // Interface to Timepix3 pixel data acquisition
    // ----------------------------------------------------------

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            spidrdaq[dev] = new SpidrDaq( spidrctrl, 0x10000000, dev);
            string errstr = spidrdaq[dev]->errorString();
            if (!errstr.empty()) 
                cout << "Dev "<< dev << " ### SpidrDaq: " << errstr << endl; 
            
            //spidrdaq[dev]->setBufferSize( 0x00001000 ); // 16 MByte
            spidrdaq[dev]->setFlush( false ); // Don't flush when no file is open
            
            spidrdaq[dev]->setSampling( false ); // no sampling
        }
    }

    // ----------------------------------------------------------
    // SPIDR-TPX3 and Timepix3 timers
    // ----------------------------------------------------------
    // TODO: remove?, it will be replaced by external T0-sync
    //
    if( !spidrctrl->restartTimers() )
      cout_spidr_err( "###restartTimers" );

    
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
            cout << "Client " << inet_ntoa(address.sin_addr) << " is connected" << endl << endl;
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
        prev_run_nr = -1;
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

            cmd_length = recv(new_socket, buffer, bufsize, 0);
            cmd_recognised = false;
            if (cmd_length > 0) { 
                buffer[cmd_length] = '\0';
                cout << "------------------------------------------" << endl;
                cout << "message received: " << buffer << endl;  // " length " << cmd_length <<  endl;
                cout << "------------------------------------------" << endl;
                sscanf(buffer, "%s", cmd);
            }
            else {
                sprintf( cmd, "no_cmd" );
            }
    
            if ( strcmp(cmd,"start_mon")==0 ) {
                cmd_recognised = true;
                cout << "[Note] starting monitoring " << run_nr << endl;
                int err = pthread_create( &ts_thread, NULL, &timestamp_per_sec, (void *) spidrctrl );
                if (err != 0)
                    cout << "[Error] Can not create timestamp thread, error: " << strerror(err) << endl;
                else
                    cout << "[Note] Timestamp thread created successfully" << endl;
            }

            if ( strcmp(cmd,"stop_mon")==0 ) {
                cmd_recognised = true;
                cout << "[Note] stopping monitoring " << run_nr << endl;
                pthread_cancel( ts_thread );
            }

            if ( strcmp(cmd,"configure")==0 ) {
                cmd_recognised = true;
                cout << "[Note] configuring ..." << endl;
                status = configure( spidrctrl );
                if (status == true) {
                    sprintf(buffer,"OK done configuring");
                    cout << "[Note] " << buffer << endl;
                }
                else {
                    sprintf(buffer,"FAILED to configure; status %d", status);
                    cout << "[Error] " << buffer << endl;
                }
            }
    
            if ( strcmp(cmd,"start_run")==0 ) {
                cmd_recognised = true;
                char run_nr_str[16];
                sscanf(buffer, "%s %s", dummy, run_nr_str);
                char *ptr = strstr( buffer, run_nr_str); // point to first char in run_nr
                int len = strlen(run_nr_str);  
                strcpy( description, ptr+len+1);  // read in rest of string as comment
                sscanf( run_nr_str, "%d", &run_nr);
		/* NO LONGER NECESSARY (HB, 29 sep 2014)
                if ( run_nr != prev_run_nr ) { // reset fileCntr to 1
                    for (int dev=0; dev<ndev; dev++) {
                        if ( dev_ena[dev] ) {
                            spidrdaq[dev]->setFileCntr( 1 );
                        }
                    }
                    prev_run_nr = run_nr;
                }
		*/
                    
                cout << "[Note] starting run " << run_nr << endl;

                char rundir[256];
                sprintf( rundir, "Run%d/", run_nr );
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
                    run_started = false;
                    sprintf(buffer,"FAILED start run %d status %d", run_nr, status);
                    cout << "[Error] " << buffer << endl;
                }

                int extcntr, intcntr;
                if ( !spidrctrl->getExtShutterCounter( &extcntr) )
		  cout_spidr_err( "###getExtShutterCounter" );;
                cout << "[Note] External shutter counter: " << extcntr << endl;
                if ( !spidrctrl->getShutterCounter( &intcntr) )
		  cout_spidr_err( "###getShutterCounter" );
                cout << "[Note] Number of shutters given " << intcntr << endl;
                if ( extcntr != intcntr ) 
                    cout << "[Error] mismatch in shutter counters" << endl;
                else 
                    cout << "[Note] shutter counters match" << endl;

                
                // start thread for 1 sec heartbeat
                
                int err = pthread_create( &ts_thread, NULL, &timestamp_per_sec, (void *) spidrctrl );
                if (err != 0)
                    cout << "[Error] Can not create timestamp thread, error: " << strerror(err) << endl;
                else
                    cout << "[Note] Timestamp thread created successfully" << endl;                
            }
    
            if (strcmp(cmd,"stop_run")==0) {
                cmd_recognised = true;
                cout << "[Note] stopping run " << run_nr << endl;

                if (ts_thread) pthread_cancel( ts_thread );
                if ( !spidrctrl->closeShutter() )
		  cout_spidr_err( "###closeShutter" );

                status = stop_run( spidrctrl, spidrdaq);

                // put back shutter mode to 0 (closeShutter sets it to 4)
                int trig_mode = 0; // external shutter
                int trig_length_us = 10000; //  us
                int trig_freq_hz = 5; // Hz
                int nr_of_trigs = 1; // 1 triggers
            
                if( !spidrctrl->setShutterTriggerConfig( trig_mode, trig_length_us,
                                                 trig_freq_hz, nr_of_trigs ) )
		  cout_spidr_err( "###setShutterTriggerConfig" );


                if (status == true) {
                    run_started = false;
                    sprintf(buffer,"OK run %d stopped", run_nr);
                    cout << "[Note] " << buffer << endl;
                }
                else {
                    sprintf(buffer,"FAILED stop run %d status %d", run_nr, status);
                    cout << "[Error] " << buffer << endl;
                }

                int extcntr, intcntr;
                if ( !spidrctrl->getExtShutterCounter( &extcntr) )
		  cout_spidr_err( "###getExtShutterCounter" );
                cout << "[Note] External shutter counter: " << extcntr << endl;
                if ( !spidrctrl->getShutterCounter( &intcntr) )
		  cout_spidr_err( "###getShutterCounter" );
                cout << "[Note] Number of shutters given " << intcntr << endl;
                if ( extcntr != intcntr ) 
                    cout << "[Error] mismatch in shutter counters" << endl;
                else 
                    cout << "[Note] shutter counters match" << endl;

            }
            
            if ( !cmd_recognised && (cmd_length>0) ) {
                sprintf(buffer,"FAILED unknown command");
                cout << "[Warning] " << buffer << endl;
            }
    
            if (cmd_length > 0) { // send a reply
                cout << "[Note] sending reply to client" << endl;
                send( new_socket, buffer, strlen(buffer), 0 );
            }

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
        bool retval;

        status = configure( spidrctrl ); 
 
        char prefix[256];
        sprintf( prefix, "Test/%s", fileprefix );
        sprintf(description, "test of %s", prefix);

        start_run( spidrctrl, spidrdaq, prefix, 0, description);
        retval = spidrctrl->openShutter();
        if ( !retval )
        {
	    cout_spidr_err( "###openShutter" );
            status &= retval;
        }

         
        int maxtime = 20;
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
    
        retval = spidrctrl->closeShutter(); 
        {
	    cout_spidr_err( "###closeShutter" );
            status &= retval;
        }
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
                    if (numpar == 2) {
                        retval = spidrctrl->setDac( dev, dac_nr, dac_val);
                        if ( !retval ) {
			    cout_spidrdev_err( dev, "###setDac" );
                            status = false;
                        }
                    }
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
            if( !retval ) {
	        cout_spidrdev_err( dev, "###resetPixels" );
                status = false;
            }
        }
    }
            
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            // Clear local pixel config (testpulse disabled, not masked, threshold=0), no return value
            spidrctrl->resetPixelConfig() ;
        
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
		        cout_spidrdev_err( dev, "###setPixelThreshold" );
                        status = false;
                    }

                    if (mask) {
                        retval = spidrctrl->setPixelMask( col, row, true );
                        if ( !retval ) {
			    cout_spidrdev_err( dev, "###setPixelMask" );
                            status = false;
                        }
                    }
                }
            }
            fclose(fp);

            //retval = spidrctrl->setPixelTestEna( 127, 127, true);
            //if( !retval ) {
            //    cout_spidrdev_err( dev, "###setPixelTestEna" );
            //    status = false;
            //}
            // Write pixel config to chip
            retval = spidrctrl->setPixelConfig( dev );
            if( !retval ) {
                if ( strstr(spidrctrl->errorString().c_str(), "ERR_UNEXP") ) ; // known feature, do nothing
                else {
		    cout_spidrdev_err( dev, "###setPixelConfig" );
                    status = false;
                }
            }
            
            // read back configuration
            int gen_cfg;
            retval = spidrctrl->getGenConfig( dev, &gen_cfg);
            if( !retval ) {
	        cout_spidrdev_err( dev, "###getGenConfig" );
                status = false;
            }
        
        }
    }
    //cout << "masking pixels" << endl;
    //retval = spidrctrl->setSpidrReg( 0x394, 0x9c50);
    //retval = spidrctrl->setSpidrRegBit( 0x394, 16, true);
    //retval = spidrctrl->setSpidrReg( 0x398, 0x9a54 );
    //retval = spidrctrl->setSpidrRegBit( 0x398, 16, true);
    
    
    // ----------------------------------------------------------
    // TPX3 Testpulse configuration
    // ----------------------------------------------------------

    // Test pulse and CTPR configuration
    // Timepix3 test pulse configuration
     for (int dev=0; dev<ndev; dev++) {
         if ( dev_ena[dev] ) {
             retval = spidrctrl->setTpPeriodPhase( dev, 10, 0 );
             if( !retval ) {
	         cout_spidrdev_err( dev, "###setTpPeriodPhase" );
                 status = false;
             }
             retval = spidrctrl->setTpNumber( dev, 1 );
             if( !retval ) {
	         cout_spidrdev_err( dev, "###setTpNumber" );
                 status = false;
             }
         }
     }

    // Enable test-pulses for some columns
     //int col;
     for (int dev=0; dev<ndev; dev++) {
         if ( dev_ena[dev] ) {
             for( col=0; col<256; ++col ) {
                 if( col == 127 )
                     spidrctrl->setCtprBit( col, 1 );
             }
	     retval = spidrctrl->setCtpr( dev );
	     if( !retval ) {
	         cout_spidrdev_err( dev, "###setCtpr" );
	         status = false;
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
    
    retval = spidrctrl->setShutterTriggerConfig( trig_mode, trig_length_us,
                                     trig_freq_hz, nr_of_trigs );
    if ( !retval ) {
        cout_spidr_err( "###setShutterTriggerConfig" );
        status = false;
    }

    int i1, i2, i3, i4;
    retval = spidrctrl->getShutterTriggerConfig( &i1, &i2, &i3, &i4 );
    if ( !retval ) {
        cout_spidr_err( "###getShutterTriggerConfig" );
        status = false;
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
                                     TPX3_ACQMODE_TOA_TOT |
                                     TPX3_GRAYCOUNT_ENA |
                                   TPX3_TESTPULSE_ENA |
                                   TPX3_SELECTTP_EXT_INT |
                                   TPX3_SELECTTP_DIGITAL |
                                   TPX3_FASTLO_ENA
                                   ); 
    
            if( !retval ) {
	        cout_spidrdev_err( dev, "###setGenCfg" );
                status = false;
            }
        
            // read back configuration
            int gen_cfg;
            retval = spidrctrl->getGenConfig( dev, &gen_cfg);
            if( !retval ) {
	        cout_spidrdev_err( dev, "###getGenConfig" );
                status = false;
            }
            cout << "[Note] dev " << dev << " gen config " << hex << gen_cfg << dec <<  endl;
        
            // PLL configuration: 40 MHz on pixel matrix
            //int pll_cfg = 0x01E;
            int pll_cfg = 0x01E | 0x100;  // 40 MHz, 16 clock phases
            retval = spidrctrl->setPllConfig(dev, pll_cfg);
            if( !retval ) {
	        cout_spidrdev_err( dev, "###setPllConfig" );
                status = false;
            }
            retval = spidrctrl->getPllConfig(dev, &pll_cfg);
            if( !retval ) {
	        cout_spidrdev_err( dev, "###setPllConfig" );
                status = false;
            }
            cout << "[Note] dev " << dev << " PLL config " << hex << pll_cfg <<  dec << endl;
        }
    } 


    // Set Timepix3 into acquisition mode
    retval = spidrctrl->datadrivenReadout();
    if( !retval ) {
        cout_spidr_err( "###datadrivenReadout" );
        status = false;
    }

    // reapply ethernet mask
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask );
            if( !retval ) {
	        cout_spidrdev_err( dev, "###getHeaderFilter" );
                status = false;
            }
            //cpu_mask = 0x0080;
            eth_mask = 0xFFFF;
            retval = spidrctrl->setHeaderFilter  ( dev, eth_mask,   cpu_mask );
            if( !retval ) {
	        cout_spidrdev_err( dev, "###setHeaderFilter" );
                status = false;
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
    bool status = true;
    bool retval;
    char filename[256];
    char dacfile[256];
    char dacfile_out[256];
    char line[256];

    cout << "[Note] starting run " << endl;

    // reset shutter counters a.o.
    retval = spidrctrl->resetCounters();
    if( !retval ) {
        cout_spidr_err( "###resetCounters" );
        status = false;
    }
    
    // resetting counter for statistic of packets
    retval = spidrctrl->resetPacketCounters();
    if( !retval ) {
        cout_spidr_err( "###resetPacketCounters" );
        status = false;
    }
  
    retval = spidrctrl->getDataPacketCounter( &spidr_data_packets_sent[0] );
    if( !retval ) {
        cout_spidr_err( "###getDataPacketCounter" );
        status = false;
    }
    retval = spidrctrl->getMonPacketCounter( &spidr_mon_packets_sent[0] );
    if( !retval ) {
        cout_spidr_err( "###getMonPacketCounter" );
        status = false;
    }
    retval = spidrctrl->getPixelPacketCounter( &spidr_pixel_packets_sent[0] );
    if( !retval ) {
        cout_spidr_err( "###getPixelPacketCounter" );
        status = false;
    }
    retval = spidrctrl->getPausePacketCounter( &spidr_pause_packets_rec[0] );
    if( !retval ) {
        cout_spidr_err( "###getPausePacketCounter" );
        status = false;
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
            retval = spidrdaq[dev]->startRecording( filename, run_nr, description, true );
            if ( !retval ) {
	        cout_daqdev_err( dev, "###startRecording" );
                status = false;
            }
            retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
            if ( !retval ) {
	        cout_spidrdev_err( dev, "###getTimer" );
                status = false;
            }
        }
    }

    if ( run_control ) {
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
    }
   
    return status;

}
        

//=============================================================
bool stop_run( SpidrController *spidrctrl, SpidrDaq **spidrdaq ) 
//=============================================================
{
    bool status = true;
    bool retval;
    unsigned int timer_lo1, timer_lo2, timer_hi1, timer_hi2;

    timestamp( spidrctrl );
    //for (int dev=0; dev<2; dev++) {
    //    retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
    //    status &&= retval;
    //}

    sleep(1);  // waiting time to collect last ethernet packets
    
    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrctrl->getShutterStart(dev, &timer_lo1, &timer_hi1);
            if ( !retval ) {
	        cout_spidrdev_err( dev, "###getShutterStart" );
                status = false;
            }
            retval = spidrctrl->getShutterEnd(dev, &timer_lo2, &timer_hi2);
            if ( !retval ) {
	        cout_spidrdev_err( dev, "###getShutterEnd" );
                status = false;
            }
            //cout << dec << "shutter timer high : " << (timer_hi2 - timer_hi1)*25E-9 << endl; 
            //cout << "shutter timer low : " << (timer_lo2 - timer_lo1)*25E-9 << endl; 
            //cout << "shutter timer_lo1 " << timer_lo1 << "  shutter timer_lo2 " << timer_lo2 << endl;
        }
    }

    for (int dev=0; dev<ndev; dev++) {
        if ( dev_ena[dev] ) {
            retval = spidrdaq[dev]->stopRecording();
	    if ( !retval ) {
	      cout_daqdev_err( dev, "###stopRecording" );
	      status = false;
	    }
        }
    }

    retval = spidrctrl->getDataPacketCounter( &spidr_data_packets_sent[1] );
    if( !retval ) {
        cout_spidr_err( "###getDatatPacketCounter" );
        status = false;
    }
    retval = spidrctrl->getMonPacketCounter( &spidr_mon_packets_sent[1] );
    if( !retval ) {
        cout_spidr_err( "###getMonPacketCounter" );
        status = false;
    }
    retval = spidrctrl->getPixelPacketCounter( &spidr_pixel_packets_sent[1] );
    if( !retval ) {
        cout_spidr_err( "###getPixelPacketCounter" );
        status = false;
    }
    retval = spidrctrl->getPausePacketCounter( &spidr_pause_packets_rec[1] );
    if( !retval ) {
        cout_spidr_err( "###getPausePacketCounter" );
        status = false;
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
	        cout_spidr_err( "###getTimer" );
                status = false;
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
    int adc_val1, adc_val2;
    int navg = 1;
    int cnt = 0;
    int chip = 0;
    float Tpx3Temp[2]; 
    char buffer[1024];
    long long bytecount[2];

    SpidrController *spidrctrl = (SpidrController *)ctrl;
    while (1) {
        for (int dev=0; dev<ndev; dev++) {
            if ( dev_ena[dev] ) {
                retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
                if( !retval ) {
		    cout_spidr_err( "###getTimer" );
                    status = false;
                }
            }
        }
        //cout << "adding timestamp" << endl;
        int trigcntr; 
        retval = spidrctrl->getTdcTriggerCounter( &trigcntr );
        if( !retval ) {
	    cout_spidr_err( "###getTdcTriggerCounter" );
            status = false;
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
        spidrctrl->getAdc( &adc_val1, navg );
        usleep(200000);
        spidrctrl->setSenseDac( chip, TPX3_BANDGAP_TEMP );
        usleep(200000);
        spidrctrl->getAdc( &adc_val2, navg );
        usleep(200000);
    
        //cout << "adc_val1: " << adc_val1 << endl;
        //cout << "adc_val2: " << adc_val2 << endl;
    
        float AdcConversion = 1500.0/4095.0; // conversion factor of ADC in mv/lsb, full scale == 1500 mV
        float BandGapOutput = (AdcConversion * adc_val1) / (float) navg / 1000.0;
        float BandGapTemp = AdcConversion * adc_val2 / (float) navg / 1000.0 ;
        //cout << "BandGapOutput " << BandGapOutput << endl;
        //cout << "BandGapTemp " << BandGapTemp << endl;
        Tpx3Temp[chip] = 88.75 - 607.3 * ( BandGapTemp - BandGapOutput );   // from Timepix3 manual

        for (int dev=0; dev<ndev; dev++) {
            if ( dev_ena[dev] ) {
                bytecount[dev] = spidrdaq[dev]->bytesReceivedCount();
            }
        }
    
        sprintf(buffer, "triggers: %6d   chip0 temp: %.1f   chip1 temp: %.1f   chip0 bytecount: %lld   chip1 bytecount: %lld", trigcntr, Tpx3Temp[0], Tpx3Temp[1], bytecount[0], bytecount[1]);
        send( new_socket, buffer, strlen(buffer), 0 );

        cout << buffer << endl;
        //cout << "triggers: " << std::setw(6) << trigcntr << "  chip0 temp: " << Tpx3Temp[0] << "  chip1 temp : " << Tpx3Temp[1] << endl;
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
