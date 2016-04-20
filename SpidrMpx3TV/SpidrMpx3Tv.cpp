#include "mpx3defs.h"
#include "SpidrMpx3Tv.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include <QHostAddress>

QString   VERSION( "v2.1.0   24-Mar-2016" );
//QString VERSION( "v2.0.0   22-Jul-2013" );

// ----------------------------------------------------------------------------

SpidrMpx3Tv::SpidrMpx3Tv()
  : QMainWindow(),
    _controller( 0 ), _daq( 0 ),
    _isCompactSpidr( false ),
    _image1( QSize(256,256), QImage::Format_Indexed8 ),
    _image4( QSize(512,512), QImage::Format_Indexed8 ),
    _deviceCount( 0 )
{
  this->setupUi(this);

  _lbVersion->setText( VERSION );

  connect( _actionExit, SIGNAL(activated()), this, SLOT(close()) );

  connect( _tbOn, SIGNAL(clicked()), this, SLOT(onOff()) );

  connect( _comboBoxCounterDepth, SIGNAL(currentIndexChanged(int)),
	   this, SLOT(changeCounterDepth()) );

  connect( _checkBoxLutEnable, SIGNAL(stateChanged(int)),
	   this, SLOT(changeLutEnabled()) );

  // Initializes _counterDepth
  this->changeCounterDepth();

  startTimer( 50 );
}

// ----------------------------------------------------------------------------

SpidrMpx3Tv::~SpidrMpx3Tv()
{
  if( _controller ) this->onOff();
}

// ----------------------------------------------------------------------------

void SpidrMpx3Tv::decodeAndDisplay()
{
  // The (decoded) frame data (from the first device present, so index 0)
  int  size;
  int *pixeldata;

  int  max = _sbMaxValue->value();
  int  min = _sbMinValue->value();

  if( _deviceCount == 1 )
    {
      // Convert the pixel data to QImage bytes
      uchar *img = _image1.bits();
      int    val;
      pixeldata = _daq->frameData( 0, &size );
      for( int i=0; i<256*256; ++i, ++img, ++pixeldata )
	{
	  val = *pixeldata;
	  if( val >= max )
	    *img = 255;
	  else if( val <= min )
	    *img = 0;
	  else
	    *img = (unsigned char) ((255 * (val-min)) / (max-min));
	}
    }
  else
    {
      uchar *img;
      int dev, x_offs, y_offs, x, y, val;
      for( dev=0; dev<_deviceCount; ++dev )
	{
	  // A device frame covers one 4th of the resulting image
	  // (the 512x512 image being divided into four 256x256 squares,
	  //  device numbers in a 'quad' are as follows: 0 1
	  //                                             3 2
	  pixeldata = _daq->frameData( dev, &size );
	  x_offs = 0;
	  y_offs = 0;
	  if( dev == 1 || dev == 2 ) x_offs = 256;
	  if( dev == 2 || dev == 3 ) y_offs = 256;
	  img = _image4.bits();
	  img += y_offs*512 + x_offs;
	  for( y=0; y<256; ++y )
	    {
	      for( x=0; x<256; ++x, ++img, ++pixeldata )
		{
		  val = *pixeldata;
		  if( val >= max )
		    *img = 255;
		  else if( val <= min )
		    *img = 0;
		  else
		    *img = (unsigned char) ((255 * (val-min)) / (max-min));
		}
	      img += 256;
	    }
	}
    }
  _daq->releaseFrame();

  // Display the image
  if( _deviceCount == 1 )
    _lbView->setPixmap( QPixmap::fromImage(_image1) );
  else
    _lbView->setPixmap( QPixmap::fromImage(_image4) );
}

// ----------------------------------------------------------------------------

