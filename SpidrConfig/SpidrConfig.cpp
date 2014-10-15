#include <QIntValidator>
#include <QMessageBox>
#include <QString>
#include <QTimer>

#include "SpidrConfig.h"
#include "SpidrController.h"

QString VERSION( "v1.0.0  15-Oct-2014" );

//const int UPDATE_INTERVAL_MS = 750;
const int UPDATE_INTERVAL_MS = 1500;

// ----------------------------------------------------------------------------

SpidrConfig::SpidrConfig()
  : QDialog(),
    _spidrController( 0 ),
    _startupOptions( 0 )
{
  this->setupUi(this);

  this->setWindowFlags( Qt::WindowMinimizeButtonHint |
			Qt::WindowCloseButtonHint );

  connect( _pushButtonConnectOrDisconnect, SIGNAL( clicked() ),
	   this, SLOT( connectOrDisconnect() ) );
  connect( _pushButtonRead, SIGNAL( clicked() ),
	   this, SLOT( readStartupOptions() ) );
  connect( _pushButtonErase, SIGNAL( clicked() ),
	   this, SLOT( eraseStartupOptions() ) );
  connect( _pushButtonStore, SIGNAL( clicked() ),
	   this, SLOT( storeStartupOptions() ) );

  _ipAddrValidator = new QIntValidator( 1, 255, this );
  _lineEditAddr3->setValidator( _ipAddrValidator );
  _lineEditAddr2->setValidator( _ipAddrValidator );
  _lineEditAddr1->setValidator( _ipAddrValidator );
  _lineEditAddr0->setValidator( _ipAddrValidator );

  _ipPortValidator = new QIntValidator( 1, 65535, this );
  _lineEditPort->setValidator( _ipPortValidator );

  _labelDisconnected->hide();

  // Not yet implemented...
  _groupBoxTpxMpx->hide();

  QTimer::singleShot( 0, this, SLOT(myResize()) );
}

// ----------------------------------------------------------------------------

SpidrConfig::~SpidrConfig()
{
  if( _spidrController ) delete _spidrController;
}

// ----------------------------------------------------------------------------

void SpidrConfig::connectOrDisconnect()
{
  if( _spidrController )
    {
      delete _spidrController;
      _spidrController = 0;
      _pushButtonConnectOrDisconnect->setText( "Connect" );

      _pushButtonRead->setEnabled( false );
      _pushButtonErase->setEnabled( false );
      _pushButtonStore->setEnabled( false );

      _checkBoxTpxPowerEna->setEnabled( false );
      _checkBoxTpxHiSpeed->setEnabled( false );

      _checkBoxTpxPowerEna->setChecked( false );
      _checkBoxTpxHiSpeed->setChecked( false );

      _groupBoxSpidr->setTitle( "SPIDR Startup Options" );
    }
  else
    {
      _labelDisconnected->hide();

      if( _lineEditAddr3->text().isEmpty() ||
	  _lineEditAddr2->text().isEmpty() ||
	  _lineEditAddr1->text().isEmpty() ||
	  _lineEditAddr0->text().isEmpty() ||
	  _lineEditPort->text().isEmpty() )
	{
	  QMessageBox::warning( this, "Connecting to SPIDR-TPX3",
				"Provide a complete IP address "
				"and a port number" );
	  return;
	}

      QApplication::setOverrideCursor( Qt::WaitCursor );

      _spidrController =
	new SpidrController( _lineEditAddr3->text().toInt(),
			     _lineEditAddr2->text().toInt(),
			     _lineEditAddr1->text().toInt(),
			     _lineEditAddr0->text().toInt(),
			     _lineEditPort->text().toInt() );
      if( _spidrController->isConnected() )
	{
	  _pushButtonConnectOrDisconnect->setText( "Disconnect" );
	  QApplication::restoreOverrideCursor();

	  _pushButtonRead->setEnabled( true );
	  _pushButtonErase->setEnabled( true );
	  _pushButtonStore->setEnabled( true );

	  _checkBoxTpxPowerEna->setEnabled( true );
	  _checkBoxTpxHiSpeed->setEnabled( true );

	  QTimer::singleShot( 0, this, SLOT(readStartupOptions()) );
	}
      else
	{
	  delete _spidrController;
	  _spidrController = 0;
	  QApplication::restoreOverrideCursor();
	  QMessageBox::warning( this, "Connecting to SPIDR-TPX3",
				"Failed to connect!" );
	}
    }
}

// ----------------------------------------------------------------------------

void SpidrConfig::readStartupOptions()
{
  if( !_spidrController || !_spidrController->isConnected() ) return;

  QString qs( "SPIDR Startup Options: " );
  int options = 0;
  if( _spidrController->getStartupOptions( &options ) )
    {
      if( (options & 0xC0000000) == 0x40000000 )
	{
	  // Timepix3 power: bit 0
	  if( options & 0x01 )
	    _checkBoxTpxPowerEna->setChecked( true );
	  else
	    _checkBoxTpxPowerEna->setChecked( false );

	  // Timepix3 readout speed: bit 1
	  if( options & 0x02 )
	    _checkBoxTpxHiSpeed->setChecked( true );
	  else
	    _checkBoxTpxHiSpeed->setChecked( false );

	  // Bias powersupply: bit 7
	  /*
	  if( options & 0x80 )
	    _checkBoxBiasSupplyEna->setChecked( true );
	  else
	    _checkBoxBiasSupplyEna->setChecked( false );
	  */
	  // Bias voltage: valid bit: bit15=0, voltage value in bit 8-14
	  if( (options & 0x8000) == 0 )
	    {
	      int volts = ((options & 0x7F00) >> 8);
	    }
	  qs += "CONFIGURED";
	}
      else
	{
	  _checkBoxTpxPowerEna->setChecked( false );
	  _checkBoxTpxHiSpeed->setChecked( false );

	  qs += "*NOT* CONFIGURED";
	}
    }
  else
    {
      _checkBoxTpxPowerEna->setChecked( false );
      _checkBoxTpxHiSpeed->setChecked( false );

      qs += "### FAILED TO GET";
    }
  _groupBoxSpidr->setTitle( qs );
}

// ----------------------------------------------------------------------------

void SpidrConfig::eraseStartupOptions()
{
  if( !_spidrController || !_spidrController->isConnected() ) return;

  int options = 0xFFFFFFFF;
  _spidrController->storeStartupOptions( options );

  this->readStartupOptions();
}

// ----------------------------------------------------------------------------

void SpidrConfig::storeStartupOptions()
{
  if( !_spidrController || !_spidrController->isConnected() ) return;

  int options = 0;
  if( _spidrController->getStartupOptions( &options ) )
    {
      // Set 'is valid' bits
      options &= 0x7FFFFFFF;
      options |= 0x40000000;

      // Timepix3 power: bit 0
      if( _checkBoxTpxPowerEna->isChecked() )
	options |= 0x01;
      else
	options &= ~0x01;

      // Timepix3 readout speed: bit 1
      if( _checkBoxTpxHiSpeed->isChecked() )
	options |= 0x02;
      else
	options &= ~0x02;

      // Bias powersupply: bit 7
      /*
      if( _checkBoxBiasSupplyEna->isChecked() )
	options |= 0x80;
      else
	options &= ~0x80;
      */
      _spidrController->storeStartupOptions( options );
    }

  this->readStartupOptions();
}

// ----------------------------------------------------------------------------

void SpidrConfig::myResize()
{
  this->resize( this->minimumSize() );
}

// ----------------------------------------------------------------------------
