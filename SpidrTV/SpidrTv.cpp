#include "SpidrTv.h"
//#include "emmintrin.h"

QString VERSION( "v1.2.0   25-Mar-2013" );

// ----------------------------------------------------------------------------

SpidrTv::SpidrTv()
  : QMainWindow(),
    _recvr( 0 ),
    _image( QSize(256,256), QImage::Format_Indexed8 )
{
  this->setupUi(this);

  _lbVersion->setText( VERSION );

  connect( _tbOn, SIGNAL(clicked()), this, SLOT(onOff()) );

  connect( _cbCounterDepth, SIGNAL(currentIndexChanged(int)),
	   this, SLOT(changeCounterDepth()) );

  connect( _cbDeviceType, SIGNAL(currentIndexChanged(int)),
	   this, SLOT(changeDeviceType()) );

  this->changeCounterDepth();
  this->changeDeviceType();

  // Generate the 6-bit look-up table for Medipix3RX pixel decoding
  int pixcode = 0, bit, i;
  for( i=0; i<64; i++ )
    {
      _mpx3Rx6Bits[pixcode] = i;
      // Next code = (!b0 & !b1 & !b2 & !b3 & !b4) ^ b4 ^ b5
      bit = (pixcode & 0x01) ^ ((pixcode & 0x20)>>5);
      if( (pixcode & 0x1F) == 0 ) bit ^= 1;
      pixcode = ((pixcode << 1) | bit) & 0x3F;
    }

  // Generate the 12-bit look-up table for Medipix3RX pixel decoding
  pixcode = 0;
  for( i=0; i<4096; i++ )
    {
      _mpx3Rx12Bits[pixcode] = i;
      // Next code = (!b0 & !b1 & !b2 & !b3 & !b4& !b5& !b6 & !b7 &
      //              !b8 & !b9 & !b10) ^ b0 ^ b3 ^ b5 ^ b11
      bit = ((pixcode & 0x001) ^ ((pixcode & 0x008)>>3) ^
             ((pixcode & 0x020)>>5) ^ ((pixcode & 0x800)>>11));
      if( (pixcode & 0x7FF) == 0 ) bit ^= 1;
      pixcode = ((pixcode << 1) | bit) & 0xFFF;
    }

  startTimer( 200 );
}

// ----------------------------------------------------------------------------

SpidrTv::~SpidrTv()
{
}

// ----------------------------------------------------------------------------
#ifdef OLD_CODE
void SpidrTv::decodeAndDisplay()
{
  short mtx[16];
  __m128i op;
  __m128i posMask = _mm_set_epi16( 0x01, 0x02, 0x04, 0x08,
				   0x10, 0x20, 0x40, 0x80 );
  __m128i bitMask;
  __m128i output;
  int maxV = this->sbMaxValue->value();
  int minV = this->sbMinValue->value();
  int p;
  uchar *pix  = _image.bits();
  char  *line = _rawFrame;
  for (int y = 0; y < 255; ++y)
    {
      for (int x = 0; x < 32; x++)
	{
	  // SSE2 baby!
	  bitMask =   _mm_set1_epi16(1 << 11);
	  output =    _mm_setzero_si128();
	  for (int bits = 0; bits < 12; bits++)
	    {
	      /* 
		 Take 8 bits (each bit is for one pixel),
		 put all 8 x 16 bit registers (LSW). Eventually
		 each register is one pixel value.

		 input: L = line[x + bits * 32]

		 SSE2 reg 'op' after operation (example for L = 0x57):
		 +-----+-----+-----+-----+-----+-----+-----+-----+
		 |x0057|x0057|x0057|x0057|x0057|x0057|x0057|x0057|
		 +-----+-----+-----+-----+-----+-----+-----+-----+

	      */
	      op = _mm_set1_epi16(line[x + bits * 32]);    
	      /*
		'AND' the positional mask for each register, 
		so that only the bit corresponding to that pixel
		remains in that register. (bit may still be 1 or 0)

		SSE2 reg 'op' after operation:
		+-----+-----+-----+-----+-----+-----+-----+-----+
		|x0001|x0002|x0004|x0000|x0010|x0000|x0040|x0000|
		+-----+-----+-----+-----+-----+-----+-----+-----+
	      */
	      op = _mm_and_si128(op, posMask);             
	      /*
		Use the compare function with the positional mask.
		This will set each pixel to ffff if that bit is on
		or 0000 if it is off.

		SSE2 reg 'op' after operation:
		+-----+-----+-----+-----+-----+-----+-----+-----+
		|xFFFF|xFFFF|xFFFF|x0000|xFFFF|x0000|xFFFF|x0000|
		+-----+-----+-----+-----+-----+-----+-----+-----+

	      */
	      op = _mm_cmpeq_epi16(op, posMask);

	      /*
		And it with the bitMask, which corresponds to 
		the current bit position (bit). 

                    
		SSE2 reg 'op' after operation (for example,
		bit = 3, bitMask therefor is 1 << bit, for
		each 16 bit word)):

		SSE2 reg 'op' after operation:
		+-----+-----+-----+-----+-----+-----+-----+-----+
		|x0004|x0004|x0004|x0000|x0004|x0000|x0004|x0000|
		+-----+-----+-----+-----+-----+-----+-----+-----+
	      */
	      op = _mm_and_si128(op, bitMask);

	      /*
		Or this with the output, in which bits 11-4 
		are already set.

		SSE2 reg 'output' after operation (values are
		fictive, only bit at position 3 has been set if
		op reg had that bit set):
		+-----+-----+-----+-----+-----+-----+-----+-----+
		|x0A34|x0F24|x03A4|x0310|x0BA4|x0550|x01C4|x01F0|
		+-----+-----+-----+-----+-----+-----+-----+-----+
	      */
	      output = _mm_or_si128(op, output);

	      /*
		Shift the bitmask to the left, now we're going
		to do bit at position 2 (for example).
	      */
	      bitMask = _mm_srli_epi16(bitMask, 1);
	    }
	  _mm_storeu_si128((__m128i*)mtx, output);
            
	  for (int i = 0; i < 8; ++i)
	    {
	      p = (255 * (mtx[i] - minV)) / (maxV - minV);
	      //(*pix) = p < 0 ? 0 : p > 255 ? 255 : p;
	      if( p < 0 )
		p = 0;
	      else if( p > 255 )
		p = 255;
	      *pix = p;
	      pix++;
	    }
	}
      line += 12 * 32;
    }
  _lbView->setPixmap( QPixmap::fromImage(_image) );
}
#endif // OLD_CODE
// ----------------------------------------------------------------------------