void SpidrMpx3Tv::timerEvent( QTimerEvent * )
{
  if( !_daq ) return;

  std::string str = _daq->errorString();
  if( str.empty() )
    {
      //this->statusBar()->clearMessage();

      _lbFramesSkipped->setText( QString::number(_daq->framesLostCount()) );
      _lbFramesRecv->setText( QString::number(_daq->framesCount()) );
      _lbPacketsRecv->setText( QString::number(_daq->packetsReceivedCount()) );
      //_lbDebugCounter->setText( QString::number(_recvr->debugCounter()) );
      _lbPacketsLost->setText( QString::number(_daq->lostCount()) );
      /*
	this->lbShutterCount->
	setText( QString::number(_recvr->lastShutterCount()) );
	this->lbFrameCount->
	setText( QString::number(_recvr->lastTriggerCount()) );
	this->lbSeqNumber->
	setText( QString::number(_recvr->lastSequenceNr()) );
      */
      if( _daq->hasFrame() )
	{
	  this->decodeAndDisplay();
	}
    }
  else
    {
      this->statusBar()->
	showMessage( QString("ERROR: ") + QString::fromStdString( str ) );
      _tbOn->setText( "On" );
      _daq->stop();
      delete _daq;
      _daq = 0;
      delete _controller;
      _controller = 0;
    }
}

// ----------------------------------------------------------------------------

