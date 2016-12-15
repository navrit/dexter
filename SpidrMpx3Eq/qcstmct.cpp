#include "qcstmct.h"
#include "ui_qcstmct.h"

#include <QThread>


#include "StepperMotorController.h"

QCstmCT::QCstmCT(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::QCstmCT)
{
    ui->setupUi(this);
    // Do rest of UI init here like labels
    // Connect signals
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
    connect(this,SIGNAL(sig_connectToMotors(bool)), _mpx3gui->getStepperMotor(), SLOT(on_stepperMotorCheckBox_toggled(bool)));

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
    qDebug() << ">> Starting CT function - stop and shoot.";
    // Get acquisition settings from other view

    // Get corrections from other view?

    // Initialise for measurement
    // Auto-save to ~/CT- <DATE TIME>

    int numberOfProjections = ui->spinBox_numberOfProjections->value();

    // Begin CT loop
    for (int i = 0; i < numberOfProjections; i++){
        // Rotate small angle
        qDebug() << "Rotate small angle";
        emit sig_connectToMotors(true);

        // Take some frames



        // Correct image?


        // Save File


        // Update UI

    } // End CT loop

    qDebug() << "> End CT <";

    // Cleanup - finished CT. Get ready to start again.


}

void QCstmCT::stopCT()
{
    qDebug() << ">> GUI Interrupt: Stopping CT function.";


    // Cleanup

}

void QCstmCT::on_CTPushButton_clicked()
{

    if ( ui->CTPushButton->text() == "Connect to motors" ){
        // Connect to motors
        qDebug() << "Connect to motors if not connected";

        // Update UI for next click - occurs once
        ui->CTPushButton->setText("Start CT");

        //CTPushButton_state = 1;
    } else if ( ui->CTPushButton->text() == "Start CT" ){
        // Start CT
        //CTPushButton_state = 2;

        // Update UI
        ui->CTPushButton->setText("Stop CT");

        startCT();

        // Update UI
        ui->CTPushButton->setText("Start CT");

    } else if ( ui->CTPushButton->text() == "Stop CT" ) {
        // Stop CT
        //CTPushButton_state = 1;

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
