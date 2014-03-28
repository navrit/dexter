#include <QMessageBox>
#include <QString>
#include <QTimer>

#include "DummyGen.h"
#include "SpidrController.h"
#include "tpx3defs.h"

QString VERSION( "v1.0.0  21-Mar-2014" );

const int SPIDR_TPX_FE_CONFIG_I      = 0x0210;
const int SPIDR_DUMMYGEN_ENA         = 0x00000002;
const int SPIDR_DUMMYGEN_DELAY_MASK  = 0x000003FC;
const int SPIDR_DUMMYGEN_FRAMES_MASK = 0x0003FC00;
const int SPIDR_DUMMYGEN_HEADER_MASK = 0x003C0000;

const int UPDATE_INTERVAL_MS = 1000;

// ----------------------------------------------------------------------------

DummyGen::DummyGen()
  : QDialog(),
    _spidrController( 0 )
{
  this->setupUi(this);

  this->setWindowFlags( Qt::WindowMinimizeButtonHint |
			Qt::WindowCloseButtonHint );

  connect( _pushButtonConnectOrDisconnect, SIGNAL( clicked() ),
	   this, SLOT( connectOrDisconnect() ) );
  connect( _pushButtonStartOrStop, SIGNAL( clicked() ),
	   this, SLOT( startOrStop() ) );

  _ipAddrValidator = new QIntValidator( 1, 255, this );
  _lineEditAddr3->setValidator( _ipAddrValidator );
  _lineEditAddr2->setValidator( _ipAddrValidator );
  _lineEditAddr1->setValidator( _ipAddrValidator );
  _lineEditAddr0->setValidator( _ipAddrValidator );

  _ipPortValidator = new QIntValidator( 1, 65535, this );
  _lineEditPort->setValidator( _ipPortValidator );

  _labelDisconnected->hide();
  _labelFailed->hide();

  _spinBoxDelay->setSpecialValueText( "---" );
  _spinBoxFrames->setSpecialValueText( "---" );
}

// ----------------------------------------------------------------------------

DummyGen::~DummyGen()
{
  if( _pushButtonStartOrStop->text() == QString("Stop") )
    this->startOrStop();

  if( _spidrController ) delete _spidrController;
}

// ----------------------------------------------------------------------------

void DummyGen::connectOrDisconnect()
{
  if( _spidrController )
    {
      this->killTimer( _timerId );

      delete _spidrController;
      _spidrController = 0;
      _pushButtonConnectOrDisconnect->setText( "Connect" );
      _spinBoxDelay->setSpecialValueText( "---" );
      _spinBoxFrames->setSpecialValueText( "---" );
      _spinBoxDelay->setValue( _spinBoxDelay->minimum() );
      _spinBoxFrames->setValue( _spinBoxFrames->minimum() );
      // Clear the Header combobox
      for( int hdr=0; hdr<0x10; ++hdr )
	_comboBoxHeader->removeItem( 0xF - hdr );
      _lineEditFeBlockControl->setText( "" );

      _pushButtonStartOrStop->setEnabled( false );
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
	  _pushButtonStartOrStop->setEnabled( true );
	  _spinBoxDelay->setSpecialValueText( "" );
	  _spinBoxFrames->setSpecialValueText( "" );
	  // Populate the Header combobox
	  for( int h=0; h<0x10; ++h )
	    _comboBoxHeader->addItem( QString("%1").arg(h,0,16).toUpper() );
	  QApplication::restoreOverrideCursor();

	  QTimerEvent te(1);
	  this->timerEvent( &te );

	  _spinBoxDelay->setValue( (_regVal &
				    SPIDR_DUMMYGEN_DELAY_MASK) >> 2 );
	  _spinBoxFrames->setValue( (_regVal &
				     SPIDR_DUMMYGEN_FRAMES_MASK) >> (2+8) );
	  _comboBoxHeader->setCurrentIndex( (_regVal &
					     SPIDR_DUMMYGEN_HEADER_MASK)
					    >> (2+8+8) );
	  if( _regVal & SPIDR_DUMMYGEN_ENA )
	    {
	      _spinBoxDelay->setEnabled( false );
	      _spinBoxFrames->setEnabled( false );
	      _comboBoxHeader->setEnabled( false );
	      _pushButtonStartOrStop->setText( "Stop" );
	    }
	  else
	    {
	      _spinBoxDelay->setEnabled( true );
	      _spinBoxFrames->setEnabled( true );
	      _comboBoxHeader->setEnabled( true );
	      _pushButtonStartOrStop->setText( "Start" );
	    }

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
    }
}

// ----------------------------------------------------------------------------

void DummyGen::startOrStop()
{
  if( _spidrController == 0 ) return;

  if( _pushButtonStartOrStop->text() == QString("Start") )
    {
      // Start
      _regVal &= ~SPIDR_DUMMYGEN_ENA;
      _regVal &= ~(SPIDR_DUMMYGEN_DELAY_MASK |
		   SPIDR_DUMMYGEN_FRAMES_MASK | SPIDR_DUMMYGEN_HEADER_MASK);
      _regVal |= (_spinBoxDelay->value() << 2);
      _regVal |= (_spinBoxFrames->value() << (2+8));
      _regVal |= (_comboBoxHeader->currentIndex() << (2+8+8));

      if( _spidrController->setSpidrReg( SPIDR_TPX_FE_CONFIG_I, _regVal ) )
	{
	  _regVal |= SPIDR_DUMMYGEN_ENA;
	  if( _spidrController->setSpidrReg( SPIDR_TPX_FE_CONFIG_I, _regVal ) )
	    _labelFailed->hide();
	  else
	    _labelFailed->show();
	}
      else
	{
	  _labelFailed->show();
	}

      _spinBoxDelay->setEnabled( false );
      _spinBoxFrames->setEnabled( false );
      _comboBoxHeader->setEnabled( false );
      _pushButtonStartOrStop->setText( "Stop" );
    }
  else
    {
      // Stop
      _regVal &= ~SPIDR_DUMMYGEN_ENA;
      if( _spidrController->setSpidrReg( SPIDR_TPX_FE_CONFIG_I, _regVal ) )
	_labelFailed->hide();
      else
	_labelFailed->show();

      _spinBoxDelay->setEnabled( true );
      _spinBoxFrames->setEnabled( true );
      _comboBoxHeader->setEnabled( true );
      _pushButtonStartOrStop->setText( "Start" );
    }
}

// ----------------------------------------------------------------------------

void DummyGen::timerEvent(QTimerEvent *)
{
  if( _spidrController == 0 ) return;

  if( !_spidrController->isConnected() )
    {
      // Got disconnected ?
      this->connectOrDisconnect();
      _labelDisconnected->show();
      return;
    }

  int regval;
  unsigned int *u = (unsigned int *) &regval;
  if( _spidrController->getSpidrReg( SPIDR_TPX_FE_CONFIG_I, &regval ) )
    {
      // NB:
      // For negative 'int' the displayed result becomes 64-bit.
      // Qt doc says this:
      // "QString QString::arg( int a, int fieldWidth=0, int base=10,
      //                  const QChar &fillChar = QLatin1Char(' ') ) const
      //  ....
      //  For bases other than 10, 'a' is treated as an unsigned integer."
      QString qs = QString("%1").arg( *u, 8, 16, QChar('0') ).toUpper();
      _lineEditFeBlockControl->setText( qs );
      // Remember current register contents
      _regVal = regval;
    }
  else
    {
      _lineEditFeBlockControl->setText( "--------" );
    }

}

// ----------------------------------------------------------------------------
