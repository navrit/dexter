#include "qstmBHdialog.h"
#include "ui_qstmBHdialog.h"

qstmBHdialog::qstmBHdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::qstmBHdialog)
{
    ui->setupUi(this);
}

qstmBHdialog::~qstmBHdialog()
{
    delete ui;
}

void qstmBHdialog::on_pushButton_clicked()
{
	this->close();
}

void qstmBHdialog::on_pushButton_2_clicked()
{
    this->close();
}
