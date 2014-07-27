

/*This is the Run control script.                       
Written by University of Bristol, Sophie Richards                            
For use at the July PS testbeam.
The connections are between the TLU, and the four DAQs
If more servers are needed it is easy to add or comment out...
*/                                                                                                            

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <iostream>
#include <arpa/inet.h>
#include <errno.h>

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

#define MAXSERVERS 8   // max number of DAQ machines, 4 for July

int main(int argc, char *argv[])
{
     
    struct sockaddr_in  tluAddr;
    struct sockaddr_in daqAddr[MAXSERVERS];
    int tluSockfd = 0;
    int daqSockfd[MAXSERVERS];
    int numDaqConn = 0;   // number of available DAQ connections according to config file
    int numDaqServers = 0;   // number of DAQ servers responding 
    bool tluServerActive;
    bool daqServerActive[MAXSERVERS];

    //int sockfd, sockfd2;// sockfd3, sockfd4, sockfd5;
    //struct sockaddr_in serv_addr, serv_addr2;// serv_addr3, serv_addr4, serv_addr5;
    //struct hostent *server, *server2;// *server3, *server4, *server5;
    int buffersize = 1024;    
    char buffer[buffersize];   // message buffer to/from servers
    char rcbuffer[buffersize]; // message buffer from run control command line
    char tluCmd[32];
    int  retval;
   


    //The IP address can now be read in from a file as well as the portnumber.( the port number will not change for each connection but the ip address will.
    int portnumber;
    char line[128];
    char ipno[128];
    char name[128];
    FILE *fp = fopen("ServerAddressTable.txt","r");
 
    if (fp != NULL)  
    { 
        while ( fgets(line, 128 , fp) )
        {
            if (line[0] != '#')
	    {	    
                sscanf(line, "%s %d %s", ipno, &portnumber, name);

                if ( strstr( name, "TLU") ) 
                {
                    tluSockfd = socket(AF_INET, SOCK_STREAM, 0);
                    if ( tluSockfd < 0) 
                    {
                        cout << "[Error] failed to create TLU socket" << endl; 
                        return -1; 
                    }
                    cout << "[Note] creating TLU socket at ip number " << ipno << endl;
                    tluAddr.sin_family = AF_INET;
                    tluAddr.sin_port = htons(portnumber);
                    inet_aton( ipno, &(tluAddr.sin_addr) );
                }

                if ( strstr( name, "DAQ") ) 
                {
                    daqSockfd[numDaqConn] = socket(AF_INET, SOCK_STREAM, 0);
                    if ( daqSockfd[numDaqConn] < 0) 
                    {
                        cout << "[Error] failed to create DAQ socket " << numDaqConn << endl; 
                        return -1; 
                    }
                    cout << "[Note] creating DAQ socket " << numDaqConn << " at ip number " << ipno << endl; 
                    daqAddr[numDaqConn].sin_family = AF_INET;
                    daqAddr[numDaqConn].sin_port = htons(portnumber);
                    inet_aton( ipno, &daqAddr[numDaqConn].sin_addr );
                    numDaqConn++;
                }
                //cout << "found " << ipno << " " << portnumber << endl;

 	    } 
        }
    }   
  
    //sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
    // sockfd3 = socket(AF_INET, SOCK_STREAM, 0); 
    // sockfd4 = socket(AF_INET, SOCK_STREAM, 0);
    // sockfd5 = socket(AF_INET, SOCK_STREAM, 0);

    //if (sockfd > 0)// && sockfd2 > 0)// | sockfd3 > 0 | sockfd4 > 0 | sockfd5 > 0)
    //  printf("Socket Connected \n");

    //serv_addr.sin_family = AF_INET; 
    //serv_addr2.sin_family = AF_INET;
    //serv_addr3.sin_family = AF_INET;   
    //serv_addr4.sin_family = AF_INET;                                                                                                        
    //serv_addr5.sin_family = AF_INET;
    
    //serv_addr.sin_port = htons(portnumber);
    //serv_addr2.sin_port = htons(portnumber);
    //serv_addr3.sin_port = htons(portnumber);                                             
    //serv_addr4.sin_port = htons(portnumber);   
    //serv_addr5.sin_port = htons(portnumber);
  
    //inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
    //inet_pton(AF_INET, ipno, &serv_addr.sin_addr);
    //inet_pton(AF_INET, argv[2], &serv_addr2.sin_addr);
    //inet_pton(AF_INET, argv[3], &serv_addr3.sin_addr);
    //inet_pton(AF_INET, argv[4], &serv_addr4.sin_addr);
    //inet_pton(AF_INET, argv[5], &serv_addr5.sin_addr); 

    
    //if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0 && connect(sockfd2, (struct sockaddr *) &serv_addr2, sizeof(serv_addr2)) == 0){// |connect(sockfd3, (struct sockaddr *) &serv_addr3, sizeof(serv_addr3)) == 0 | connect(sockfd4, (struct sockaddr *) &serv_addr4, sizeof(serv_addr4)) == 0 |
    //int retval2 = connect(sockfd2, (struct sockaddr *) &serv_addr2, sizeof(serv_addr2)); 
    //int retval = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    //int retval2 = connect(sockfd2, (struct sockaddr *) &serv_addr2, sizeof(serv_addr2));


    if ( tluSockfd > 0 ) {
        cout << "[Note] connecting to TLU server" << endl;
        retval = connect( tluSockfd, (struct sockaddr *) &tluAddr, sizeof(tluAddr) );
        if ( retval == 0) {
            cout << "[Note] connection to TLU server established" << endl;
            tluServerActive = true;
        }
        else {
            cout << "[Error] connection to TLU server failed (errno " << errno << ")" << endl;
            cout << "[Error] exiting RunControl client" << endl;
            return -1;
            tluServerActive = false;
        }
    }
    
    for ( int i=0; i<numDaqConn; i++ ) {
        if ( daqSockfd > 0 ) {
            cout << "[Note] connecting to DAQ server " << i << ": " << inet_ntoa( daqAddr[i].sin_addr ) << endl;
            retval = connect( daqSockfd[i], (struct sockaddr *) &daqAddr[i], sizeof( daqAddr[i] ) );
            if ( retval == 0) {
                cout << "[Note] connection to DAQ server " << i << " established" << endl;
                daqServerActive[i] = true;
                numDaqServers++;
            }
            else {
                cout << "[Warning] connection to DAQ server " << i << " failed (errno " << errno << ")" << endl;
                daqServerActive[i] = false;
            }
                
        }
    }
    
    if (  numDaqServers > 0 ) {
        if ( numDaqServers != numDaqConn ) {
            cout << "[Warning] not all DAQ servers are running" << endl;
            cout << "[Note] continuing with remaining DAQ servers" << endl;
        }
    } 
    else {
        cout << "[Error] no DAQ servers running" << endl;
        cout << "[Error] exiting RunControl client" << endl;
        return -1;
    }
        
    // cout<< retval<<endl;
    
    int  msglen, statlen;
    char cmd[128]; 
    char retstatus[128]; 
    char retcmd[128]; 

    cout << endl << "============================================" << endl;
    cout << "available RunControl commands: " << endl;
    cout << "    configure" << endl;
    cout << "    start_run run_nr message_string" << endl;
    cout << "    stop_run"  << endl;
    cout << "============================================" << endl << endl;
    do{
            cout << "--------------------------------------------" << endl;
            printf("command to send: ");
            gets( rcbuffer );
            cout << "--------------------------------------------" << endl;

            // this part of the code should go to the start_run_button handler
           if ( strstr( rcbuffer, "start_run") ) {
           
               cout << "[Note] RunControl: begin of start_run command" << endl;
               if ( tluServerActive ) {
                   //  setVeto
                   sprintf( tluCmd , "setVeto");
                   send( tluSockfd, tluCmd , strlen(tluCmd), 0);
                   msglen = recv( tluSockfd, buffer, buffersize, 0);
	           buffer[msglen] = '\0';
                   //sscanf( buffer, "%s", retstatus);
                   //statlen = strlen( retstatus );
	           cout << "Message received from TLU: " << buffer << endl;
               }

               // send command (from command line) to DAQ servers
               for (int i=0 ; i<numDaqConn ; i++ ) {
                   if ( daqServerActive[i] ) {
                       send( daqSockfd[i], rcbuffer , strlen(rcbuffer), 0);
                   }
               }
               cout << "[Note] RunControl is waiting for a replies" << endl;
               for (int i=0 ; i<numDaqConn ; i++ ) {
                   if ( daqServerActive[i] ) {
                       msglen = recv( daqSockfd[i], buffer, buffersize, 0);
	               buffer[msglen] = '\0';
	               cout << "[Note] Message received from DAQ" << i << ": " << buffer << endl;
                   }
               }

               if ( tluServerActive ) {
                   // syncT0
                   sprintf( tluCmd , "pulseT0");
                   send( tluSockfd, tluCmd , strlen(tluCmd), 0);
                   msglen = recv( tluSockfd, buffer, buffersize, 0);
	           buffer[msglen] = '\0';
	           cout << "[Note] Message received from TLU: " << buffer << endl;
               }
               
               //  resetVeto
               if ( tluServerActive ) {
                   sprintf( tluCmd , "resetVeto");
                   send( tluSockfd, tluCmd , strlen(tluCmd), 0);
                   msglen = recv( tluSockfd, buffer, buffersize, 0);
	           buffer[msglen] = '\0';
	           cout << "[Note] Message received from TLU: " << buffer << endl;
               }

               cout << "[Note] RunControl end of start_run command" << endl << endl;
           }

                        
           if ( strstr( rcbuffer, "configure") || strstr( rcbuffer, "stop_run") ) { 
               sscanf( rcbuffer, "%s", cmd);
           
               cout << "[Note] RunControl: begin of " << cmd << " command" << endl;
               if ( tluServerActive ) {
                   //  setVeto
                   sprintf( tluCmd , "setVeto");
                   send( tluSockfd, tluCmd , strlen(tluCmd), 0);
                   msglen = recv( tluSockfd, buffer, buffersize, 0);
	           buffer[msglen] = '\0';
	           cout << "[Note] Message received from TLU: " << buffer << endl;
               }

               // send command (from command line) to DAQ servers
               for (int i=0 ; i<numDaqConn ; i++ ) {
                   if ( daqServerActive[i] ) {
                       send( daqSockfd[i], rcbuffer , strlen(rcbuffer), 0);
                   }
               }
               for (int i=0 ; i<numDaqConn ; i++ ) {
                   if ( daqServerActive[i] ) {
                       msglen = recv( daqSockfd[i], buffer, buffersize, 0);
	               buffer[msglen] = '\0';
	               cout << "[Note] Message received from DAQ" << i << ": " << buffer << endl;
                   }
               }

               cout << "[Note] RunControl: end of " << cmd << " command" << endl << endl;
           }
                       
            //send(sockfd, buffer, strlen(buffer), 0);
	    //	    send(sockfd2, buffer, strlen(buffer), 0);
	    //send(sockfd3, buffer, strlen(buffer), 0);
	    //send(sockfd4, buffer, strlen(buffer), 0);
	    //send(sockfd5, buffer, strlen(buffer), 0);
        //int msglen = recv(sockfd, buffer, buffersize, 0);
	//buffer[msglen] = '\0';
	//printf("Message received from TLU: %s\n",buffer, msglen);
	//	int msglen2 = recv(sockfd2, buffer, buffersize, 0);
	///	buffer[msglen2] = '\0';
	//printf("Message received from DAQ 1: %s\n",buffer, msglen2);
	//int msglen3 = recv(sockfd3, buffer, buffersize, 0);
	//buffer[msglen3] = '\0';  
	//printf("Message received from DAQ 2: %s\n",buffer, msglen3);
	//int msglen4 = recv(sockfd4, buffer, buffersize, 0);
	//buffer[msglen4] = '\0';
	//printf("Message received from DAQ 3: %s\n",buffer, msglen4);
	//int msglen5 = recv(sockfd5, buffer, buffersize, 0);
	//buffer[msglen5] = '\0';
	//printf("Message received from DAQ 4: %s\n",buffer, msglen5);
                                                                                                                                                         
    } while ( strcmp(buffer,"/q"));
    //close(sockfd);
    //close(sockfd2);
    //close(sockfd3);
    //close(sockfd4);
    //close(sockfd5);
}
