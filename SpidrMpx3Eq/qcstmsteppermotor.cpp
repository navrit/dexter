#include "qcstmsteppermotor.h"
#include "ui_qcstmsteppermotor.h"

#include "qstandarditemmodel.h"
#include "qtableview.h"
#include <QtWidgets>

#include "StepperMotorController.h"
#include "mpx3config.h"
#include "mpx3gui.h"
#include "ui_mpx3gui.h"

QCstmStepperMotor::QCstmStepperMotor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::QCstmStepperMotor)
{
    ui->setupUi(this);

    ui->motorDial->setNotchesVisible(true);

    _stepperThread = nullptr;
    _stepper = nullptr;

    // Set inactive whatever is needed to be inactive
    activeInGUI();

    QStandardItemModel *tvmodel = new QStandardItemModel(3, 2, this); // 3 Rows and 2 Columns
    tvmodel->setHorizontalHeaderItem(0, new QStandardItem(QString("pos")));
    tvmodel->setHorizontalHeaderItem(1, new QStandardItem(QString("angle")));

    ui->stepperCalibrationTableView->setModel(tvmodel);
}

QCstmStepperMotor::~QCstmStepperMotor()
{
    if (_stepperThread)
        delete _stepperThread;
    delete ui;
}

void QCstmStepperMotor::SetMpx3GUI(Mpx3GUI *p)
{
    _mpx3gui = p;

    Mpx3Config *config = _mpx3gui->getConfig();

    // Stepper Motor Controller
    _stepper = nullptr;
    connect(ui->stepperUseCalibCheckBox,
            SIGNAL(clicked(bool)),
            config,
            SLOT(setStepperConfigUseCalib(bool)));
    connect(config,
            SIGNAL(UseCalibChanged(bool)),
            ui->stepperUseCalibCheckBox,
            SLOT(setChecked(bool)));

    connect(ui->accelerationSpinBox,
            SIGNAL(valueChanged(double)),
            config,
            SLOT(setStepperConfigAcceleration(double)));
    connect(config,
            SIGNAL(AccelerationChanged(double)),
            ui->accelerationSpinBox,
            SLOT(setValue(double)));

    connect(ui->speedSpinBox,
            SIGNAL(valueChanged(double)),
            config,
            SLOT(setStepperConfigSpeed(double)));
    connect(config, SIGNAL(SpeedChanged(double)), ui->speedSpinBox, SLOT(setValue(double)));

    // When the slider released, talk to the hardware
    QObject::connect(ui->motorDial, SIGNAL(sliderReleased()), this, SLOT(motorDialReleased()));
    QObject::connect(ui->motorDial, SIGNAL(sliderMoved(int)), this, SLOT(motorDialMoved(int)));

    connect(this,
            SIGNAL(sig_motorsConnected()),
            _mpx3gui->GetUI()->ctTab,
            SLOT(slot_connectedToMotors()));
}

//! This could be removed
void QCstmStepperMotor::setWindowWidgetsStatus(win_status s)
{
    switch (s) {
    case win_status::startup:
        break;

    case win_status::connected:
        break;

    default:
        break;
    }
}

void QCstmStepperMotor::ConnectionStatusChanged(bool conn)
{
    // Widgets status
    if (conn) {
        setWindowWidgetsStatus(win_status::connected);
    } else {
        setWindowWidgetsStatus(win_status::disconnected);
    }
}

void QCstmStepperMotor::activeInGUI()
{
    // stepper part
    if (_stepper) {
        if (!_stepper->isStepperReady()) {
            deactivateItemsGUI();
        } else {
            activateItemsGUI();
        }
    } else {
        deactivateItemsGUI();
    }
}

void QCstmStepperMotor::activateItemsGUI()
{
    emit sig_statusBarAppend(tr("Connected to motor"), "green");
    emit sig_motorsConnected();
    ui->motorDial->setEnabled(true);
    ui->motorGoToTargetButton->setEnabled(true);
    ui->motorResetButton->setEnabled(true);
    ui->motorTestButton->setEnabled(true);
    ui->stepperCalibrationTableView->setEnabled(true);
    ui->stepperUseCalibCheckBox->setEnabled(true);
    ui->accelerationSpinBox->setEnabled(true);
    ui->speedSpinBox->setEnabled(true);
    ui->targetPosSpinBox->setEnabled(true);
    ui->currentISpinBox->setEnabled(true);
    ui->motorIdSpinBox->setEnabled(true);
    ui->stepperSetZeroPushButton->setEnabled(true);
}

