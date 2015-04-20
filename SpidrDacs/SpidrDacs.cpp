#include <QCloseEvent>
#include <QFont>
#include <QIntValidator>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QSignalMapper>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTimer>

#include "../SpidrMpx3Lib/mpx3defs.h"
#include "../SpidrMpx3Lib/mpx3dacsdescr.h"
#include "tpx3defs.h"
#include "tpx3dacsdescr.h"
#include "SpidrDacs.h"
#include "SpidrController.h"

QString   VERSION( "v1.1.0  9-Dec-2014" );
//QString VERSION( "v1.0.0  25-Oct-2013" );

const int CHECK_INTERVAL_MS = 1000;

// ----------------------------------------------------------------------------

SpidrDacs::SpidrDacs()
  : QDialog(),
    _spidrController( 0 ),
    _deviceIndex( 0 ),
    _minWidth( 0 ),
    _signalMapper( 0 ),
    _disableSetDac( false )
{
  this->setupUi(this);

  this->setWindowFlags( Qt::WindowMinimizeButtonHint |
			Qt::WindowMaximizeButtonHint |
			Qt::WindowCloseButtonHint );

  _labelVersion->setText( VERSION );
  _groupBoxSpidr->setLayout( _horizontalLayoutSpidr );

  connect( _pushButtonConnectOrDisconnect, SIGNAL( clicked() ),
	   this, SLOT( connectOrDisconnect() ) );
  connect( _pushButtonReadDacs, SIGNAL( clicked() ),
	   this, SLOT( readDacs() ) );
  connect( _pushButtonSetDacsDefaults, SIGNAL( clicked() ),
	   this, SLOT( setDacsDefaults() ) );
  connect( _comboBoxDeviceIndex, SIGNAL( currentIndexChanged(int) ),
	   this, SLOT( setDeviceIndex(int) ) );
  connect( _comboBoxDeviceType, SIGNAL( currentIndexChanged(int) ),
	   this, SLOT( setDeviceType(int) ) );
  connect( _pushButtonStore, SIGNAL( clicked() ),
	   this, SLOT( storeDacs() ) );
  connect( _pushButtonErase, SIGNAL( clicked() ),
	   this, SLOT( eraseDacs() ) );

  _ipAddrValidator = new QIntValidator( 1, 255, this );
  _lineEditAddr3->setValidator( _ipAddrValidator );
  _lineEditAddr2->setValidator( _ipAddrValidator );
  _lineEditAddr1->setValidator( _ipAddrValidator );
  _lineEditAddr0->setValidator( _ipAddrValidator );

  _ipPortValidator = new QIntValidator( 1, 65535, this );
  _lineEditPort->setValidator( _ipPortValidator );

  _labelDisconnected->hide();
  _labelOkay->hide();
  _labelErr->hide();

  //_comboBoxDeviceIndex->hide();
  _comboBoxDeviceType->addItem( "T3" );
  _comboBoxDeviceType->addItem( "M3" );
  _comboBoxDeviceType->addItem( "M3RX" );

  _qpOkay  = this->palette();
  _qpError = _qpOkay;
  _qpError.setColor( QPalette::Base, QColor("yellow") ); // Text entry backgr

  this->setDeviceType( _comboBoxDeviceType->currentIndex() );

  this->readAppSettings();
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
      _pushButtonSetDacsDefaults->setEnabled( false );
      _pushButtonStore->setEnabled( false );
      _pushButtonErase->setEnabled( false );
    }
  else
    {
      _labelDisconnected->hide();
      _labelErr->hide();

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
	  _pushButtonSetDacsDefaults->setEnabled( true );

	  _comboBoxDeviceIndex->clear();
	  int devices;
	  if( _spidrController->getDeviceCount( &devices ) )
	    {
	      QString qs = "Device ";
	      //_comboBoxDeviceIndex->show();
	      int i;
	      for( i=0; i<devices; ++i )
		_comboBoxDeviceIndex->addItem( qs + QString::number(i) );
	    }
	  else
	    {
	      //_comboBoxDeviceIndex->hide();
	    }

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
  // Disable DAC widgets
  this->initDacWidgets( false );
}

