#ifndef DATASAMPLERTHREAD_H
#define DATASAMPLERTHREAD_H

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
typedef uint64_t u64;
typedef int64_t  i64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

#define FRAME_BUF_SIZE  0x001000000 // 16 MByte

class ReceiverThread;

class DatasamplerThread : public QThread
{
  Q_OBJECT

 public:
  DatasamplerThread( ReceiverThread *recvr,
		      QObject *parent = 0 );
  ~DatasamplerThread();

  void stop();
  void run();

  void setFlush      ( bool enable )     { _flush = enable; }

  // Data sampling: 'frames' or data blocks
  void  setSampling  ( bool enable )     { _sampling = enable; }
  void  setSampleAll ( bool enable )     { _sampleAll = enable; }
  bool  getFrame     ( int timeout_ms );
  void  freeFrame    ()                  { freeSample(); }
  char *frameData    ( int *size )       { return sampleData( size ); }
  void  freeSample   ();
  char *sampleData   ( int *size );
  bool  nextPixel    ( int *x, int *y, int *data, int *timestamp );
  u64   nextPixel    ();

  // File operations
  bool openFile      ( std::string filename, bool overwrite = false );
  bool closeFile     ();

  // Statistics
  long long framesSampled() { return _framesSampled; }
  long long bytesWritten()  { return _bytesWritten; }
  long long bytesFlushed()  { return _bytesFlushed; }

  // Error
  std::string errorString();
  void clearErrorString()   { _errString.clear(); };

 private:
  int copyFrameToBuffer();

 private:
  // Pointer to receiver
  ReceiverThread * _receiver;

  bool _stop;

  long long _framesSampled;
  long long _bytesWritten,_bytesFlushed;

  bool  _sampling, _sampleAll;
  QFile _file;
  bool  _fileOpen;
  bool  _flush; // Flush or not,
                // when data sampling is not enabled or file not opened

  // String containing a description of the last error that occurred
  QString _errString;

  // Semaphores to indicate the availability of sampled data in the buffer
  QSemaphore _sampleBufferEmpty, _sampleAvailable;

  int _bufIndex;
  int _pixIndex;

  bool _bigEndian;

  // Pixel data buffer: a pixel data block or one 'frame' at a time
  // is copied into this buffer on request
  u64   _sampleBufferUlong[FRAME_BUF_SIZE/8];
  char *_sampleBuffer;
};

#endif // DATASAMPLERTHREAD_H
