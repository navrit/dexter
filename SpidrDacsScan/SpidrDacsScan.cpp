#include <QFont>
#include <QIntValidator>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>

#include "SpidrDacsScan.h"
#include "SpidrController.h"
#include "../SpidrTpx3Lib/tpx3defs.h"
#include "../SpidrMpx3Lib/mpx3defs.h"

#include "qcustomplot.h"

QString VERSION( "v1.1.0  26-Nov-2014" );

// ----------------------------------------------------------------------------
// (From LEON software: tpx3.c (Timepix3) and dacs.c (Medipix3))

// Structure containing info about a DAC
typedef struct dac_s
{
  int         code;
  const char *name;
  int         bits;
  int         dflt;
} dac_t;

// Tables with info about the DACs
static const dac_t TPX3_DAC_TABLE[TPX3_DAC_COUNT] = {
  { TPX3_IBIAS_PREAMP_ON,  "Ibias_Preamp_ON",   8, 128 },
  { TPX3_IBIAS_PREAMP_OFF, "Ibias_Preamp_OFF",  4, 8   },
  { TPX3_VPREAMP_NCAS,     "VPreamp_NCAS",      8, 128 },
  { TPX3_IBIAS_IKRUM,      "Ibias_Ikrum",       8, 128 },
  { TPX3_VFBK,             "Vfbk",              8, 128 },
  { TPX3_VTHRESH_FINE,     "Vthreshold_fine",   9, 256 },
  { TPX3_VTHRESH_COARSE,   "Vthreshold_coarse", 4, 8   },
  { TPX3_IBIAS_DISCS1_ON,  "Ibias_DiscS1_ON",   8, 128 },
  { TPX3_IBIAS_DISCS1_OFF, "Ibias_DiscS1_OFF",  4, 8   },
  { TPX3_IBIAS_DISCS2_ON,  "Ibias_DiscS2_ON",   8, 128 },
  { TPX3_IBIAS_DISCS2_OFF, "Ibias_DiscS2_OFF",  4, 8   },
  { TPX3_IBIAS_PIXELDAC,   "Ibias_PixelDAC",    8, 128 },
  { TPX3_IBIAS_TPBUFIN,    "Ibias_TPbufferIn",  8, 128 },
  { TPX3_IBIAS_TPBUFOUT,   "Ibias_TPbufferOut", 8, 128 },
  { TPX3_VTP_COARSE,       "Vtp_coarse",        8, 128 },
  { TPX3_VTP_FINE,         "Vtp_fine",          9, 256 },
  { TPX3_IBIAS_CP_PLL,     "Ibias_CP_PLL",      8, 128 },
  { TPX3_PLL_VCNTRL,       "Pll_Vcntrl",        8, 128 },

  { TPX3_BANDGAP_OUTPUT,   "BandGap_output",    0, 0   },
  { TPX3_BANDGAP_TEMP,     "BandGap_Temp",      0, 0   },
  { TPX3_IBIAS_DAC,        "Ibias_dac",         0, 0   },
  { TPX3_IBIAS_DAC_CAS,    "Ibias_dac_cas",     0, 0   },
  { TPX3_SENSEOFF,         "SenseOFF",          0, 0   }
};

