#include <QUdpSocket>
#include <QAbstractSocket>

#include "ReceiverThreadC.h"
#include "mpx3defs.h"

#define ALLOW_BIGENDIAN_OPTION

#define byteswap(x) ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

// ----------------------------------------------------------------------------

ReceiverThreadC::ReceiverThreadC( int *ipaddr,
				int  port,
				QObject *parent )
  : _rowCnt( 0 ),
    _rowPixels( MPX_PIXEL_COLUMNS ),
    _pixelsReceived( 0 ),
    _pixelsLost( 0 ),
    _framePtr( (u64 *)_frameBuffer[0] ),
    _shutterCnt( 1 ),
    _bigEndian( false ),
    ReceiverThread( ipaddr, port, parent )
{
  for( u32 i=0; i<NR_OF_FRAMEBUFS; ++i )
    _frameSize[i] = 0;

  for( u32 i=0; i<NR_OF_FRAMEBUFS; ++i )
    _pixelsLostFrame[i] = 8888;
  _pixelsLostFrame[_head] = 0;

  // Maintain a 'shutter counter' in the 'SPIDR header'
  // (not produced by the Compact-SPIDR hardware)
  u16 *header = (u16 *) _headerBuffer[_head];
  header[1] = byteswap( _shutterCnt );
}

// ----------------------------------------------------------------------------

ReceiverThreadC::~ReceiverThreadC()
{
}

// ----------------------------------------------------------------------------

