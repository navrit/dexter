#include "qcstmct.h"
#include "ui_qcstmct.h"

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
