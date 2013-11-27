#ifndef RECEIVERTHREAD_H
#define RECEIVERTHREAD_H

#include <QString>
#include <QThread>

#ifdef WIN32
  #include "stdint.h"
#else
  #include </usr/include/stdint.h>
#endif
typedef int64_t  i64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

//#define RECV_BUF_SIZE  0x001000000 //   16 MByte
//#define RECV_BUF_SIZE  0x010000000 //  256 MByte
#define RECV_BUF_SIZE    0x020000000 //  512 MByte
//#define RECV_BUF_SIZE  0x040000000 // 1024 MByte

class QUdpSocket;

class ReceiverThread : public QThread
{
  Q_OBJECT

 public:
  ReceiverThread( int     *ipaddr,
		  int      port = 8192,
		  QObject *parent = 0 );
  ~ReceiverThread();
  
  void  stop();

  void  run();

  void  readDatagrams();
  bool  hasData()           { return( _head != _tail || _full ); }
  char *data()              { return &_recvBuffer[_tail]; }
  long long bytesAvailable();
  void  updateBytesConsumed( long long bytes );
  bool  empty()             { return ( _head == _tail && !_full ); }
  bool  full()              { return _full; }
  bool  fullOccurred()      { return _fullOccurred; }
  void  resetFullOccurred() { _fullOccurred = false; }
  char *buffer()            { return _recvBuffer; } 
  void  reset();
  bool  setBufferSize( long long size );
  long long bufferSize()    { return _bufferSize; }
  long long maxBufferSize() { return RECV_BUF_SIZE; }

  std::string ipAddressString();
  std::string errorString();
  void clearErrorString()   { _errString.clear(); };

  int  packetsReceived()    { return _packetsReceived; }
  int  packetsLost()        { return _packetsLost; }
  int  lastPacketSize()     { return _lastPacketSize; }

  long long bytesReceived() { return _bytesReceived; }
  long long bytesLost()     { return _bytesLost; }

  int bufferWraps()         { return _bufferWraps; }

 private:
  bool suspended();

 private:
  QUdpSocket *_sock;
  quint32     _addr;
  QString     _addrStr;
  int         _port;
  bool        _suspend, _suspended;
  bool        _stop;

  // Statistics
  int       _packetsReceived, _packetsLost;
  long long _bytesReceived, _bytesLost;
  int       _lastPacketSize;
  int       _bufferWraps;

  // String containing a description of the last error that occurred
  QString   _errString;

  // Buffer administration
  long long _bufferSize;
  long long _head, _tail, _headEnd;
  bool      _full, _fullOccurred;

  // Buffer to dump data blocks into when necessary (e.g. when buffer is full)
  char      _flushBuffer[16384];

  // Receive buffer
  char      _recvBuffer[RECV_BUF_SIZE];
};

#endif // RECEIVERTHREAD_H
