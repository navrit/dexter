#include "udp_server.h"


UDPServer::UDPServer()
{
sockfd =0;
err=0;
}


void UDPServer::start(unsigned int port)
{
  int res=pthread_create( &recv_thread, NULL, receive_helper, this);
  if(res!=0)
    {
      perror("acsRecv_thread1 creation failed");
      exit(EXIT_FAILURE);
    }
}
#define LOGINTERVAL 1 //number of ethernet packets before a log message appears

#define MAXBUFLEN (64*1024)
void*   UDPServer::receive()
{

  struct addrinfo hints, *servinfo, *p;
  int rv;
  
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; // set to AF_INET to force IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // use my IP
  if ((rv = getaddrinfo(NULL, "8192", &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return NULL;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) 
  {
     if ((sockfd = socket(p->ai_family, p->ai_socktype,  p->ai_protocol)) == -1) 
     {
        perror("listener: socket");
        err=1;
        continue;
     }

     if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
     {
       close(sockfd);
       err=1;
       perror("listener: bind");
       continue;
     }
     break;
  }

  if (p == NULL) 
  {
    fprintf(stderr, "listener: failed to bind socket\n");
    return NULL;
  }

  freeaddrinfo(servinfo);
//  memory.resize(MEM_SIZE);

  socklen_t addr_len;
   struct sockaddr_storage their_addr;
  addr_len = sizeof their_addr;
  char buf[MAXBUFLEN];
  unsigned long *package;
  unsigned int m_numbytes;
  while(1)
  {
    if ((m_numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
    {
      perror("recvfrom...");
      return this;
    }
  	data.push_pre();
    for(int i=0;i<m_numbytes;i+=8)
    {
       package=(unsigned long *) &buf[i];
       data.push(*package);
    }
	data.push_post();
  }
}

std::list<unsigned long> UDPServer::getN(unsigned int N, unsigned int debug)
{
  std::list<unsigned long> l;
  unsigned int i;
  unsigned long pck;
  for(i=0;i<N;i++)
  {
    pck=0;
    data.waitAndPop(pck);
    if (pck==0) //timeout
      return l;
    if (debug)
    {
      printf("%c%d %16lx",13,i,pck);
      
      fflush(stdout);
    }
    l.push_back(pck);
  }
  return l;
}

unsigned long flipbytes(unsigned long b)
{
  unsigned long b0=b&0xFF;
  b>>=8;
  unsigned long b1=b&0xFF;
  b>>=8;
  unsigned long b2=b&0xFF;
  b>>=8;
  unsigned long b3=b&0xFF;
  b>>=8;
  unsigned long b4=b&0xFF;
  b>>=8;
  unsigned long b5=b&0xFF;
  b>>=8;
  unsigned long b6=b&0xFF;
  b>>=8;
  unsigned long b7=b&0xFF;

  unsigned long bo= b0<<56 | b1<<48 | b2<<40 | b3<<32 | b4<<24 | b5<<16 | b6<<8 | b7 ;
  return bo;
  
}
void UDPServer::flush()
{
  data.flush();
}

  bool UDPServer::isStarted()
  {
    return (err==0);
  }
  

#define MAX_QUEUE_LEN (256*256*2)
#undef DEBUG_UDP 0
std::list<unsigned long> UDPServer::getH(unsigned long val, unsigned long mask, unsigned int debug)
{
  std::list<unsigned long> l;
  unsigned int i=0;
  unsigned long pck;
  unsigned long pmasked;

//  printf("-> %16lx %16lx\n",val,mask);
  debug=0;
#ifdef DEBUG_UDP
    std::cout << "GO!" << std::endl;
#endif
  while(1)
  {
    pck=0;
    
    data.waitAndPop(pck);
    if (pck==0) //timeout
      break;

#ifdef DEBUG_UDP
    if ( debug)
    {
      printf("\n%c%d %16lx %16lx %16lx",13,i,pck,pmasked,mask);
      fflush(stdout);
    }
#endif

    l.push_back(pck);
    pmasked=pck&mask;
    if(pmasked==val) break;

    if (data.sizeOfQueue()>0) //vomiting
    {
      int cnt=256*256;
      data.pop_pre();
      while(cnt-- && data.pop_fast(pck))
      {
        l.push_back(pck);
#ifdef DEBUG_UDP
    if (debug)
    {
      printf("\n%c%d %16lx %16lx %16lx",13,i,pck,pmasked,mask);
      fflush(stdout);
    }
#endif
        pmasked=pck&mask;
        if(pmasked==val) break;
      }
      data.pop_post();
      if(pmasked==val) break;
    }

    if (l.size()>MAX_QUEUE_LEN)
    {
      l.push_back(1);
      return l;
    }
  }
#ifdef DEBUG_UDP
  if ( debug)  
    std::cout << "\nEND "<<l.size()<<"!" << std::endl;
#endif
  return l;

}

/*


// get sockaddr, IPv4 or IPv6:
void*   UDPServer::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

double   UDPServer::GetTime()
{
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	return ((double)tv.tv_sec) + ((double)tv.tv_usec) / 1000000 ;
			
}

void   UDPServer::send_msg(unsigned char *buf, unsigned int len)
{
}

void   UDPServer::send_msg(unsigned char b0,unsigned char b1,unsigned char b2)
{
  unsigned char buf[8];
  buf[0]=0xAA; // Global Sync Header
  buf[1]=0x00; // chip id 
  buf[2]=0x00; // chip id 
  buf[3]=0x00; // chip id 
  buf[4]=0x00; // chip id 
  buf[5]=b0; // enable ack
  buf[6]=b1; // 
  buf[7]=b2; //
  _send_msg(buf,8);
}


void   UDPServer::_send_msg(unsigned char *data, unsigned int len)
{
  
  char buf[len+8];
  unsigned int *iptr;
  unsigned int i;
  iptr=(unsigned int *)&buf[0];
  *iptr=htonl(1);
  iptr=(unsigned int *)&buf[4];
  *iptr=htonl(len);

  printf("-> [");
  for(i=0;i<len;i++)
  {
    buf[8+i]=data[i];
    printf("%02X",data[i]);
  }
  printf("] (%d)\n",8+len);
  unsigned int n = write(tcpfd, buf, 8+len);
  
  if (n < 0) 
   perror("ERROR writing to socket");
   
 n = read(tcpfd,buf,8);
    if (n < 0) 
         perror("ERROR reading from socket");
}
*/
