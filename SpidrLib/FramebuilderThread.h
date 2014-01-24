#ifndef FRAMEBUILDERTHREAD_H
#define FRAMEBUILDERTHREAD_H

#include <QFile>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QWaitCondition>

#include <vector>

#ifdef WIN32
  #include "stdint.h"
#else
  #include </usr/include/stdint.h>
#endif
typedef int64_t  i64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
#include "spidrdata.h"

class ReceiverThread;

typedef void (*CallbackFunc)( int id );

class FramebuilderThread : public QThread
{
  Q_OBJECT

 public:
  FramebuilderThread( std::vector<ReceiverThread *> recvrs,
		    QObject *parent = 0 );
  ~FramebuilderThread();

  void stop();
  void run();
  void inputNotification();
  void abortFrame();
  void processFrame();
  void writeFrameToFile();
  void writeRawFrameToFile();
  void writeDecodedFrameToFile();
  bool hasDecodedFrame() { return _hasDecodedFrame; }
  int *decodedFrameData( int index, int *size );
  void releaseDecodedFrame();
  i64    decodedFrameTimestamp();
  double decodedFrameTimestampDouble();
  i64    decodedFrameTimestampSpidr();

  void setAddrInfo( int *ipaddr, int *ports );
  void setDeviceIdsAndTypes( int *ids, int *types );
  void setPixelDepth( int nbits );
  void setDecodeFrames( bool decode );
  void setCompressFrames( bool compress );
  void setFlush( bool flush ) { _flush = flush; }

  void setCallbackId( int id ) { _id = id; }
  void setCallback( CallbackFunc cbf ) { _callbackFunc = cbf; }

  bool openFile( std::string filename, bool overwrite = false );
  bool closeFile();

  int  framesReceived() { return _framesReceived; }
  int  framesWritten()  { return _framesWritten; }
  int  framesProcessed(){ return _framesProcessed; }
  int  packetsLost()    { return _packetsLost; }

  std::string errString();
  void clearErrString() { _errString.clear(); };

 private:
  // Vector with pointers to frame receivers (up to 4)
  std::vector<ReceiverThread *> _receivers;
  u32 _n; // To contain size of _receivers

  QMutex         _mutex;
  QWaitCondition _inputCondition;
  QWaitCondition _outputCondition;
  bool           _stop;

  // For external notification purposes, a general-purpose callback function
  // with 1 parameter (initially for Pixelman)
  int          _id;
  CallbackFunc _callbackFunc;

  // Event header buffer
  EvtHeader_t _evtHdr;
  // Device headers
  DevHeader_t _devHdr[4];

  int   _framesReceived;
  int   _framesWritten;
  int   _framesProcessed;
  int   _packetsLost;
  bool  _decode;
  bool  _compress;
  bool  _flush;
  bool  _hasDecodedFrame;
  bool  _abortFrame;

  QFile _file;
  bool  _fileOpen;

  // String containing a description of the last error that occurred
  QString _errString;

  // Look-up tables for Medipix3RX pixel data decoding
  int _mpx3Rx6BitsLut[64];
  int _mpx3Rx12BitsLut[4096];

  // Intermediate buffers for a decoded frame from each of the 4 receivers
  i64 _timeStamp, _timeStampSpidr;
  int _frameSz[4];
  int _decodedFrame[4][256*256];

  int mpx3RawToPixel( unsigned char *raw_bytes,
		      int           *pixels,
		      int            counter_depth,
		      int            device_type,
		      bool           compress );
};

#endif // FILEWRITERTHREAD_H