static const dac_t MPX3_DAC_TABLE[MPX3_DAC_COUNT] =
{
  { MPX3_DAC_THRESH_0,   "Threshold[0]",        9, 150 },
  { MPX3_DAC_THRESH_1,   "Threshold[1]",        9, 150 },
  { MPX3_DAC_THRESH_2,   "Threshold[2]",        9, (1<<9)/2 },
  { MPX3_DAC_THRESH_3,   "Threshold[3]",        9, (1<<9)/2 },
  { MPX3_DAC_THRESH_4,   "Threshold[4]",        9, (1<<9)/2 },
  { MPX3_DAC_THRESH_5,   "Threshold[5]",        9, (1<<9)/2 },
  { MPX3_DAC_THRESH_6,   "Threshold[6]",        9, (1<<9)/2 },
  { MPX3_DAC_THRESH_7,   "Threshold[7]",        9, (1<<9)/2 },
  { MPX3_DAC_PREAMP,     "Preamp",              8, 120 },
  { MPX3_DAC_IKRUM,      "Ikrum",               8, 20 },
  { MPX3_DAC_SHAPER,     "Shaper",              8, 173 },
  { MPX3_DAC_DISC,       "Disc",                8, 255 },
  { MPX3_DAC_DISC_LS,    "Disc_LS",             8, 170 },
  { MPX3_DAC_THRESH_N,   "ThresholdN",          8, 20 },
  { MPX3_DAC_PIXEL,      "DAC_pixel",           8, 0x76 },
  { MPX3_DAC_DELAY,      "Delay",               8, (1<<8)/2 },
  { MPX3_DAC_TP_BUF_IN,  "TP_BufferIn",         8, (1<<8)/2 },
  { MPX3_DAC_TP_BUF_OUT, "TP_BufferOut",        8, 0x32 },
  { MPX3_DAC_RPZ,        "RPZ",                 8, (1<<8)-1 },
  { MPX3_DAC_GND,        "GND",                 8, 0x6E },
  { MPX3_DAC_TP_REF,     "TP_REF",              8, (1<<8)/2 },
  { MPX3_DAC_FBK,        "FBK",                 8, 0x8F },
  { MPX3_DAC_CAS,        "Cas",                 8, 191 },
  { MPX3_DAC_TP_REF_A,   "TP_REFA",             9, (1<<9)-1 },
  { MPX3_DAC_TP_REF_B,   "TP_REFB",             9, (1<<9)-1 }
};

static const dac_t MPX3RX_DAC_TABLE[MPX3RX_DAC_COUNT] =
{
  { MPX3RX_DAC_THRESH_0,    "Threshold[0]",     9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_1,    "Threshold[1]",     9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_2,    "Threshold[2]",     9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_3,    "Threshold[3]",     9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_4,    "Threshold[4]",     9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_5,    "Threshold[5]",     9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_6,    "Threshold[6]",     9, (1<<9)/2 },
  { MPX3RX_DAC_THRESH_7,    "Threshold[7]",     9, (1<<9)/2 },
  { MPX3RX_DAC_PREAMP,      "Preamp",           8, (1<<8)/2 },
  { MPX3RX_DAC_IKRUM,       "Ikrum",            8, (1<<8)/2 },
  { MPX3RX_DAC_SHAPER,      "Shaper",           8, (1<<8)/2 },
  { MPX3RX_DAC_DISC,        "Disc",             8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_LS,     "Disc_LS",          8, (1<<8)/2 },
  { MPX3RX_DAC_SHAPER_TEST, "Shaper_Test",      8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_L,      "DAC_DiscL",        8, (1<<8)/2 },
  { MPX3RX_DAC_TEST,        "DAC_test",         8, (1<<8)/2 },
  { MPX3RX_DAC_DISC_H,      "DAC_DiscH",        8, (1<<8)/2 },
  { MPX3RX_DAC_DELAY,       "Delay",            8, (1<<8)/2 },
  { MPX3RX_DAC_TP_BUF_IN,   "TP_BufferIn",      8, (1<<8)/2 },
  { MPX3RX_DAC_TP_BUF_OUT,  "TP_BufferOut",     8, (1<<8)/2 },
  { MPX3RX_DAC_RPZ,         "RPZ",              8, (1<<8)/2 },
  { MPX3RX_DAC_GND,         "GND",              8, (1<<8)/2 },
  { MPX3RX_DAC_TP_REF,      "TP_REF",           8, (1<<8)/2 },
  { MPX3RX_DAC_FBK,         "FBK",              8, (1<<8)/2 },
  { MPX3RX_DAC_CAS,         "Cas",              8, (1<<8)/2 },
  { MPX3RX_DAC_TP_REF_A,    "TP_REFA",          9, (1<<9)/2 },
  { MPX3RX_DAC_TP_REF_B,    "TP_REFB",          9, (1<<9)/2 }
};

