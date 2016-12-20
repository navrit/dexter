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
    qDebug() << "Resetting motor to position 0";

}

void QCstmCT::startCT()
{
    _stop = false;

    while (!_stop){
        qDebug() << ">> Starting CT function - stop and shoot.";
        // Get acquisition settings from other view

        // Get corrections from other view?

        // Initialise for measurement
        // Auto-save to ~/ASI-Medipix3RX-CT-measurements/<DATE TIME>

        double targetAngle = 0;
        double currentAngle = 0;
        int numberOfProjections = ui->spinBox_numberOfProjections->value();
        float angleDelta = ui->spinBox_rotationAngle->value()/ui->spinBox_numberOfProjections->value();

        qDebug() << "Rotate by a small angle increment: " << angleDelta;
        qDebug() << "Take " << ui->spinBox_numberOfProjections->value()+1 << "frames";

        // Begin CT loop
        for (int i = 0; i < numberOfProjections; i++) {
            _mpx3gui->getStepperMotor()->GetUI()->speedSpinBox->setValue(16384);
            _mpx3gui->getStepperMotor()->GetUI()->accelerationSpinBox->setValue(102000);

            // When it's at the target angle (within 1%), wait, rotate, update
            QString tooltip = _mpx3gui->getStepperMotor()->GetUI()->motorCurrentPoslcdNumber->toolTip();
            if( tooltip == "Requested target couldn't be reached exactly." || tooltip == "Target reached."){
                // Take some frames
                qDebug() << "Sleeping for " << int(ui->spinBox_ExposureTimePerPosition->value());
                usleep(1000000 * int(ui->spinBox_ExposureTimePerPosition->value()));

                // Correct image?


                // Save File

                // Rotate by a small angle
                angleDelta = ui->spinBox_rotationAngle->value()/ui->spinBox_numberOfProjections->value();

                // Update stepper motor UI
                _mpx3gui->getStepperMotor()->GetUI()->targetPosSpinBox->setValue(targetAngle + angleDelta);
                // Move the motor
                _mpx3gui->getStepperMotor()->on_motorGoToTargetButton_clicked();

                // Update target angle to match new rotation state.
                targetAngle += angleDelta;

                qDebug() << "Target angle: " << targetAngle;
            }

//            if ((targetAngle*0.99 <= currentAngle && currentAngle <= targetAngle*1.01) || (targetAngle == currentAngle)){

//            }

            // Update UI

        } // End CT loop

        qDebug() << "> End CT <";

        // Cleanup - finished CT. Get ready to start again.


        // Reset back to 0 ready for another measurement
        // Update stepper motor UI
        _mpx3gui->getStepperMotor()->GetUI()->targetPosSpinBox->setValue(0);
        _mpx3gui->getStepperMotor()->GetUI()->speedSpinBox->setValue(32768);
        // Move the motor
        _mpx3gui->getStepperMotor()->on_motorGoToTargetButton_clicked();

        _stop = true;
    }

}

void QCstmCT::stopCT()
{
    qDebug() << ">> GUI Interrupt: Stopping CT function.";


    // Cleanup

}

void QCstmCT::slot_connectedToMotors()
{
    activeMotors = _mpx3gui->getStepperMotor()->GetUI()->stepperMotorCheckBox->isChecked();
    // Update UI
    ui->CTPushButton->setText("Start CT");
}

void QCstmCT::on_CTPushButton_clicked()
{
    activeMotors = _mpx3gui->getStepperMotor()->GetUI()->stepperMotorCheckBox->isChecked();

    if ( ui->CTPushButton->text() == "Connect to motors" ){
        // Connect to motors
        qDebug() << "Connect to motors";

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
        qDebug() << "---------------------\n CT MASSIVE ERROR \n-----------------------\n";
        return;
    }
}
