#include "SpidrTpx3Tv.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "tpx3defs.h"

#include <QHostAddress>

QString   VERSION( "v2.0.0   13-Dec-2015" );
//QString VERSION( "v1.0.0   08-Nov-2013" );

// ----------------------------------------------------------------------------

SpidrTpx3Tv::SpidrTpx3Tv()
  : QMainWindow(),
    _controller( 0 ), _daq( 0 ),
    _deviceNr( 0 ), _resetPixelConfig( true ), _cntr( 0 ),
    _image( QSize(256,256), QImage::Format_Indexed8 )
{
  this->setupUi(this);

  _lbVersion->setText( VERSION );

  connect( _actionExit, SIGNAL(triggered()),
	   this, SLOT(close()) );

  connect( _pbConnect, SIGNAL(clicked()),
	   this, SLOT(onOff()) );
  connect( _pbResetImage, SIGNAL(clicked()),
	   this, SLOT(resetPixelCounters()) );

  // Test mode
  connect( _cbTestMode, SIGNAL(stateChanged(int)),
	   this, SLOT(changeTestMode(int) ) );
  this->changeTestMode( Qt::Unchecked );
  connect( _pbShutter, SIGNAL(toggled(bool)),
	   this, SLOT(changeShutter(bool)) );
  connect( _sbDacCoarse, SIGNAL(valueChanged(int)),
	   this, SLOT(setDacCoarse(int)) );
  connect( _sliderDacFine, SIGNAL(valueChanged(int)),
	   this, SLOT(setDacFine(int)) );

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

	  //_lbSampleSize->setText( QString::number(cnt) ); // Nr of pixels
	  _lbSampleSize->setText( QString::number(size) ); // Nr of bytes
	  //_cntr += size;
	  //_lbSampleSize->setText( QString::number(_cntr) );

	  // Update the displayed image (if there was a minimum of activity)
	  if( cnt > 5 )
	    this->displayImage();
	}
    }
  else
    {
      this->statusBar()->
	showMessage( QString("ERROR: ") + QString::fromStdString( str ) );
      _pbConnect->setText( "Connect" );
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

      // Number of devices
      disconnect( _cbDeviceNr, 0, 0, 0 );
      _cbDeviceNr->clear();
      int devices;
      if( _controller->getDeviceCount( &devices ) )
	{
	  int i;
	  for( i=0; i<devices; ++i )
	    _cbDeviceNr->addItem( QString::number(i) );
	  _cbDeviceNr->setEnabled( true );
	  connect( _cbDeviceNr, SIGNAL(currentIndexChanged(int)),
		   this, SLOT(selectDeviceNr(int)) );
	}
      else
	{
	  this->displayError( "Get device-count failed" );
	}

      this->selectDeviceNr( 0 );

      _pbConnect->setText( "Disconnect" );

      _cbTestMode->setEnabled( true );
      _pbShutter->setEnabled( true );
    }
  else
    {
      _pbConnect->setText( "Connect" );
      _cbDeviceNr->setEnabled( false );
      _leHostIpAddr->setText( "" );
      _lePort->setText( "" );
      _daq->stop();
      delete _daq;
      _daq = 0;
      delete _controller;
      _controller = 0;
      this->statusBar()->clearMessage();
      _cbTestMode->setEnabled( false );
      _pbShutter->setEnabled( false );
    }
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::selectDeviceNr( int index )
{
  _deviceNr = index;

  // Device port
  int port;
  if( _controller->getServerPort( _deviceNr, &port ) )
    _lePort->setText( QString::number(port) );

  // Get the host adapter IP address SPIDR uses, to display
  int ipaddr;
  if( _controller->getIpAddrDest( _deviceNr, &ipaddr ) )
    {
      QHostAddress qha( ipaddr );
      //qha.setAddress( ipaddr );
      _leHostIpAddr->setText( qha.toString() );
    }
  else
    {
      _leHostIpAddr->setText( "" );
    }

  // Instantiate a new SpidrDaq for the selected device
  if( _daq )
    {
      _daq->stop();
      delete _daq;
      _daq = 0;
    }
  _daq = new SpidrDaq( _controller, 0x20000000, _deviceNr );
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

  // Configure the data sampling for this application
  _daq->setSampleAll( false );
  _daq->setSampling( true );
  //_daq->openFile( "test.dat", true );

  this->resetPixelCounters();
  _cntr = 0;

  // Test mode stuff
  if( _cbTestMode->isChecked() )
    // Update DAC values shown
    this->changeTestMode( Qt::Checked );
  // First time opening shutter: then reset device's pixel configuration
  _resetPixelConfig = true;
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::resetPixelCounters()
{
  unsigned long *cnt = &_pixelCounter[0][0];
  for( int i=0; i<256*256; ++i, ++cnt ) *cnt = 0;

  this->displayImage();
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::changeTestMode( int state )
{
  this->statusBar()->clearMessage();

  if( state == Qt::Checked )
    {
      _labelShutter->show();
      _pbShutter->show();
      _labelDacCoarse->show();
      _sbDacCoarse->show();
      _labelDacFine->show();
      _sliderDacFine->show();

      int dac_val;
      if( _controller->getDac( _deviceNr, TPX3_VTHRESH_COARSE, &dac_val ) )
	{
	  _sbDacCoarse->setValue( dac_val );
	}
      else
	{
	  this->displayError( "Get DAC Vthr-Coarse failed" );
	}

      if( _controller->getDac( _deviceNr, TPX3_VTHRESH_FINE, &dac_val ) )
	{
	  _sliderDacFine->setValue( dac_val );
	  _labelDacFine->setText( QString("DAC Vthr-fine %1").arg(dac_val) );
	}
      else
	{
	  this->displayError( "Get DAC Vthr-Fine failed" );
	}
    }
  else
    {
      _labelShutter->hide();
      _pbShutter->hide();
      _labelDacCoarse->hide();
      _sbDacCoarse->hide();
      _labelDacFine->hide();
      _sliderDacFine->hide();

      if( _pbShutter->isChecked() )
	_pbShutter->setChecked( false );
    }
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::changeShutter( bool open )
{
  if( !_controller ) return;

  if( open )
    {
      if( !_controller->resetPixels( _deviceNr ) )
	this->displayError( "Reset-pixels failed" );

      if( _resetPixelConfig )
	{
	  QLabel *label =
	    new QLabel( "Uploading pixel configuration...", this );
	  QFont font = label->font();
	  font.setPointSize( font.pointSize()+2 );
	  label->setFont( font );
	  label->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	  label->setAlignment( Qt::AlignCenter );
	  label->setAutoFillBackground( true );
	  label->setPalette( QColor("yellow") );
	  // Place the label centered on the parent widget
	  label->move( (this->size().width() -
			label->size().width())/2,
		       (this->size().height() -
			label->size().height())/2 );
	  label->resize( 300, 100 );
	  label->show();
	  QApplication::processEvents();

	  _controller->resetPixelConfig();
	  if( !_controller->setPixelConfig( _deviceNr ) )
	    this->displayError( "Pixelconfig upload failed" );
	  _resetPixelConfig = false;

	  delete label;
	}

      if( !_controller->setGenConfig( _deviceNr,
				      TPX3_POLARITY_EMIN |
				      TPX3_ACQMODE_TOA_TOT |
				      TPX3_GRAYCOUNT_ENA |
				      TPX3_FASTLO_ENA ) )
	this->displayError( "Set config-reg failed" );

      if( !_controller->datadrivenReadout() )
	this->displayError( "Enable datadriven-readout failed" );

      if( !_controller->openShutter() )
	this->displayError( "Open shutter failed" );

      _pbShutter->setText( "Close" );
    }
  else
    {
      _controller->closeShutter();
      _pbShutter->setText( "Open" );
    }
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::setDacCoarse( int dac_val )
{
  if( !_controller->setDac( _deviceNr, TPX3_VTHRESH_COARSE, dac_val ) )
    this->displayError( "Set Vthr-Coarse failed" );
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::setDacFine( int dac_val )
{
  if( !_controller->setDac( _deviceNr, TPX3_VTHRESH_FINE, dac_val ) )
    this->displayError( "Set Vthr-Fine failed" );
}

// ----------------------------------------------------------------------------

void SpidrTpx3Tv::displayError( const char *str )
{
  QString qs( str );
  qs += ": ";
  qs += QString::fromStdString( _controller->errorString() );
  this->statusBar()->showMessage( qs );
}

// ----------------------------------------------------------------------------
