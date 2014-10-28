#include <QDateTime>
#include <QDir>

#define RETURN_AVAILABLE_SAMPLE
#define USE_BIGENDIAN

#include "DatasamplerThread.h"
#include "ReceiverThread.h"
#include "SpidrController.h"

// ----------------------------------------------------------------------------

DatasamplerThread::DatasamplerThread( ReceiverThread *recvr,
				    QObject *parent /* = 0 */ )
  : QThread( parent ),
    _receiver( recvr ),
    _stop( false ),
    _fileDirName( "." ),
    _fileBaseName( "tpx" ),
    _fileName( "" ),
    _fileExt( "dat" ),
    _fileCntr( 1 ),
    _fileOpen( false ),
    _recording( false ),
    _flush( true ),
    _fileChunkSize( 0x1000000 ), // 16 MB max per write operation
    _fileMaxSize( 0x40000000 ),  // 1 GB file size
    _runNr( 0 ),
    _sampling( false ),
    _sampleAll( false ),
    _timeOut( false ),
    _requestFrame( false ),
    _sampleMinSize( 0 ),
    _sampleMaxSize( 0 ),
    _sampleIndex( 0 ),
    _pixIndex( 0 ),
    _bigEndian( false ),
    _framesSampled( 0 ),
    _bytesToFile( 0 ),
    _bytesRecorded( 0 ),
    _bytesRecordedInRun( 0 ),
    _bytesSampled( 0 ),
    _bytesFlushed( 0 ),
    _pixelConfig( 0 )
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
  //this->freeSample();
  // NO: sampling only starts for real (_sampleMaxSize > 0)
  // after a request (getSample) has been made

  while( !_stop )
    {
      // Handle file opening and closing
      if( _recording )
	{
	  if( !_fileOpen || _bytesToFile > _fileMaxSize )
	    this->openFilePrivate();
	}
      else
	{
	  if( _fileOpen )
	    this->closeFilePrivate();
	}

      // Handle time-out on getting the requested data sample
      if( _timeOut ) this->handleTimeOut();

      if( _receiver->hasData() )
	{
	  if( _sampling || _fileOpen )
	    {
	      if( _sampling )
		{
		  if( _sampleAll )
		    {
		      // No data to be lost for sampling...
		      // Wait until there is a request
		      // and the sample buffer is free
		      if( _sampleMaxSize > 0 &&
			  _sampleBufferEmpty.tryAcquire( 1, 50 ) )
			{
			  if( _requestFrame )
			    bytes = this->copyFrameToBuffer();
			  else
			    bytes = this->copySampleToBuffer();

			  // Write the sampled data to file too if file is open
			  if( _fileOpen )
			    {
			      // ###NB: what to do if not all bytes are written
			      // but we did copy them to the sample buffer?
			      _file.write( _receiver->data(), bytes );
			      _bytesToFile += bytes;
			      _bytesRecorded += bytes;
			      _bytesRecordedInRun += bytes;
			    }

			  // Notify the receiver
			  _receiver->updateBytesConsumed( bytes );
			  _bytesSampled += bytes;
			}
		    }
		  else
		    {
		      // Sample as often as we can,
		      // i.e. sample data only when there is a request
		      // and the data buffer is free
		      // (but it does mean that in frame-mode we may start
		      //  sampling in the middle of a frame in frame-mode
		      //  ###DO SOMETHING?
		      //  e.g. to skip everything up to the next EoR and
		      //  start sampling from there? but then we'll never
		      //  sample if the frames come at a low rate because
		      //  we'll time out)
		      if( _sampleMaxSize > 0 &&
			  _sampleBufferEmpty.available() )
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
			    {
			      // Adhere to file-write chunk size
			      bytes = _receiver->bytesAvailable();
			      if( bytes > _fileChunkSize )
				bytes = _fileChunkSize;
			      bytes = _file.write( _receiver->data(), bytes );
			    }
			  else
			    // ###NB: what to do if not all bytes are written
			    // but we did copy them to the sample buffer?
			    // Note that we do not write more than the bytes
			    // sampled, since we may be in the process of
			    // collecting a frame
			    _file.write( _receiver->data(), bytes );

			  _bytesToFile += bytes;
			  _bytesRecorded += bytes;
			  _bytesRecordedInRun += bytes;
			}

		      if( bytes == 0 )
			{
			  // If nothing was sampled and not writing to file
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
		  // Not sampling and file is open: write data to file

		  // Adhere to file-write chunk size
		  bytes = _receiver->bytesAvailable();
		  if( bytes > _fileChunkSize )
		    bytes = _fileChunkSize;
		  bytes = _file.write( _receiver->data(), bytes );

		  // Notify the receiver
		  _receiver->updateBytesConsumed( bytes );
		  _bytesToFile += bytes;
		  _bytesRecorded += bytes;
		  _bytesRecordedInRun += bytes;
		}
	    }
	  else
	    {
	      // If not sampling or writing to file, flush the data
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
	  this->msleep( 20 );
	}
    }
  this->closeFilePrivate(); // In case a file is still open
}

// ----------------------------------------------------------------------------

bool DatasamplerThread::getSample( int min_size, int max_size, int timeout_ms )
{
  if( !_sampling ) return false;
  if( min_size < 0 ) return false;
  if( max_size < 1 ) return false;
  if( timeout_ms < 1 ) return false;

  // If necessary wait for the sample buffer clean-up (by the thread),
  // after an earlier time-out
  volatile bool b = _timeOut;
  while( b ) b = this->timeOut();

  this->freeSample();

  _pixIndex = 0;
  _requestFrame = false;

  // Round up to the next multiple of 8 bytes (Timepix3 pixel data packet size)
  min_size = ((min_size+7)/8)*8;
  max_size = ((max_size+7)/8)*8;

  // Can not request more data in a sample than fits in the buffer
  if( max_size > FRAME_BUF_SIZE )
    max_size = FRAME_BUF_SIZE;

  // Minimum size cannot be larger than maximum size
  if( min_size > max_size )
    min_size = 0;

  // _sampleMinSize and _sampleMaxSize are used in the thread
  // so update them only here in a single operation;
  // setting _sampleMaxSize unequal to zero is necessary to start sampling
  _sampleMinSize = min_size;
  _sampleMaxSize = max_size;

  // Wait for a sample to become available or time out
  if( _sampleAvailable.tryAcquire( 1, timeout_ms ) )
    return true;

  // Timed out !
#ifdef RETURN_AVAILABLE_SAMPLE
  // We're still interested in anything sampled, even if it is
  // not the minimum size we requested; let the thread handle this
  // properly in handleTimeOut()
  if( _sampleIndex > 0 )
    {
      // Wait for the thread by means of handleTimeOut()
      // to properly hand over the sample data in the buffer..
      _mutex.lock();
      _timeOut = true;
      _condition.wait( &_mutex );
      _mutex.unlock();
      // The sample data should've been made available
      if( _sampleAvailable.tryAcquire() )
	return true;
      else
	return false;
    }
  else
#endif // RETURN_AVAILABLE_SAMPLE
    {
      _timeOut = true;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool DatasamplerThread::getFrame( int timeout_ms )
{
  if( !_sampling ) return false;

  // If necessary wait for the sample buffer clean-up (by the thread),
  // after an earlier time-out
  volatile bool b = _timeOut;
  while( b ) b = this->timeOut();

  this->freeSample();

  _pixIndex = 0;
  _requestFrame = true;

  // Setting _sampleMaxSize unequal to zero is necessary to start sampling
  _sampleMaxSize = 256*256*8;

  // Wait for a frame to become available or time out
  if( _sampleAvailable.tryAcquire( 1, timeout_ms ) )
    return true;

  // Timed out !
  _timeOut = true;
  return false;
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

bool DatasamplerThread::timeOut()
{
  // This function was only added here to prevent the compiler from optimizing
  // a while-loop (see getSample/Frame()) on the _timeOut variable!
  return _timeOut;
}

// ----------------------------------------------------------------------------

void DatasamplerThread::handleTimeOut()
{
  // Clean up an ongoing sample operation after a time-out occurred
  // (this function is to be called from within the thread only)

#ifdef RETURN_AVAILABLE_SAMPLE
  // But still return any sample data available..
  if( _sampleIndex > 0 )
    {
      // Okay, a data sample is now available in the sample buffer
      // (eventhough it is less than the requested minimum size)

      // Indicate the sample buffer can not be filled any further
      if( _sampleBufferEmpty.available() )
	_sampleBufferEmpty.acquire();

      // Indicate a sample is available
      if( _sampleAvailable.available() == 0 )
	_sampleAvailable.release();

      // No further sampling until getSample() is called again
      _sampleMaxSize = 0;

      // getSample() to return with data after all, despite a time-out
      _mutex.lock();
      _condition.wakeOne();
      _mutex.unlock();
    }
  else
#endif // RETURN_AVAILABLE_SAMPLE
    {
      // Just in case a sample has become available right after we timed out...
      if( _sampleAvailable.available() ) _sampleAvailable.acquire();

      if( _sampleBufferEmpty.available() == 0 ) _sampleBufferEmpty.release();
      _sampleIndex = 0;
      _sampleMaxSize = 0;
      _requestFrame = false;
    }

  // Reset the time-out boolean
  _timeOut = false;
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
	  char *bytes = (char *) &pixdata;
	  for( int i=0; i<8; ++i )
	    bytes[i] = _sampleBuffer[_pixIndex+7-i];
	  //pixdata = *((u64 *) bytes);
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
	  char *bytes = (char *) &pixdata;
	  for( int i=0; i<8; ++i )
	    bytes[i] = _sampleBuffer[_pixIndex+7-i];
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

u64 DatasamplerThread::nextPacket()
{
  // Return the next 64-bit/8-byte pixel data packet in the sample buffer
  u64 pixdata=0;
  if( _pixIndex < _sampleIndex )
    {
#ifdef USE_BIGENDIAN
      if( _bigEndian )
        {
          // Reverse the byte order
          char *bytes = (char *) &pixdata;
          for( int i=0; i<8; ++i )
            bytes[i] = _sampleBuffer[_pixIndex+7-i];
        }
      else
      {
        pixdata = _sampleBufferUlong[_pixIndex/8];
      }
#else
      pixdata = _sampleBufferUlong[_pixIndex/8];
#endif

      // Next Timepix3 packet
      _pixIndex += 8;
    }
  return pixdata;
}

// ----------------------------------------------------------------------------

int DatasamplerThread::copySampleToBuffer()
{
  // Collect pixel data into the sample buffer up to a maximum size
  long long bytes = _receiver->bytesAvailable();
  if( bytes + _sampleIndex > _sampleMaxSize )
    bytes = _sampleMaxSize - _sampleIndex;

  // Adhere to the maximum file-write chunk size
  // (data is being sampled synchronously with writing to file, if enabled)
  if( bytes > _fileChunkSize )
    bytes = _fileChunkSize;

  // Copy the data to the sample buffer
  memcpy( static_cast<void *> (&_sampleBuffer[_sampleIndex]),
	  static_cast<void *> (_receiver->data()), bytes );
  _sampleIndex += bytes;

  if( _sampleIndex >= _sampleMinSize )
    {
      // Okay, a data sample is now available in the sample buffer
      _sampleAvailable.release();

      // No further sampling until getSample() is called again
      _sampleMaxSize = 0;
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

	      // No further sampling until getSample/Frame() is called again
	      _sampleMaxSize = 0;
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

bool DatasamplerThread::startRecording( std::string    filename,
					int            runnr,
					unsigned char *pixelconfig )
{
  if( !this->stopRecording() ) return false;

  // Separate given name in a path (optional), a file name and
  // an extension (optional)
  int last_i;
  QString fname = QString::fromStdString( filename );
  fname.replace( QChar('\\'), QChar('/') );
  if( fname.contains( QChar('/') ) ) // Path name present?
    {
      last_i        = fname.lastIndexOf( QChar('/') );
      _fileDirName  = fname.left( last_i );
      _fileBaseName = fname.right( fname.size() - last_i-1 );
    }
  else
    {
      _fileDirName  = QString(".");
      _fileBaseName = fname;
    }
  if( _fileBaseName.contains( QChar('.') ) ) // Extension present?
    {
      last_i        = _fileBaseName.lastIndexOf( QChar('.') );
      _fileExt      = _fileBaseName.right( _fileBaseName.size() - last_i-1 );
      _fileBaseName = _fileBaseName.left( last_i );
    }
  else
    {
      _fileExt = QString("dat");
    }

  // Check existence of the directory and create if necessary
  // (empty _fileDirName equals ".")
  QDir qd;
  if( !_fileDirName.isEmpty() && !qd.exists(_fileDirName) )
    {
      if( !qd.mkpath(_fileDirName) )
	{
	  _errString = "Failed to create dir \"" + _fileDirName + "\"";  
	  return false;
	}
    }

  // Initialize file counter when the run number changes
  if( runnr != _runNr )
    {
      _fileCntr = 1;
      _bytesRecordedInRun = 0;
    }
  _bytesRecorded = 0;

  // Remember run number (for file names)
  _runNr = runnr;

  // Remember pointer to pixel configuration for inclusion in the fileheader
  // (unless the pointer is NULL)
  _pixelConfig = pixelconfig;

  // Instruct the thread to open a file
  _recording = true;

  // Wait for file opened, with time-out
  int cnt = 0;
  while( !_fileOpen && cnt < 200 )
    {
      this->msleep( 10 );
      ++cnt;
    }
  if( cnt == 200 )
    {
      _errString = "Time-out opening file in dir \"" + _fileDirName + "\"";  
      return false;
    }
  return true;
}

// ----------------------------------------------------------------------------

bool DatasamplerThread::stopRecording()
{
  // Return false only in case of time-out on closing the file

  if( !_recording ) return true;

  // Instruct the thread to close any open file
  _recording = false;

  // Wait for file closed, with time-out
  int cnt = 0;
  while( _fileOpen && cnt < 100 )
    {
      this->msleep( 10 );
      ++cnt;
    }
  if( cnt == 100 )
    {
      _errString = "Time-out closing file";  
      return false;
    }
  return true;
}

// ----------------------------------------------------------------------------

bool DatasamplerThread::openFilePrivate()
{
  // Open file function called by thread
  // (this function is to be called from within the thread only)
  this->closeFilePrivate();

  _fileName = this->makeFileName();
  _file.setFileName( _fileName );
  _file.open( QIODevice::WriteOnly );
  if( _file.isOpen() )
    {
      // Write the file header to the file
      _file.write( reinterpret_cast<char *>(&_fileHdr), SPIDRTPX3_HEADER_SIZE );

      // If provided, write the device's pixel configuration to file as well
      if( _pixelConfig )
	_file.write( reinterpret_cast<char *>(_pixelConfig), 256*256 );

      _bytesToFile = 0;
      _fileOpen = true;
      return true;
    }
  _errString = "Failed to open file \"" + _fileName + "\"";  
  return false;
}

// ----------------------------------------------------------------------------

void DatasamplerThread::closeFilePrivate()
{
  // Close file function called by thread
  // (this function is to be called from within the thread only)
  if( _file.isOpen() ) _file.close();
  _fileOpen = false;

  // ### Keep these counters up-to-date rather than updating
  // ### once when closing a file (changed 14 Oct 2014)
  //_bytesRecorded += _bytesToFile;
  //_bytesRecordedInRun += _bytesToFile;
}

// ----------------------------------------------------------------------------

bool DatasamplerThread::openFileOld( std::string filename, bool overwrite )
{
  //this->closeFile();
  this->closeFilePrivate();

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
      _bytesToFile = 0;
      _fileOpen = true;
      return true;
    }
  _errString = "Failed to open file \"" + fname + "\"";  
  return false;
}

// ----------------------------------------------------------------------------

QString DatasamplerThread::makeFileName()
{
  // File name is composed like this:
  // "<dir>/<basename>-<year><month><day>-<hour><min><sec>-<runnr>-<cnt>.<ext>"

  // The directory
  QString qs = _fileDirName;
  if( _fileDirName.length() > 0 ) qs += '/';
  qs += _fileBaseName;

  // Add date and time
  QDateTime dt = QDateTime::currentDateTime();
  qs += dt.toString( QString("-yyMMdd-hhmmss-") );

  // Update parts of the file header
  _fileHdr.seqNr = _fileCntr;
  QDate da = dt.date();
  int y = da.year();
  _fileHdr.yyyyMmDd = (y/1000) << 28;
  y = y - (y/1000)*1000;
  _fileHdr.yyyyMmDd |= (y/100) << 24;
  y = y - (y/100)*100;
  _fileHdr.yyyyMmDd |= ((y/10) << 20) | ((y%10) << 16);
  _fileHdr.yyyyMmDd |= (da.month()/10) << 12 | ((da.month()%10) << 8);
  _fileHdr.yyyyMmDd |= (da.day()/10) << 4 | ((da.day()%10) << 0);
  QTime tm = dt.time();
  _fileHdr.hhMmSsMs  = (tm.hour()/10) << 28 | ((tm.hour()%10) << 24);
  _fileHdr.hhMmSsMs |= (tm.minute()/10) << 20 | ((tm.minute()%10) << 16);
  _fileHdr.hhMmSsMs |= (tm.second()/10) << 12 | ((tm.second()%10) << 8);
  int hsec = tm.msec()/10;
  _fileHdr.hhMmSsMs |= (hsec/10) << 4 | ((hsec%10) << 0);

  if( _runNr != 0 )
    // Add run number, a counter and the extension
    qs += QString( "%1-%2.%3").arg( _runNr ).arg( _fileCntr ).arg( _fileExt );
  else
    // Add a counter and the extension
    qs += QString( "%2.%3").arg( _fileCntr ).arg( _fileExt );

  ++_fileCntr;

  _fileName = qs;
  return qs;
}

// ----------------------------------------------------------------------------

std::string DatasamplerThread::errorString()
{
  if( _errString.isEmpty() ) return std::string( "" );
  QString qs = "Datawriter/sampler: " + _errString;
  return qs.toStdString();
}

// ----------------------------------------------------------------------------
