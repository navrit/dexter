#include "qcstmvoxeltab.h"
#include "ui_qcstmvoxeltab.h"

QCstmVoxeltab::QCstmVoxeltab(QWidget *parent) :  QWidget(parent),  ui(new Ui::QCstmVoxeltab)
{
  ui->setupUi(this);
}

QCstmVoxeltab::~QCstmVoxeltab()
{
  delete ui;
}
