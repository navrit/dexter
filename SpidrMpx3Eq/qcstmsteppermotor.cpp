#include "qcstmsteppermotor.h"
#include "ui_qcstmsteppermotor.h"

QCstmStepperMotor::QCstmStepperMotor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmStepperMotor)
{
    ui->setupUi(this);
}

QCstmStepperMotor::~QCstmStepperMotor()
{
    delete ui;
}

void QCstmStepperMotor::SetMpx3GUI(Mpx3GUI *p)
{
    _mpx3gui = p;
}