void QCstmStepperMotor::deactivateItemsGUI()
{
    emit sig_statusBarAppend(tr("Disconnected from motor"), "black");
    ui->motorDial->setDisabled(true);
    ui->motorGoToTargetButton->setDisabled(true);
    ui->motorResetButton->setDisabled(true);
    ui->motorTestButton->setDisabled(true);
    ui->stepperCalibrationTableView->setDisabled(true);
    ui->stepperUseCalibCheckBox->setDisabled(true);
    ui->accelerationSpinBox->setDisabled(true);
    ui->speedSpinBox->setDisabled(true);
    ui->targetPosSpinBox->setDisabled(true);
    ui->currentISpinBox->setDisabled(true);
    ui->stepperSetZeroPushButton->setDisabled(true);

    ui->motorIdSpinBox->setEnabled(true);
}

void QCstmStepperMotor::angleModeGUI()
{
    ui->stepperTargetPosLabel->setText("Target angle:");
    ui->stepperCurrentPosLabel->setText("Current angle:");
}

void QCstmStepperMotor::stepsModeGUI()
{
    ui->stepperTargetPosLabel->setText("Target pos:");
    ui->stepperCurrentPosLabel->setText("Current pos:");
}

void QCstmStepperMotor::on_stepperMotorCheckBox_toggled(bool checked)
{
    // if the handler hasn't been initialized
    if (!_stepper) {
        QMessageBox::information(this,
                                 tr("Starting search for stepper motor"),
                                 tr("This should take less than 10s"));
        _stepper = new StepperMotorController;
    }

    // Make the table unsensitive with respect to the config for a moment.
    // This reconnects at the end of this function.
    // This line deals with all elements in the calibration table :)
    disconnect(ui->stepperCalibrationTableView->model(),
               SIGNAL(itemChanged(QStandardItem *)),
               _mpx3gui->getConfig(),
               SLOT(setStepperConfigCalib(QStandardItem *)));
    disconnect(ui->accelerationSpinBox,
               SIGNAL(valueChanged(double)),
               this,
               SLOT(setAcceleration(double)));
    disconnect(ui->speedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeed(double)));
    disconnect(ui->currentISpinBox,
               SIGNAL(valueChanged(double)),
               this,
               SLOT(setCurrentILimit(double)));

    // the missing boolean (ui->stepperUseCalibCheckBox) is managed by an implicit
    // slot (on_stepperUseCalibCheckBox_toggled)

    // On turn on --> setup, on turn off --> close
    if (checked) {
        if (!_stepper->arm_stepper()) {
            ui->stepperMotorCheckBox->setChecked(false);

            QMessageBox::warning(this,
                                 tr("Connect stepper motor"),
                                 tr("Could not find stepper motor, check your connections."));

            return; // problems attaching
        }

        // make stuff needed active
        activeInGUI();

        // Block the maximum and minimum number of motors
        ui->motorIdSpinBox->setMinimum(0);
        ui->motorIdSpinBox->setMaximum(_stepper->getNumMotors() - 1);
        QString motorIdS = "Supported motors: ";
        motorIdS += QString::number(_stepper->getNumMotors(), 'd', 0);
        ui->motorIdSpinBox->setToolTip(motorIdS);

        // Get parameters to propagate to GUI
        map<int, motorPars> parsMap = _stepper->getPars();

        int motorid = int(ui->motorIdSpinBox->value());

        // Speed
        ui->speedSpinBox->setMinimum(parsMap[motorid].minVel);
        ui->speedSpinBox->setMaximum(parsMap[motorid].maxVel);
        ui->speedSpinBox->setValue(parsMap[motorid].maxVel * 0.08); // Could be .vel
        QString speedS = "Set velocity from ";
        speedS += QString::number(parsMap[motorid].minVel, 'f', 1);
        speedS += " to ";
        speedS += QString::number(parsMap[motorid].maxVel, 'f', 1);
        ui->speedSpinBox->setToolTip(speedS);

        // Acc
        ui->accelerationSpinBox->setMinimum(parsMap[motorid].minAcc);
        ui->accelerationSpinBox->setMaximum(parsMap[motorid].maxAcc);
        ui->accelerationSpinBox->setValue(parsMap[motorid].maxAcc * 0.002); // Could be .acc
        QString accS = "Set acceleration from ";
        accS += QString::number(parsMap[motorid].minAcc, 'f', 1);
        accS += " to ";
        accS += QString::number(parsMap[motorid].maxAcc, 'f', 1);
        ui->accelerationSpinBox->setToolTip(accS);

        // CurrentI limit
        ui->currentISpinBox->setMinimum(0);
        ui->currentISpinBox->setMaximum(1.0); // is this Amps ?
        ui->currentISpinBox->setValue(1.0);
        QString clS = "Set current limit from ";
        clS += QString::number(0, 'f', 1);
        clS += " to ";
        clS += QString::number(1.0, 'f', 1);
        ui->currentISpinBox->setToolTip(clS);

        // From config
        // ////////////////////////////////////////////////////////////////// The
        // rest of the values should have been read from the configuration at
        // startup
        double configAcc = _mpx3gui->getConfig()->getStepperAcceleration();
        if (configAcc >= parsMap[motorid].minAcc && configAcc <= parsMap[motorid].maxAcc) {
            ui->accelerationSpinBox->setValue(configAcc);
            setAcceleration(configAcc);
        }
        double configSpeed = _mpx3gui->getConfig()->getStepperSpeed();
        if (configSpeed >= parsMap[motorid].minVel && configSpeed <= parsMap[motorid].maxVel) {
            ui->speedSpinBox->setValue(configSpeed);
            setSpeed(configSpeed);
        }

        QStandardItemModel *model = (QStandardItemModel *) ui->stepperCalibrationTableView->model();
        QModelIndex index = model->index(0, 0);
        double t1 = _mpx3gui->getConfig()->getStepperCalibPos0();
        QVariant var0(t1);
        model->setData(index, var0);

        index = model->index(0, 1);
        QVariant var1(_mpx3gui->getConfig()->getStepperCalibAngle0());
        model->setData(index, var1);

        index = model->index(1, 0);
        QVariant var2(_mpx3gui->getConfig()->getStepperCalibPos1());
        model->setData(index, var2);

        index = model->index(1, 1);
        QVariant var3(_mpx3gui->getConfig()->getStepperCalibAngle1());
        model->setData(index, var3);

        if (_mpx3gui->getConfig()->getStepperUseCalib()) {
            ui->stepperUseCalibCheckBox->setChecked(true);
            // and call the corresponding actions as if the user had toggled it
            on_stepperUseCalibCheckBox_toggled(true);
        }

    } else {
        _stepper->disarm_stepper();
        // make stuff needed inactive
        activeInGUI();
    }

    // This line deals with all elements in the calibration table :)
    connect(ui->stepperCalibrationTableView->model(),
            SIGNAL(itemChanged(QStandardItem *)),
            _mpx3gui->getConfig(),
            SLOT(setStepperConfigCalib(QStandardItem *)));
    connect(ui->accelerationSpinBox,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(setAcceleration(double)));
    connect(ui->speedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setSpeed(double)));
    connect(ui->currentISpinBox, SIGNAL(valueChanged(double)), this, SLOT(setCurrentILimit(double)));

    // the missing boolean (ui->stepperUseCalibCheckBox) is managed by an implicit
    // slot (on_stepperUseCalibCheckBox_toggled)
}