void ReceiverThreadC::readDatagrams()
{
  int  i, recvd_sz;
  bool copy;
  u64 *pixelpkt, pixelword;
  u64  type;
  int  pix_per_word = 60/_pixelDepth;

#ifndef USE_NATIVE_SOCKET
  while( _sock->hasPendingDatagrams() )
    {
      recvd_sz = _sock->readDatagram( _recvBuffer, RECV_BUF_SIZE );
      if( recvd_sz <= 0 ) continue;
#else
    {
      recvd_sz = _sock->read( _recvBuffer, RECV_BUF_SIZE );
      if( recvd_sz <= 0 ) return;
#endif // USE_NATIVE_SOCKET
      ++_packetsReceived;

      // Parse the data in the received UDP packet
      pixelpkt = (u64 *) _recvBuffer;
      for( i=0; i<recvd_sz/sizeof(u64); ++i, ++pixelpkt )
	{
#ifdef ALLOW_BIGENDIAN_OPTION
	  if( _bigEndian )
	    {
	      // Reverse the byte order
	      char *bytes = (char *) &pixelword;
	      char *p     = (char *) pixelpkt;
	      for( int i=0; i<8; ++i )
		bytes[i] = p[7-i];
	      *pixelpkt = pixelword;
	    }
#endif // ALLOW_BIGENDIAN_OPTION

	  type = (*pixelpkt) & PKT_TYPE_MASK;
	  copy = true;

	  switch( type )
	    {
	    case PIXEL_DATA_SOR:
	      if( _rowPixels < MPX_PIXEL_COLUMNS )
		{
		  // Lost some pixels of the previous row?
		  _pixelsLost             += MPX_PIXEL_COLUMNS - _rowPixels;
		  _pixelsLostFrame[_head] += MPX_PIXEL_COLUMNS - _rowPixels;
		}
	      if( _rowCnt == 0 ) ++_rowCnt; // Must've missed SOF
	      ++_rowCnt;
	      if( _rowCnt > MPX_PIXEL_ROWS )
		{
		  // Can't be correct.. what to do?
		  // Start filling a next frame
		  this->nextFrame();
		}
	      _rowPixels = 0;
	      _rowPixels += pix_per_word;
	      break;

	    case PIXEL_DATA_EOR:
	      _rowPixels += pix_per_word;
	      if( _rowPixels < MPX_PIXEL_COLUMNS )
		{
		  // Lost some pixels of this row?
		  _pixelsLost             += MPX_PIXEL_COLUMNS - _rowPixels;
		  _pixelsLostFrame[_head] += MPX_PIXEL_COLUMNS - _rowPixels;
		  // Don't count them again at next SOR
		  _rowPixels = MPX_PIXEL_COLUMNS;
		}
	      if( _rowCnt >= MPX_PIXEL_ROWS )
		{
		  // Can't be correct.. what to do?
		  _rowPixels = 0;
		  _rowPixels += pix_per_word;

		  // Start filling a next frame
		  this->nextFrame();
		}
	      break;

	    case PIXEL_DATA_SOF:
	      if( _rowPixels < MPX_PIXEL_COLUMNS )
		{
		  // Lost some pixels ?
		  _pixelsLost             += MPX_PIXEL_COLUMNS - _rowPixels;
		  _pixelsLostFrame[_head] += MPX_PIXEL_COLUMNS - _rowPixels;
		}
	      ++_rowCnt;
	      _rowPixels = 0;
	      _rowPixels += pix_per_word;
	      break;

	    case PIXEL_DATA_EOF:
	      _rowPixels += pix_per_word;
	      if( _rowPixels < MPX_PIXEL_COLUMNS )
		{
		  // Lost some pixels ?
		  _pixelsLost             += MPX_PIXEL_COLUMNS - _rowPixels;
		  _pixelsLostFrame[_head] += MPX_PIXEL_COLUMNS - _rowPixels;
		  // Don't count them again at next SOF/SOR
		  _rowPixels = MPX_PIXEL_COLUMNS;
		}
	      break;

	    case PIXEL_DATA_MID:
	      _rowPixels += pix_per_word;
	      if( _rowPixels >= MPX_PIXEL_COLUMNS )
		{
		  // Unexpected: did we miss a SOR ?
		  ++_rowCnt;
		  if( _rowCnt > MPX_PIXEL_ROWS )
		    {
		      // Can't be correct.. what to do?
		      // Start filling a next frame
		      this->nextFrame();
		    }
		  _rowPixels = 0;
		  _rowPixels += pix_per_word;
		}
	      break;

	    default:
	      // Skip this packet
	      copy = false;
	      break;
	    }

	  if( copy )
	    {
	      *_framePtr = *pixelpkt;
	      ++_framePtr;
	      _frameSize[_head] += 8;
	      // Don't count 'padding' (empty/unused) pixel data
	      if( _rowPixels > MPX_PIXEL_COLUMNS )
		_pixelsReceived += (pix_per_word -
				    (_rowPixels-MPX_PIXEL_COLUMNS));
	      else
		_pixelsReceived += pix_per_word;

	      if( type == PIXEL_DATA_EOF )
		// Start filling a next frame
		this->nextFrame();
	    }
	}
    }
}

// ----------------------------------------------------------------------------

void ReceiverThreadC::nextFrame()
{
  // Count our losses...
  if( _rowCnt < MPX_PIXEL_ROWS )
    {
      // Lost whole pixel rows ?
      _pixelsLost += (MPX_PIXEL_ROWS-_rowCnt) * MPX_PIXEL_COLUMNS;
      _pixelsLostFrame[_head] +=
	(MPX_PIXEL_ROWS-_rowCnt) * MPX_PIXEL_COLUMNS;
      _rowCnt = MPX_PIXEL_ROWS;
    }

  // Start filling next frame
  ++_framesReceived;
  this->nextFrameBuffer();

  // Additional initializations
  _framePtr               = (u64 *) _frameBuffer[_head];
  _frameSize[_head]       = 0;
  _rowCnt                 = 0;
  _pixelsLostFrame[_head] = 0;

  // Maintain a 'shutter counter' in the 'SPIDR header'
  // (not produced by the Compact-SPIDR hardware)
  ++_shutterCnt;
  u16 *header = (u16 *) _headerBuffer[_head];
  header[1] = byteswap( _shutterCnt );
}

// ----------------------------------------------------------------------------
