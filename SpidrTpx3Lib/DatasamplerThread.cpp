#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep(1000*ms)
#endif

#include "DatasamplerThread.h"
#include "ReceiverThread.h"

// ----------------------------------------------------------------------------

DatasamplerThread::DatasamplerThread( ReceiverThread *recvr,
				    QObject *parent /* = 0 */ )
  : QThread( parent ),
    _receiver( recvr ),
    _stop( false ),
    _sampling( false ),
    _sampleAll( false ),
    _requestFrame( false ),
    _requestedMinSize( 0 ),
    _requestedMaxSize( 0 ),
    _framesSampled( 0 ),
    _bytesWritten( 0 ),
    _bytesSampled( 0 ),
    _bytesFlushed( 0 ),
    _fileOpen( false ),
    _flush( true ),
    _sampleIndex( 0 ),
    _pixIndex( 0 ),
    _bigEndian( false )
{
  _sampleBuffer = (char *) _sampleBufferUlong;

  // Start the thread (see run())
  this->start();
}

// ----------------------------------------------------------------------------

DatasamplerThread::~DatasamplerThread()
{
  // In case the thread is still running...
  this->stop();
}

// ----------------------------------------------------------------------------

void DatasamplerThread::stop()
{
  if( this->isRunning() )
    {
      _stop = true;
      this->wait(); // Wait until this thread (i.e. function run()) exits
    }
}

// ----------------------------------------------------------------------------

