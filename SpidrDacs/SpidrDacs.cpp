#include <QFont>
#include <QIntValidator>
#include <QLabel>
#include <QMessageBox>
#include <QSignalMapper>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTimer>

#include "tpx3defs.h"
#include "dacsdescr.h"
#include "SpidrDacs.h"
#include "SpidrController.h"

QString VERSION( "v1.0.0  25-Oct-2013" );

const int CHECK_INTERVAL_MS = 1000;

// ----------------------------------------------------------------------------

SpidrDacs::SpidrDacs()
  : QDialog(),
    _spidrController( 0 )
{
  this->setupUi(this);

  this->setWindowFlags( Qt::WindowMinimizeButtonHint |
			Qt::WindowCloseButtonHint );

  connect( _pushButtonConnectOrDisconnect, SIGNAL( clicked() ),
	   this, SLOT( connectOrDisconnect() ) );
  connect( _pushButtonReadDacs, SIGNAL( clicked() ),
	   this, SLOT( readDacs() ) );

  _ipAddrValidator = new QIntValidator( 1, 255, this );
  _lineEditAddr3->setValidator( _ipAddrValidator );
  _lineEditAddr2->setValidator( _ipAddrValidator );
  _lineEditAddr1->setValidator( _ipAddrValidator );
  _lineEditAddr0->setValidator( _ipAddrValidator );

  _ipPortValidator = new QIntValidator( 1, 65535, this );
  _lineEditPort->setValidator( _ipPortValidator );

  _labelDisconnected->hide();

  _signalMapper = new QSignalMapper( this );

  // Add sliders and spinboxes for the defined DACs
  QLabel   *label;
  QSlider  *slidr;
  QSpinBox *spbox;
  QString   qs;
  QFont     f;
  for( int i=0; i<TPX3_DAC_COUNT_TO_SET; ++i )
    {
      label = new QLabel( this );
      label->setAlignment( Qt::AlignHCenter );
      qs = QString::number( (1<<TPX3_DAC_TABLE[i].bits)-1 );
      label->setText( qs );
      _gridLayoutDacs->addWidget( label, 0, i, Qt::AlignHCenter );

      label = new QLabel( this );
      label->setAlignment( Qt::AlignHCenter );
      //label->setForegroundRole( QPalette::Highlight );
      label->setFrameStyle( QFrame::Panel | QFrame::Raised );
      //label->setMargin( 2 );
      qs = QString( TPX3_DAC_TABLE[i].name );
      // Replace "_" in the string by "-\n"
      qs = qs.replace( QChar('_'), QString("-\n") );
      // Replace "threshold" in the string by "thresh"
      // (some names are a bit too wide...)
      qs = qs.replace( QString("threshold"), QString("thresh") );
      // Replace "buffer" in the string by "buffer\n"
      // (some names are a bit too wide...)
      qs = qs.replace( QString("buffer"), QString("buffer\n") );
      label->setText( qs );
      QFont f = label->font();
      //f.setPointSize( f.pointSize()-1 );
      f.setBold( true );
      label->setFont( f );
      _gridLayoutDacs->addWidget( label, 3, i, Qt::AlignHCenter );
      _labels.append( label );

      slidr = new QSlider( this );
      slidr->setTracking( true );
      slidr->setRange( 0, (1<<TPX3_DAC_TABLE[i].bits)-1 );
      if( slidr->maximum() < 200 ) slidr->setTickInterval( 10 );
      else if( slidr->maximum() < 300 ) slidr->setTickInterval( 20 );
      else slidr->setTickInterval( 50 );
      slidr->setTickPosition( QSlider::TicksLeft );
      _gridLayoutDacs->addWidget( slidr, 1, i, Qt::AlignHCenter );
      _slidrs.append( slidr );

      spbox = new QSpinBox( this );
      spbox->setRange( 0, (1<<TPX3_DAC_TABLE[i].bits)-1 );
      _gridLayoutDacs->addWidget( spbox, 2, i, Qt::AlignHCenter );
      _spboxs.append( spbox );

      connect( slidr, SIGNAL(valueChanged(int)), spbox, SLOT(setValue(int)) );
      connect( spbox, SIGNAL(valueChanged(int)), slidr, SLOT(setValue(int)) );

      // Add a slider or spinbox signal to the signal mapper, which will
      // emit a signal with the slider (i.e. DAC table) index
      //connect( slidr, SIGNAL(sliderReleased()), _signalMapper, SLOT(map()) );
      connect( spbox, SIGNAL(valueChanged(int)), _signalMapper, SLOT(map()) );
      _signalMapper->setMapping( spbox, i );
    }

  connect( _signalMapper, SIGNAL(mapped(int)), this, SLOT(dacChanged(int)) );

  for( int i=0; i<_slidrs.size(); ++i ) _slidrs[i]->setEnabled( false );
  for( int i=0; i<_spboxs.size(); ++i ) _spboxs[i]->setEnabled( false );

  // Column width equalization can only be done after the widget has been shown
  // (see adjustLayout() below and Qt documentation on QGridLayout::cellRect)
  QTimer::singleShot( 200, this, SLOT( adjustLayout() ) );
}