void QCstmStepperMotor::on_stepperUseCalibCheckBox_toggled(bool checked)
{
    // On turn on --> verify and use calib, on turn off --> stop using calibration
    if (checked) {
        QStandardItemModel *model = (QStandardItemModel *) ui->stepperCalibrationTableView->model();
        int columns = model->columnCount();
        if (columns < 2)
            return; // this table is expected to have pairs <pos, angle>

        int rows = model->rowCount();

        vector<pair<double, double>> vals;
        // Check how many pairs available
        for (int row = 0; row < rows; row++) {
            QStandardItem *itempos = model->item(row, 0);
            QStandardItem *itemangle = model->item(row, 1);
            if (!itempos || !itemangle)
                continue; // meaning no data or not a full pair in this row

            QVariant datapos = itempos->data(Qt::DisplayRole);
            QVariant dataang = itemangle->data(Qt::DisplayRole);

            // See if these can be converted to QString
            if (datapos.canConvert<QString>() && dataang.canConvert<QString>()) {
                // Now check if the vlues are alpha numeric
                QString posS = datapos.toString();
                QString angS = dataang.toString();

                bool posok = false;
                double pos = posS.toDouble(&posok);
                bool angok = false;
                double ang = angS.toDouble(&angok);

                // push them if conversion is ok
                if (posok && angok)
                    vals.push_back(make_pair(pos, ang));
            }
        }

        // If calibration OK
        if (_stepper->SetStepAngleCalibration(int(ui->motorIdSpinBox->value()), vals)) {
            cout << "[STEP] Calibrated with " << vals.size() << " points." << endl;
        } else {
            // User message
            string messg = "Insufficient points to calibrate or incorrect format.";
            QMessageBox::warning(this, tr("Can't calibrate stepper"), tr(messg.c_str()));
            // force uncheck
            ui->stepperUseCalibCheckBox->setChecked(false);
            // and get out
            return;
        }

        // Now prepare the interface
        angleModeGUI();

        // In the angle space things may not be matching.
        // For instance step --> 3, means an angle of 2.7 (for 0.9 deg per 1 step
        // resolution) Make it match.
        int motorId = int(ui->motorIdSpinBox->value());
        double targetPos = ui->targetPosSpinBox->value();
        double targetAng = _stepper->StepToAngle(motorId, targetPos);
        if (!qFuzzyCompare(targetPos, targetAng)) {
            ui->targetPosSpinBox->setValue(targetAng);
        }
        QString posS;
        posS = QString::number(targetAng, 'f', 1);
        ui->motorCurrentPoslcdNumber->display(posS);
        // If the wasn't a match between target and current I make it match here.
        // Go green.
        ui->motorCurrentPoslcdNumber->setPalette(Qt::green);
        ui->motorCurrentPoslcdNumber->setToolTip("Target reached.");

        // Do something about the range of the dial
        // With calibration we're talking about an angle.
        // Go from 0 to 360 deg
        ui->motorDial->setMinimum(0);
        ui->motorDial->setMaximum(360);

    } else {
        // Interface back to steps mode
        stepsModeGUI();

        // In the step space things may not be matching either.
        // Same situation as immediately above
        // Make it match.
        int motorId = int(ui->motorIdSpinBox->value());
        // In this case we inquiry the current angle
        double currentPos = _stepper->getCurrPos(motorId);
        // double currentAng = _stepper->StepToAngle(motorId, currentPos);
        // if ( currentPos !=  currentAng ) {
        ui->targetPosSpinBox->setValue(currentPos);
        //}
        QString posS = QString::number(int(currentPos));
        ui->motorCurrentPoslcdNumber->display(posS);

        // Do something about the range of the dial
        // int motorid = ui->motorIdSpinBox->value();
        map<int, motorPars> parsMap = _stepper->getPars();
        // The minimum is normally -1*max. I will set zero here for convenience.
        ui->motorDial->setMinimum(0);
        // This is just an strategy to get a maximum of steps
        //  which makes almost a full turn.
        // _stepper->getPositionMax( motorid )
        ui->motorDial->setMaximum(int(parsMap[motorId].maxVel / 2.));

        // cout << "[STEP] pos min = "
        //		<< _stepper->getPositionMin( motorid )
        //		<< " | pos max = "
        //		<< _stepper->getPositionMax( motorid )
        //		<< endl;
    }
    // send to config
    _mpx3gui->getConfig()->setStepperConfigUseCalib(checked);
}

