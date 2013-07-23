#include "FramebuilderThread.h"
#include "ReceiverThread.h"
#include "mpx3conf.h"

#define HEADER_FILLER_WORD 0xDDDDDDDD

// ----------------------------------------------------------------------------

FramebuilderThread::FramebuilderThread( std::vector<ReceiverThread *> recvrs,
					QObject *parent )
  : QThread( parent ),
    _receivers( recvrs ),
    _stop( false ),
    _id( 0 ),
    _callbackFunc( 0 ),
    _framesReceived( 0 ),
    _framesWritten( 0 ),
    _framesProcessed( 0 ),
    _packetsLost( 0 ),
    _decode( false ),
    _compress( false ),
    _flush( false ),
    _hasDecodedFrame( false ),
    _abortFrame( false ),
    _fileOpen( false )
{
  _n = _receivers.size();
  // Preset the headers
  u32 i;
  _evtHdr.headerId   = EVT_HEADER_ID;
  _evtHdr.headerSize = EVT_HEADER_SIZE;
  _evtHdr.format     = EVT_HEADER_VERSION;
  for( i=0; i<sizeof(_evtHdr.unused)/sizeof(u32); ++i )
    _evtHdr.unused[i] = HEADER_FILLER_WORD;
  for( i=0; i<4; ++i )
    {
      _devHdr[i].headerId   = DEV_HEADER_ID;
      _devHdr[i].headerSize = DEV_HEADER_SIZE;
      _devHdr[i].format     = DEV_HEADER_VERSION;
      _devHdr[i].deviceId   = (i+1) * 0x11111111; // Dummy ID
      _devHdr[i].deviceType = MPX_TYPE_NC; // Not connected
      for( u32 j=0; j<sizeof(_devHdr[i].unused)/sizeof(u32); ++j )
	_devHdr[i].unused[j] = HEADER_FILLER_WORD;
    }

  // Generate the 6-bit look-up table (LUT) for Medipix3RX decoding
  int pixcode = 0, bit;
  for( i=0; i<64; i++ )
    {
      _mpx3Rx6BitsLut[pixcode] = i;
      // Next code = (!b0 & !b1 & !b2 & !b3 & !b4) ^ b4 ^ b5
      bit = (pixcode & 0x01) ^ ((pixcode & 0x20)>>5);
      if( (pixcode & 0x1F) == 0 ) bit ^= 1;
      pixcode = ((pixcode << 1) | bit) & 0x3F;
    }

  // Generate the 12-bit look-up table (LUT) for Medipix3RX decoding
  pixcode = 0;
  for( i=0; i<4096; i++ )
    {
      _mpx3Rx12BitsLut[pixcode] = i;
      // Next code = (!b0 & !b1 & !b2 & !b3 & !b4& !b5& !b6 & !b7 &
      //              !b8 & !b9 & !b10) ^ b0 ^ b3 ^ b5 ^ b11
      bit = ((pixcode & 0x001) ^ ((pixcode & 0x008)>>3) ^
             ((pixcode & 0x020)>>5) ^ ((pixcode & 0x800)>>11));
      if( (pixcode & 0x7FF) == 0 ) bit ^= 1;
      pixcode = ((pixcode << 1) | bit) & 0xFFF;
    }

  // Start the thread 
  this->start();
}

// ----------------------------------------------------------------------------

FramebuilderThread::~FramebuilderThread()
{
  // In case still running...
  this->stop();
}

// ----------------------------------------------------------------------------

void FramebuilderThread::stop()
{
  if( this->isRunning() )
    {
      _stop = true;
      _abortFrame = true;
      _mutex.lock();
      _inputCondition.wakeOne();
      _outputCondition.wakeOne();
      _mutex.unlock();
      this->wait(); // Wait until this thread (i.e. function run()) exits
    }
}

// ----------------------------------------------------------------------------

