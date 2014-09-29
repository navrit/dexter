#include <QIntValidator>
#include <QMessageBox>
#include <QString>
#include <QTimer>

#include "SpidrMon.h"
#include "SpidrController.h"
#include "tpx3defs.h"

QString VERSION( "v2.1.0  16-Jan-2014" );

//const int UPDATE_INTERVAL_MS = 750;
const int UPDATE_INTERVAL_MS = 1500;

// ----------------------------------------------------------------------------

SpidrMon::SpidrMon()
  : QDialog(),
    _spidrController( 0 ),
    _dacCode( TPX3_BANDGAP_OUTPUT ),
    _doubleSpidr( false )
{
  this->setupUi(this);

  this->setWindowFlags( Qt::WindowMinimizeButtonHint |
			Qt::WindowCloseButtonHint );

  connect( _pushButtonConnectOrDisconnect, SIGNAL( clicked() ),
	   this, SLOT( connectOrDisconnect() ) );
  connect( _checkBoxSpidrX2, SIGNAL( stateChanged(int) ),
           this, SLOT( doubleSpidrModeChanged() ) );

  _ipAddrValidator = new QIntValidator( 1, 255, this );
  _lineEditAddr3->setValidator( _ipAddrValidator );
  _lineEditAddr2->setValidator( _ipAddrValidator );
  _lineEditAddr1->setValidator( _ipAddrValidator );
  _lineEditAddr0->setValidator( _ipAddrValidator );

  _ipPortValidator = new QIntValidator( 1, 65535, this );
  _lineEditPort->setValidator( _ipPortValidator );

  _labelDisconnected->hide();
  _lePowerOffLed->hide();

  // Data update 'LED's
  _leUpdateSpidrLed->hide();
  _leUpdateTpxLed->hide();
  _leUpdateSpidrLed_2->hide();
  _leUpdateTpxLed_2->hide();

  // Hide for the time being... (no fan)
  labelFan->hide();
  _lineEditFanSpidr->hide();

  // Hide until '2 Chipboards' is ticked
  _groupBoxSpidr_2->hide();
  _groupBoxTpx_2->hide();
  this->resize( this->minimumSize() );
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

      _spidrController->setSenseDac( 0, TPX3_SENSEOFF );
      if( _cbMonitorTpx->isChecked() )
	_spidrController->setSenseDac( 1, TPX3_SENSEOFF );

      delete _spidrController;
      _spidrController = 0;
      _pushButtonConnectOrDisconnect->setText( "Connect" );

      _lineEditRemoteTemp->setEnabled( false );
      _lineEditLocalTemp->setEnabled( false );
      _lineEditFpgaTemp->setEnabled( false );
      _lineEditAvddMvolt->setEnabled( false );
      _lineEditAvddMamp->setEnabled( false );
      _lineEditAvddMwatt->setEnabled( false );
      _lineEditDvddMvolt->setEnabled( false );
      _lineEditDvddMamp->setEnabled( false );
      _lineEditDvddMwatt->setEnabled( false );
      _lineEditVdda->setEnabled( false );
      _lineEditBias->setEnabled( false );
      _lineEditFanSpidr->setEnabled( false );
      _lineEditFanVc707->setEnabled( false );
      _lineEditDac1->setEnabled( false );
      _lineEditDac2->setEnabled( false );
      _lineEditDac3->setEnabled( false );
      _lineEditDac4->setEnabled( false );

      _lineEditRemoteTemp_2->setEnabled( false );
      _lineEditLocalTemp_2->setEnabled( false );
      _lineEditAvddMvolt_2->setEnabled( false );
      _lineEditAvddMamp_2->setEnabled( false );
      _lineEditAvddMwatt_2->setEnabled( false );
      _lineEditDvddMvolt_2->setEnabled( false );
      _lineEditDvddMamp_2->setEnabled( false );
      _lineEditDvddMwatt_2->setEnabled( false );
      _lineEditVdda_2->setEnabled( false );
      _lineEditBias_2->setEnabled( false );
      _lineEditDac1_2->setEnabled( false );
      _lineEditDac2_2->setEnabled( false );
      _lineEditDac3_2->setEnabled( false );
      _lineEditDac4_2->setEnabled( false );

      _dacCode = TPX3_BANDGAP_OUTPUT;
    }
  else
    {
      _labelDisconnected->hide();
      _lePowerOffLed->hide();

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

      this->initDataDisplay();

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
	  _lineEditFpgaTemp->setEnabled( true );
	  _lineEditAvddMvolt->setEnabled( true );
	  _lineEditAvddMamp->setEnabled( true );
	  _lineEditAvddMwatt->setEnabled( true );
	  _lineEditDvddMvolt->setEnabled( true );
	  _lineEditDvddMamp->setEnabled( true );
	  _lineEditDvddMwatt->setEnabled( true );
	  _lineEditVdda->setEnabled( true );
	  _lineEditBias->setEnabled( true );
	  _lineEditFanSpidr->setEnabled( true );
	  _lineEditFanVc707->setEnabled( true );
	  //_lineEditDac1->setEnabled( true );
	  //_lineEditDac2->setEnabled( true );
	  //_lineEditDac3->setEnabled( true );
	  //_lineEditDac4->setEnabled( true );

	  _lineEditRemoteTemp_2->setEnabled( true );
	  _lineEditLocalTemp_2->setEnabled( true );
	  _lineEditAvddMvolt_2->setEnabled( true );
	  _lineEditAvddMamp_2->setEnabled( true );
	  _lineEditAvddMwatt_2->setEnabled( true );
	  _lineEditDvddMvolt_2->setEnabled( true );
	  _lineEditDvddMamp_2->setEnabled( true );
	  _lineEditDvddMwatt_2->setEnabled( true );
	  _lineEditVdda_2->setEnabled( true );
	  _lineEditBias_2->setEnabled( true );

	  //if( _cbMonitorTpx->isChecked() )
	    // Set output of Timepix3 SenseDAC
	    // in preparation for the first ADC reading
	  _spidrController->setSenseDac( 0, _dacCode );
	  if( _cbMonitorTpx->isChecked() )
	    _spidrController->setSenseDac( 1, _dacCode );

	  // Just to make sure: set I2C switch to 1st SPIDR board
	  _spidrController->selectChipBoard( 1 );

	  // (Re)enable power to Timepix3
	  _spidrController->setTpxPowerEna( true );

	  QTimerEvent te(1);
	  this->timerEvent( &te );
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
      if( mdegrees/1000 == 255 )
	_lineEditRemoteTemp->setText( "--.--" );
      else
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
  if( _spidrController->getFpgaTemp( &mdegrees ) )
    {
      QString qs = QString("%1.%2").arg( mdegrees/1000 ).
	arg( mdegrees%1000, 3, 10, QChar('0') );
      _lineEditFpgaTemp->setText( qs );
    }
  else
    {
      _lineEditFpgaTemp->setText( "--.---" );
    }

  int mvolt, mamp, mwatt;
  if( _spidrController->getAvddNow( &mvolt, &mamp, &mwatt ) )
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
  if( _spidrController->getDvddNow( &mvolt, &mamp, &mwatt ) )
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

  int rpm;
  if( _spidrController->getFanSpeed( &rpm ) )
    _lineEditFanSpidr->setText( QString::number( rpm ) );
  else
    _lineEditFanSpidr->setText( "----" );
  if( _spidrController->getFanSpeedVC707( &rpm ) )
    _lineEditFanVc707->setText( QString::number( rpm ) );
  else
    _lineEditFanVc707->setText( "----" );

  if( _cbMonitorTpx->isChecked() )
    {
      QString qs("--.---");
      int adc_val;
      if( _spidrController->getAdc( &adc_val ) )
	{
	  // Full-scale is 1.5V = 1500mV
	  adc_val = (adc_val*1500) / 4095;
	  qs = QString("%1.%2").arg( adc_val/1000 )
	    .arg( adc_val%1000, 3, 10, QChar('0') );

	  if( _dacCode == TPX3_BANDGAP_OUTPUT )
	    {
	      _bgOut1 = adc_val; // In mV
	    }
	  else if( _dacCode == TPX3_BANDGAP_TEMP )
	    {
	      // Apply formula from Medipix3 manual;
	      /// display temperature as "x.yy" (hundredths)
	      float temp = 88.75 - (607.3 * (float)(adc_val-_bgOut1))/1000.0;
	      QString ts = QString("(T approx. [C]: %1.%2 )")
		.arg( (int) temp ).arg( ((int)((temp*1000.0))%1000/10) );
	      // Display only when a reasonable value is found
	      if( temp < -60.0 || temp > 120.0 )
		{
		  // Not reasonable ?
		  _labelT->setText( "" );
		}
	      else
		{
		  _labelT->setText( ts );
		  // Over-temperature protection
		  if( _cbOverTempProt->isChecked() &&
		      temp > (float) _sbOverTemp->value() )
		    {
		      _spidrController->setTpxPowerEna( false );
		      _lePowerOffLed->show();
		    }
		}
	    }
	}
      if( _dacCode == TPX3_BANDGAP_OUTPUT )
	{
	  _lineEditDac1->setText( qs );
	  _lineEditDac1->setEnabled( true );
	}
      else if( _dacCode == TPX3_BANDGAP_TEMP )
	{
	  _lineEditDac2->setText( qs );
	  _lineEditDac2->setEnabled( true );
	}
      else if( _dacCode == TPX3_IBIAS_DAC )
	{
	  _lineEditDac3->setText( qs );
	  _lineEditDac3->setEnabled( true );
	}
      else if( _dacCode == TPX3_IBIAS_DAC_CAS )
	{
	  _lineEditDac4->setText( qs );
	  _lineEditDac4->setEnabled( true );
	}
    }
  else
    {
      _lineEditDac1->setEnabled( false );
      _lineEditDac2->setEnabled( false );
      _lineEditDac3->setEnabled( false );
      _lineEditDac4->setEnabled( false );
    }

  _leUpdateSpidrLed->show();
  QTimer::singleShot( UPDATE_INTERVAL_MS/4, this, SLOT(updateLedOff()) );

  if( !_doubleSpidr )
    {
      if( _cbMonitorTpx->isChecked() )
	{
	  // Next time next 'DAC'
	  if( _dacCode != TPX3_IBIAS_DAC_CAS )
	    ++_dacCode;
	  else
	    _dacCode = TPX3_BANDGAP_OUTPUT;
	  // Set output of Timepix3 SenseDAC in preparation
	  // for the next ADC reading
	  _spidrController->setSenseDac( 0, _dacCode );
	}
      return;
    }

  // Select 2nd SPIDR board's I2C
  if( !_spidrController->selectChipBoard( 2 ) )
    {
      // .....
      return;
    }

  if( _spidrController->getRemoteTemp( &mdegrees ) )
    {
      QString qs = QString("%1.%2").arg( mdegrees/1000 ).
	arg( mdegrees%1000, 3, 10, QChar('0') );
      if( mdegrees/1000 == 255 )
	_lineEditRemoteTemp_2->setText( "--.--" );
      else
	_lineEditRemoteTemp_2->setText( qs );
    }
  else
    {
      _lineEditRemoteTemp_2->setText( "--.---" );
    }
  if( _spidrController->getLocalTemp( &mdegrees ) )
    {
      QString qs = QString("%1.%2").arg( mdegrees/1000 ).
	arg( mdegrees%1000, 3, 10, QChar('0') );
      _lineEditLocalTemp_2->setText( qs );
    }
  else
    {
      _lineEditLocalTemp_2->setText( "--.---" );
    }

  if( _spidrController->getAvddNow( &mvolt, &mamp, &mwatt ) )
    {
      _lineEditAvddMvolt_2->setText( QString::number( mvolt ) );
      _lineEditAvddMwatt_2->setText( QString::number( mwatt ) );
      QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
      _lineEditAvddMamp_2->setText( qs );
    }
  else
    {
      _lineEditAvddMvolt_2->setText( "----" );
      _lineEditAvddMamp_2->setText( "----" );
      _lineEditAvddMwatt_2->setText( "----" );
    }
  if( _spidrController->getDvddNow( &mvolt, &mamp, &mwatt ) )
    {
      _lineEditDvddMvolt_2->setText( QString::number( mvolt ) );
      _lineEditDvddMwatt_2->setText( QString::number( mwatt ) );
      QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
      _lineEditDvddMamp_2->setText( qs );
    }
  else
    {
      _lineEditDvddMvolt_2->setText( "----" );
      _lineEditDvddMamp_2->setText( "----" );
      _lineEditDvddMwatt_2->setText( "----" );
    }

  if( _spidrController->getBiasVoltage( &mvolt ) )
    _lineEditBias_2->setText( QString::number( mvolt ) );
  else
    _lineEditBias_2->setText( "----" );

  if( _spidrController->getVdda( &mvolt ) )
    _lineEditVdda_2->setText( QString::number( mvolt ) );
  else
    _lineEditVdda_2->setText( "----" );

  if( _cbMonitorTpx->isChecked() )
    {
      QString qs("--.---");
      int adc_val;
      if( _spidrController->getAdc( &adc_val ) )
	{
	  // Full-scale is 1.5V = 1500mV
	  adc_val = (adc_val*1500) / 4095;
	  qs = QString("%1.%2").arg( adc_val/1000 )
	    .arg( adc_val%1000, 3, 10, QChar('0') );

	  if( _dacCode == TPX3_BANDGAP_OUTPUT )
	    {
	      _bgOut2 = adc_val; // In mV
	    }
	  else if( _dacCode == TPX3_BANDGAP_TEMP )
	    {
	      // Apply formula from Medipix3 manual;
	      /// display temperature as "x.yy" (hundredths)
	      float temp = 88.75 - (607.3 * (float)(adc_val-_bgOut2))/1000.0;
	      QString ts = QString("(T approx. [C]: %1.%2 )")
		.arg( (int) temp ).arg( ((int)((temp*1000.0))%1000/10) );
	      // Display only when a reasonable value is found
	      if( temp < 0.0 || temp > 120.0 )
		_labelT_2->setText( "" );
	      else
		_labelT_2->setText( ts );
	    }
	}
      if( _dacCode == TPX3_BANDGAP_OUTPUT )
	{
	  _lineEditDac1_2->setText( qs );
	  _lineEditDac1_2->setEnabled( true );
	}
      else if( _dacCode == TPX3_BANDGAP_TEMP )
	{
	  _lineEditDac2_2->setText( qs );
	  _lineEditDac2_2->setEnabled( true );
	}
      else if( _dacCode == TPX3_IBIAS_DAC )
	{
	  _lineEditDac3_2->setText( qs );
	  _lineEditDac3_2->setEnabled( true );
	}
      else if( _dacCode == TPX3_IBIAS_DAC_CAS )
	{
	  _lineEditDac4_2->setText( qs );
	  _lineEditDac4_2->setEnabled( true );
	}
    }
  else
    {
      _lineEditDac1_2->setEnabled( false );
      _lineEditDac2_2->setEnabled( false );
      _lineEditDac3_2->setEnabled( false );
      _lineEditDac4_2->setEnabled( false );
    }

  if( _cbMonitorTpx->isChecked() )
    {
      // Next time next 'DAC'
      if( _dacCode != TPX3_IBIAS_DAC_CAS )
	++_dacCode;
      else
	_dacCode = TPX3_BANDGAP_OUTPUT;
      // Set output of Timepix3 SenseDAC in preparation
      // for the next ADC reading
      _spidrController->setSenseDac( 0, _dacCode );
      _spidrController->setSenseDac( 1, _dacCode );
    }

  // Switch (I2C) back to 1st SPIDR board
  _spidrController->selectChipBoard( 1 );

  _leUpdateSpidrLed_2->show();
}

