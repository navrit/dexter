#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(1000*ms)
#endif

#include "FramesamplerThread.h"
#include "ReceiverThread.h"

// ----------------------------------------------------------------------------

FramesamplerThread::FramesamplerThread( ReceiverThread *recvr,
				    QObject *parent /* = 0 */ )
  : QThread( parent ),
    _receiver( recvr ),
    _stop( false ),
    _framesSampled( 0 ),
    _bytesWritten( 0 ),
    _bytesFlushed( 0 ),
    _sampling( false ),
    _fileOpen( false ),
    _flush( true ),
    _bufIndex( 0 ),
    _pixIndex( 0 )
{
  // Start the thread (see run())
  this->start();
}

// ----------------------------------------------------------------------------

FramesamplerThread::~FramesamplerThread()
{
  // In case the thread is still running...
  this->stop();
}

// ----------------------------------------------------------------------------

void FramesamplerThread::stop()
{
  if( this->isRunning() )
    {
      _stop = true;
      this->wait(); // Wait until this thread (i.e. function run()) exits
    }
}

// ----------------------------------------------------------------------------

void FramesamplerThread::run()
{
  long long bytes;

  if( !_receiver ) _stop = true;

  while( !_stop )
    {
      if( _receiver->hasData() )
	{
	  if( _sampling || _fileOpen )
	    {
	      if( _sampling )
		{
		  // Wait until the frame buffer is free
		  if( _frameBufferEmpty.tryAcquire( 1, 50 ) )
		    {
		      bytes = this->copyFrameToBuffer();

		      // Write the frame data to file too if appropriate
		      if( _fileOpen )
			// ###NB: what to do if not all bytes are written?
			//        (and we do have them in the frame buffer)
			_file.write( _receiver->data(), bytes );

		      // Update the receiver's data buffer administration
		      _receiver->updateBytesConsumed( bytes );
		      _bytesWritten += bytes;
		    }
		}
	      else
		{
		  // Write data to file
		  bytes = _file.write( _receiver->data(),
				       _receiver->bytesAvailable() );

		  // Update the receiver's data buffer administration
		  _receiver->updateBytesConsumed( bytes );
		  _bytesWritten += bytes;
		}
	    }
	  else
	    {
	      // Flush the data, if enabled...
	      if( _flush )
		{
		  bytes = _receiver->bytesAvailable();
		  _receiver->updateBytesConsumed( bytes );
		  _bytesFlushed += bytes;
		}
	    }
	}
      else
	{
	  // Doze off briefly, while waiting for new data...
	  Sleep( 50 );
	}
    }
  this->closeFile(); // In case a file is opened
}

// ----------------------------------------------------------------------------

bool FramesamplerThread::getFrame( int timeout_ms )
{
  this->freeFrame();

  _pixIndex = 0;

  if( !_sampling ) return false;

  return _frameAvailable.tryAcquire( 1, timeout_ms );
}

// ----------------------------------------------------------------------------

void FramesamplerThread::freeFrame()
{
  if( _frameBufferEmpty.available() == 0 )
    {
      _bufIndex = 0;
      _frameBufferEmpty.release();
    }
}

// ----------------------------------------------------------------------------

char *FramesamplerThread::frameData( int *size )
{
  *size = _bufIndex;
  return _frameBuffer;
}

// ----------------------------------------------------------------------------

bool FramesamplerThread::nextFramePixel( int *x, int *y,
					 int *data, int *timestamp )
{
  u32 addr, header, dcol, spix, pix;
  u8 *pixel = (u8 *) &_frameBuffer[_pixIndex];
  while( _pixIndex < _bufIndex )
    {
      addr = (((u32) pixel[0] << 16) |
	      ((u32) pixel[1] << 8) |
	      ((u32) pixel[2] << 0));

      header = addr & 0xF00000;

      if( header == 0xB00000 || header == 0xA00000 )
	{
	  dcol  = ((addr & 0x0FE000) >> (4+9-1)); // doublecolumn * 2
	  spix  = ((addr & 0x001F80) >> (4+3-2)); // superpixel * 4
	  pix   = ((addr & 0x000070) >> 4);       // pixel
	  *x    = dcol + pix/4;
	  *y    = spix + (pix & 0x3);
	  *data = (((u32) (pixel[2] & 0x0F) << 24) |
		   ((u32) pixel[3] << 16) |
		   ((u32) pixel[4] << 8) |
		   ((u32) pixel[5] << 0));
	  *timestamp = (((u32) pixel[6] << 8) |
			((u32) pixel[7] << 0));
	  _pixIndex += 8;
	  return true;
	}
      else
	{
	  _pixIndex += 8;
	  pixel += 8;
	}
    }
  return false;
}

// ----------------------------------------------------------------------------

int FramesamplerThread::copyFrameToBuffer()
{
  long long bytes = _receiver->bytesAvailable();
  char     *data  = _receiver->data();
  int       size  = 0;
  // Find the first End-of-Readout Timepix3 packet
  while( size < bytes )
    {
      if( *data == (char) 0x71 &&
	  (*(data+1) == (char) 0xA0 || *(data+1) == (char) 0xB0 ) )
	{
	  // EoR found...

	  // Include the EoR
	  size += 8;

	  if( _bufIndex + size > FRAME_BUF_SIZE )
	    {
	      // It's not going to fit, so skip this frame...
	      _bufIndex = 0;
	      if( _frameBufferEmpty.available() == 0 )
		_frameBufferEmpty.release();
	    }
	  else
	    {
	      // Copy the frame
	      memcpy( static_cast<void *> (&_frameBuffer[_bufIndex]),
		      static_cast<void *> (_receiver->data()), size );
	      _bufIndex += size;

	      // Okay, a frame is now available in the frame buffer
	      _frameAvailable.release();
	    }
	  return size;
	}
      data += 8;
      size += 8;
    }

  // Copy the data up to this point (but the frame is not yet complete!)
  memcpy( static_cast<void *> (&_frameBuffer[_bufIndex]),
	  static_cast<void *> (_receiver->data()), size );
  _bufIndex += size;

  // Allow another access to the frame buffer to continue filling it
  if( _frameBufferEmpty.available() == 0 )
    _frameBufferEmpty.release();

  return size;
}

// ----------------------------------------------------------------------------

bool FramesamplerThread::openFile( std::string filename, bool overwrite )
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
      _fileOpen = true;
      return true;
    }
  _errString = "Failed to open file \"" + fname + "\"";  
  return false;
}

// ----------------------------------------------------------------------------

bool FramesamplerThread::closeFile()
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

std::string FramesamplerThread::errorString()
{
  if( _errString.isEmpty() ) return std::string( "" );
  QString qs = "Framesampler: " + _errString;
  return qs.toStdString();
}

// ----------------------------------------------------------------------------