void FramebuilderThread::run()
{
  if( _receivers.empty() ) _stop = true;

  u32 i;
  while( !_stop )
    {
      if( _receivers[0]->hasFrame() )
	{
	  // Wait for the other receivers if necessary, before proceeding
	  _abortFrame = false; // Allows us to abort waiting for frame data
	  for( i=1; i<_n; ++i )
	    while( !(_receivers[i]->hasFrame() || _abortFrame) );

	  if( _fileOpen )
	    this->writeFrameToFile();
	  else
	    this->processFrame();

	  // Release this frame buffer on all receivers
	  // (###NB: how do we prevent 1 receiver to start filling
	  //  the newly released buffer while another receiver
	  //  still has a full buffer and starts (again) overwriting
	  //  its last received frame, potentially causing desynchronized
	  //  event buffers in the various receivers?
	  //  A task for a busy/inhibit mechanism?: set busy before
	  //  filling up any buffer)
	  for( i=0; i<_n; ++i ) _receivers[i]->releaseFrame();

	  ++_framesReceived;
	}
      else
	{
	  _mutex.lock();
	  if( !_receivers[0]->hasFrame() ) _inputCondition.wait( &_mutex );
	  _mutex.unlock();
	}
    }
}

// ----------------------------------------------------------------------------

void FramebuilderThread::inputNotification()
{
  // There should be input data, so check for it in run()
  // (Mutex to make sure that the 'wake' does not occur just in front
  //  of the 'wait' in run())
  _mutex.lock();
  _inputCondition.wakeOne();
  _mutex.unlock();
}

// ----------------------------------------------------------------------------

void FramebuilderThread::abortFrame()
{
  _abortFrame = true;

  // In case the thread is waiting in 'processFrame()'
  _mutex.lock();
  _outputCondition.wakeOne();
  _mutex.unlock();
}

// ----------------------------------------------------------------------------

void FramebuilderThread::processFrame()
{
  // Not writing to file, so if not flushing it all,
  // we expect at least to decode the pixel data
  // otherwise just 'absorb' the frames...
  unsigned int i;
  if( !_flush && _decode )
    {
      // If necessary wait until the previous frame has been consumed
      _mutex.lock();
      if( _hasDecodedFrame ) _outputCondition.wait( &_mutex );
      _mutex.unlock();

      if( _abortFrame ) return; // Bail out

      // The following decoding operations could be done
      // in separate threads (i.e. by QConcurrent?)
      for( i=0; i<_n; ++i )
	_frameSz[i] = this->mpx3RawToPixel( _receivers[i]->frameData(),
					    _decodedFrame[i],
					    _evtHdr.pixelDepth,
					    _devHdr[i].deviceType,
					    _compress );
      _timeStamp = _receivers[0]->timeStampFrame();
      _timeStampSpidr = _receivers[0]->timeStampFrameSpidr();
      _hasDecodedFrame = true;
      ++_framesProcessed;

      //if( _callbackFunc ) _callbackFunc( _id );
    }
}

// ----------------------------------------------------------------------------

void FramebuilderThread::writeFrameToFile()
{
  if( _decode )
    this->writeDecodedFrameToFile();
  else
    this->writeRawFrameToFile();

  _file.flush();
  ++_framesWritten;
}

// ----------------------------------------------------------------------------

void FramebuilderThread::writeRawFrameToFile()
{
  // Get and format the data for this frame from all receivers
  u32 i, sz, evt_sz;

  // Get the frame data sizes
  // (NB: in fact the size is known beforehand from the selected pixel depth)
  evt_sz = _n * DEV_HEADER_SIZE;
  for( i=0; i<_n; ++i )
    {
      sz = _receivers[i]->dataSizeFrame();
      _devHdr[i].dataSize = sz;
      evt_sz += sz;
    }
  _evtHdr.dataSize = evt_sz;

  // Fill in the rest of the event header and write it
  i64 timestamp = _receivers[0]->timeStampFrame();
  _evtHdr.secs  = (u32) (timestamp / 1000);
  _evtHdr.msecs = (u32) (timestamp % 1000);
  _evtHdr.evtNr = _framesReceived;
  _file.write( (const char *) &_evtHdr, EVT_HEADER_SIZE );

  DevHeader_t *p_devhdr;
  for( i=0; i<_n; ++i )
    {
      // Fill in the rest of the device header and write it
      p_devhdr = &_devHdr[i];
      p_devhdr->lostPackets = _receivers[i]->packetsLostFrame();
      _packetsLost += p_devhdr->lostPackets;
      // Copy the saved SPIDR 'header' (6 short ints)
      memcpy( (void *) p_devhdr->spidrHeader,
	      (void *) _receivers[i]->spidrHeaderFrame(),
	      SPIDR_HEADER_SIZE );
      _file.write( (const char *) p_devhdr, DEV_HEADER_SIZE );

      // Write the device frame data
      _file.write( (const char *) _receivers[i]->frameData(),
		   p_devhdr->dataSize );
    }
}

