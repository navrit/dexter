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
	// = ui->thicknessBox->text;
	QString valS = ui->thicknessBox->text();
	emit talkToForm(valS.toDouble());



	this->close();
}

void qstmBHdialog::on_pushButton_2_clicked()
{
    this->close();
}