// ----------------------------------------------------------------------------

void SpidrDacs::readDacs()
{
  _pushButtonStore->setEnabled( false );
  _pushButtonErase->setEnabled( false );

  if( !_spidrController || !_spidrController->isConnected() ) return;

  // Enable DAC widgets
  this->initDacWidgets( true );

  // Get the current DAC settings and display them
  // without triggering DAC-set commands
  _disableSetDac = true;
  bool noerr = true;
  int dac_val;
  for( int i=0; i<_slidrs.size(); ++i )
    {
      if( _spidrController->getDac( _deviceIndex,
				    TPX3_DAC_TABLE[i].code, &dac_val ) )
	{
	  _slidrs[i]->setValue( dac_val );
	  _spboxs[i]->setPalette( _qpOkay );
	}
      else
	{
	  _slidrs[i]->setValue( 0 );
	  _spboxs[i]->setPalette( _qpError );
	  noerr = false;
	}
    }
  if( noerr )
    {
      // Require all settings to be read out properly before allowing
      // operations on non-volatile memory
      _pushButtonStore->setEnabled( true );
      _pushButtonErase->setEnabled( true );
    }
  _disableSetDac = false;
}

// ----------------------------------------------------------------------------

void SpidrDacs::setDacsDefaults()
{
  if( !_spidrController || !_spidrController->isConnected() ) return;

  if( _spidrController->setDacsDflt( _deviceIndex ) ) this->readDacs();
}

// ----------------------------------------------------------------------------

