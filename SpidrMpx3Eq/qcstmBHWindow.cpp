#include "qcstmBHWindow.h"
#include "ui_qcstmBHWindow.h"

#include "qcstmglvisualization.h" 

QCstmBHWindow::QCstmBHWindow(QWidget *parent) :
	QDialog(parent),
  ui(new Ui::QCstmBHWindow)
{
  ui->setupUi(this);
  connect(this, SIGNAL(selectedItem()), this, SLOT(on_selectedItem()));

}

QCstmBHWindow::~QCstmBHWindow()
{
  delete ui;
}

void QCstmBHWindow::SetMpx3GUI(Mpx3GUI *p){

	_mpx3gui = p;
	connect(this, SIGNAL(takeData()), _mpx3gui->getVisualization(), SLOT(StartDataTaking()));
	connect(this, SIGNAL(switchDataView), _mpx3gui->getVisualization(), SLOT(on_reload_all_layers()));
    connect(this, &QCstmBHWindow::openData, _mpx3gui, &Mpx3GUI::open_data);

}

void QCstmBHWindow::on_addButton_clicked()
{

	_bhdialog = nullptr;
	
	if (!_bhdialog) {
		_bhdialog = new qcstmBHdialog(this);
        _bhdialog->open();
		_bhdialog->raise();
		_bhdialog->activateWindow();

		connect(_bhdialog, SIGNAL(talkToForm(double)), this, SLOT(on_talkToForm(double)));
		
}

	

}

void QCstmBHWindow::on_dataButton_clicked()
{
	emit(takeData());
	correctionMap[selectedItemNo] = _mpx3gui->getDataset();

}

void QCstmBHWindow::on_clearButton_clicked()
{
	correctionMap[selectedItemNo]->clear();
	delete ui->list->item(selectedItemNo);
	selectedItemNo--;
	ui->list->item(selectedItemNo)->setSelected(true);

}

void QCstmBHWindow::on_saveButton_clicked()
{

}

void QCstmBHWindow::on_loadButton_clicked()
{
    emit openData();
    //correctionMap[selectedItemNo] = _mpx3gui->getDataset();
}

void QCstmBHWindow::on_optionsButton_clicked()
{

}

void QCstmBHWindow::on_list_itemClicked(QListWidgetItem *item)
{
	selectedItemNo = item->listWidget()->row(item);
	emit selectedItem();
	emit switchDataView();
}

void QCstmBHWindow::on_talkToForm(double thickness)
{
	QString description = ui->comboBox->currentText() + " ";
	description.append(QString("%1").arg(thickness));
	description += " um";
	ui->list->addItem(description);

	correctionMap[mapCounter] = new Dataset;
	mapCounter++;
}

void QCstmBHWindow::on_selectedItem()
{
	ui->clearButton->setEnabled(true);
	ui->dataButton->setEnabled(true);
	ui->saveButton->setEnabled(true);
	ui->loadButton->setEnabled(true);
}

