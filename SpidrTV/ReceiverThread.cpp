#include "ReceiverThread.h"
#ifdef WIN32
#include "windows.h"
#endif

#ifdef OLD_CODE
#define PAYLOAD_SIZE  512
#define PAYLOAD_SIZE  1024
#define EXP_PACKETS   ((RAW_DATA_SIZE + (PAYLOAD_SIZE-1)) / PAYLOAD_SIZE)
#endif // OLD_CODE

#ifdef WIN32
#pragma pack(push)
#pragma pack(1)
struct Packet
{
  unsigned __int16 triggerCnt;
  unsigned __int16 shutterCnt;
  unsigned __int16 sequenceNr;
  unsigned __int16 timeLo;
  unsigned __int16 timeMi;
  unsigned __int16 timeHi;
#ifdef OLD_CODE
  unsigned __int8  payload[PAYLOAD_SIZE];
#endif // OLD_CODE
};
#pragma pack(pop)
#else
struct Packet
{
  unsigned short triggerCnt;
  unsigned short shutterCnt;
  unsigned short sequenceNr;
  unsigned short timeLo;
  unsigned short timeMi;
  unsigned short timeHi;
};
#endif // WIN32

// Byte index to payload
#define PAYLOAD_I  12

//#define b2ls(x)     (0x00FF & ((x) >> 8) | (0xFF00 & ((x) << 8)))
#define byteswap(x) ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

// ----------------------------------------------------------------------------
#ifdef WIN32
void __cdecl odprintf( const char *format, ... )
{
  char    buf[4096], *p = buf;
  va_list args;
  int     n;

  va_start(args, format);
  // buf-3 is room for CR/LF/NUL:
  n = _vsnprintf(p, sizeof buf - 3, format, args);
  va_end(args);

  p += (n < 0) ? sizeof buf - 3 : n;

  while ( p > buf && isspace(p[-1]) )
    *--p = '\0';

  *p++ = '\r';
  *p++ = '\n';
  *p   = '\0';

  OutputDebugStringA(buf);
}
#endif // WIN32
// ----------------------------------------------------------------------------

ReceiverThread::ReceiverThread( QString        adapter,
				unsigned short port,
				int            mpx3_counter_bits,
				QObject       *parent )
  : QThread(parent),
    _stop(false),
    _frameIndex(0),
    _framesSkipped(0),
    _framesReceived(0),
    _packetsLost(0),
    _packetsReceived(0),
    _debugCounter(0)
{
  _port = port;
  _adapter = adapter;

  // Determine the expected frame size in bytes
  if( mpx3_counter_bits == 1  ||
      mpx3_counter_bits == 4  || // MPX3 only
      mpx3_counter_bits == 6  || // MPX3-RX only
      mpx3_counter_bits == 12 ||
      mpx3_counter_bits == 24 )
    _frameSize = (256 * 256 * mpx3_counter_bits) / 8;
  else
    _frameSize = MPX3_12BIT_RAW_SZ;

  // Allocate frame buffers
  _frameBuffer[0] = new unsigned char[_frameSize];
  _frameBuffer[1] = new unsigned char[_frameSize];
  _currFrame      = _frameBuffer[0];

  // Initialize frame buffers
  memset( _frameBuffer[0], 0xFF, _frameSize );
  memset( _frameBuffer[1], 0xFF, _frameSize );
  _frameFull[0] = false;
  _frameFull[1] = false;

  start();
}

// ----------------------------------------------------------------------------

ReceiverThread::~ReceiverThread()
{
  this->stop();
}

// ----------------------------------------------------------------------------

