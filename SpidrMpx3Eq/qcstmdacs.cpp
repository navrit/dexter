#include "qcstmdacs.h"
#include "ui_qcstmdacs.h"

QCstmDacs::QCstmDacs(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::QCstmDacs)
{
  ui->setupUi(this);
}

QCstmDacs::~QCstmDacs()
{
  delete ui;
}