void DatasamplerThread::run()
{
  long long bytes;

  if( !_receiver ) _stop = true;

  // Sample buffer is initialized to 'available'
  this->freeSample();

  while( !_stop )
    {
      if( _receiver->hasData() )
	{
	  if( _sampling || _fileOpen )
	    {
	      if( _sampling )
		{
		  if( _sampleAll )
		    {
		      // No data to be lost for sampling...
		      // Wait until the sample buffer is free
		      if( _sampleBufferEmpty.tryAcquire( 1, 50 ) )
			{
			  if( _requestFrame )
			    bytes = this->copyFrameToBuffer();
			  else
			    bytes = this->copySampleToBuffer();

			  // Write the sampled data to file too if open
			  if( _fileOpen )
			    {
			      // ###NB: what to do if not all bytes are written
			      // but we did copy them to the sample buffer?
			      _file.write( _receiver->data(), bytes );
			      _bytesWritten += bytes;
			    }

			  // Notify the receiver
			  _receiver->updateBytesConsumed( bytes );
			  _bytesSampled += bytes;
			}
		    }
		  else
		    {
		      // Sample as often as we can,
		      // i.e. copy data only when the data buffer is free
		      // (but it does mean that we may start sampling
		      //  in the middle of a frame... ###DO SOMETHING?
		      //  e.g. to skip everything up to the next EoR and
		      //  start sampling from there? but then we'll never
		      //  sample if the frames come at a low rate)
		      if( _sampleBufferEmpty.available() )
			{
			  _sampleBufferEmpty.acquire();

			  if( _requestFrame )
			    bytes = this->copyFrameToBuffer();
			  else
			    bytes = this->copySampleToBuffer();

			  _bytesSampled += bytes;
			}
		      else
			{
			  bytes = 0;
			}

		      // Write the data (sampled or not) to file if open
		      if( _fileOpen )
			{
			  if( bytes == 0 )
			    bytes = _file.write( _receiver->data(),
						 _receiver->bytesAvailable() );
			  else
			    // ###NB: what to do if not all bytes are written
			    // but we did copy them to the sample buffer?
			    // Note that we do not write more than the bytes
			    // sampled, since we may be in the process of
			    // collecting a frame
			    _file.write( _receiver->data(), bytes );

			  _bytesWritten += bytes;
			}

		      if( bytes == 0 )
			{
			  // If nothing sampled and not writing to file
			  // flush the data if flushing is enabled,
			  // otherwise keep it stored in buffer...
			  if( _flush )
			    {
			      bytes = _receiver->bytesAvailable();
			      _receiver->updateBytesConsumed( bytes );
			      _bytesFlushed += bytes;
			    }
			}
		      else
			{
			  // Notify the receiver
			  _receiver->updateBytesConsumed( bytes );
			}
		    }
		}
	      else
		{
		  // File is open: write data to file
		  bytes = _file.write( _receiver->data(),
				       _receiver->bytesAvailable() );

		  // Notify the receiver
		  _receiver->updateBytesConsumed( bytes );
		  _bytesWritten += bytes;
		}
	    }
	  else
	    {
	      // If not sampling or writing to file flush the data
	      // if flushing is enabled, otherwise keep it stored in buffer
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

bool DatasamplerThread::getSample( int min_size, int max_size, int timeout_ms )
{
  _pixIndex = 0;

  if( !_sampling ) return false;

  _requestFrame = false;

  // Round up to next multiple of 8 bytes
  _requestedMinSize = ((min_size+7)/8)*8;
  _requestedMaxSize = ((max_size+7)/8)*8;

  // Can not request more data in a sample than fits in the buffer
  if( _requestedMaxSize > FRAME_BUF_SIZE )
    _requestedMaxSize = FRAME_BUF_SIZE;

  // Minimum size cannot be larger than maximum size
  if( _requestedMinSize > _requestedMaxSize )
    _requestedMinSize = 0;

  if( timeout_ms == 0 )
    {
      // Return immediately
      if( _sampleAvailable.available() )
	{
	  _sampleAvailable.acquire();
	  return true;
	}
      else
	{
	  return false;
	}
    }
  // Wait for a sample to become available
  return _sampleAvailable.tryAcquire( 1, timeout_ms );
}

// ----------------------------------------------------------------------------

bool DatasamplerThread::getFrame( int timeout_ms )
{
  _pixIndex = 0;

  if( !_sampling ) return false;

  _requestFrame = true;

  // Wait for a frame to become available
  return _sampleAvailable.tryAcquire( 1, timeout_ms );
}

// ----------------------------------------------------------------------------

void DatasamplerThread::freeSample()
{
  if( _sampleBufferEmpty.available() == 0 )
    {
      _sampleIndex = 0;
      _sampleBufferEmpty.release();
    }
}

// ----------------------------------------------------------------------------
/*
bool DatasamplerThread::nextPixel( int *x, int *y,
				    int *data, int *timestamp )
{
// Extract data from the pixel data in a byte-wise manner...
  u32 addr, header, dcol, spix, pix;
  u8 *pixel = (u8 *) &_sampleBuffer[_pixIndex];
  while( _pixIndex < _sampleIndex )
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
*/
// ----------------------------------------------------------------------------

bool DatasamplerThread::nextPixel( int *x,
				   int *y,
				   int *data,
				   int *timestamp )
{
  // Extract data from the next 64-bit/8-byte pixel data packet
  // in the sample buffer and return them in the given pointer locations
  // (if 'data' and 'timestamp' are NULL, return x and y only)
  u64 pixdata, header, dcol, spix, pix;
  while( _pixIndex < _sampleIndex )
    {
#ifdef USE_BIGENDIAN
      if( _bigEndian )
	{
	  // Reverse the byte order
	  char bytes[8];
	  for( int i=0; i<8; ++i )
	    bytes[i] = _sampleBuffer[_pixIndex+7-i];
	  pixdata = *((u64 *) bytes);
	}
      else
	{
	  pixdata = _sampleBufferUlong[_pixIndex/8];
	}
#else
      pixdata = _sampleBufferUlong[_pixIndex/8];
#endif

      // Data-driven or sequential readout pixel data header ?
      header = pixdata & 0xF000000000000000;
      if( header == 0xB000000000000000 || header == 0xA000000000000000 )
	{
	  // doublecolumn * 2
	  dcol  = ((pixdata & 0x0FE0000000000000) >> 52); //(16+28+9-1)
	  // superpixel * 4
	  spix  = ((pixdata & 0x001F800000000000) >> 45); //(16+28+3-2)
	  // pixel
	  pix   = ((pixdata & 0x0000700000000000) >> 44); //(16+28)
	  *x    = (int) (dcol + pix/4);
	  *y    = (int) (spix + (pix & 0x3));
	  if( data )
	    *data = (int) ((pixdata & 0x00000FFFFFFF0000) >> 16);
	  if( timestamp )
	    *timestamp = (int) (pixdata & 0x000000000000FFFF);
	  // Next Timepix3 packet
	  _pixIndex += 8;
	  return true;
	}
      else
	{
	  // Skip non-pixel Timepix3 data packet, go to next packet
	  _pixIndex += 8;
	}
    }
  return false;
}

// ----------------------------------------------------------------------------

u64 DatasamplerThread::nextPixel()
{
  // Return the next 64-bit/8-byte pixel data packet in the sample buffer
  u64 pixdata, header;
  while( _pixIndex < _sampleIndex )
    {
#ifdef USE_BIGENDIAN
      if( _bigEndian )
	{
	  // Reverse the byte order
	  char bytes[8];
	  for( int i=0; i<8; ++i )
	    bytes[i] = _sampleBuffer[_pixIndex+7-i];
	  pixdata = *((u64 *) bytes);
	}
      else
	{
	  pixdata = _sampleBufferUlong[_pixIndex/8];
	}
#else
      pixdata = _sampleBufferUlong[_pixIndex/8];
#endif

      // Data-driven or sequential readout pixel data header ?
      header = pixdata & 0xF000000000000000;
      if( header == 0xB000000000000000 || header == 0xA000000000000000 )
	{
	  // Next Timepix3 packet
	  _pixIndex += 8;
	  return pixdata;
	}
      else
	{
	  // Skip non-pixel Timepix3 data packet, go to next packet
	  _pixIndex += 8;
	}
    }
  return 0;
}

// ----------------------------------------------------------------------------

int DatasamplerThread::copySampleToBuffer()
{
  // Collect pixel data into the sample buffer up to a maximum size
  long long bytes = _receiver->bytesAvailable();
  if( bytes > _requestedMaxSize ) bytes = _requestedMaxSize;

  // Copy the data to the sample buffer
  memcpy( static_cast<void *> (&_sampleBuffer[_sampleIndex]),
	  static_cast<void *> (_receiver->data()), bytes );
  _sampleIndex += bytes;

  if( _sampleIndex >= _requestedMinSize )
    {
      // Okay, a data sample is now available in the sample buffer
      _sampleAvailable.release();
    }
  else
    {
      // Allow another access to the sample buffer to continue filling it
      // until the minimum requested sample size is obtained
      if( _sampleBufferEmpty.available() == 0 )
	_sampleBufferEmpty.release();
    }

  return bytes;
}

// ----------------------------------------------------------------------------

int DatasamplerThread::copyFrameToBuffer()
{
  // Collect pixel data into the sample buffer up to an End-of-Readout packet
  long long bytes = _receiver->bytesAvailable();
  char     *data  = _receiver->data();
  char      hdr1, hdr2;
  int       size  = 0;
  // Find the first End-of-Readout Timepix3 packet
  while( size < bytes )
    {
#ifdef USE_BIGENDIAN
      if( _bigEndian )
	{
	  hdr1 = *data;
	  hdr2 = *(data+1);
	}
      else
	{
	  hdr1 = *(data+7);
	  hdr2 = *(data+6);
	}
#else
      hdr1 = *(data+7);
      hdr2 = *(data+6);
#endif
      if( hdr1 == (char) 0x71 &&
	  (hdr2 == (char) 0xA0 || hdr2 == (char) 0xB0 ) )
	{
	  // EoR found...

	  // Include the EoR
	  size += 8;

	  if( _sampleIndex + size > FRAME_BUF_SIZE )
	    {
	      // It's not going to fit, so skip this frame...
	      _sampleIndex = 0;
	      if( _sampleBufferEmpty.available() == 0 )
		_sampleBufferEmpty.release();
	    }
	  else
	    {
	      // Copy the data up to and including the EoR-packet
	      memcpy( static_cast<void *> (&_sampleBuffer[_sampleIndex]),
		      static_cast<void *> (_receiver->data()), size );
	      _sampleIndex += size;
	      ++_framesSampled;

	      // Okay, a frame is now available in the sample buffer
	      _sampleAvailable.release();
	    }
	  return size;
	}
      data += 8;
      size += 8;
    }

  // EoR not (yet) found

  // Copy the data up to this point (but the frame is not yet complete!)
  memcpy( static_cast<void *> (&_sampleBuffer[_sampleIndex]),
	  static_cast<void *> (_receiver->data()), size );
  _sampleIndex += size;

  // Allow another access to the sample buffer to continue filling it
  // until an EoR is found
  if( _sampleBufferEmpty.available() == 0 )
    _sampleBufferEmpty.release();

  return size;
}

// ----------------------------------------------------------------------------

bool DatasamplerThread::openFile( std::string filename, bool overwrite )
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

bool DatasamplerThread::closeFile()
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

std::string DatasamplerThread::errorString()
{
  if( _errString.isEmpty() ) return std::string( "" );
  QString qs = "Datasampler: " + _errString;
  return qs.toStdString();
}

// ----------------------------------------------------------------------------
