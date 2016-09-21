#include "qcstmct.h"
#include "ui_qcstmct.h"

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
	setGradient(0);
}

void QCstmCT::setGradient(int index)
{
	ui->displayCT->setGradient( _mpx3gui->getGradient(index) );
}

void QCstmCT::rotatePushButton_clicked() {

	cout << "..." << endl;

}


