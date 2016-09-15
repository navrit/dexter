#include "qcstmStepperMotor.h"
#include "ui_qcstmStepperMotor.h"

qcstmStepperMotor::qcstmStepperMotor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::qcstmStepperMotor)
{
    ui->setupUi(this);
}

qcstmStepperMotor::~qcstmStepperMotor()
{
    delete ui;
}
