#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"
#include "mpx3config.h"

#include "SpidrController.h"
#include "StepperMotorController.h"

#include "qtableview.h"
#include "qstandarditemmodel.h"

QCstmConfigMonitoring::QCstmConfigMonitoring(QWidget *parent) :
QWidget(parent),
ui(new Ui::QCstmConfigMonitoring) {
	ui->setupUi(this);
	_timerId = this->startTimer( 1000 );
	ui->gainModeCombobox->addItem("Super High Gain Mode");
	ui->gainModeCombobox->addItem("Low Gain Mode");
	ui->gainModeCombobox->addItem("High Gain Mode");
	ui->gainModeCombobox->addItem("Super Low Gain Mode");

	ui->motorDial->setNotchesVisible(true);

	_stepperThread = 0x0;
	_stepper = 0x0;

	// Set inactive whatever is needed to be inactive
	activeInGUI();

	QStandardItemModel * tvmodel = new QStandardItemModel(3,2, this); // 3 Rows and 2 Columns
	tvmodel->setHorizontalHeaderItem( 0, new QStandardItem(QString("pos")) );
	tvmodel->setHorizontalHeaderItem( 1, new QStandardItem(QString("angle")) );

	ui->stepperCalibrationTableView->setModel( tvmodel );

	ui->stepperMotorCheckBox->setToolTip( "enable/disable stepper motor control" );

}

QCstmConfigMonitoring::~QCstmConfigMonitoring()
{
	if(_stepperThread) delete _stepperThread;
	delete ui;
}

void QCstmConfigMonitoring::activeInGUI(){

	// stepper part
	if ( _stepper ) {
		if ( ! _stepper->isStepperReady() ) {
			deactivateItemsGUI();
		} else {
			activateItemsGUI();
		}
	} else {
		deactivateItemsGUI();
	}

}

void QCstmConfigMonitoring::activateItemsGUI(){
	ui->motorDial->setEnabled( true );
	ui->motorGoToTargetButton->setEnabled( true );
	ui->motorResetButton->setEnabled( true );
	ui->stepperCalibrationTableView->setEnabled( true );
	ui->stepperUseCalibCheckBox->setEnabled( true );
	ui->accelerationSpinBox->setEnabled( true );
	ui->speedSpinBox->setEnabled( true );
	ui->targetPosSpinBox->setEnabled( true );
	ui->motorIdSpinBox->setEnabled( true );
}

void QCstmConfigMonitoring::deactivateItemsGUI(){
	ui->motorDial->setDisabled( true );
	ui->motorGoToTargetButton->setDisabled( true );
	ui->motorResetButton->setDisabled( true );
	ui->stepperCalibrationTableView->setDisabled( true );
	ui->stepperUseCalibCheckBox->setDisabled( true );
	ui->accelerationSpinBox->setDisabled( true );
	ui->speedSpinBox->setDisabled( true );
	ui->targetPosSpinBox->setDisabled( true );
	ui->motorIdSpinBox->setDisabled( true );
}


void QCstmConfigMonitoring::SetMpx3GUI(Mpx3GUI *p) {

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

	_stepper = 0x0;

	// stepper
	// When the slider released, talk to the hardware
	QObject::connect( ui->motorDial, SIGNAL(sliderReleased()), this, SLOT(motorDialReleased()) );
	QObject::connect( ui->motorDial, SIGNAL(sliderMoved(int)), this, SLOT(motorDialMoved(int)) );

	connect(ui->accelerationSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setAcceleration(double)) );
	connect(ui->speedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeed(double)) );

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

void QCstmConfigMonitoring::on_LoadButton_clicked() {
	QString filename = QFileDialog::getOpenFileName(this, tr("Open configuration"), tr("./config"), tr("Json files (*.json)"));
	_mpx3gui->getConfig()->fromJsonFile(filename, ui->IncludeDacsCheck->isChecked());
}

void QCstmConfigMonitoring::on_ipLineEdit_editingFinished() {
	_mpx3gui->getConfig()->setIpAddress(ui->ipLineEdit->text());
}

void QCstmConfigMonitoring::on_ColourModeCheckBox_toggled(bool checked) {
	_mpx3gui->clear_data();
	if(checked)
		_mpx3gui->resize(_mpx3gui->getDataset()->x()/2, _mpx3gui->getDataset()->y()/2);
	else
		_mpx3gui->resize(_mpx3gui->getDataset()->x()*2, _mpx3gui->getDataset()->y()*2);
}

void QCstmConfigMonitoring::on_stepperUseCalibCheckBox_toggled(bool checked) {

	// On turn on --> verify and use calib, on turn off --> stop using calibration
	if ( checked == true ) {

		QStandardItemModel * model = (QStandardItemModel *) ui->stepperCalibrationTableView->model();
		int columns = model->columnCount();
		if(columns < 2) return; // this table is expected to have pairs <pos, angle>

		int rows = model->rowCount();

		vector<pair<double, double> > vals;
		// Check how many pairs available
		for ( int row = 0 ; row < rows ; row++ ) {

			QStandardItem * itempos = model->item(row, 0);
			QStandardItem * itemangle = model->item(row, 1);
			if( !itempos || !itemangle ) continue; // meaning no data or not a full pair in this row

			QVariant datapos = itempos->data(Qt::DisplayRole);
			QVariant dataang = itemangle->data(Qt::DisplayRole);
			// See if these are good values
			if ( datapos.canConvert<double>() && dataang.canConvert<double>() ) {
				vals.push_back( make_pair( datapos.toDouble(), dataang.toDouble() ) );
			}

		}

		cout << "[STEP] " << vals.size() << " points available for calibration" << endl;

		//_stepper->SetStepAngleCalibration();

	} else {

	}

}

