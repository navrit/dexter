#include "qcstmBHdialog.h"
#include "ui_qcstmBHdialog.h"

qcstmBHdialog::qcstmBHdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::qcstmBHdialog)
{
    ui->setupUi(this);
}

qcstmBHdialog::~qcstmBHdialog()
{
    delete ui;
}

void qcstmBHdialog::on_okButton_clicked()
{
    QString valS = ui->thicknessBox->text();
    emit talkToForm(valS.toDouble(), ui->comboBox->currentText());
    this->close();
}

void qcstmBHdialog::on_closeButton_clicked()
{
    this->close();
}