void QCstmStepperMotor::on_motorGoToTargetButton_clicked()
{
    ////////////////////////////////////////////////////////////////////////////
    // Update the data structure in StepperMotorController
    // 1) Target pos
    int motorId = int(ui->motorIdSpinBox->value());
    // fetch target pos and enter it in the structure
    double targetPos = ui->targetPosSpinBox->value();

    // Look if we are working calibrated or uncalibrated
    if (ui->stepperUseCalibCheckBox->isChecked()) {
        targetPos = _stepper->AngleToStep(motorId, (double) targetPos);
    }
    _stepper->setTargetPos(motorId, targetPos);

    // If curr and target are the same there's nothing to do here
    if (_stepper->getCurrPos(motorId) == _stepper->getTargetPos(motorId))
        return;

    ////////////////////////////////////////////////////////////////////////////
    // 2) If a rotation is ready to be launched, send the command to the hardware
    // If in angle mode the entry of the user will be interpreted as an angle

    // 3) and launch the thread
    if (!_stepperThread) {
        _stepperThread = new ConfigStepperThread(_mpx3gui, ui, this);
        connect(_stepperThread, SIGNAL(finished()), this, SLOT(stepperGotoTargetFinished()));
        connect(_stepperThread,
                SIGNAL(finished()),
                _mpx3gui->getCT(),
                SLOT(slot_motorReachedTarget()));
    }

    _stepperThread->start();

    _stepper->goToTarget((long long int) targetPos, motorId);
}