// ----------------------------------------------------------------------------

void FramebuilderThread::writeDecodedFrameToFile()
{
  u32 i;

  // The following decoding operations could be done
  // in separate threads (i.e. by QConcurrent)
  int frame_sz[4];
  for( i=0; i<_n; ++i )
    frame_sz[i] = this->mpx3RawToPixel( _receivers[i]->frameData(),
					_decodedFrame[i],
					_evtHdr.pixelDepth,
					_devHdr[i].deviceType,
					_compress );

  _evtHdr.dataSize = _n * DEV_HEADER_SIZE;
  for( i=0; i<_n; ++i )
    _evtHdr.dataSize += frame_sz[i];

  // Fill in the rest of the event header and write it
  i64 timestamp = _receivers[0]->timeStampFrame();
  _evtHdr.secs  = (u32) (timestamp / 1000);
  _evtHdr.msecs = (u32) (timestamp % 1000);
  _evtHdr.evtNr = _framesWritten;
  _file.write( (const char *) &_evtHdr, EVT_HEADER_SIZE );

  DevHeader_t *p_devhdr;
  for( i=0; i<_n; ++i )
    {
      // Fill in the rest of the device header and write it
      p_devhdr = &_devHdr[i];
      p_devhdr->lostPackets = _receivers[i]->packetsLostFrame();
      _packetsLost += p_devhdr->lostPackets;
      // Copy the saved SPIDR 'header' (6 short ints)
      memcpy( (void *) p_devhdr->spidrHeader,
	      (void *) _receivers[i]->spidrHeaderFrame(),
	      SPIDR_HEADER_SIZE );
      _file.write( (const char *) p_devhdr, DEV_HEADER_SIZE );

      // Write the decoded frame data
      _file.write( (const char *) _decodedFrame[i], frame_sz[i] );
    }
}

// ----------------------------------------------------------------------------

