#include "SpidrTpx3Tv.h"
#include "SpidrController.h"
#include "SpidrDaq.h"

#include <QHostAddress>

QString VERSION( "v1.0.0   08-Nov-2013" );

// ----------------------------------------------------------------------------

SpidrTpx3Tv::SpidrTpx3Tv()
  : QMainWindow(),
    _controller( 0 ), _daq( 0 ),
    _image( QSize(256,256), QImage::Format_Indexed8 )
{
  this->setupUi(this);

  _lbVersion->setText( VERSION );

  connect( _actionExit, SIGNAL(triggered()), this, SLOT(close()) );

  connect( _pbOn, SIGNAL(clicked()), this, SLOT(onOff()) );
  connect( _pbResetImage, SIGNAL(clicked()), this, SLOT(resetPixelCounters()) );

  unsigned long *cnt = &_pixelCounter[0][0];
  for( int i=0; i<256*256; ++i, ++cnt ) *cnt = 0;

  startTimer( 50 );
}

// ----------------------------------------------------------------------------

SpidrTpx3Tv::~SpidrTpx3Tv()
{
  if( _controller ) this->onOff();
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::displayImage()
{
  unsigned int max = _sbMaxValue->value();
  unsigned int min = _sbMinValue->value();

  // Convert the pixel counter data to QImage bytes
  unsigned long *ctr = &_pixelCounter[0][0];
  unsigned char *img = _image.bits();
  for( int i=0; i<256*256; ++i, ++img, ++ctr )
    {
      if( *ctr >= max )
	*img = 255;
      else if( *ctr <= min )
	*img = 0;
      else
	*img = (unsigned char) ((255 * ((*ctr)-min)) / (max-min));
    }

  // Display the image
  _lbView->setPixmap( QPixmap::fromImage(_image) );
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::timerEvent( QTimerEvent * )
{
  if( !_daq ) return;

  std::string str = _daq->errorString();
  if( str.empty() )
    {
      //this->statusBar()->clearMessage();

      _lbBytesReceived->setText( QString::number(_daq->bytesReceivedCount()) );
      _lbBytesSampled->setText( QString::number(_daq->bytesSampledCount()) );
      _lbBytesFlushed->setText( QString::number(_daq->bytesFlushedCount()) );
      _lbPacketsReceived->setText( QString::number(_daq->packetsReceivedCount()) );
      _lbLastPacketSize->setText( QString::number(_daq->lastPacketSize()) );

      if( _daq->getSample( 0x100000, 20 ) )
	{
	  // Update the pixel counters with the sampled pixel data
	  int cnt = 0, x, y;
	  int size = _daq->sampleSize();
	  while( _daq->nextPixel( &x, &y ) )
	    {
	      ++_pixelCounter[y][x];
	      ++cnt;
	    }

	  //_lbSampleSize->setText( QString::number(cnt) );
	  _lbSampleSize->setText( QString::number(size) );

	  // Update the displayed image (if there was a minimum of activity)
	  if( cnt > 5 )
	    this->displayImage();
	}
    }
  else
    {
      this->statusBar()->
	showMessage( QString("ERROR: ") + QString::fromStdString( str ) );
      _pbOn->setText( "On" );
      _daq->stop();
      delete _daq;
      _daq = 0;
      delete _controller;
      _controller = 0;
    }
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::onOff()
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

      _pbOn->setText( "Off" );

      // Device port
      int port;
      if( _controller->getServerPort( 0, &port ) )
	_lePort->setText( QString::number(port) );

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

      // Configure the data sampling for this application
      _daq->setSampleAll( false );
      _daq->setSampling( true );
      //_daq->openFile( "test.dat", true );

      this->resetPixelCounters();
    }
  else
    {
      _pbOn->setText( "On" );
      _leHostIpAddr->setText( "" );
      _lePort->setText( "" );
      _daq->stop();
      delete _daq;
      _daq = 0;
      delete _controller;
      _controller = 0;
      this->statusBar()->clearMessage();
    }
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::resetPixelCounters()
{
  unsigned long *cnt = &_pixelCounter[0][0];
  for( int i=0; i<256*256; ++i, ++cnt ) *cnt = 0;

  this->displayImage();
}

// ----------------------------------------------------------------------------
