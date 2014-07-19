// 
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
bool devId_tostring ( int devId, char *devIdstring);


int main(int argc, char *argv[])
{
    int run_nr;
    char fileprefix[256];
    char description[128];
    //unsigned int timer_lo1, timer_hi1;
    clock_t start_time = clock();
    clock_t cur_time = clock();
    double sec_elapsed = 0, prev_sec_elapsed = 0;
    bool run_control;
    extern char *optarg; 
    int portnumber = 51000; // for run control


    int npackets_before;
    int npackets_after;
    int pc_packets_before_rec0;
    int pc_packets_before_rec1;
    int pc_packets_before_lost0;
    int pc_packets_before_lost1;
    int pc_packets_after_rec0;
    int pc_packets_after_rec1;
    int pc_packets_after_lost0;
    int pc_packets_after_lost1;

    int my_socket,new_socket;
    int bufsize = 1024;
    int status;
    char *buffer = (char *)malloc(bufsize);
    char cmd[32];
    struct sockaddr_in address;
    socklen_t addrlen;

    if (argc != 3) { 
        cout << "usage: Tpx3daq <-r> <port number>" << endl;
        cout << "       Tpx3daq <-s> <filename prefix>" << endl;
        cout << "       -r starts TCP/IP client on port <port number>" << endl;  
        cout << "       -s starts stand-alone DAQ and writes to file with the given filename " << endl;  
        return -1; 
    } 

    char c = getopt(argc, argv, "r:s:");
    switch (c) { 
        case 'r': run_control = true;
                  sscanf( optarg, "%d", &portnumber);
                  cout << "starting server for run control, listening on port " << portnumber << endl;
                  break; 
        case 's': run_control = false; 
                  sscanf( optarg, "%s", fileprefix);
                  cout << "starting stand-alone DAQ with file prefix " << fileprefix << endl;
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
    spidrctrl->getIpAddrDest( 0, &addr );

    cout << (int) ((addr >> 24) & 0xFF) << "  " << 
            (int) ((addr >> 16) & 0xFF) << "  " << 
            (int) ((addr >>  8) & 0xFF) << "  " << 
            (int) ((addr >>  0) & 0xFF) << endl;
 

    int errstat;
    if( !spidrctrl->reset( &errstat ) ) {
        cout << "errorstat " << hex << errstat << dec << endl;
    }

    // ----------------------------------------------------------
    // SPIDR configuration
    // ----------------------------------------------------------
    // set packet filter: ethernet no filter
    int eth_mask, cpu_mask;
    for (int dev=0; dev<2; dev++) 
    {
        if ( !spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask ) )
            cout << "###getHeaderFilter: " << spidrctrl->errorString() << endl;
        eth_mask = 0xFFFF;
        //cpu_mask = 0x0080;
        if ( !spidrctrl->setHeaderFilter  ( dev, eth_mask,   cpu_mask ) )
            cout << "###setHeaderFilter: " << spidrctrl->errorString() << endl;
        if ( !spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask ) )
            cout << "###getHeaderFilter: " << spidrctrl->errorString() << endl;
        cout << "device " << dev << hex << " eth_mask = " << eth_mask << "  cpu_mask = " << cpu_mask << dec << endl;
    }

     
    if (!spidrctrl->setExtRefClk(true) ) 
    //if (!spidrctrl->setExtRefClk(false) ) 
            cout << "###setExtRefClk: " << spidrctrl->errorString() << endl;


    // select whether or not the let the FPGA do the ToT/ToA decoding
    // gray decoding (for ToA only) has priority over LFSR decoding
    if ( !spidrctrl->setDecodersEna(true) )
    //if ( !spidrctrl->setDecodersEna(false) )
        cout << "###setDecodersEna: " << spidrctrl->errorString() << endl;


    // ----------------------------------------------------------
    // Interface to Timepix3 pixel data acquisition
    // ----------------------------------------------------------

    SpidrDaq *spidrdaq[2];
    for (int dev=0; dev<2; dev++)
    {
        spidrdaq[dev] = new SpidrDaq( spidrctrl, 0x10000000, dev);
        string errstr = spidrdaq[dev]->errorString();
        if (!errstr.empty()) 
            cout << "### SpidrDaq: " << errstr << endl; 
        
        //spidrdaq[dev]->setBufferSize( 0x00001000 ); // 16 MByte
        spidrdaq[dev]->setFlush( false ); // Don't flush when no file is open
        
        // Sample 'frames' as well as write pixel data to file
        spidrdaq[dev]->setSampling( false ); // no sampling
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
        //int flags = fcntl(s,F_GETFL,0);
        //assert(flags != -1);
        //fcntl(s, F_SETFL, flags | O_NONBLOCK);
     
    
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
            if (run_started) {
                cur_time = clock();
                sec_elapsed = (cur_time - start_time) / (double) CLOCKS_PER_SEC;
                if (sec_elapsed > prev_sec_elapsed) {
                    timestamp( spidrctrl );
                    prev_sec_elapsed = sec_elapsed;    
                }
            }

            cmd_recognised = false;
            cmd_length = recv(new_socket, buffer, bufsize, 0);
            cout << "Message received: " << buffer << " length " << cmd_length <<  endl;
            buffer[cmd_length] = '\0';
            sscanf(buffer, "%s", cmd);
    
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
                cout << "starting run " << run_nr << endl;

                start_time = clock();
  
                // resetting counter for statistic of packets
                spidrctrl->selectChipBoard(1);
                spidrctrl->resetPacketCounters();
                spidrctrl->getDataPacketCounter(&npackets_before);
                pc_packets_before_rec0 = spidrdaq[0]->packetsReceivedCount();
                pc_packets_before_rec1 = spidrdaq[1]->packetsReceivedCount();
                pc_packets_before_lost0 = spidrdaq[0]->packetsLostCount();
                pc_packets_before_lost1 = spidrdaq[1]->packetsLostCount();

                char rundir[256];
                sprintf( rundir, "Run%d/Run%d", run_nr, run_nr );
                status = start_run( spidrctrl, spidrdaq, rundir, run_nr, description);
                if (status == true) {
                    run_started = true;
                    sprintf(buffer,"OK run %d started", run_nr);
                    cout << buffer << endl;
                }
                else {
                    status = stop_run( spidrctrl, spidrdaq);   // stop DAQ threads
                    sprintf(buffer,"FAILED start run %d status %d", run_nr, status);
                    cout << buffer << endl;
                }
            }
    
            if (strcmp(cmd,"stop_run")==0) {
                cmd_recognised = true;
                cout << "stopping run " << run_nr << endl;
                status = stop_run( spidrctrl, spidrdaq);
                if (status == 0) {
                    run_started = false;
                    sprintf(buffer,"OK run %d stopped", run_nr);
                    cout << buffer << endl;
                }
                else {
                    sprintf(buffer,"FAILED stop run %d status %d", run_nr, status);
                    cout << buffer << endl;
                }
            }
            
            if ( !cmd_recognised && (cmd_length>0) ) {
                sprintf(buffer,"FAILED unknown command");
                cout << buffer << endl;
            }
    
            if (cmd_length > 0) // send a reply
                send(new_socket,buffer,bufsize,0);

            usleep(1000); // slow down loop?
    
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
        if ( status == false ) return status;

        spidrctrl->selectChipBoard(1);
        spidrctrl->resetPacketCounters();
        spidrctrl->getDataPacketCounter(&npackets_before);
        pc_packets_before_rec0 = spidrdaq[0]->packetsReceivedCount();
        pc_packets_before_rec1 = spidrdaq[1]->packetsReceivedCount();
        pc_packets_before_lost0 = spidrdaq[0]->packetsLostCount();
        pc_packets_before_lost1 = spidrdaq[1]->packetsLostCount();

 
        char prefix[256];
        sprintf( prefix, "Test/%s", fileprefix );
        sprintf(description, "test of %s", prefix);
        start_run( spidrctrl, spidrdaq, prefix, 0, description);

        int maxtime = 10;
        for (int j=0 ; j<maxtime; j++) {
            sleep(1);
            timestamp( spidrctrl );
            //for (int dev=0; dev<2; dev++) 
            //    spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
            cout << "second " << j << " of " << maxtime << endl;
        }
    
        stop_run ( spidrctrl, spidrdaq); 


        spidrctrl->getDataPacketCounter(&npackets_after);
        pc_packets_after_rec0 = spidrdaq[0]->packetsReceivedCount();
        pc_packets_after_rec1 = spidrdaq[1]->packetsReceivedCount();
        pc_packets_after_lost0 = spidrdaq[0]->packetsLostCount();
        pc_packets_after_lost1 = spidrdaq[1]->packetsLostCount();

        cout << "chip before " << npackets_before << " after " << npackets_after << endl;
        cout << "daq0 before " << pc_packets_before_rec0 << " after " << pc_packets_after_rec0 << endl;
        cout << "daq1 before " << pc_packets_before_rec1 << " after " << pc_packets_after_rec1 << endl;
        cout << "daq0 lost before " << pc_packets_before_lost0 << " after " << pc_packets_after_lost0 << endl;
        cout << "daq1 lost before " << pc_packets_before_lost1 << " after " << pc_packets_after_lost1 << endl;
        cout << "difference " << npackets_after - pc_packets_after_rec0 - pc_packets_after_rec1 << endl;

    }



    //----------------------------------------------------------
    // common
    //----------------------------------------------------------
    
    for (int dev=0; dev<2; dev++) 
        spidrdaq[dev]->stop();

    delete spidrdaq[0];
    delete spidrdaq[1];
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
    int devIds[2];
    char devIdstrings[2][16];
    char *cptr;

    // get device IDs in readable form
    spidrctrl->getDeviceIds( devIds );
    for (int dev=0; dev<2; dev++) {
        //cout << hex << devIds[dev] << dec << endl;
        devId_tostring( devIds[dev], devIdstrings[dev] );
        cout << devIdstrings[dev] << endl;
    }

    for (int dev=0; dev<2; dev++) {
        if ( ( (devIds[dev] & 0xFFFFF) == 0x00000 ) || ( (devIds[dev] & 0xFFFFF) == 0xFFFFF ) ) // no chip ID
             sprintf(dacfile, "%s/default_dacs.txt", CFG_PATH);
        else
             sprintf(dacfile, "%s/%s_dacs.txt", CFG_PATH, devIdstrings[dev]);
        FILE *fp = fopen(dacfile, "r");
        if (fp == NULL) { cout << "can not open dac file: " << dacfile << endl; return -1;}
        else cout << "reading " << dacfile << endl;
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


    // ----------------------------------------------------------
    // TPX3 Pixel configuration + thresholds
    // ----------------------------------------------------------
    int row, col, thr, mask, tp_ena;
    char trimfile[256];

    for (int dev=0; dev<2; dev++) {
        // Clear pixel configuration in chip
        retval = spidrctrl->resetPixels( dev );
        if( !retval )
        {
            cout << "###resetPixels: " << spidrctrl->errorString() << endl;
            status &= retval;
        }
        
        // Clear local pixel config
        spidrctrl->resetPixelConfig();
        // Disable all testpulse bits
        spidrctrl->setPixelTestEna( ALL_PIXELS, ALL_PIXELS, false); 
        // Enable all pixels 
        spidrctrl->setPixelMask( ALL_PIXELS, ALL_PIXELS, false ); 
    
        // read thresholds from file
        char *charptr; 
    
        if ( ( (devIds[dev] & 0xFFFFF) == 0x00000 ) || ( (devIds[dev] & 0xFFFFF) == 0xFFFFF ) ) // no chip ID
             sprintf(trimfile, "%s/default_trimdacs.txt", CFG_PATH);
        else
             sprintf(trimfile, "%s/%s_trimdacs.txt", CFG_PATH, devIdstrings[dev]);
        FILE *fp = fopen(trimfile, "r");
        if (fp == NULL) { cout << "can not open trim-dac file: " << trimfile << endl; return -1;}
        else cout << "reading " << trimfile << endl; 
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

        // Write pixel config to chip
        retval = spidrctrl->setPixelConfig( dev );
        if( !retval )
        {
            cout << "###setPixelConfig: " << spidrctrl->errorString() << endl;
            status &= retval;
        }
    }

    
    // ----------------------------------------------------------
    // TPX3 Testpulse configuration
    // ----------------------------------------------------------

    // Test pulse and CTPR configuration
    // Timepix3 test pulse configuration
    // for (int dev=0; dev<2; dev++) {
    //     if( !spidrctrl->setTpPeriodPhase( dev, 10, 0 ) )
    //         cout << "###setTpPeriodPhase: " << spidrctrl->errorString() << endl;
    //     if( !spidrctrl->setTpNumber( dev, 1 ) )
    //         cout << "###setTpNumber: " << spidrctrl->errorString() << endl;
    // }

    // Enable test-pulses for some columns
    // int col;
    // for (int dev=0; dev<2; dev++) {
    //     for( col=0; col<256; ++col )
    //         if( col >= 10 && col < 11 )
    //             spidrctrl->configCtpr( dev, col, 1 );
    //         if( !spidrctrl->setCtpr( dev ) )
    //            cout << "###setCtpr: " << spidrctrl->errorString() << endl;
    // }


    // ----------------------------------------------------------
    // Trigger configuration, parameter not relevant if shutter is opened/close 'manually'
    // ----------------------------------------------------------

    // Configure the shutter trigger
    //int trig_mode = 4; // SPIDR_TRIG_AUTO;
    //int trig_length_us = 10000; //  us
    //int trig_freq_hz = 5; // Hz
    //int nr_of_trigs = 1; // 1 triggers
    
    //if( !spidrctrl->setTriggerConfig( trig_mode, trig_length_us,
    //                                 trig_freq_hz, nr_of_trigs ) )
    //    cout << "###setTriggerConfig: " << spidrctrl->errorString() << endl;


    // ----------------------------------------------------------
    // Acquisition mode
    // ----------------------------------------------------------
    // Set Timepix3 acquisition mode

    for (int dev=0; dev<2; dev++) {
        retval = spidrctrl->setGenConfig( dev,
                                     TPX3_POLARITY_HPLUS |
                                     TPX3_ACQMODE_TOA_TOT |
                                     TPX3_GRAYCOUNT_ENA |
    //                               TPX3_TESTPULSE_ENA |
    //                               TPX3_SELECTTP_EXT_INT |
    //                               TPX3_SELECTTP_DIGITAL
                                     TPX3_FASTLO_ENA
                                   ); 




        if( !retval )
        {
            cout << "###setGenCfg: " << spidrctrl->errorString() << endl;
            status &= retval;
        }
    
        // read back configuration
        int gen_cfg;
        retval = spidrctrl->getGenConfig( dev, &gen_cfg);
        if( !retval )
        {
            cout << "###getGenCfg: " << spidrctrl->errorString() << endl;
            status &= retval;
        }
        cout << "device " << dev << " gen config " << hex << gen_cfg << dec <<  endl;
    
        // PLL configuration: 40 MHz on pixel matrix
        int pll_cfg = 0x01E;
        retval = spidrctrl->setPllConfig(dev, pll_cfg);
        if( !retval )
        {
            cout << "###setPllCfg: " << spidrctrl->errorString() << endl;
            status &= retval;
        }
        retval = spidrctrl->getPllConfig(dev, &pll_cfg);
        if( !retval )
        {
            cout << "###setPllCfg: " << spidrctrl->errorString() << endl;
            status &= retval;
        }
        cout << "device " << dev << " PLL config " << hex << pll_cfg <<  dec << endl;
    }


    // Set Timepix3 into acquisition mode
    retval = spidrctrl->datadrivenReadout();
    if( !retval )
    {
        cout << "###ddrivenReadout: " << spidrctrl->errorString() << endl;
        status &= retval;
    }

    // reapply ethernet mask
    for (int dev=0; dev<2; dev++) 
    {
        retval = spidrctrl->getHeaderFilter  ( dev, &eth_mask, &cpu_mask );
        if( !retval )
        {
            cout << "###getHeaderFilter: " << spidrctrl->errorString() << endl;
            status &= retval;
        }
        eth_mask = 0xFFFF;
        retval = spidrctrl->setHeaderFilter  ( dev, eth_mask,   cpu_mask );
        if( !retval )
        {
            cout << "###setHeaderFilter: " << spidrctrl->errorString() << endl;
            status &= retval;
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

    spidrctrl->resetPacketCounters();

    for (int dev=0; dev<2; dev++) 
    {
        sprintf(filename, "%s/CHIP%d/%s.dat", DATA_PATH, dev, prefix);
        cout << "opening file " << filename << endl;
        retval = spidrdaq[dev]->startRecording( filename, run_nr, description );
        if ( !retval ) {
            cout << "###startRecording: " << spidrctrl->errorString() << endl;
            status &= retval;
        }
        retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
        if ( !retval ) {
            cout << "###getTimer: " << spidrctrl->errorString() << endl;
            //status &= retval;
        }
    }

    retval = spidrctrl->openShutter();
    if( !retval )
    {
        cout << "###openShutter: " << spidrctrl->errorString() << endl;
        status &= retval;
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


    spidrctrl->closeShutter(); 

    timestamp( spidrctrl );
    //for (int dev=0; dev<2; dev++) {
    //    retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
    //    status |= retval;
    //}

    sleep(1);  // waiting time to collect last ethernet packets
    
    for (int dev=0; dev<2; dev++) 
    {
        retval = spidrctrl->getShutterStart(dev, &timer_lo1, &timer_hi1);
        status |= retval;
        retval = spidrctrl->getShutterEnd(dev, &timer_lo2, &timer_hi2);
        status |= retval;
        //cout << dec << "shutter timer high : " << (timer_hi2 - timer_hi1)*25E-9 << endl; 
        //cout << "shutter timer low : " << (timer_lo2 - timer_lo1)*25E-9 << endl; 
        //cout << "shutter timer_lo1 " << timer_lo1 << "  shutter timer_lo2 " << timer_lo2 << endl;
    }

    for (int dev=0; dev<2; dev++) {
        retval = spidrdaq[dev]->stopRecording();
        status |= retval;
    }
 
    return status;
}

//=============================================================
bool timestamp( SpidrController *spidrctrl ) 
//=============================================================
{    
    bool status = true;
    bool retval;

    unsigned int timer_lo1, timer_hi1;
    for (int dev=0; dev<2; dev++) {
        retval = spidrctrl->getTimer(dev, &timer_lo1, &timer_hi1);  // adds timestamp to datafile
        status |= retval;
    }
    return status;
}

//=============================================================
bool devId_tostring ( int devId, char *devIdstring)
//=============================================================
{
    int waferno = (devId >> 8) & 0xFFF;
    int id_y = (devId >> 4) & 0xF;
    int id_x = (devId >> 0) & 0xF;
    sprintf(devIdstring,"W%04d_%c%02d", waferno, (char)id_x+64, id_y);  // make readable device identifier
    return true;
}