int FramebuilderThread::mpx3RawToPixel( unsigned char *raw_bytes,
					int           *pixels,
					int            counter_depth,
					int            device_type,
					bool           compress )
{
  // Convert MPX3 raw bit stream in byte array 'raw_bytes'
  // into n-bits pixel values in array 'pixel' (with n=counter_depth)
  int            counter_bits, row, col, offset, pixelbit;
  int            bitmask;
  int           *ppix;
  unsigned char  byte;
  unsigned char *praw;

  // Necessary to globally clear the pixels array
  // as we only use '|' (OR) in the assignments below
  memset( static_cast<void *> (pixels), 0, MPX_PIXELS * sizeof(int) );

  // Raw data arrives as: all bits n+1 from 1 row of pixels,
  // followed by all bits n from the same row of pixels, etc.
  // (so bit 'counter-bits-1', the highest bit comes first),
  // until all bits of this pixel row have arrived,
  // then the same happens for the next row of pixels, etc.
  // NB: for 24-bits readout data arrives as:
  // all bits 11 to 0 for the 1st row, bits 11 to 0 for the 2nd row,
  // etc for all 256 rows as for other pixel depths,
  // then followed by bits 23 to 12 for the 1st row, etc,
  // again for all 256 rows.
  if( counter_depth <= 12 )
    counter_bits = counter_depth;
  else
    counter_bits = 12;
  offset = 0;
  praw = raw_bytes;
  for( row=0; row<MPX_PIXEL_ROWS; ++row )
    {
      bitmask = (1 << (counter_bits-1));
      for( pixelbit=counter_bits-1; pixelbit>=0; --pixelbit )
	{
	  ppix = &pixels[offset];
	  // All bits 'pixelbit' of one pixel row (= 256 pixels or columns)
	  for( col=0; col<MPX_PIXEL_COLUMNS; col+=8 )
	    {
	      // Process raw data byte-by-byte
	      byte = *praw;
	      if( byte & 0x80 ) ppix[0] |= bitmask;
	      if( byte & 0x40 ) ppix[1] |= bitmask;
	      if( byte & 0x20 ) ppix[2] |= bitmask;
	      if( byte & 0x10 ) ppix[3] |= bitmask;
	      if( byte & 0x08 ) ppix[4] |= bitmask;
	      if( byte & 0x04 ) ppix[5] |= bitmask;
	      if( byte & 0x02 ) ppix[6] |= bitmask;
	      if( byte & 0x01 ) ppix[7] |= bitmask;
	      ppix += 8;
	      ++praw; // Next raw byte
	    }
	  bitmask >>= 1;
	}
      offset += MPX_PIXEL_COLUMNS;
    }

  // In case of 24-bit pixels a second 'frame' follows
  // containing bits 23 to 12 of each pixel
  if( counter_depth == 24 )
    {
      offset = 0;
      for( row=0; row<MPX_PIXEL_ROWS; ++row )
	{
	  bitmask = (1 << (24-1));
	  for( pixelbit=24-1; pixelbit>=12; --pixelbit )
	    {
	      ppix = &pixels[offset];
	      // All bits 'pixelbit' of one pixel row (= 256 pixels or columns)
	      for( col=0; col<MPX_PIXEL_COLUMNS; col+=8 )
		{
		  // Process raw data byte-by-byte
		  byte = *praw;
		  if( byte & 0x80 ) ppix[0] |= bitmask;
		  if( byte & 0x40 ) ppix[1] |= bitmask;
		  if( byte & 0x20 ) ppix[2] |= bitmask;
		  if( byte & 0x10 ) ppix[3] |= bitmask;
		  if( byte & 0x08 ) ppix[4] |= bitmask;
		  if( byte & 0x04 ) ppix[5] |= bitmask;
		  if( byte & 0x02 ) ppix[6] |= bitmask;
		  if( byte & 0x01 ) ppix[7] |= bitmask;
		  ppix += 8;
		  ++praw; // Next raw byte
		}
	      bitmask >>= 1;
	    }
	  offset += MPX_PIXEL_COLUMNS;
	}
    }

  // If necessary, apply a look-up table (LUT)
  if( device_type == MPX_TYPE_MPX3RX && counter_depth > 1 )
    {
      // Medipix3RX device: apply LUT
      if( counter_depth == 6 )
	{
	  for( int i=0; i<MPX_PIXELS; ++i )
	    pixels[i] = _mpx3Rx6BitsLut[pixels[i] & 0x3F];
	}
      else if( counter_depth == 12 )
	{
	  for( int i=0; i<MPX_PIXELS; ++i )
	    pixels[i] = _mpx3Rx12BitsLut[pixels[i] & 0xFFF];
	}
      else if( counter_depth == 24 )
	{
	  int pixval;
	  for( int i=0; i<MPX_PIXELS; ++i )
	    {
	      pixval     = pixels[i];
	      // Lower 12 bits
	      pixels[i]  = _mpx3Rx12BitsLut[pixval & 0xFFF];
	      // Upper 12 bits
	      pixval     = (pixval >> 12) & 0xFFF;
	      pixels[i] |= (_mpx3Rx12BitsLut[pixval] << 12);
	    }
	}
    }

  // Return a size in bytes
  int size = (MPX_PIXELS * sizeof(int));
  if( !compress ) return size;
  
  // Compress 4- and 6-bit frames into 1 byte per pixel
  // and 12-bit frames into 2 bytes per pixel (1-bit frames already
  // available 'compressed' into 1 bit per pixel in array 'raw_bytes')
  if( counter_depth == 12 )
    {
      u16 *pixels16 = (u16 *) pixels;
      int *pixels32 = (int *) pixels;
      for( int i=0; i<MPX_PIXELS; ++i, ++pixels16, ++pixels32 )
	*pixels16 = (u16) ((*pixels32) & 0xFFFF);
      size = (MPX_PIXELS * sizeof( u16 ));
    }
  else if( counter_depth == 4 || counter_depth == 6 )
    {
      u8  *pixels8  = (u8 *)  pixels;
      int *pixels32 = (int *) pixels;
      for( int i=0; i<MPX_PIXELS; ++i, ++pixels8, ++pixels32 )
	*pixels8 = (u8) ((*pixels32) & 0xFF);
      size = (MPX_PIXELS * sizeof( u8 ));
    }
  else if( counter_depth == 1 )
    {
      // 1-bit frame: just copy the raw frame over into array 'pixels'
      // so that it becomes a 'decoded' and 'compressed' frame
      memcpy( (void *) pixels, (void *) raw_bytes, MPX_PIXELS/8 );
      size = (MPX_PIXELS / 8);
    }
  return size;
}