// ----------------------------------------------------------------------------

SpidrDacs::~SpidrDacs()
{
  if( _spidrController ) delete _spidrController;
}

// ----------------------------------------------------------------------------

void SpidrDacs::connectOrDisconnect()
{
  if( _spidrController )
    {
      this->killTimer( _timerId );
      delete _spidrController;
      _spidrController = 0;
      _pushButtonConnectOrDisconnect->setText( "Connect" );
      _pushButtonReadDacs->setEnabled( false );
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

	  _pushButtonReadDacs->setEnabled( true );

	  _timerId = this->startTimer( CHECK_INTERVAL_MS );
	}
      else
	{
	  delete _spidrController;
	  _spidrController = 0;
	  QApplication::restoreOverrideCursor();
	  QMessageBox::warning( this, "Connecting to SPIDR",
				"Failed to connect!" );
	}
    }
  this->initDacs();
}

// ----------------------------------------------------------------------------

void SpidrDacs::readDacs()
{
  if( !_spidrController || !_spidrController->isConnected() ) return;

  // Get the current DAC settings and display them
  int dac_val;
  for( int i=0; i<_slidrs.size(); ++i )
    {
      if( _spidrController->getDac( 0, TPX3_DAC_TABLE[i].code, &dac_val ) )
	_slidrs[i]->setValue( dac_val );
      else
	_slidrs[i]->setValue( 0 );
    }
}

// ----------------------------------------------------------------------------

void SpidrDacs::dacChanged( int index )
{
  if( !_spidrController || !_spidrController->isConnected() ) return;

  _spidrController->setDac( 0, TPX3_DAC_TABLE[index].code,
			    _spboxs[index]->value() );
}

// ----------------------------------------------------------------------------

void SpidrDacs::timerEvent(QTimerEvent *)
{
  if( _spidrController == 0 ) return;

  if( !_spidrController->isConnected() )
    {
      // Got disconnected ?
      this->connectOrDisconnect();
      _labelDisconnected->show();
      return;
    }
}

// ----------------------------------------------------------------------------

void SpidrDacs::initDacs()
{
  int i;
  if( !_spidrController || !_spidrController->isConnected() )
    {
      for( i=0; i<_slidrs.size(); ++i ) _slidrs[i]->setValue( 0 );
      for( i=0; i<_slidrs.size(); ++i ) _slidrs[i]->setEnabled( false );
      for( i=0; i<_spboxs.size(); ++i ) _spboxs[i]->setEnabled( false );
    }
  else
    {
      for( i=0; i<_slidrs.size(); ++i ) _slidrs[i]->setEnabled( true );
      for( i=0; i<_spboxs.size(); ++i ) _spboxs[i]->setEnabled( true );

      // Get the current DAC settings and display them
      this->readDacs();
    }
}

// ----------------------------------------------------------------------------

void SpidrDacs::adjustLayout()
{
  int col, min_width = 0;
  QRect r;

  // Find maximum column size
  for( col=0; col<_gridLayoutDacs->columnCount(); ++col )
    {
      r = _gridLayoutDacs->cellRect( 2, col );
      if( min_width < r.width() ) min_width = r.width();
    }

  // Set equal column sizes
  for( col=0; col<_gridLayoutDacs->columnCount(); ++col )
    _gridLayoutDacs->setColumnMinimumWidth( col, min_width );

  // Set equal label widths
  for( col=0; col<_labels.size(); ++col )
    _labels[col]->setMinimumWidth( min_width );
}

// ----------------------------------------------------------------------------
