#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(1000*ms)
#endif

#include "FilewriterThread.h"
#include "ReceiverThread.h"

// ----------------------------------------------------------------------------

FilewriterThread::FilewriterThread( ReceiverThread *recvr,
				    QObject *parent /* = 0 */ )
  : QThread( parent ),
    _receiver( recvr ),
    _stop( false ),
    _bytesWritten( 0 ),
    _bytesFlushed( 0 ),
    _fileOpen( false )
{
  // Start the thread (see run())
  this->start();
}

// ----------------------------------------------------------------------------

FilewriterThread::~FilewriterThread()
{
  // In case the thread is still running...
  this->stop();
}

// ----------------------------------------------------------------------------

void FilewriterThread::stop()
{
  if( this->isRunning() )
    {
      _stop = true;
      this->wait(); // Wait until this thread (i.e. function run()) exits
    }
}

// ----------------------------------------------------------------------------

void FilewriterThread::run()
{
  long long bytes;

  if( !_receiver ) _stop = true;

  while( !_stop )
    {
      if( _receiver->hasData() )
	{
	  if( _fileOpen )
	    {
	      // Write data to file
	      bytes = _file.write( _receiver->data(),
				   _receiver->bytesAvailable() );
	      // Update the receiver's data buffer
	      _receiver->updateBytesConsumed( bytes );
	      _bytesWritten += bytes;
	    }
	  else
	    {
	      // Flush the data...
	      bytes = _receiver->bytesAvailable();
	      _receiver->updateBytesConsumed( bytes );
	      _bytesFlushed += bytes;
	    }
	}
      else
	{
	  // Doze off for a short while, while waiting for new data...
	  Sleep( 50 );
	}
    }
}

// ----------------------------------------------------------------------------

bool FilewriterThread::openFile( std::string filename, bool overwrite )
{
  this->closeFile();

  QString fname = QString::fromStdString( filename );
  if( QFile::exists( fname ) && !overwrite )
    {
      _errString = "File \"" + fname + "\" already exists";
      return false;
    }

  _file.setFileName( fname );
  _file.open( QIODevice::WriteOnly );
  if( _file.isOpen() )
    {
      _bytesWritten = 0;
      _bytesFlushed = 0;
      _fileOpen = true;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool FilewriterThread::closeFile()
{
  _fileOpen = false;
  if( _file.isOpen() )
    {
      _file.close();
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

std::string FilewriterThread::errorString()
{
  if( _errString.isEmpty() ) return std::string( "" );
  QString qs = "Filewriter: " + _errString;
  return qs.toStdString();
}

// ----------------------------------------------------------------------------