static const QColor COLOR_TABLE[] = {
  Qt::red,
  Qt::black,
  Qt::darkRed,
  Qt::green,
  Qt::darkGreen,

  Qt::blue,
  Qt::darkBlue,
  Qt::cyan,
  Qt::darkCyan,
  Qt::magenta,

  Qt::darkMagenta,
  Qt::yellow,
  Qt::darkYellow,
  QColor( "darkorange" ),
  QColor( "purple" ),

  QColor( "khaki" ),
  QColor( "gold" ),
  QColor( "dodgerblue" ),
  QColor( "light gray" ),
  QColor( "medium gray" ),

  QColor( "red" ),
  QColor( "green" ),
  QColor( "blue" ),
  QColor( "cyan" ),
  QColor( "magenta" ),

  QColor( "yellow" ),
  QColor( "dark yellow" )
};

// ----------------------------------------------------------------------------

SpidrDacsScan::SpidrDacsScan()
  : QDialog(),
    _spidrController( 0 ),
    _deviceIndex( 0 ),
    _deviceType( MPX_TYPE_NC ),
    _scanInProgress( false ),
    _dacCount( 0 ),
    _dacIndex( 0 ),
    _dacCode( TPX3_SENSEOFF ),
    _dacMax( 0 ),
    _dacVal( 0 ),
    _dacStep( 1 ),
    _samples( 1 ),
    _dacTable( 0 ),
    _plot( 0 ),
    _graph( 0 )
{
  this->setupUi(this);

  this->setWindowFlags( Qt::WindowMinimizeButtonHint |
			Qt::WindowMaximizeButtonHint |
			Qt::WindowCloseButtonHint );

  _ipAddrValidator = new QIntValidator( 1, 255, this );
  _lineEditAddr3->setValidator( _ipAddrValidator );
  _lineEditAddr2->setValidator( _ipAddrValidator );
  _lineEditAddr1->setValidator( _ipAddrValidator );
  _lineEditAddr0->setValidator( _ipAddrValidator );

  _ipPortValidator = new QIntValidator( 1, 65535, this );
  _lineEditPort->setValidator( _ipPortValidator );

  _labelDisconnected->hide();
  //_comboBoxDeviceIndex->hide();
  _labelDac->hide();
  _labelErr->hide();

  // Populate comboboxes
  int i;
  for( i=1; i<=4; ++i )
    _comboBoxDacStep->addItem( QString::number(i) );
  _comboBoxAdcSamples->addItem( "1 sample" );
  _comboBoxAdcSamples->addItem( "2 samples" );
  _comboBoxAdcSamples->addItem( "4 samples" );
  _comboBoxAdcSamples->addItem( "8 samples" );
  _comboBoxAdcSamples->setCurrentIndex( 3 );
  for( i=1; i<=4; ++i )
    _comboBoxPenWidth->addItem( QString::number(i) );
  _comboBoxDeviceType->addItem( "T3" );
  _comboBoxDeviceType->addItem( "M3" );
  _comboBoxDeviceType->addItem( "M3RX" );

  // Prepare the plot
  _plot = new QCustomPlot();
  _plot->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );
  // The title
  _title = new QCPPlotTitle( _plot );
  _title->setText( "Timepix3 DACs scan" );
  _plot->plotLayout()->insertRow( 0 );
  _plot->plotLayout()->addElement( 0, 0, _title );
  // The legend
  _plot->legend->setVisible( false ); // Nothing there yet...
  QFont f = font();  // Start out with Dialog's font..
  f.setPointSize( 9 ); // and make a bit smaller for legend
  _plot->legend->setFont( f );
  _plot->legend->setBrush( QBrush(QColor(255,255,255,230)) );
  // The axes
  _plot->xAxis->setRange( 0, 512 );
  _plot->yAxis->setRange( 0, 4096 );
  f = font();  // Start out with Dialog's font..
  f.setBold( true );
  _plot->xAxis->setLabelFont( f );
  _plot->yAxis->setLabelFont( f );
  // The labels:
  _plot->xAxis->setLabel("DAC setting");
  _plot->yAxis->setLabel("DAC out");

  // Insert the plot in the dialog window
  _verticalLayout->addWidget( _plot );

  // Connect signals
  connect( _pushButtonConnect, SIGNAL( clicked() ),
	   this, SLOT( connectOrDisconnect() ) );
  connect( _comboBoxDeviceIndex, SIGNAL( currentIndexChanged(int) ),
	   this, SLOT( changeDeviceIndex(int) ) );
  connect( _comboBoxDeviceType, SIGNAL( currentIndexChanged(int) ),
	   this, SLOT( changeDeviceType(int) ) );
  connect( _pushButtonScan, SIGNAL( clicked() ),
	   this, SLOT( startOrStopScan() ) );
}

