#include "qcstmct.h"
#include "ui_qcstmct.h"

#include "ui_mpx3gui.h"
#include <QThread>


#include "StepperMotorController.h"

QCstmCT::QCstmCT(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::QCstmCT)
{
    ui->setupUi(this);

}

QCstmCT::~QCstmCT()
{
    delete ui;
}

void QCstmCT::SetMpx3GUI(Mpx3GUI *p)
{
    _mpx3gui = p;

}

void QCstmCT::resetMotor()
{
    qDebug() << "[CT] Resetting motor to position 0";

    // Update stepper motor UI to 0
    setTargetPosition(0);
    // Move the motor
    motor_goToTarget();
}

void QCstmCT::setAcceleration(double acceleration)
{
    _mpx3gui->getStepperMotor()->GetUI()->speedSpinBox->setValue(acceleration);
}

void QCstmCT::setSpeed(double speed)
{
    _mpx3gui->getStepperMotor()->GetUI()->speedSpinBox->setValue(speed);
}

void QCstmCT::setTargetPosition(double position)
{
    _mpx3gui->getStepperMotor()->GetUI()->targetPosSpinBox->setValue(position);
}

void QCstmCT::motor_goToTarget()
{
    isMotorMoving = true;
    _mpx3gui->getStepperMotor()->on_motorGoToTargetButton_clicked();
}

void QCstmCT::update_timeGUI()
{
    double timeLeft = (((numberOfProjections+1) * ui->spinBox_ExposureTimePerPosition->value()) - (iteration*ui->spinBox_ExposureTimePerPosition->value()));
    // Fudge factor
    timeLeft *= 1.2;
    ui->label_timeLeft->setText( QString::number(timeLeft) + " s" );
    ui->progressBar->setValue( float(iteration) / float(numberOfProjections) * 100);
}

QString QCstmCT::getMotorPositionStatus()
{
    return _mpx3gui->getStepperMotor()->GetUI()->label_positionStatus->text();
}

void QCstmCT::startDataTakingThread()
{
    if( getMotorPositionStatus() == "Stopped" || getMotorPositionStatus() == "..."){
        qDebug() << "[CT] STARTED DT @ " << QDateTime::currentDateTimeUtc();

        _mpx3gui->getVisualization()->StartDataTaking(true);

        //! Note: MUST end function here to return back to Qt event loop
    }
}

void QCstmCT::startCT()
{
    _stop = false;
    iteration = 0;
    targetAngle = 0;
    _mpx3gui->getDataset()->clear();

    angleDelta = ui->spinBox_rotationAngle->value()/ui->spinBox_numberOfProjections->value();
    numberOfProjections = ui->spinBox_numberOfProjections->value() + 1;

    ui->label_timeLeft->setText(QString::number((numberOfProjections) * ui->spinBox_ExposureTimePerPosition->value()));
    ui->progressBar->setValue(0);
    ui->CTPushButton->setText("Stop CT");

    qDebug() << "[CT] --------------------------------------";
    qDebug() << "[CT] Starting CT function - stop and shoot.";
    // Get acquisition settings from other view

    // Get corrections from other view?

    // Initialise for measurement
    // Auto-save to ~/ASI-Medipix3RX-CT-measurements/<DATE TIME>

    qDebug() << "[CT] Rotate by a small angle increment: " << angleDelta << "°";
    qDebug() << "[CT] Take" << numberOfProjections << "frames";
    qDebug() << "[CT] --------------------------------------";

    setSpeed(32768);
    resetMotor();
    // These numbers happen to work reliably
    setSpeed(32000);
    setAcceleration(500000);

    update_timeGUI();
    ui->CTPushButton->setText("Stop CT");

    startDataTakingThread();
}

void QCstmCT::stopCT()
{
    qDebug() << "[CT] GUI Interrupt: Stopping CT function.";
    _stop = true;

    // Cleanup

}

void QCstmCT::slot_connectedToMotors()
{
    activeMotors = _mpx3gui->getStepperMotor()->GetUI()->stepperMotorCheckBox->isChecked();
    // Update UI
    ui->CTPushButton->setText("Start CT");
}

void QCstmCT::slot_motorReachedTarget()
{
    isMotorMoving = false;

    iteration++;
    startDataTakingThread();
}

//! Most of the time will be spent in this function
void QCstmCT::resumeCT()
{
    if (_stop){
        return;
    }

    // Essentially a global for (i < numberOfProjections) loop
    if (iteration < numberOfProjections-1) {
        // Correct image?

        // Save/send file?
        QString filename = "/home/navrit/ownCloud/ASI/img_"+QString::number(iteration)+".tif";
        //qDebug() << "[CT] Saving TIFF:" << filename;
        _mpx3gui->getVisualization()->saveImage(filename);
        _mpx3gui->getDataset()->clear();


        // Rotate by a small angle
        // ---------------------------------------------------------------------------
        // Angular change per rotation
        angleDelta = ui->spinBox_rotationAngle->value()/ui->spinBox_numberOfProjections->value();

        // Update stepper motor UI
        setTargetPosition(targetAngle + angleDelta);

        // Update target angle to match new rotation state.
        targetAngle += angleDelta;

        // Move the motor
        motor_goToTarget();

        qDebug() << "[CT] Target angle: " << targetAngle;
        // ---------------------------------------------------------------------------



    } else {
        qDebug() << "[CT] ------------ End ------------";
        resetMotor();
        ui->CTPushButton->setText("Start CT");
        _stop = true;
        return;
    }

}

void QCstmCT::on_CTPushButton_clicked()
{
    activeMotors = _mpx3gui->getStepperMotor()->GetUI()->stepperMotorCheckBox->isChecked();

    if ( ui->CTPushButton->text() == "Connect to motors" ){
        // Connect to motors
        qDebug() << "[CT] Connect to motors";

        if ( activeMotors ){

            // Update UI for next click - occurs once
            ui->CTPushButton->setText("Start CT");
        } else {
            // Inactivate motors. Switch to motor view
            _mpx3gui->GetUI()->stackedWidget->setCurrentIndex( _mpx3gui->getStepperMotorPageID() );
            return;
        }


    } else if ( ui->CTPushButton->text() == "Start CT" ){
        // Start CT

        // Update UI
        ui->CTPushButton->setText("Stop CT");

        startCT();

        // Update UI
        ui->CTPushButton->setText("Start CT");

    } else if ( ui->CTPushButton->text() == "Stop CT" ) {
        // Stop CT

        // Update UI
        ui->CTPushButton->setText("Start CT");

        stopCT();

        // Update UI
        ui->CTPushButton->setText("Stop CT");

    } else {
        qDebug() << "[CT] ---------------------\n WEIRD AF ERROR \n-----------------------\n";
        return;
    }
}