// ----------------------------------------------------------------------------

void SpidrMon::initDataDisplay()
{
  _lineEditRemoteTemp->setText( "--.---" );
  _lineEditLocalTemp->setText( "--.---" );
  _lineEditFpgaTemp->setText( "--.---" );
  _lineEditAvddMvolt->setText( "----" );
  _lineEditAvddMamp->setText( "----" );
  _lineEditAvddMwatt->setText( "----" );
  _lineEditDvddMvolt->setText( "----" );
  _lineEditDvddMamp->setText( "----" );
  _lineEditDvddMwatt->setText( "----" );
  _lineEditVdda->setText( "----" );
  _lineEditBias->setText( "----" );
  _lineEditFanSpidr->setText( "----" );
  _lineEditFanVc707->setText( "----" );
}

// ----------------------------------------------------------------------------

void SpidrMon::updateLedOff()
{
  _leUpdateSpidrLed->hide();
  _leUpdateTpxLed->hide();

  _leUpdateSpidrLed_2->hide();
  _leUpdateTpxLed_2->hide();
}

// ----------------------------------------------------------------------------

void SpidrMon::doubleSpidrModeChanged()
{
  if( _checkBoxSpidrX2->isChecked() )
    {
      _doubleSpidr = true;
      _groupBoxSpidr_2->show();
      _groupBoxTpx_2->show();
    }
  else
    {
      _doubleSpidr = false;
      _groupBoxSpidr_2->hide();
      _groupBoxTpx_2->hide();
      // Can't do the resize here, doesn't work (?)
      QTimer::singleShot( 0, this, SLOT(myResize()) );
    }
}

// ----------------------------------------------------------------------------

void SpidrMon::myResize()
{
  this->resize( this->minimumSize() );
}

// ----------------------------------------------------------------------------