// ----------------------------------------------------------------------------

SpidrDacsScan::~SpidrDacsScan()
{
  if( _scanInProgress ) this->startOrStopScan();
  if( _spidrController ) delete _spidrController;
}

// ----------------------------------------------------------------------------

void SpidrDacsScan::connectOrDisconnect()
{
  if( _spidrController )
    {
      if( _scanInProgress ) this->startOrStopScan();
      delete _spidrController;
      _spidrController = 0;
      _pushButtonConnect->setText( "Connect" );
      _pushButtonScan->setEnabled( false );
    }
  else
    {
      _labelDisconnected->hide();
      _labelDac->hide();
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
	  _pushButtonConnect->setText( "Disconnect" );
	  QApplication::restoreOverrideCursor();

	  _pushButtonScan->setEnabled( true );

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
	}
      else
	{
	  delete _spidrController;
	  _spidrController = 0;
	  QApplication::restoreOverrideCursor();
	  QMessageBox::warning( this, "Connecting to SPIDR",
				"Failed to connect!         " );
	}
    }
}

// ----------------------------------------------------------------------------

void SpidrDacsScan::changeDeviceIndex( int index )
{
  if( index < 0 ) return;
  _deviceIndex = index;
}

// ----------------------------------------------------------------------------

void SpidrDacsScan::changeDeviceType( int index )
{
  if( index < 0 ) return;
  _deviceType = index;
  switch( _deviceType )
    {
    case MPX_TYPE_MPX31:
      _dacCount = MPX3_DAC_COUNT;
      _dacTable = &MPX3_DAC_TABLE[0];
      _plot->yAxis->setRange( 0, 65536 );
      _title->setText( "Medipix3 DACs scan" );
      _comboBoxDacStep->setCurrentIndex( 3 );
      _comboBoxAdcSamples->setCurrentIndex( 0 );
      break;
    case MPX_TYPE_MPX3RX:
      _dacCount = MPX3RX_DAC_COUNT;
      _dacTable = &MPX3RX_DAC_TABLE[0];
      _plot->yAxis->setRange( 0, 65536 );
      _title->setText( "Medipix3RX DACs scan" );
      _comboBoxDacStep->setCurrentIndex( 3 );
      _comboBoxAdcSamples->setCurrentIndex( 0 );
      break;
    default:
      _dacCount = TPX3_DAC_COUNT_TO_SET;
      _dacTable = &TPX3_DAC_TABLE[0];
      _plot->yAxis->setRange( 0, 4096 );
      _title->setText( "Timepix3 DACs scan" );
      _comboBoxDacStep->setCurrentIndex( 0 );
      _comboBoxAdcSamples->setCurrentIndex( 3 );
      break;
    }
  _plot->clearGraphs();
  _plot->legend->setVisible( false );
  _plot->replot();
}

// ----------------------------------------------------------------------------

