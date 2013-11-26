#include <QIntValidator>
#include <QMessageBox>
#include <QString>
#include <QTimer>

#include "SpidrMon.h"
#include "SpidrController.h"

QString VERSION( "v2.0.0  26-Nov-2013" );

const int UPDATE_INTERVAL_MS = 750;

// ----------------------------------------------------------------------------

SpidrMon::SpidrMon()
  : QDialog(),
    _spidrController( 0 )
{
  this->setupUi(this);

  this->setWindowFlags( Qt::WindowMinimizeButtonHint |
			Qt::WindowCloseButtonHint );

  connect( _pushButtonConnectOrDisconnect, SIGNAL( clicked() ),
	   this, SLOT( connectOrDisconnect() ) );

  _ipAddrValidator = new QIntValidator( 1, 255, this );
  _lineEditAddr3->setValidator( _ipAddrValidator );
  _lineEditAddr2->setValidator( _ipAddrValidator );
  _lineEditAddr1->setValidator( _ipAddrValidator );
  _lineEditAddr0->setValidator( _ipAddrValidator );

  _ipPortValidator = new QIntValidator( 1, 65535, this );
  _lineEditPort->setValidator( _ipPortValidator );

  // Data update 'LED'
  _leUpdateSpidrLed->hide();
  _leUpdateTpxLed->hide();
  _labelDisconnected->hide();
}

// ----------------------------------------------------------------------------

SpidrMon::~SpidrMon()
{
  if( _spidrController ) delete _spidrController;
}

// ----------------------------------------------------------------------------

void SpidrMon::connectOrDisconnect()
{
  if( _spidrController )
    {
      this->killTimer( _timerId );
      delete _spidrController;
      _spidrController = 0;
      _pushButtonConnectOrDisconnect->setText( "Connect" );

      _lineEditRemoteTemp->setEnabled( false );
      _lineEditLocalTemp->setEnabled( false );
      _lineEditAvddMvolt->setEnabled( false );
      _lineEditAvddMamp->setEnabled( false );
      _lineEditAvddMwatt->setEnabled( false );
      _lineEditDvddMvolt->setEnabled( false );
      _lineEditDvddMamp->setEnabled( false );
      _lineEditDvddMwatt->setEnabled( false );
      _lineEditVdda->setEnabled( false );
      _lineEditBias->setEnabled( false );
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

	  _lineEditRemoteTemp->setEnabled( true );
	  _lineEditLocalTemp->setEnabled( true );
	  _lineEditAvddMvolt->setEnabled( true );
	  _lineEditAvddMamp->setEnabled( true );
	  _lineEditAvddMwatt->setEnabled( true );
	  _lineEditDvddMvolt->setEnabled( true );
	  _lineEditDvddMamp->setEnabled( true );
	  _lineEditDvddMwatt->setEnabled( true );
	  _lineEditVdda->setEnabled( true );
	  _lineEditBias->setEnabled( true );

	  _timerId = this->startTimer( UPDATE_INTERVAL_MS );
	}
      else
	{
	  delete _spidrController;
	  _spidrController = 0;
	  QApplication::restoreOverrideCursor();
	  QMessageBox::warning( this, "Connecting to SPIDR-TPX3",
				"Failed to connect!" );
	}
      this->initDataDisplay();
    }
}

// ----------------------------------------------------------------------------

void SpidrMon::timerEvent(QTimerEvent *)
{
  if( _spidrController == 0 ) return;

  if( !_spidrController->isConnected() )
    {
      // Got disconnected ?
      this->connectOrDisconnect();
      _labelDisconnected->show();
      return;
    }

  int mdegrees;
  if( _spidrController->getRemoteTemp( &mdegrees ) )
    {
      QString qs = QString("%1.%2").arg( mdegrees/1000 ).
	arg( mdegrees%1000, 3, 10, QChar('0') );
      _lineEditRemoteTemp->setText( qs );
    }
  else
    {
      _lineEditRemoteTemp->setText( "--.---" );
    }
  if( _spidrController->getLocalTemp( &mdegrees ) )
    {
      QString qs = QString("%1.%2").arg( mdegrees/1000 ).
	arg( mdegrees%1000, 3, 10, QChar('0') );
      _lineEditLocalTemp->setText( qs );
    }
  else
    {
      _lineEditLocalTemp->setText( "--.---" );
    }

  int mvolt, mamp, mwatt;
  if( _spidrController->getAvdd( &mvolt, &mamp, &mwatt ) )
    {
      _lineEditAvddMvolt->setText( QString::number( mvolt ) );
      _lineEditAvddMwatt->setText( QString::number( mwatt ) );
      QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
      _lineEditAvddMamp->setText( qs );
    }
  else
    {
      _lineEditAvddMvolt->setText( "----" );
      _lineEditAvddMamp->setText( "----" );
      _lineEditAvddMwatt->setText( "----" );
    }
  if( _spidrController->getDvdd( &mvolt, &mamp, &mwatt ) )
    {
      _lineEditDvddMvolt->setText( QString::number( mvolt ) );
      _lineEditDvddMwatt->setText( QString::number( mwatt ) );
      QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
      _lineEditDvddMamp->setText( qs );
    }
  else
    {
      _lineEditDvddMvolt->setText( "----" );
      _lineEditDvddMamp->setText( "----" );
      _lineEditDvddMwatt->setText( "----" );
    }

  if( _spidrController->getBiasVoltage( &mvolt ) )
    _lineEditBias->setText( QString::number( mvolt ) );
  else
    _lineEditBias->setText( "----" );

  if( _spidrController->getVdda( &mvolt ) )
    _lineEditVdda->setText( QString::number( mvolt ) );
  else
    _lineEditVdda->setText( "----" );

  _leUpdateSpidrLed->show();
  QTimer::singleShot( UPDATE_INTERVAL_MS/4, this, SLOT(updateLedOff()) );
}

// ----------------------------------------------------------------------------

void SpidrMon::initDataDisplay()
{
  _lineEditRemoteTemp->setText( "--.---" );
  _lineEditLocalTemp->setText( "--.---" );
  _lineEditAvddMvolt->setText( "----" );
  _lineEditAvddMamp->setText( "----" );
  _lineEditAvddMwatt->setText( "----" );
  _lineEditDvddMvolt->setText( "----" );
  _lineEditDvddMamp->setText( "----" );
  _lineEditDvddMwatt->setText( "----" );
  _lineEditVdda->setText( "----" );
  _lineEditBias->setText( "----" );
}

// ----------------------------------------------------------------------------

void SpidrMon::updateLedOff()
{
  _leUpdateSpidrLed->hide();
  _leUpdateTpxLed->hide();
}

// ----------------------------------------------------------------------------
