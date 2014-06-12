#ifndef _UDPServer_H_
#define _UDPServer_H_
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <stdlib.h>
#include <pthread.h>
#include <list>
#include "SynchronizedQueue.h"

using namespace std;

class UDPServer
{
public:
  UDPServer();
  SynchronizedQueue<unsigned long> data;
  ~UDPServer()
  {
    close(sockfd);
  }
  void start(unsigned int port);
  bool isStarted();
  std::list<unsigned long> getN(unsigned int N, unsigned int debug);
  std::list<unsigned long> getH(unsigned long val,unsigned long mask, unsigned int debug);
  void flush();
  
  static void *receive_helper(void *context)
  {
        return ((UDPServer *)context)->receive();
  }


  void *receive();
/*  void *FillMemOnce();
  void *disp();
  */
  int err;
private:
  pthread_t recv_thread;
  int sockfd;


/*  
 
  char s[INET6_ADDRSTRLEN];
  unsigned long swap(unsigned long d);
  void* get_in_addr(struct sockaddr *sa);
  */
};

#endif 
