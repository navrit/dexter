#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"

QCstmConfigMonitoring::QCstmConfigMonitoring(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::QCstmConfigMonitoring)
{
  ui->setupUi(this);
}

QCstmConfigMonitoring::~QCstmConfigMonitoring()
{
  delete ui;
}

void QCstmConfigMonitoring::SetMpx3GUI(Mpx3GUI *p){
  _mpx3gui = p;
}