void SpidrDacs::setDac( int index )
{
  if( !_spidrController || !_spidrController->isConnected() ) return;
  if( _disableSetDac ) return;

  if( _spidrController->setDac( _deviceIndex, TPX3_DAC_TABLE[index].code,
				_spboxs[index]->value() ) )
    _spboxs[index]->setPalette( _qpOkay );
  else
    _spboxs[index]->setPalette( _qpError );
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

void SpidrDacs::initDacWidgets( bool enable )
{
  // Don't allow the settings made here trigger a DAC set operation...
  disconnect( _signalMapper, SIGNAL(mapped(int)), this, SLOT(setDac(int)) );

  int i;
  for( i=0; i<_slidrs.size(); ++i ) _slidrs[i]->setValue( 0 );
  if( !enable || !_spidrController || !_spidrController->isConnected() )
    {
      for( i=0; i<_slidrs.size(); ++i ) _slidrs[i]->setEnabled( false );
      for( i=0; i<_spboxs.size(); ++i ) _spboxs[i]->setEnabled( false );
    }
  else
    {
      for( i=0; i<_slidrs.size(); ++i ) _slidrs[i]->setEnabled( true );
      for( i=0; i<_spboxs.size(); ++i ) _spboxs[i]->setEnabled( true );
    }

  connect( _signalMapper, SIGNAL(mapped(int)), this, SLOT(setDac(int)) );
}

// ----------------------------------------------------------------------------

void SpidrDacs::adjustLayout()
{
  int col, min_width = 0;
  QRect r;

  // Find a column width only the very first time,
  // when the window has not 'grown' yet
  // (and the columns have grown to fit the width..)
  if( _minWidth == 0 )
    {
      // Find maximum column size
      for( col=0; col<_gridLayoutDacs->columnCount(); ++col )
	{
	  r = _gridLayoutDacs->cellRect( 2, col );
	  if( min_width < r.width() ) min_width = r.width();
	}
      _minWidth = min_width;
    }
  else
    {
      min_width = _minWidth;
    }

  // Set equal column sizes
  for( col=0; col<_gridLayoutDacs->columnCount(); ++col )
    _gridLayoutDacs->setColumnMinimumWidth( col, min_width );

  // Set equal label widths
  for( col=0; col<_nameLabels.size(); ++col )
    _nameLabels[col]->setMinimumWidth( min_width );

  this->resize( 0, this->size().height() );
}

// ----------------------------------------------------------------------------

void SpidrDacs::setDeviceIndex( int index )
{
  if( index < 0 ) return;
  _deviceIndex = index;
  // Device index changed: disable widgets until DACs are read out
  this->initDacWidgets( false );
}

// ----------------------------------------------------------------------------

void SpidrDacs::setDeviceType( int index )
{
  if( index < 0 ) return;

  int dev_type = index;
  const struct dac_s *dac_table;
  int dac_count;
  if( dev_type == MPX_TYPE_MPX31 )
    {
      dac_count = MPX3_DAC_COUNT;
      dac_table = &MPX3_DAC_TABLE[0];
    }
  else if( dev_type == MPX_TYPE_MPX3RX )
    {
      dac_count = MPX3RX_DAC_COUNT;
      dac_table = &MPX3RX_DAC_TABLE[0];
    }
  else
    {
      dac_count = TPX3_DAC_COUNT_TO_SET;
      dac_table = &TPX3_DAC_TABLE[0];
    }

  // Clear the DAC widgets
  int i;
  for( i=0; i<_slidrs.size(); ++i )
    {
      _gridLayoutDacs->removeWidget( _slidrs[i] );
      delete _slidrs[i];
    }
  _slidrs.clear();
  for( i=0; i<_spboxs.size(); ++i )
    {
      _gridLayoutDacs->removeWidget( _spboxs[i] );
      delete _spboxs[i];
    }
  _spboxs.clear();
  for( i=0; i<_nameLabels.size(); ++i )
    {
      _gridLayoutDacs->removeWidget( _nameLabels[i] );
      delete _nameLabels[i];
    }
  _nameLabels.clear();
  for( i=0; i<_maxLabels.size(); ++i )
    {
      _gridLayoutDacs->removeWidget( _maxLabels[i] );
      delete _maxLabels[i];
    }
  _maxLabels.clear();

  // Create the layout anew to be able to properly resize (?)
  if( _gridLayoutDacs ) delete _gridLayoutDacs;
  _gridLayoutDacs = new QGridLayout( this );
  _gridLayoutDacs->setSpacing( 1 );
  horizontalLayout->addLayout(_gridLayoutDacs); // Necessary to add
                                                // to global layout
  _groupBoxDacs->setLayout( _gridLayoutDacs );

  if( _signalMapper ) delete _signalMapper;
  _signalMapper = new QSignalMapper( this );

  // Add sliders and spinboxes for the defined DACs
  QLabel   *label;
  QSlider  *slidr;
  QSpinBox *spbox;
  QString   qs;
  QFont     f;
  for( int i=0; i<dac_count; ++i )
    {
      label = new QLabel( this );
      label->setAlignment( Qt::AlignHCenter );
      qs = QString::number( (1<<dac_table[i].bits)-1 );
      label->setText( qs );
      _gridLayoutDacs->addWidget( label, 0, i, Qt::AlignHCenter );
      _maxLabels.append( label );

      label = new QLabel( this );
      label->setAlignment( Qt::AlignHCenter );
      //label->setForegroundRole( QPalette::Highlight );
      label->setFrameStyle( QFrame::Panel | QFrame::Raised );
      //label->setMargin( 2 );
      qs = QString( dac_table[i].name );
      // Replace "_" in the string by "-\n"
      qs = qs.replace( QChar('_'), QString("-\n") );
      // Replace "threshold" in the string by "thresh"
      // (some names are a bit too wide...)
      qs = qs.replace( QString("threshold"), QString("thresh") );
      qs = qs.replace( QString("Threshold"), QString("Thresh") );
      // Replace "buffer" in the string by "buffer\n"
      // (some names are a bit too wide...)
      qs = qs.replace( QString("buffer"), QString("buffer\n") );
      qs = qs.replace( QString("Buffer"), QString("Buff") );
      // Other
      qs = qs.remove( QChar('[') );
      qs = qs.remove( QChar(']') );
      label->setText( qs );
      QFont f = label->font();
      //f.setPointSize( f.pointSize()-1 );
      f.setBold( true );
      label->setFont( f );
      _gridLayoutDacs->addWidget( label, 3, i, Qt::AlignHCenter );
      _nameLabels.append( label );

      slidr = new QSlider( this );
      slidr->setTracking( true );
      slidr->setRange( 0, (1<<dac_table[i].bits)-1 );
      if( slidr->maximum() < 20 ) slidr->setTickInterval( 1 );
      else if( slidr->maximum() < 200 ) slidr->setTickInterval( 10 );
      else if( slidr->maximum() < 300 ) slidr->setTickInterval( 20 );
      else slidr->setTickInterval( 50 );
      slidr->setTickPosition( QSlider::TicksLeft );
      _gridLayoutDacs->addWidget( slidr, 1, i, Qt::AlignHCenter );
      _slidrs.append( slidr );

      spbox = new QSpinBox( this );
      spbox->setRange( 0, (1<<dac_table[i].bits)-1 );
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

  // Device type changed: disable widgets until DACs are read out
  this->initDacWidgets( false );

  connect( _signalMapper, SIGNAL(mapped(int)), this, SLOT(setDac(int)) );

  // Column width equalization can only be done after the widget has been shown
  // (see adjustLayout() below and Qt documentation on QGridLayout::cellRect)
  QTimer::singleShot( 0, this, SLOT( adjustLayout() ) );
}

// ----------------------------------------------------------------------------

void SpidrDacs::storeDacs()
{
  if( !_spidrController || !_spidrController->isConnected() ) return;

  // Store current DAC settings for this Timepix3 device in non-volatile memory
  if( !_spidrController->storeDacs( _deviceIndex ) )
    _labelErr->show();
  else
    {
      _labelErr->hide();
      _labelOkay->show();
      QTimer::singleShot( 1000, this, SLOT( hideOkay() ) );
    }
}

// ----------------------------------------------------------------------------

void SpidrDacs::eraseDacs()
{
  if( !_spidrController || !_spidrController->isConnected() ) return;

  // Erase DAC settings for this Timepix3 device from non-volatile memory
  if( !_spidrController->eraseDacs( _deviceIndex ) )
    _labelErr->show();
  else
    {
      _labelErr->hide();
      _labelOkay->show();
      QTimer::singleShot( 1000, this, SLOT( hideOkay() ) );
    }
}

// ----------------------------------------------------------------------------

void SpidrDacs::hideOkay()
{
  _labelOkay->hide();
}

// ----------------------------------------------------------------------------

void SpidrDacs::closeEvent( QCloseEvent *event )
{
  // When quitting the application save some of the current settings
  this->writeAppSettings();
  event->accept();
}

// ----------------------------------------------------------------------------

void SpidrDacs::readAppSettings()
{
  QSettings settings( "NIKHEF", "SPIDR" );

  int ipaddr = settings.value( "ipAddress", 0xC0A8010A ).toInt();
  _lineEditAddr3->setText( QString::number((ipaddr>>24) & 0xFF) );
  _lineEditAddr2->setText( QString::number((ipaddr>>16) & 0xFF) );
  _lineEditAddr1->setText( QString::number((ipaddr>> 8) & 0xFF) );
  _lineEditAddr0->setText( QString::number((ipaddr>> 0) & 0xFF) );

  int portnr = settings.value( "ipPort", 50000 ).toInt();
  _lineEditPort->setText( QString::number(portnr) );
}

// ----------------------------------------------------------------------------

void SpidrDacs::writeAppSettings()
{
  QSettings settings( "NIKHEF", "SPIDR" );

  int ipaddr = 0;
  ipaddr |= (_lineEditAddr3->text().toInt() & 0xFF) << 24;
  ipaddr |= (_lineEditAddr2->text().toInt() & 0xFF) << 16;
  ipaddr |= (_lineEditAddr1->text().toInt() & 0xFF) <<  8;
  ipaddr |= (_lineEditAddr0->text().toInt() & 0xFF) <<  0;
  settings.setValue( "ipAddress", ipaddr );

  settings.setValue( "ipPort", _lineEditPort->text().toInt() );
}

// ----------------------------------------------------------------------------