void SpidrDacsScan::startOrStopScan()
{
  if( _scanInProgress )
    {
      _scanInProgress = false;
      _pushButtonScan->setText( "Start Scan" );
      _comboBoxDeviceIndex->setEnabled( true );
      _comboBoxDeviceType->setEnabled( true );
      _labelDac->hide();
      if( _spidrController ) _spidrController->setLogLevel( 0 ); // DEBUG
    }
  else
    {
      _scanInProgress = true;
      _dacIndex = 0;
      _dacVal   = 0;
      _pushButtonScan->setText( "Stop Scan" );
      _comboBoxDeviceIndex->setEnabled( false );
      _comboBoxDeviceType->setEnabled( false );
      _labelDac->show();
      _labelErr->hide();

      _plot->legend->setVisible( true );
      _plot->clearGraphs();
      _plot->replot();
      this->scan();
      if( _spidrController ) _spidrController->setLogLevel( 2 ); // WARNING
    }
}

// ----------------------------------------------------------------------------

void SpidrDacsScan::scan()
{
  if( !_spidrController || !_spidrController->isConnected() )
    {
      if( _scanInProgress ) this->startOrStopScan();
      return;
    }

  if( _dacVal == 0 )
    {
      // Next DAC index
      //_dacCode = TPX3_DAC_TABLE[_dacIndex].code;
      _dacCode = _dacTable[_dacIndex].code;
      _labelDac->setText( QString("DAC %1..").arg( _dacCode ) );
      //_dacMax  = (1 << TPX3_DAC_TABLE[_dacIndex].bits) - 1;
      _dacMax  = (1 << _dacTable[_dacIndex].bits) - 1;
      if( !_spidrController->setSenseDac( _deviceIndex, _dacCode ) )
	this->inError();

      // The number of (ADC) samples to take per DAC setting
      _samples = (1 << _comboBoxAdcSamples->currentIndex());

      // The DAC settings step-size
      if( _dacMax > 32 )
	_dacStep = _comboBoxDacStep->currentIndex() + 1;
      else
	_dacStep = 1;

      // Next graph
      _plot->addGraph();
      _graph = _plot->graph( _dacIndex );
      QPen pen( COLOR_TABLE[_dacIndex] );
      pen.setWidth( _comboBoxPenWidth->currentIndex() + 1 );
      _graph->setPen( pen );
      //_graph->setName( QString(TPX3_DAC_TABLE[_dacIndex].name) );
      _graph->setName( QString(_dacTable[_dacIndex].name) );
      _graph->addToLegend();
    }

  if( !_spidrController->setDac( _deviceIndex, _dacCode, _dacVal ) )
    this->inError();

  int adc_val;
  if( !_spidrController->getDacOut( _deviceIndex, &adc_val, _samples ) )
  //if( !_spidrController->getAdc( &adc_val, _deviceIndex, _samples ) )
    {
      this->inError();
    }
  else
    {
      // Add sample to the graph, and update the plot
      adc_val /= _samples;
      _graph->addData( _dacVal, adc_val );
      _plot->replot();
    }

  // Next DAC value
  _dacVal += _dacStep;
  if( _dacVal > _dacMax )
    {
      // Set this DAC to its default setting...
      //int dflt = TPX3_DAC_TABLE[_dacIndex].dflt;
      int dflt = _dacTable[_dacIndex].dflt;
      if( !_spidrController->setDac( _deviceIndex, _dacCode, dflt ) )
	this->inError();

      // Go for next DAC or end it
      _dacVal = 0;
      ++_dacIndex;
      //if( _dacIndex == TPX3_DAC_COUNT_TO_SET )
      if( _dacIndex == _dacCount )
	this->startOrStopScan(); // The end
    }

  if( _scanInProgress )
    QTimer::singleShot( 0, this, SLOT( scan() ) );
}

// ----------------------------------------------------------------------------

void SpidrDacsScan::inError()
{
  _labelErr->show();
  if( _scanInProgress ) this->startOrStopScan();
}

// ----------------------------------------------------------------------------