void SpidrTv::decodeAndDisplay()
{
  // Decode the raw frame
  mpx3RawToPixel( _rawFrame, _pixels, _counterDepth, _deviceType );

  // Convert the pixels to QImage bytes
  int max = _sbMaxValue->value();
  int min = _sbMinValue->value();
  uchar *img = _image.bits();
  int *p = _pixels, pixval;
  for( int pix=0; pix<256*256; ++pix )
    {
      pixval = *p;
      if( pixval >= max )
	img[pix] = 255;
      else if( pixval <= min )
	img[pix] = 0;
      else
	img[pix] = (unsigned char) ((255 * (pixval-min)) / (max-min));
      ++p;
    }

  // Display the image
  _lbView->setPixmap( QPixmap::fromImage(_image) );
}

// ----------------------------------------------------------------------------

void SpidrTv::mpx3RawToPixel( unsigned char *raw_bytes,
			      int           *pixels,
			      int            counter_depth,
			      int            device_type )
{
  // Convert MPX3 raw bit stream in byte array 'raw_bytes'
  // into n-bits pixel values in array 'pixel' (with n=counter_bits)
  int            counter_bits, row, col, offset, pixelbit;
  int            bitmask;
  int           *ppix;
  unsigned char  byte;
  unsigned char *praw;

  // Necessary to globally clear the pixels array
  // as we only use '|' (OR) in the assignments below
  memset( static_cast<void *> (pixels), 0, 256*256 * sizeof(int) );

  // Data arrives as: all bits n+1 from 1 row of pixels,
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
  for( row=0; row<256; ++row )
    {
      bitmask = (1 << (counter_bits-1));
      for( pixelbit=counter_bits-1; pixelbit>=0; --pixelbit )
	{
	  ppix = &pixels[offset];
	  // All bits 'pixelbit' of one pixel row (= 256 pixels or columns)
	  for( col=0; col<256; col+=8 )
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
      offset += 256;
    }

  // In case of 24-bit pixels a second 'frame' follows
  // containing bits 23 to 12 of each pixel
  if( counter_depth == 24 )
    {
      offset = 0;
      for( row=0; row<256; ++row )
	{
	  bitmask = (1 << (24-1));
	  for( pixelbit=24-1; pixelbit>=12; --pixelbit )
	    {
	      ppix = &pixels[offset];
	      // All bits 'pixelbit' of one pixel row (= 256 pixels or columns)
	      for( col=0; col<256; col+=8 )
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
	  offset += 256;
	}
    }

  // If necessary, apply a look-up table (LUT)
  if( device_type == 2 && counter_depth > 1 )
    {
      // Medipix3RX device: apply LUT
      if( counter_depth == 6 )
	{
	  for( int i=0; i<256*256; ++i )
	    pixels[i] = _mpx3Rx6Bits[pixels[i] & 0x3F];
	}
      else if( counter_depth == 12 )
	{
	  for( int i=0; i<256*256; ++i )
	    pixels[i] = _mpx3Rx12Bits[pixels[i] & 0xFFF];
	}
      else if( counter_depth == 24 )
	{
	  int pixval;
	  for( int i=0; i<256*256; ++i )
	    {
	      pixval     = pixels[i];
	      // Lower 12 bits
	      pixels[i]  = _mpx3Rx12Bits[pixval & 0xFFF];
	      // Upper 12 bits
	      pixval     = (pixval >> 12) & 0xFFF;
	      pixels[i] |= (_mpx3Rx12Bits[pixval] << 12);
	    }
	}
    }

  // ### DEBUG
  if( 0 ) //counter_depth == 24 )
    {
  memset( static_cast<void *> (pixels), 0, 256*256 * sizeof(int) );
  if( counter_depth <= 12 )
    counter_bits = counter_depth;
  else
    counter_bits = 12;
  offset = 0;
  //praw = raw_bytes;
  for( row=0; row<256; ++row )
    {
      bitmask = (1 << (counter_bits-1));
      for( pixelbit=counter_bits-1; pixelbit>=0; --pixelbit )
	{
	  ppix = &pixels[offset];
	  // All bits 'pixelbit' of one pixel row (= 256 pixels or columns)
	  for( col=0; col<256; col+=8 )
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
      offset += 256;
    }
    }
}

// ----------------------------------------------------------------------------

void SpidrTv::timerEvent(QTimerEvent *)
{
  if( _recvr == 0 )
    {
      //_lbFramesSkipped->setText("<none>");
      //_lbFramesRecv->setText("<none>");
      //_lbPacketsRecv->setText("<none>");
      //_lbPacketsLost->setText("<none>");
      //_lbShutterCount->setText("<none>");
      //_lbFrameCount->setText("<none>");
      //_lbSeqNumber->setText("<none>");
    }
  else
    {
      if( _recvr->error().isNull() )
	{
	  this->statusBar()->clearMessage();

	  _lbFramesSkipped->setText(QString::number(_recvr->framesSkipped()));
	  _lbFramesRecv->setText(QString::number(_recvr->framesReceived()));
	  _lbPacketsRecv->setText(QString::number(_recvr->packetsReceived()));
	  _lbDebugCounter->setText(QString::number(_recvr->debugCounter()));
	  _lbPacketsLost->setText(QString::number(_recvr->packetsLost()));
	  /*
	  this->lbShutterCount->
	    setText(QString::number(_recvr->lastShutterCount()));
	  this->lbFrameCount->
	    setText(QString::number(_recvr->lastTriggerCount()));
	  this->lbSeqNumber->
	    setText(QString::number(_recvr->lastSequenceNr()));
	  */
	  if( _recvr->copyFrame( _rawFrame ) )
	    {
	      decodeAndDisplay();
	    }
	}
      else
	{
	  this->statusBar()->showMessage( _recvr->error() );
	  _tbOn->setText("On");
	  _recvr->stop();
	  _recvr->setParent(0);
	  delete _recvr;
	  _recvr = 0;
	}
    }
}

// ----------------------------------------------------------------------------

void SpidrTv::onOff()
{
  if( _recvr == 0 )
    {
      _tbOn->setText( "Off" );
      _recvr = new ReceiverThread( _leAdapter->text(), _sbPort->value(),
				   _counterDepth, this );
    }
  else
    {
      _tbOn->setText( "On" );
      _recvr->stop();
      _recvr->setParent(0);
      delete _recvr;
      _recvr = 0;
    }
}

// ----------------------------------------------------------------------------

void SpidrTv::changeCounterDepth()
{
  _counterDepth = _cbCounterDepth->currentText().toInt();

  _sbMinValue->setMaximum( (1<<_counterDepth)-1 );
  _sbMaxValue->setMaximum( (1<<_counterDepth)-1 );

  _sbMaxValue->setValue( (1<<_counterDepth)-1 );

  if( _sbMinValue->value() > _sbMaxValue->value() )
    _sbMinValue->setValue( 0 );
}

// ----------------------------------------------------------------------------

void SpidrTv::changeDeviceType()
{
  int index = _cbDeviceType->currentIndex();
  _deviceType = index + 1;
  if( index == 0 ) // Medipix3.1
    {
      int i = _cbCounterDepth->findText( "6" );
      if( i > -1 ) _cbCounterDepth->setItemText( i, "4" );
    }
  else
    {
      // Medipix3RX
      int i = _cbCounterDepth->findText( "4" );
      if( i > -1 ) _cbCounterDepth->setItemText( i, "6" );
    }
  _counterDepth = _cbCounterDepth->currentText().toInt();
}

// ----------------------------------------------------------------------------
