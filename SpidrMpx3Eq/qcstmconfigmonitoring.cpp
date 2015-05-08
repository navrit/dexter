#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"
#include "mpx3config.h"

#include "SpidrController.h"

QCstmConfigMonitoring::QCstmConfigMonitoring(QWidget *parent) :
QWidget(parent),
ui(new Ui::QCstmConfigMonitoring)
{
	ui->setupUi(this);
	_timerId = this->startTimer( 1000 );
	ui->gainModeCombobox->addItem("Super High Gain Mode");
	ui->gainModeCombobox->addItem("Low Gain Mode");
	ui->gainModeCombobox->addItem("High Gain Mode");
	ui->gainModeCombobox->addItem("Super Low Gain Mode");

}

QCstmConfigMonitoring::~QCstmConfigMonitoring()
{
	delete ui;
}

void QCstmConfigMonitoring::SetMpx3GUI(Mpx3GUI *p){
	_mpx3gui = p;
  Mpx3Config *config = _mpx3gui->getConfig();

  connect(ui->ColourModeCheckBox, SIGNAL(clicked(bool)), config, SLOT(setColourMode(bool)));
  connect(config, SIGNAL(colourModeChanged(bool)), ui->ColourModeCheckBox, SLOT(setChecked(bool)));

  connect(ui->csmSpmSpinner, SIGNAL(valueChanged(int)), config, SLOT(setCsmSpm(int)));
  connect(config, SIGNAL(csmSpmChanged(int)), ui->csmSpmSpinner, SLOT(setValue(int)));

  connect(ui->decodeFramesCheckbox, SIGNAL(clicked(bool)), config, SLOT(setDecodeFrames(bool)));
  connect(config, SIGNAL(decodeFramesChanged(bool)), ui->decodeFramesCheckbox, SLOT(setChecked(bool)));

  connect(ui->gainModeCombobox, SIGNAL(activated(int)), config, SLOT(setGainMode(int)));
  connect(config, SIGNAL(gainModeChanged(int)), ui->gainModeCombobox, SLOT(setCurrentIndex(int)));

  connect(ui->maxPacketSizeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setMaxPacketSize(int)));
  connect(config, SIGNAL(MaxPacketSizeChanged(int)), ui->maxPacketSizeSpinner, SLOT(setValue(int)));

  connect(ui->nTriggersSpinner, SIGNAL(valueChanged(int)), config, SLOT(setNTriggers(int)));
  connect(config, SIGNAL(nTriggersChanged(int)), ui->nTriggersSpinner, SLOT(setValue(int)));

  connect(ui->operationModeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setOperationMode(int)));
  connect(config, SIGNAL(operationModeChanged(int)), ui->operationModeSpinner, SLOT(setValue(int)));

  connect(ui->pixelDepthSpinner, SIGNAL(valueChanged(int)), config, SLOT(setPixelDepth(int)));
  connect(config, SIGNAL(pixelDepthChanged(int)), ui->pixelDepthSpinner, SLOT(setValue(int)));

  connect(ui->triggerLengthSpinner, SIGNAL(valueChanged(int)), config, SLOT(setTriggerLength(int)));
  connect(config, SIGNAL(TriggerLengthChanged(int)), ui->triggerLengthSpinner, SLOT(setValue(int)));

  connect(ui->triggerModeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setTriggerMode(int)));
  connect(config, SIGNAL(TriggerModeChanged(int)), ui->triggerModeSpinner, SLOT(setValue(int)));

  connect(ui->portSpinner, SIGNAL(valueChanged(int)), config, SLOT(setPort(int)));
  connect(config, SIGNAL(portChanged(int)), ui->portSpinner, SLOT(setValue(int)));

  connect(config, SIGNAL(IpAdressChanged(QString)), ui->ipLineEdit, SLOT(setText(QString)));
  //connect(ui->ipLineEdit, SIGNAL(textEdited(QString)), config, SLOT(setIpAddress(QString)));//Can't turn of keyboard tracking for this
}

void QCstmConfigMonitoring::timerEvent(QTimerEvent *) {


	/*
	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

	if ( spidrcontrol ) {

		int mdegrees;
		if( spidrcontrol->getRemoteTemp( &mdegrees ) ) {
			QString qs = QString("%1.%2").arg( mdegrees/1000 ).arg( mdegrees%1000, 3, 10, QChar('0') );
			ui->remoteTempMeasLabel->setText( qs );
		} else {
			ui->remoteTempMeasLabel->setText( "--.---" );
		}

		if( spidrcontrol->getLocalTemp( &mdegrees ) ) {
			QString qs = QString("%1.%2").arg( mdegrees/1000 ).arg( mdegrees%1000, 3, 10, QChar('0') );
			ui->localTempMeasLabel->setText( qs );
		} else {
			ui->localTempMeasLabel->setText( "--.---" );
		}

	}
*/

	/*
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
  if( !_skipVdd )
    {
      if( _spidrController->getVddNow( &mvolt, &mamp, &mwatt ) )
	{
	  _lineEditVddMvolt->setText( QString::number( mvolt ) );
	  _lineEditVddMwatt->setText( QString::number( mwatt ) );
	  QString qs = QString("%1.%2").arg( mamp/10 ).arg( mamp%10 );
	  _lineEditVddMamp->setText( qs );
	}
      else
	{
	  _skipVdd = true; // SPIDR-TPX3 does not have VDD
	  _lineEditVddMvolt->setText( "----" );
	  _lineEditVddMamp->setText( "----" );
	  _lineEditVddMwatt->setText( "----" );
	}
    }

  _leUpdateLed->show();
  QTimer::singleShot( UPDATE_INTERVAL_MS/4, this, SLOT(updateLedOff()) );
	 */

}

void QCstmConfigMonitoring::on_SaveButton_clicked()//TODO: automatically append .json
{
  QFileDialog saveDialog(this, tr("Save configuration"), tr("./config"), tr("Json files (*.json)"));
  saveDialog.setAcceptMode(QFileDialog::AcceptSave);
  saveDialog.setDefaultSuffix("json");
  //saveDialog.setDirectory("./config");
  saveDialog.exec();
  QString filename = saveDialog.selectedFiles().first();
  //QFileDialog dialog;
  //dialog.setDefaultSuffix("json");//Bugged under Linux?
  //QString filename = dialog.getSaveFileName(this, tr("Save configuration"), tr("./config"), tr("Json files (*.json)"));
  _mpx3gui->getConfig()->toJsonFile(filename, ui->IncludeDacsCheck->isChecked());
}

void QCstmConfigMonitoring::on_LoadButton_clicked()
{
  QString filename = QFileDialog::getOpenFileName(this, tr("Open configuration"), tr("./config"), tr("Json files (*.json)"));
  _mpx3gui->getConfig()->fromJsonFile(filename, ui->IncludeDacsCheck->isChecked());
}

void QCstmConfigMonitoring::on_ipLineEdit_editingFinished()
{
  _mpx3gui->getConfig()->setIpAddress(ui->ipLineEdit->text());
}

void QCstmConfigMonitoring::on_ColourModeCheckBox_toggled(bool checked)
{
  _mpx3gui->clear_data();
    if(checked)
        _mpx3gui->getDataset()->resize(_mpx3gui->getDataset()->x()/2, _mpx3gui->getDataset()->y()/2);
    else
        _mpx3gui->getDataset()->resize(_mpx3gui->getDataset()->x()*2, _mpx3gui->getDataset()->y()*2);
}
