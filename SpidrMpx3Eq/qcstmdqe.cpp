#include "qcstmdqe.h"
#include "ui_qcstmdqe.h"

QCstmDQE::QCstmDQE(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmDQE)
{
    ui->setupUi(this);
}

QCstmDQE::~QCstmDQE()
{
    delete ui;
}

void QCstmDQE::SetMpx3GUI(Mpx3GUI *p) {

    _mpx3gui = p;

}