// ----------------------------------------------------------------------------

int *FramebuilderThread::decodedFrameData( int index, int *size )
{
  if( _hasDecodedFrame )
    *size = _frameSz[index];
  else
    *size = 0;
  return &_decodedFrame[index][0];
}

// ----------------------------------------------------------------------------

void FramebuilderThread::releaseDecodedFrame()
{
  _mutex.lock();
  _hasDecodedFrame = false;
  _outputCondition.wakeOne();
  _mutex.unlock();
}

// ----------------------------------------------------------------------------

i64 FramebuilderThread::decodedFrameTimestamp()
{
  return _timeStamp;
}

// ----------------------------------------------------------------------------

i64 FramebuilderThread::decodedFrameTimestampSpidr()
{
  return _timeStampSpidr;
}

// ----------------------------------------------------------------------------

double FramebuilderThread::decodedFrameTimestampDouble()
{
  u32 secs  = (u32) (_timeStamp / (i64)1000);
  u32 msecs = (u32) (_timeStamp % (i64)1000);
  return( (double) secs + ((double) msecs)/1000.0 );
}

// ----------------------------------------------------------------------------

void FramebuilderThread::setAddrInfo( int *ipaddr,
				      int *ports )
{
  _evtHdr.ipAddress = ((ipaddr[0] << 0)  | (ipaddr[1] << 8) |
		       (ipaddr[2] << 16) | (ipaddr[3] << 24));
  u32 ndevs = 0;
  for( int i=0; i<4; ++i )
    {
      _evtHdr.ports[i] = ports[i];
      if( ports[i] > 0 ) ++ndevs;
    }
  _evtHdr.nrOfDevices = ndevs;
}

// ----------------------------------------------------------------------------

void FramebuilderThread::setDeviceIds( int *ids )
{
  for( u32 i=0; i<4; ++i )
    if( ids[i] != 0 ) _devHdr[i].deviceId = ids[i];
}

// ----------------------------------------------------------------------------

void FramebuilderThread::setDeviceTypes( int *types )
{
  for( u32 i=0; i<4; ++i ) _devHdr[i].deviceType = types[i];
}

// ----------------------------------------------------------------------------

void FramebuilderThread::setPixelDepth( int nbits )
{
  _evtHdr.pixelDepth = nbits;
}

// ----------------------------------------------------------------------------

void FramebuilderThread::setDecodeFrames( bool decode )
{
  _decode = decode;
  for( int i=0; i<4; ++i )
    {
      if( decode )
	_devHdr[i].format |= DEV_DATA_DECODED;
      else
	_devHdr[i].format &= ~DEV_DATA_DECODED;
    }
}

// ----------------------------------------------------------------------------

void FramebuilderThread::setCompressFrames( bool compress )
{
  _compress = compress;
  for( int i=0; i<4; ++i )
    {
      if( compress )
	_devHdr[i].format |= DEV_DATA_COMPRESSED;
      else
	_devHdr[i].format &= ~DEV_DATA_COMPRESSED;
    }
}

// ----------------------------------------------------------------------------

bool FramebuilderThread::openFile( std::string filename, bool overwrite )
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
      _framesWritten = 0;
      _packetsLost = 0;
      _fileOpen = true;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

bool FramebuilderThread::closeFile()
{
  if( _file.isOpen() )
    {
      _file.close();
      _fileOpen = false;
      return true;
    }
  return false;
}

// ----------------------------------------------------------------------------

std::string FramebuilderThread::errString()
{
  if( _errString.isEmpty() ) return std::string( "" );
  QString qs = "Framebuilder: " + _errString;
  return qs.toStdString();
}

// ----------------------------------------------------------------------------
