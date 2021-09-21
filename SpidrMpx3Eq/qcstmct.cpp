#include <QThread>
#include "qcstmct.h"
#include "ui_qcstmct.h"
#include "ui_mpx3gui.h"
#include "StepperMotorController.h"

const static int __max_phidgets_motor_value = 32768;

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

void QCstmCT::resetMotor()
{
    qDebug() << "[CT]\tResetting motor to position 0";

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

//void QCstmCT::applyCorrection(QString correctionMethod)
//{
//    if (correctionMethod == "None") {
//        return;
//    } else if (correctionMethod == "Open Beam"){
//        qDebug() << "[CT]\tCorrections disabled";

//        // Get an Open Beam image
//        qDebug() << "[CT]\tCorrection: Open Beam" << correctionFilename << endl;

//        _mpx3gui->getDataset()->removeCorrection();
//        QFile OBfile(correctionFilename);
//        if ( !OBfile.open(QIODevice::ReadOnly) ) {
//            string messg = "Couldn't open: ";
//            messg += correctionFilename.toStdString();
//            messg += "\nNo output written!";
//            QMessageBox::warning ( this, tr("Error opening data"), tr( messg.c_str() ) );
//            return;
//        }

//        _mpx3gui->getDataset()->loadCorrection(OBfile.readAll());
//        _mpx3gui->getDataset()->applyOBCorrection();

//    } else if (correctionMethod == "Beam Hardening"){
//        qDebug() << "[CT]\tCorrections disabled";

//        // Get a Beam hardening JSON
//        qDebug() << "[CT]\tCorrection: Beam Hardening";
//        emit doBHCorrection();

//    } else {
//        qDebug() << "[CT]\tCorrection: Unknown option?!?!?!";
//        return;
//    }
//}

//QString QCstmCT::getCorrectionFile()
//{
//    if (ui->correctionMethod->currentText() == "Open Beam"){
//        return QFileDialog::getOpenFileName(this, tr("Select Open Beam image"), ".", BIN_FILES );
//    } else if (ui->correctionMethod->currentText() == "Beam Hardening") {
//        return QFileDialog::getOpenFileName(this, tr("Select Beam Hardening JSON"), ".", JSON_FILES );
//    } else {
//        return "";
//    }
//}

QString QCstmCT::getMotorPositionStatus()
{
    return _mpx3gui->getStepperMotor()->GetUI()->label_positionStatus->text();
}

void QCstmCT::startDataTakingThread()
{
    const QString motorPositionStatus = getMotorPositionStatus();
    if (motorPositionStatus == "Stopped" || motorPositionStatus == "...") {

        if (_mpx3gui->getConfig()->isConnected() ) {
            if (_mpx3gui->getVisualization()->isTakingData()) {
                if (_mpx3gui->getConfig()->getNTriggers() == 0) {
                    qDebug() << "[CT]\tStopped data acquisition, tried to start a CT scan with inf running. Fix it.";
                    _mpx3gui->getVisualization()->StopDataTakingThread();
                    stopCT();
                }
                qDebug() << "[CT]\tData taking thread already running, will not try to start it again. Doing nothing.";
                return;
            }

            qDebug() << "[CT]\tStarted DataTaking @ " << QDateTime::currentDateTimeUtc();

            _mpx3gui->getVisualization()->StartDataTaking("CT");

            //! Note: MUST end function here to return back to Qt event loop

        } else {
            qDebug() << "[CT]\tNot connected to a detector, I guess you're just playing with the motors!";
        }
    }
}

void QCstmCT::startCT()
{
    if (_mpx3gui->getConfig()->isConnected()) {
        //! Use acquisition settings from other view
        _stop = false;
        iteration = 0;
        targetAngle = 0;
        _mpx3gui->getDataset()->clear();



        angleDelta = ui->spinBox_rotationAngle->value() /
                ui->spinBox_numberOfProjections->value();
        numberOfProjections = ui->spinBox_numberOfProjections->value() + 1;

        //! Get correction file
        // correctionFilename = getCorrectionFile();

        ui->label_timeLeft->setText(QString::number((numberOfProjections) *
                                                    ui->spinBox_ExposureTimePerPosition->value()));
        ui->progressBar->setValue(0);
        ui->CTPushButton->setText("Stop CT");

        qDebug() << "[CT]\t--------------------------------------";
        qDebug() << "[CT]\tStarting CT function - stop and shoot.";

        //! Initialise for measurement
        qDebug() << "[CT]\tRotate by a small angle increment: " << angleDelta << "°";
        qDebug() << "[CT]\tTake" << numberOfProjections << "frames";
        qDebug() << "[CT]\t--------------------------------------";

        setSpeed(__max_phidgets_motor_value);
        setAcceleration(__max_phidgets_motor_value);
        resetMotor();
        setSpeed(1500);

        update_timeGUI();
        ui->CTPushButton->setText("Stop CT");

        startDataTakingThread();

    } else {
        qDebug() << "[CT]\tNot connected to a detector, cannot run a CT scan...";
    }
}

void QCstmCT::stopCT()
{
    qDebug() << "[CT]\tGUI Interrupt: Stopping CT function.";
    _stop = true;

    // Cleanup
    ui->label_timeLeft->setText("GUI Interrupted at " +
                                QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
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
    if (_stop)
        return;

    // Essentially a global for (i < numberOfProjections) loop
    if (iteration < numberOfProjections-1) {
        // Correct image
        // QString corrMethod = ui->correctionMethod->currentText();
        // applyCorrection( corrMethod );

        // Save/send file?
        QString filename = CTfolder; // CT folder is currently hardcoded to the home directory.

//        if (corrMethod == "Beam Hardening"){
//            _mpx3gui->getVisualization()->saveImage(filename, corrMethod);
//        } else {
//
        _mpx3gui->getVisualization()->saveImage(filename);
//        }


        // Rotate by a small angle
        // ---------------------------------------------------------------------------
        // Angular change per rotation
        angleDelta = ui->spinBox_rotationAngle->value() / ui->spinBox_numberOfProjections->value();

        // Update target angle to match new rotation state.
        targetAngle += angleDelta;

        // Update stepper motor UI
        setTargetPosition(targetAngle);

        // Move the motor
        motor_goToTarget();

        update_timeGUI();

        qDebug() << "[CT]\tTarget angle: " << targetAngle;
        // ---------------------------------------------------------------------------

    } else {
        ui->label_timeLeft->setText(tr("Done"));
        ui->progressBar->setValue(100);

        resetMotor();

        ui->CTPushButton->setText("Start CT");
        _stop = true;

        if (!ui->textEdit->toPlainText().isEmpty()){
            QString filename = CTfolder +
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate) +
                    "_notes.txt";
            QFile saveNotes(filename);
            if (!saveNotes.open(QIODevice::WriteOnly)) {
                string messg = "Couldn't open: ";
                messg.append(filename.toStdString());
                messg.append("\nNo output written.");
                QMessageBox::warning ( this, tr("Error saving data"), tr(messg.c_str()) );
                return;
            }
            QTextStream stream(&saveNotes);
            stream << ui->textEdit->toPlainText();
            saveNotes.close();
        }

        qDebug() << "[CT]\t------------ End ------------";
        return;
    }
}

void QCstmCT::on_CTPushButton_clicked()
{
    activeMotors = _mpx3gui->getStepperMotor()->GetUI()->stepperMotorCheckBox->isChecked();

    if ( ui->CTPushButton->text() == "Connect to motors" ){
        // Connect to motors
        qDebug() << "[CT]\tConnect to motors";

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

    } else if ( ui->CTPushButton->text() == "Stop CT" ) {
        // Stop CT

        // Update UI
        ui->CTPushButton->setText("Start CT");

        stopCT();

    } else {
        qDebug() << "[CT]\t---------------------\n WEIRD AF ERROR \n-----------------------\n";
        return;
    }
}
