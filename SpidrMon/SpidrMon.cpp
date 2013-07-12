#include <QIntValidator>
#include <QMessageBox>
#include <QString>
#include <QTimer>

#include "SpidrMon.h"
#include "SpidrController.h"

QString VERSION( "v1.0.1  05-Jun-2013" );

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
  _leUpdateLed->hide();
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
	  QMessageBox::warning( this, "Connecting to SPIDR",
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
	  _timerId = this->startTimer( 1000 );
	}
      else
	{
	  delete _spidrController;
	  _spidrController = 0;
	  QApplication::restoreOverrideCursor();
	  QMessageBox::warning( this, "Connecting to SPIDR",
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
  if( _spidrController->getVdd( &mvolt, &mamp, &mwatt ) )
    {
      _lineEditVddMvolt->setText( QString::number( mvolt ) );
      _lineEditVddMwatt->setText( QString::number( mwatt ) );
      QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
      _lineEditVddMamp->setText( qs );
    }
  else
    {
      _lineEditVddMvolt->setText( "----" );
      _lineEditVddMamp->setText( "----" );
      _lineEditVddMwatt->setText( "----" );
    }

  _leUpdateLed->show();
  QTimer::singleShot( 200, this, SLOT(updateLedOff()) );
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
  _lineEditVddMvolt->setText( "----" );
  _lineEditVddMamp->setText( "----" );
  _lineEditVddMwatt->setText( "----" );
}

// ----------------------------------------------------------------------------

void SpidrMon::updateLedOff()
{
  _leUpdateLed->hide();
}

// ----------------------------------------------------------------------------