void SpidrMpx3Tv::onOff()
{
  if( !_controller )
    {
      this->statusBar()->showMessage( "Connecting...." );
      QApplication::setOverrideCursor( Qt::WaitCursor );
      QApplication::processEvents();

      QHostAddress qha( _leSpidrIpAddr->text() );
      unsigned int addr = qha.toIPv4Address();
      _controller = new SpidrController( (addr>>24)&0xFF,
					 (addr>>16)&0xFF,
					 (addr>> 8)&0xFF,
					 (addr>> 0)&0xFF );
      QApplication::restoreOverrideCursor();
      if( _controller->isConnected() )
	{
	  this->statusBar()->showMessage( "Connected" );
	}
      else
	{
	  this->statusBar()->
	    showMessage( QString("ERROR: ") +
			 QString::fromStdString(_controller->
						connectionErrString()) );
	  delete _controller;
	  _controller = 0;
	  return;
	}

      int readout_mask = _comboBoxDeviceMask->currentIndex();
      if( readout_mask == 0 ) readout_mask = 0xF;

      _daq = new SpidrDaq( _controller, readout_mask );
      std::string str = _daq->errorString();
      if( !str.empty() )
	{
	  this->statusBar()->
	    showMessage( QString("ERROR: ") + QString::fromStdString( str ) );
	  _daq->stop();
	  delete _daq;
	  _daq = 0;
	  delete _controller;
	  _controller = 0;
	  return;
	}

      _tbOn->setText( "Off" );
      _comboBoxCounterDepth->setEnabled( false );
      _comboBoxDeviceMask->setEnabled( false );
      _checkBoxLutEnable->setEnabled( true );

      // Get the host adapter IP address SPIDR uses, to display
      int ipaddr;
      if( _controller->getIpAddrDest( 0, &ipaddr ) )
	{
	  qha.setAddress( ipaddr );
	  _leHostIpAddr->setText( qha.toString() );
	}
      else
	{
	  _leHostIpAddr->setText( "" );
	}

      // Determine the number of devices, taking part in readout
      int ids[4];
      _controller->getDeviceIds( ids );
      _deviceCount = 0;
      for( int i=0; i<4; ++i )
	if( ids[i] != 0 && (readout_mask & (1<<i)) )
	  ++_deviceCount;
      if( _deviceCount > 1 )
	_lbPorts->setText( "Data IP ports" );
      else
	_lbPorts->setText( "Data IP port" );

      // Get the server ports SPIDR uses for its devices, to display
      int port;
      QString qs;
      for( int i=0; i<4; ++i )
	if( ids[i] != 0 && (readout_mask & (1<<i)) )
	  {
	    if( _controller->getServerPort( i, &port ) )
	      {
		if( !qs.isEmpty() ) qs += QString( ", " );
		qs += QString::number( port );
	      }
	  }
      _lePort->setText( qs );

      // Get the device type of its first device, to display
      for( int i=0; i<4; ++i )
	if( ids[i] != 0 && (readout_mask & (1<<i)) )
	  {
	    int type;
	    if( _controller->getDeviceType( i, &type ) )
	      {
		if( type == MPX_TYPE_MPX31 )
		  {
		    _comboBoxDeviceType->setCurrentIndex( 0 );
		    // Medipix3.1 features a 4-bit option
		    int i = _comboBoxCounterDepth->findText( "6" );
		    if( i > -1 ) _comboBoxCounterDepth->setItemText( i, "4" );
		  }
		else if( type == MPX_TYPE_MPX3RX )
		  {
		    _comboBoxDeviceType->setCurrentIndex( 1 );
		    // Medipix3RX features a 6-bit option
		    int i = _comboBoxCounterDepth->findText( "4" );
		    if( i > -1 ) _comboBoxCounterDepth->setItemText( i, "6" );
		  }
		else
		  {
		    _comboBoxDeviceType->setCurrentIndex( 2 ); // "UNKNOWN DEVICE"
		  }
	      }
	    else
	      {
		_comboBoxDeviceType->setCurrentIndex( 3 );  // "NO DEVICE"
	      }
	    break;
	  }

      int size;
      if( _controller->getMaxPacketSize( &size ) )
	_lePacketSize->setText( QString::number(size) );

      // Set the selected pixel counterdepth
      /* On the devices: to be done by the controlling application...
      for( int i=0; i<4; ++i )
	if( ids[i] != 0 )
	  _controller->setPixelDepth( i, _counterDepth );
      */
      _daq->setPixelDepth( _counterDepth );

      // If this is a Compact-SPIDR module there is no need
      // apply LUTs to decode the pixel data (not yet implemented, 9 Mar 2016)
      _isCompactSpidr = _controller->isCompactSpidr();
      //if( _isCompactSpidr )
      //_daq->disableLut( true );

      // Let SpidrDaq decode the frame data (otherwise it will simply absorb
      // all frames when no output file is opened..)
      _daq->setDecodeFrames( true );
    }
  else
    {
      _tbOn->setText( "On" );
      _leHostIpAddr->setText( "" );
      _lePort->setText( "" );
      _lePacketSize->setText( "" );
      _comboBoxDeviceType->setCurrentIndex( 3 );  // "NO DEVICE"
      _daq->stop();
      delete _daq;
      _daq = 0;
      delete _controller;
      _controller = 0;
      this->statusBar()->clearMessage();
      _deviceCount = 0;
      _comboBoxCounterDepth->setEnabled( true );
      _comboBoxDeviceMask->setEnabled( true );
      _checkBoxLutEnable->setEnabled( false );
    }
}

// ----------------------------------------------------------------------------

void SpidrMpx3Tv::changeCounterDepth()
{
  _counterDepth = _comboBoxCounterDepth->currentText().toInt();

  _sbMinValue->setMaximum( (1<<_counterDepth)-1 );
  _sbMaxValue->setMaximum( (1<<_counterDepth)-1 );

  _sbMaxValue->setValue( (1<<_counterDepth)-1 );

  if( _sbMinValue->value() > _sbMaxValue->value() )
    _sbMinValue->setValue( 0 );
}

// ----------------------------------------------------------------------------

void SpidrMpx3Tv::changeLutEnabled()
{
  if( _checkBoxLutEnable->isChecked() )
    {
      // Look-Up-Table decoding done by (SPIDR) hardware, not (SpidrDaq) API
      if( _controller ) _controller->setLutEnable( true );
      if( _daq ) _daq->setLutEnable( false );
    }
  else
    {
      // Look-Up-Table decoding done by (SpidrDaq) API, not (SPIDR) hardware
      if( _controller ) _controller->setLutEnable( false );
      if( _daq ) _daq->setLutEnable( true );
    }
}

// ----------------------------------------------------------------------------
