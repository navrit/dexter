#ifndef FILEWRITERTHREAD_H
#define FILEWRITERTHREAD_H

#include <QFile>
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

class ReceiverThread;

class FilewriterThread : public QThread
{
  Q_OBJECT

 public:
  FilewriterThread( ReceiverThread *recvr,
		    QObject *parent = 0 );
  ~FilewriterThread();

  void stop();
  void run();

  // File operations
  bool openFile( std::string filename, bool overwrite = false );
  bool closeFile();
  void setFlush( bool flush ) { _flush = flush; }

  // Statistics
  long long bytesWritten()  { return _bytesWritten; }
  long long bytesFlushed()  { return _bytesFlushed; }

  // Error
  std::string errorString();
  void clearErrorString() { _errString.clear(); };

 private:
  // Pointer to receiver
  ReceiverThread * _receiver;

  bool _stop;

  long long _bytesWritten, _bytesFlushed;

  QFile _file;
  bool  _fileOpen;
  bool  _flush; // Flush or not, when no file is open

  // String containing a description of the last error that occurred
  QString _errString;
};

#endif // FILEWRITERTHREAD_H