void QCstmStepperMotor::on_motorResetButton_clicked()
{
    // Disarm
    _stepper->disarm_stepper();
    delete _stepper;
    _stepper = 0x0;

    // And arm again
    on_stepperMotorCheckBox_toggled(true);
}

/*
void QCstmStepperMotor::ConfigCalibAngle1Changed(double val) {
    QStandardItemModel * model = (QStandardItemModel *)
ui->stepperCalibrationTableView->model(); QModelIndex index = model->index(1,1);
    QVariant var(val);
    model->setData( index, var );
}
 */

void QCstmStepperMotor::on_stepperSetZeroPushButton_clicked()
{
    // Current position is selected as zero

    // Reset the controller
    int motorId = int(ui->motorIdSpinBox->value());
    _stepper->setZero(motorId);

    // Reset displays
    ui->targetPosSpinBox->setValue(0.0);
    QString posS;
    if (ui->stepperUseCalibCheckBox->isChecked()) {
        posS = QString::number(0.0, 'f', 1);
        ui->motorCurrentPoslcdNumber->display(posS);
    } else {
        posS = QString::number(0);
        ui->motorCurrentPoslcdNumber->display(posS);
    }

    ui->motorDial->setValue(0);
}

void QCstmStepperMotor::motorDialReleased()
{
    // same behaviour as clicking on motorGoToTargetButton
    on_motorGoToTargetButton_clicked();
}

void QCstmStepperMotor::motorDialMoved(int val)
{
    // Change the target pos
    ui->targetPosSpinBox->setValue((double) val);
}

void QCstmStepperMotor::setAcceleration(double acc)
{
    // avoid to set if the device is not connected
    if (!_stepper)
        return;

    // Set the new acceleration
    int motorId = int(ui->motorIdSpinBox->value());
    _stepper->SetAcceleration(motorId, acc);

    // send to config
    _mpx3gui->getConfig()->setStepperConfigAcceleration(acc);
}

void QCstmStepperMotor::setSpeed(double speed)
{
    // avoid to set if the device is not connected
    if (!_stepper)
        return;

    // Set the new acceleration
    int motorId = int(ui->motorIdSpinBox->value());
    _stepper->SetSpeed(motorId, speed);

    // send to config
    _mpx3gui->getConfig()->setStepperConfigSpeed(speed);
}

void QCstmStepperMotor::setCurrentILimit(double limitI)
{
    // avoid to set if the device is not connected
    if (!_stepper)
        return;

    // Set the new CurrentI
    int motorId = int(ui->motorIdSpinBox->value());
    _stepper->SetCurrentILimit(motorId, limitI);

    // send to config
    //_mpx3gui->getConfig()->
}

void QCstmStepperMotor::on_motorTestButton_clicked()
{
    /*! Follow a sequeance of angles and back-to-zero to test.
   Every time on_motorGoToTargetButton_clicked is called,
    which launches the StepperThread. Then when the thread
    is finished and sends its signal we can continue
    checking if the sequence has been exhausted.
  */

    // Fill in a sequance
    m_stepperTestSequence.clear();

    /*
  m_stepperTestSequence.push_back( 45.0 );
  for ( int i = 1 ; i < 9 ; i++ ) {
      m_stepperTestSequence.push_back( 0.0 );           // a return to zero
      m_stepperTestSequence.push_back( 45.0 * i );    // and some test angle
  }
  m_stepperTestSequence.push_back( 0 ); // and come back to zero
  */

    m_stepperTestSequence.push_back(-15.0);
    m_stepperTestSequence.push_back(15.0);

    m_stepperTestSequence.push_back(0); // and come back to zero

    m_stepperTestCurrentStep = 0;

    /////////////////////////////////////////////////////////////////////////
    int motorId = int(ui->motorIdSpinBox->value());
    double targetPos = m_stepperTestSequence[m_stepperTestCurrentStep++];

    // by hand set the value
    ui->targetPosSpinBox->setValue(targetPos);

    // Look if we are working calibrated or uncalibrated
    if (ui->stepperUseCalibCheckBox->isChecked()) {
        targetPos = _stepper->AngleToStep(motorId, (double) targetPos);
    }
    _stepper->setTargetPos(motorId, targetPos);

    // And launch the thread
    if (!_stepperThread) {
        _stepperThread = new ConfigStepperThread(_mpx3gui, ui, this);
        connect(_stepperThread, SIGNAL(finished()), this, SLOT(stepperGotoTargetFinished()));
        connect(_stepperThread,
                SIGNAL(finished()),
                _mpx3gui->getCT(),
                SLOT(slot_motorReachedTarget()));
    }

    // go !
    _stepperThread->start();
    _stepper->goToTarget((long long int) targetPos, motorId);
}

