#include "mpx3conf.h"
#include "SpidrMpx3Tv.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include <QHostAddress>

QString VERSION( "v2.0.0   16-Jul-2013" );

// ----------------------------------------------------------------------------

SpidrMpx3Tv::SpidrMpx3Tv()
  : QMainWindow(),
    _controller( 0 ), _daq( 0 ),
    _image( QSize(256,256), QImage::Format_Indexed8 )
{
  this->setupUi(this);

  _lbVersion->setText( VERSION );

  connect( _actionExit, SIGNAL(activated()), this, SLOT(close()) );

  connect( _tbOn, SIGNAL(clicked()), this, SLOT(onOff()) );

  connect( _cbCounterDepth, SIGNAL(currentIndexChanged(int)),
	   this, SLOT(changeCounterDepth()) );

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
  int size;
  int *pixeldata = _daq->frameData( 0, &size );

  // Convert the pixel data to QImage bytes
  int    max = _sbMaxValue->value();
  int    min = _sbMinValue->value();
  uchar *img = _image.bits();
  int    val;
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
  _daq->releaseFrame();

  // Display the image
  _lbView->setPixmap( QPixmap::fromImage(_image) );
}

// ----------------------------------------------------------------------------

void SpidrMpx3Tv::timerEvent( QTimerEvent * )
{
  if( !_daq ) return;

  std::string str = _daq->errString();
  if( str.empty() )
    {
      //this->statusBar()->clearMessage();

      _lbFramesSkipped->setText( QString::number(_daq->framesLostCount()) );
      _lbFramesRecv->setText( QString::number(_daq->framesCount()) );
      _lbPacketsRecv->setText( QString::number(_daq->packetsReceivedCount()) );
      //_lbDebugCounter->setText( QString::number(_recvr->debugCounter()) );
      _lbPacketsLost->setText( QString::number(_daq->packetsLostCount()) );
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

      _daq = new SpidrDaq( _controller );
      std::string str = _daq->errString();
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

      // Get the host adapter IP address SPIDR uses, to display
      int ipaddr;
      if( _controller->getIpAddrDest( &ipaddr ) )
	{
	  qha.setAddress( ipaddr );
	  _leHostIpAddr->setText( qha.toString() );
	}
      else
	{
	  _leHostIpAddr->setText( "" );
	}

      // Find the first occupied device position
      int ids[4];
      _controller->getDeviceIds( ids );
      int devnr = 0;
      for( int i=0; i<4; ++i )
	if( ids[i] != 0 )
	  {
	    devnr = i;
	    break;
	  }

      // Get the server port SPIDR uses for its first device, to display
      int port;
      if( _controller->getServerPort( devnr, &port ) )
	_lePort->setText( QString::number(port) );
      else
	_lePort->setText( "" );

      // Get the device type of its first device, to display
      int type;
      if( _controller->getDeviceType( devnr, &type ) )
	{
	  if( type == MPX_TYPE_MPX31 )
	    {
	      _cbDeviceType->setCurrentIndex( 0 );
	      // Medipix3.1 features a 4-bit option
	      int i = _cbCounterDepth->findText( "6" );
	      if( i > -1 ) _cbCounterDepth->setItemText( i, "4" );
	    }
	  else if( type == MPX_TYPE_MPX3RX )
	    {
	      _cbDeviceType->setCurrentIndex( 1 );
	      // Medipix3RX features a 6-bit option
	      int i = _cbCounterDepth->findText( "4" );
	      if( i > -1 ) _cbCounterDepth->setItemText( i, "6" );
	    }
	  else
	    {
	      _cbDeviceType->setCurrentIndex( 2 ); // "UNKNOWN DEVICE"
	    }
	}
      else
	{
	  _cbDeviceType->setCurrentIndex( 3 );  // "NO DEVICE"
	}

      // Set the selected pixel counterdepth
      _controller->setPixelDepth( _counterDepth );
      _daq->setPixelDepth( _counterDepth );

      // Let SpidrDaq decode the frame data (otherwise it will simply absorb
      // all frames when no output file is opened..)
      _daq->setDecodeFrames( true );
    }
  else
    {
      _tbOn->setText( "On" );
      _leHostIpAddr->setText( "" );
      _lePort->setText( "" );
      _cbDeviceType->setCurrentIndex( 3 );  // "NO DEVICE"
      _daq->stop();
      delete _daq;
      _daq = 0;
      delete _controller;
      _controller = 0;
      this->statusBar()->clearMessage();
    }
}

// ----------------------------------------------------------------------------

void SpidrMpx3Tv::changeCounterDepth()
{
  _counterDepth = _cbCounterDepth->currentText().toInt();

  _sbMinValue->setMaximum( (1<<_counterDepth)-1 );
  _sbMaxValue->setMaximum( (1<<_counterDepth)-1 );

  _sbMaxValue->setValue( (1<<_counterDepth)-1 );

  if( _sbMinValue->value() > _sbMaxValue->value() )
    _sbMinValue->setValue( 0 );

  if( _controller ) _controller->setPixelDepth( _counterDepth );
  if( _daq )        _daq->setPixelDepth( _counterDepth );
}

// ----------------------------------------------------------------------------
