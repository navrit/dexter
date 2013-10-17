#ifndef FRAMESAMPLERTHREAD_H
#define FRAMESAMPLERTHREAD_H

#include <QFile>
#include <QSemaphore>
#include <QString>
#include <QThread>

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

#define FRAME_BUF_SIZE  0x001000000 // 16 MByte

class ReceiverThread;

class FramesamplerThread : public QThread
{
  Q_OBJECT

 public:
  FramesamplerThread( ReceiverThread *recvr,
		      QObject *parent = 0 );
  ~FramesamplerThread();

  void stop();
  void run();

  void setFlush( bool enable )     { _flush = enable; }

  // Frame sampling
  void setSampling( bool enable )  { _sampling = enable; }
  bool getFrame( int timeout_ms );
  void freeFrame();
  char *frameData( int *size );
  bool nextFramePixel( int *x, int *y, int *data, int *timestamp );

  // File operations
  bool openFile( std::string filename, bool overwrite = false );
  bool closeFile();

  // Statistics
  long long framesSampled()  { return _framesSampled; }
  long long bytesWritten()   { return _bytesWritten; }
  long long bytesFlushed()   { return _bytesFlushed; }

  // Error
  std::string errorString();
  void clearErrorString() { _errString.clear(); };

 private:
  int copyFrameToBuffer();

 private:
  // Pointer to receiver
  ReceiverThread * _receiver;

  bool _stop;

  long long _framesSampled;
  long long _bytesWritten,_bytesFlushed;

  bool  _sampling;
  QFile _file;
  bool  _fileOpen;
  bool  _flush; // Flush or not,
                // when frame sampling is not enabled or file not opened

  // String containing a description of the last error that occurred
  QString _errString;

  // Semaphores to indicate the availability of a frame in the buffer
  QSemaphore _frameBufferEmpty, _frameAvailable;

  int _bufIndex;
  int _pixIndex;

  // Frame buffer: one frame at a time is copied to this buffer on request
  char _frameBuffer[FRAME_BUF_SIZE];
};

#endif // FRAMESAMPLERTHREAD_H
