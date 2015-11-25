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

void qcstmBHdialog::on_pushButton_clicked()
{
	// = ui->thicknessBox->text;
	QString valS = ui->thicknessBox->text();
	emit talkToForm(valS.toDouble());



	this->close();
}

void qcstmBHdialog::on_pushButton_2_clicked()
{
    this->close();
}
