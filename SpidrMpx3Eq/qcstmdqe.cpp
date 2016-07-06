#include "qcstmdqe.h"
#include "ui_qcstmdqe.h"
//#include "mpx3gui.h"
#include "ui_mpx3gui.h"

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

    connect( this, SIGNAL(start_takingData()), _mpx3gui->GetUI()->visualizationGL, SLOT(StartDataTaking()) );

}


void QCstmDQE::on_takeDataPushButton_clicked()
{
   _mpx3gui->GetUI()->stackedWidget->setCurrentIndex(__visualization_page_Id);
   emit start_takingData();

}
