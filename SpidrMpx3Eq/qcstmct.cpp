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

    // Do rest of UI init here like labels
    // Connect signals

    //connect(this, SIGNAL(sig_moveMotor), _mpx3gui->GetUI()->stepperMotorTab, SLOT(on_motorGoToTargetButton_clicked()));
}

QCstmCT::~QCstmCT()
{
    delete ui;
}

void QCstmCT::SetMpx3GUI(Mpx3GUI *p)
{
    _mpx3gui = p;
    setGradient(0);

    //TODO implement this
    /*
    connect( this, SIGNAL(start_takingData()), _mpx3gui->GetUI()->visualizationGL, SLOT(StartDataTaking()) );
    connect( ui->comboBox, SIGNAL(currentIndexChanged(QString)), _mpx3gui->GetUI()->visualizationGL, SLOT(on_layerSelector_activated(QString)) );
    */

}

void QCstmCT::setGradient(int index)
{
    ui->displayCT->setGradient( _mpx3gui->getGradient(index) );
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
    _mpx3gui->getStepperMotor()->on_motorGoToTargetButton_clicked();
}

void QCstmCT::update_timeGUI(int i, int numberOfProjections)
{
    double timeLeft = (((numberOfProjections+2) * ui->spinBox_ExposureTimePerPosition->value()) - (i*ui->spinBox_ExposureTimePerPosition->value()));
    // Fudge factor
    timeLeft *= 1.2;
    ui->label_timeLeft->setText( QString::number(timeLeft) + " s" );
    ui->progressBar->setValue( float(i) / float(numberOfProjections+1) * 100);
}

void QCstmCT::startCT()
{
    _stop = false;

    while (!_stop){
        double targetAngle = 0;
        float angleDelta = ui->spinBox_rotationAngle->value()/ui->spinBox_numberOfProjections->value();
        int numberOfProjections = ui->spinBox_numberOfProjections->value();
        ui->label_timeLeft->setText(QString::number((numberOfProjections+1) * ui->spinBox_ExposureTimePerPosition->value()));

        QElapsedTimer timer;
        timer.start();

        qDebug() << "[CT] --------------------------------------";
        qDebug() << "[CT] Starting CT function - stop and shoot.";
        // Get acquisition settings from other view

        // Get corrections from other view?

        // Initialise for measurement
        // Auto-save to ~/ASI-Medipix3RX-CT-measurements/<DATE TIME>

        qDebug() << "[CT] Rotate by a small angle increment: " << angleDelta << "°";
        qDebug() << "[CT] Take" << numberOfProjections+1 << "frames";
        qDebug() << "[CT] --------------------------------------";


        // Begin CT loop
        int i = 0;
        while (i < (numberOfProjections+1)) {
            // These numbers happen to work reliably
            setSpeed(32000);
            setAcceleration(500000);
            update_timeGUI(i, numberOfProjections);

            QString motorPositionStatus = _mpx3gui->getStepperMotor()->GetUI()->label_positionStatus->text();
            //
            if( motorPositionStatus == "Stopped" || motorPositionStatus == "..."){

                // Take frames
                // ---------------------------------------------------------------------------
                qDebug() << "[CT] STARTED DT";
                _mpx3gui->getVisualization()->StartDataTaking();
                // ---------------------------------------------------------------------------

                //qDebug() << "[CT] Sleeping for " << int(ui->spinBox_ExposureTimePerPosition->value());
                qDebug() << QDateTime::currentDateTimeUtc();
                //usleep(int(ui->spinBox_ExposureTimePerPosition->value()*1000*1000));

                // Correct image?

                // Save/send file?
                QString filename = "/home/navrit/CT/img_"+QString::number(i)+".tif";
                qDebug() << "[CT] Saving TIFF to: " << filename;
                _mpx3gui->getVisualization()->saveImage(filename);


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


                qDebug() << timer.elapsed();
                i++;

            } else {
                usleep(50000);
            }

            // Update UI

        } // End CT loop

        qDebug() << timer.elapsed();

        ui->progressBar->setValue(100);
        ui->label_timeLeft->setText( QString::number(0) );
        qDebug() << "[CT]                                    >> End CT <<";

        // Cleanup - finished CT. Get ready to start again.


        // Reset back to 0 ready for another measurement
        setSpeed(32000);
        resetMotor();

        ui->progressBar->setValue(0);

        _stop = true;
    }

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

void QCstmCT::resumeCT()
{
    qDebug() << "[CT] RESUMED";
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