void QCstmConfigMonitoring::on_stepperMotorCheckBox_toggled(bool checked) {

	// if the handler hasn't been initialized
	if ( ! _stepper ) _stepper = new StepperMotorController;

	// On turn on --> setup, on turn off --> close
	if ( checked == true ) {

		if ( ! _stepper->arm_stepper( ) ) return; // problems attaching

		// make stuff needed active
		activeInGUI();

		// Block the maximum and minimum number of motors
		ui->motorIdSpinBox->setMinimum( 0 );
		ui->motorIdSpinBox->setMaximum( _stepper->getNumMotors() - 1 );
		QString motorIdS = "supported motors: ";
		motorIdS += QString::number( _stepper->getNumMotors() , 'd', 0 );
		ui->motorIdSpinBox->setToolTip( motorIdS );

		// Get parameters to propagate to GUI
		map<int, motorPars> parsMap = _stepper->getPars( );

		int motorid = ui->motorIdSpinBox->value();

		// Speed
		ui->speedSpinBox->setMinimum( parsMap[motorid].minVel );
		ui->speedSpinBox->setMaximum( parsMap[motorid].maxVel );
		ui->speedSpinBox->setValue( parsMap[motorid].vel );
		QString speedS = "Set velocity from ";
		speedS += QString::number( parsMap[motorid].minVel , 'f', 1 );
		speedS += " to ";
		speedS += QString::number( parsMap[motorid].maxVel , 'f', 1 );
		ui->speedSpinBox->setToolTip( speedS );

		// Acc
		ui->accelerationSpinBox->setMinimum( parsMap[motorid].minAcc );
		ui->accelerationSpinBox->setMaximum( parsMap[motorid].maxAcc );
		ui->accelerationSpinBox->setValue( parsMap[motorid].acc );
		QString accS = "Set acceleration from ";
		accS += QString::number( parsMap[motorid].minAcc , 'f', 1 );
		accS += " to ";
		accS += QString::number( parsMap[motorid].maxAcc , 'f', 1 );
		ui->accelerationSpinBox->setToolTip( accS );



	} else {

		_stepper->disarm_stepper();
		// make stuff needed inactive
		activeInGUI();

	}


}

void QCstmConfigMonitoring::on_motorResetButton_clicked() {

	// Disarm
	_stepper->disarm_stepper();
	delete _stepper;
	_stepper = 0x0;

	// And arm again
	on_stepperMotorCheckBox_toggled( true );

}


void QCstmConfigMonitoring::on_motorGoToTargetButton_clicked() {

	////////////////////////////////////////////////////////////////////////////
	// Update the data structure in StepperMotorController
	// 1) Target pos
	int motorId = ui->motorIdSpinBox->value();
	// fetch target pos and enter it in the structure
	long long int targetPos = (long long int) ui->targetPosSpinBox->value();
	_stepper->setTargetPos( motorId, targetPos );
	// If curr and target are the same there's nothing to do here
	if ( _stepper->getCurrPos( motorId ) == _stepper->getTargetPos( motorId ) ) return;

	////////////////////////////////////////////////////////////////////////////
	// 2) If a rotation is ready to be launched, send the command to the hardware
	_stepper->goToTarget( targetPos, motorId );

	// 3) and launch the thread which actualizes the GUI
	if ( ! _stepperThread ) _stepperThread = new ConfigStepperThread(_mpx3gui, ui, this);
	_stepperThread->start();


}

void QCstmConfigMonitoring::motorDialReleased() {

	// same behaviour as clicking on motorGoToTargetButton
	on_motorGoToTargetButton_clicked();

}

void QCstmConfigMonitoring::motorDialMoved(int val) {

	// Change the target pos
	ui->targetPosSpinBox->setValue( (double) val );

}

void QCstmConfigMonitoring::setAcceleration(double acc) {

	// Set the new acceleration
	int motorId = ui->motorIdSpinBox->value();
	_stepper->SetAcceleration( motorId, acc );

}

void QCstmConfigMonitoring::setSpeed(double speed) {

	// Set the new acceleration
	int motorId = ui->motorIdSpinBox->value();
	_stepper->SetSpeed(motorId, speed);

}


ConfigStepperThread::ConfigStepperThread(Mpx3GUI * mpx3gui, Ui::QCstmConfigMonitoring * ui, QCstmConfigMonitoring * stepperController) {

	_mpx3gui = mpx3gui;
	_ui = ui;
	_stepperController = stepperController;

}

/**
 * This Thread deals with the GUI refreshing while the engine reaches from Current position to Target position
 */
void ConfigStepperThread::run() {

	bool reachedTarget = false;
	int motorId = _ui->motorIdSpinBox->value();
	long long int curr_pos = 0;

	// Keep running until the stepper has reached the desired position
	while ( ! reachedTarget ) {

		// get current position
		curr_pos = _stepperController->getMotorController()->getCurrPos( motorId );

		// Update display
		QString posS;
		posS = QString::number( curr_pos , 'lld', 0 );
		_ui->motorCurrentPoslcdNumber->display( posS );

		// Update dial
		_ui->motorDial->setValue( curr_pos );

		// terminating condition
		if ( curr_pos ==  _stepperController->getMotorController()->getTargetPos( motorId ) ) reachedTarget = true;

	}

}

