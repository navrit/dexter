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
