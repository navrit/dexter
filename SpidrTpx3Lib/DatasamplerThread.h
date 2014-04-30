#ifndef DATASAMPLERTHREAD_H
#define DATASAMPLERTHREAD_H

#include <QFile>
#include <QMutex>
#include <QSemaphore>
#include <QString>
#include <QThread>
#include <QWaitCondition>

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

#include "spidrtpx3data.h"

#define FRAME_BUF_SIZE  0x001000000 // 16 MByte

class ReceiverThread;

class DatasamplerThread : public QThread
{
  Q_OBJECT

 public:
  DatasamplerThread( ReceiverThread *recvr,
		     QObject *parent = 0 );
  ~DatasamplerThread();

  void  stop();
  void  run();

  void  setFlush      ( bool enable ) { _flush = enable; }

  // Data sampling: pixel data blocks ('samples') or 'frames'
  void  setSampling  ( bool enable ) { _sampling = enable; }
  void  setSampleAll ( bool enable ) { _sampleAll = enable; }
  bool  getSample    ( int min_size, int max_size, int timeout_ms );
  bool  getFrame     ( int timeout_ms );
  void  freeSample   ();
  int   sampleSize   ()              { return _sampleIndex; }
  char *sampleData   ()              { return _sampleBuffer; }
  bool  nextPixel    ( int *x, int *y, int *data = 0, int *timestamp = 0 );
  u64   nextPixel    ();

  // File operations
  bool startRecording( std::string filename, int runnr );
  bool stopRecording ();
  i64  fileMaxSize   ()           { return _fileMaxSize; }
  void setFileMaxSize( i64 size ) { if( size > 0 ) _fileMaxSize = size; }
  std::string fileName()          { return _fileName.toStdString(); }
  SpidrTpx3Header_t *fileHdr()    { return &_fileHdr; }

  // Statistics
  i64  framesSampled() { return _framesSampled; }
  i64  bytesWritten()  { return _bytesWritten; }
  i64  bytesSampled()  { return _bytesSampled; }
  i64  bytesFlushed()  { return _bytesFlushed; }

  // Error
  std::string errorString();
  void clearErrorString()   { _errString.clear(); };

 private:
  bool    timeOut();
  void    handleTimeOut();
  int     copySampleToBuffer();
  int     copyFrameToBuffer();
  bool    openFilePrivate();
  void    closeFilePrivate();
  bool    openFileOld( std::string filename, bool overwrite = false );
  QString makeFileName();

 private:
  // Pointer to receiver
  ReceiverThread * _receiver;

  bool      _stop;

  // Data file writing
  QFile   _file;
  QString _fileDirName;   // File directory
  QString _fileBaseName;  // Datetime and counter appended to this name
  QString _fileName;      // Fully qualified current file name
  QString _fileExt;       // File extension to use
  int     _fileCntr;
  bool    _recording;     // Whether data is written to file(s)
  bool    _fileOpen;      // A file is currently open
  bool    _flush;         // Flush or not, when data sampling
                          // is not enabled or file not opened
  i64     _fileChunkSize; // Max data chunk size to write in one go
  i64     _fileMaxSize;   // Maximum file size (before auto-opening new one)

  // Run info
  int     _runNr;
  QString _runDescr;

  // Data sampling
  bool    _sampling;
  bool    _sampleAll;
  bool    _timeOut;
  bool    _requestFrame;
  i64     _sampleMinSize, _sampleMaxSize;
  int     _sampleIndex;
  int     _pixIndex;
  bool    _bigEndian;

  // Statistics
  i64     _framesSampled;
  i64     _bytesWritten;
  i64     _bytesSampled;
  i64     _bytesFlushed;

  // String containing a description of the last error that occurred
  QString _errString;

  // Semaphores to indicate the availability of sampled data in the buffer
  QSemaphore _sampleBufferEmpty;
  QSemaphore _sampleAvailable;

  // Additional mutex and condition used after a time-out during sampling
  // (see handleTimeOut())
  QMutex         _mutex;
  QWaitCondition _condition;

  // File header
  SpidrTpx3Header_t _fileHdr;

  // Pixel data buffer: a pixel data block or one 'frame' at a time
  // is copied into this buffer on request
  u64   _sampleBufferUlong[FRAME_BUF_SIZE/8];
  char *_sampleBuffer;
};

#endif // DATASAMPLERTHREAD_H