void QCstmStepperMotor::stepperGotoTargetFinished()
{
    // See if this is a sequence
    if (m_stepperTestCurrentStep != 0 && m_stepperTestCurrentStep < m_stepperTestSequence.size()) {
        qDebug() << m_stepperTestCurrentStep << " : " << m_stepperTestSequence.size();

        /////////////////////////////////////////////////////////////////////////
        int motorId = int(ui->motorIdSpinBox->value());
        double targetPos = m_stepperTestSequence[m_stepperTestCurrentStep++];

        // by hand set the value
        ui->targetPosSpinBox->setValue(targetPos);

        // Look if we are working calibrated or uncalibrated
        if (ui->stepperUseCalibCheckBox->isChecked()) {
            targetPos = _stepper->AngleToStep(motorId, (double) targetPos);
        }
        _stepper->setTargetPos(motorId, targetPos);

        // And launch the thread
        if (!_stepperThread) {
            _stepperThread = new ConfigStepperThread(_mpx3gui, ui, this);
            connect(_stepperThread, SIGNAL(finished()), this, SLOT(stepperGotoTargetFinished()));
            connect(_stepperThread,
                    SIGNAL(finished()),
                    _mpx3gui->getCT(),
                    SLOT(slot_motorReachedTarget()));
        }

        // go !
        _stepperThread->start();
        _stepper->goToTarget((long long int) targetPos, motorId);

    } else {
        // ready for next test sequence if needed
        m_stepperTestCurrentStep = 0;
    }
}

ConfigStepperThread::ConfigStepperThread(Mpx3GUI *mpx3gui,
                                         Ui::QCstmStepperMotor *ui,
                                         QCstmStepperMotor *stepperController)
{
    _mpx3gui = mpx3gui;
    _ui = ui;
    _stepperController = stepperController;
}

/**
 * This Thread deals with the GUI refreshing while the engine reaches from
 * Current position to Target position
 */
void ConfigStepperThread::run()
{
    reachedTarget = false;
    int motorId = int(_ui->motorIdSpinBox->value());

    _ui->motorCurrentPoslcdNumber->setPalette(Qt::blue);

    // Keep running until the stepper has reached the desired position
    double posDisplay;
    while (!reachedTarget) {
        long long int curr_pos = 0;
        _ui->label_positionStatus->setText(tr("Moving"));

        // get current position
        curr_pos = _stepperController->getMotorController()->getCurrPos(motorId);

        // terminating condition good for both calibrated and uncalibrated
        if (curr_pos == _stepperController->getMotorController()->getTargetPos(motorId))
            reachedTarget = true;

        posDisplay = (double) curr_pos;

        QString posS;
        if (_ui->stepperUseCalibCheckBox->isChecked()) {
            posDisplay = _stepperController->getMotorController()->StepToAngle(motorId,
                                                                               (double) curr_pos);
            // Update display
            posS = QString::number(posDisplay, 'f', 1);
            _ui->motorCurrentPoslcdNumber->display(posS);

        } else {
            // Update display
            posS = QString::number(curr_pos);
            _ui->motorCurrentPoslcdNumber->display(posS);
        }

        // Update dial
        _ui->motorDial->setValue((int) posDisplay);
    }

    _ui->label_positionStatus->setText(tr("Stopped"));

    /** At the end the following may happen
   * Say the user request and angle of 3 degrees.  With a given resolution (ex.
   * 0.9 deg per step) the best the stepper can do is 2.7 degrees.  In that case
   * color the LCD
   */
    if (qFuzzyCompare(posDisplay, _ui->targetPosSpinBox->value())) {
        _ui->motorCurrentPoslcdNumber->setPalette(Qt::green);
        _ui->motorCurrentPoslcdNumber->setToolTip("Target reached.");
    } else {
        _ui->motorCurrentPoslcdNumber->setPalette(Qt::yellow);
        _ui->motorCurrentPoslcdNumber->setToolTip("Requested target couldn't be reached exactly.");
    }

    // qDebug() << "Stepper motor ConfigStepperThread done";
}