bool ReceiverThread::copyFrame( unsigned char *data )
{
  // Copy frame from the frame buffer not currently being filled,
  // if available, for decode and display
  int other = (_frameIndex + 1) & 0x1;
  if( _frameFull[other] )
    {
      memcpy( data, _frameBuffer[other], _frameSize );
      _frameFull[other] = false;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

void ReceiverThread::gotoNextFrameBuffer()
{
  int nextIndex = (_frameIndex + 1) & 0x1;
  // If the previous frame has not been read, we overwrite the current frame
  if( !_frameFull[nextIndex] )
    {
      _frameFull[_frameIndex] = true;
      _frameIndex = nextIndex;
    }
  else
    {
      ++_framesSkipped;
    }

  _currFrame = _frameBuffer[_frameIndex];
  memset( _currFrame, 0xFF, _frameSize);
}

// ----------------------------------------------------------------------------

void ReceiverThread::run()
{
  QUdpSocket * sock = new QUdpSocket(0);
  if( !sock->bind( QHostAddress(_adapter), _port) )
    {
      _error = "Failed to bind to port or adapter: " + sock->errorString();
      _stop = true;
    }
  //connect( sock, SIGNAL(readyRead()), this, SLOT(waitForData()) );

  int     bufSize  = 16384;
  char   *buffer   = new char[bufSize];
  Packet *datagram = (Packet *) buffer;

  // Expected shutter counter value, normally starts at 1
  int expShutter = 0;

  // Expected sequence counter value, starts at 1, but we will subtract 1..
  int expSequenceNr = 0;

  int recvSize, payloadSize = 0, expPackets = 1;
  int sequenceNr, shutterCnt;

  while( !_stop )
    {
      //_mutex.lock();
      //_condition.wait( &_mutex );
      //_mutex.unlock();

      recvSize = sock->readDatagram( buffer, bufSize );
      if( recvSize <= 0 ) continue;

      // Process the packet as soon as it is received
      ++_packetsReceived;
      sequenceNr = byteswap( datagram->sequenceNr ) - 1; // NB: minus 1...
      shutterCnt = byteswap( datagram->shutterCnt );

      // Initialize shutter counter if necessary
      if( expShutter == 0 )
	{
	  expShutter = shutterCnt;
	  // Determine the used payload size
	  payloadSize = recvSize - PAYLOAD_I;
	  // ..and from that the expected number of packets per image frame
	  expPackets = (_frameSize + (payloadSize-1)) / payloadSize;
	}

      if( shutterCnt != expShutter ||
	  // Another frame with the same shutter counter as previously
	  sequenceNr < expSequenceNr )
	{
	  expShutter = shutterCnt;

	  if( expSequenceNr != expPackets )
	    {
	      // Starting a new frame/image, prematurely apparently...
	      // (see further down for a properly completed frame)
	      ++_framesReceived;
	      gotoNextFrameBuffer();
	    }

	  // Any last packets lost in the previous sequence
	  // or any lost packets at the start of this new sequence ?
	  _packetsLost += (expPackets - expSequenceNr + sequenceNr);
	}
      else if( sequenceNr > expSequenceNr )
	{
	  // Packets lost in the ongoing sequence
	  _packetsLost += sequenceNr - expSequenceNr;
	}

      // Next sequence number (minus 1!) to expect
      expSequenceNr = sequenceNr + 1;

      if( sequenceNr < expPackets ) // Safeguard against funny seq numbers
	// Copy the packet's payload to the proper location
	// in the image frame buffer
	memcpy( &_currFrame[sequenceNr * payloadSize],
		//&buffer[PAYLOAD_I], payloadSize );
		&buffer[PAYLOAD_I], recvSize - PAYLOAD_I );

      if( expSequenceNr == expPackets )
	{
	  // We can assume the frame is complete
	  // so starting a new frame/image
	  ++_framesReceived;
	  gotoNextFrameBuffer();
	}
    }
  sock->close();
  delete sock;
  delete[] buffer;
  _stop = true;
}

// ----------------------------------------------------------------------------

void ReceiverThread::waitForData()
{
  _mutex.lock();
  _condition.wakeOne();
  _mutex.unlock();
}

// ----------------------------------------------------------------------------

void ReceiverThread::stop()
{
  if (_stop) return;
  _stop = true;
  _mutex.lock();
  _condition.wakeOne();
   _mutex.unlock();
  this->wait();
}

// ----------------------------------------------------------------------------
#ifdef OLD_CODE
void ReceiverThread::run_alternative()
{
  QUdpSocket * sock = new QUdpSocket(0);
  if( !sock->bind( QHostAddress(_adapter), _port) )
    {
      _error = "Failed to bind to port or adapter: " + sock->errorString();
      _stop = true;
    }
  connect( sock, SIGNAL(readyRead()), this, SLOT(waitForData()) );

  //int bufSize = sizeof( Packet ) * EXP_PACKETS * 110;
  int   bufSize = sizeof( Packet ) * EXP_PACKETS * 2;
  char *buffer  = new char[bufSize];

  // Expected shutter counter value, starts at 1
  int expShutter = 0;
  // Expected sequence counter value, starts at 1, but we subtract 1..
  int expSequence = 0;

  while( !_stop )
    {
      int recvSize  = 1;
      int recvIndex = 0;

      //int packets   = 0;
      //while( packets != EXP_PACKETS && !_stop )

      //_mutex.lock();
      //_condition.wait( &_mutex );
      //_mutex.unlock();

      //sock->waitForReadyRead( -1 );

      while( recvSize > 0 && !_stop )
	{
	  recvSize = sock->readDatagram( &buffer[recvIndex],
					 bufSize - recvIndex );
	  if( recvSize <= 0 )
	    {
	      // Waiting for the first packet for this buffer?
	      // If yes then continue, else end this while-loop
	      // to start processing the data available in the buffer
	      if( recvIndex == 0 ) recvSize = 1;
	      continue;
	    }

	  //if( recvSize > 0 ) ++packets;
	  //++_debugCounter;

	  recvIndex += recvSize;
	  if( recvIndex >= bufSize ) break;
	}

      // Process the packets in the buffer
      ++_debugCounter; // ### DEBUG: count how often we process

      int index = 0;
      Packet *datagram;
      while( index < recvIndex )
	{
	  datagram = (Packet *) &buffer[index];
	  datagram->sequenceNr = byteswap( datagram->sequenceNr ) - 1;
	  datagram->shutterCnt = byteswap( datagram->shutterCnt );
	  ++_packetsReceived;

	  //odprintf("Seq: %d, Frame: %d\n",
	  //         datagram->sequenceNr, datagram->shutterCnt);

	  // Initialize shutter counter if necessary
	  if( expShutter == 0 ) expShutter = datagram->shutterCnt;

	  //if( datagram->shutterCnt > expShutter )
	  if( datagram->shutterCnt != expShutter ||
	      // Another frame with the same shutter counter as previous
	      datagram->sequenceNr < expSequence )
	    {
	      // Starting a new frame/image, apparently...
	      ++_framesReceived;
	      expShutter = datagram->shutterCnt;
	      gotoNextFrameBuffer();

	      // Any last packets lost in the previous sequence
	      // or any lost packets in this new sequence ?
	      _packetsLost += (EXP_PACKETS - expSequence +
			       datagram->sequenceNr);
	    }
	  //else if( datagram->shutterCnt < expShutter )
	  //{
	  //    // Old ? Forget it... ### No, could be restart of autotrigger
	  //    index += sizeof( Packet );
	  //    continue;
	  //  }
	  else if( datagram->sequenceNr > expSequence )
	    {
	      // Packets lost in the ongoing sequence
	      _packetsLost += datagram->sequenceNr - expSequence;
	    }

	  // Next sequence number (minus 1!) to expect
	  expSequence = datagram->sequenceNr + 1;

	  // Copy the packet's payload to the proper location
	  // in the frame/image buffer
	  memcpy( &_currFrame[datagram->sequenceNr * PAYLOAD_SIZE],
		  datagram->payload, PAYLOAD_SIZE );

	  index += sizeof( Packet );
	}
    }
  sock->close();
  delete sock;
  delete[] buffer;
  _stop = true;
}
#endif // OLD_CODE
// ----------------------------------------------------------------------------
