#include "qcstmBHWindow.h"
#include "ui_qcstmBHWindow.h"

QCstmBHWindow::QCstmBHWindow(QWidget *parent) :
	QMainWindow(parent),
  ui(new Ui::QCstmBHWindow)
{
  ui->setupUi(this);
}

QCstmBHWindow::~QCstmBHWindow()
{
  delete ui;
}

void QCstmBHWindow::SetMpx3GUI(Mpx3GUI *p){

	_mpx3gui = p;

}

void QCstmBHWindow::on_addButton_clicked()
{
	if (!_bhdialog) {
		_bhdialog = new qstmBHdialog(this);
		_bhdialog->show();
		_bhdialog->raise();
		_bhdialog->activateWindow();
	}
	else {
		_bhdialog->show();
		_bhdialog->raise();
		_bhdialog->activateWindow();
	}

	//ui->list->addItem(const QString &ding)

}

void QCstmBHWindow::on_dataButton_clicked()
{

}

void QCstmBHWindow::on_clearButton_clicked()
{

}

void QCstmBHWindow::on_saveButton_clicked()
{

}

void QCstmBHWindow::on_loadButton_clicked()
{
	_mpx3gui->open_data();
	//ui->list->addItem
}

void QCstmBHWindow::on_optionsButton_clicked()
{

}

void QCstmBHWindow::on_list_itemClicked(QListWidgetItem *item)
{

}
