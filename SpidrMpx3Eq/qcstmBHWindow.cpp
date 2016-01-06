#include "qcstmBHWindow.h"
#include "ui_qcstmBHWindow.h"

#include "qcstmglvisualization.h" 

QCstmBHWindow::QCstmBHWindow(QWidget *parent) :
	QDialog(parent),
  ui(new Ui::QCstmBHWindow)
{
  ui->setupUi(this);
}

QCstmBHWindow::~QCstmBHWindow()
{
  delete ui;
  delete _bhdialog;
}

void QCstmBHWindow::SetMpx3GUI(Mpx3GUI *p){

	_mpx3gui = p;
	connect(this, SIGNAL(takeData()), _mpx3gui->getVisualization(), SLOT(StartDataTaking()));
    connect(this, &QCstmBHWindow::openData, _mpx3gui, &Mpx3GUI::open_data);
    connect(this, SIGNAL(reload()),_mpx3gui->getVisualization(),SLOT(on_reload_all_layers()));
    connect(_mpx3gui, SIGNAL(open_data_failed()),this,SLOT(on_open_data_failed()));
    _mpx3gui->saveOriginalDataset();    // Keep a copy of the original dataset

}

void QCstmBHWindow::on_addButton_clicked()
{

	_bhdialog = nullptr;
	
    if (!_bhdialog)
    {
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
    if(!correctionMap.contains(thicknessvctr.back())) correctionMap.insert(thicknessvctr.back(), *_mpx3gui->getDataset());
    emptyCorrectionCounter--;
    if(emptyCorrectionCounter == 0) ui->startButton->setEnabled(true);

}

void QCstmBHWindow::on_clearButton_clicked()
{
	delete ui->list->item(selectedItemNo);
    if(correctionMap.contains(thicknessvctr[selectedItemNo])) emptyCorrectionCounter--;
    correctionMap.remove(thicknessvctr[selectedItemNo]);
    thicknessvctr.erase (thicknessvctr.begin()+std::max(0,selectedItemNo-1));
    if(selectedItemNo>1)
    {
        selectedItemNo--;
        ui->list->item(selectedItemNo)->setSelected(true);
    }else
    {
        selectedItemNo = 0;
        ui->clearButton->setEnabled(false);
        ui->dataButton->setEnabled(false);
        ui->saveButton->setEnabled(false);
        ui->loadButton->setEnabled(false);
        ui->startButton->setEnabled(false);
    }

    if(emptyCorrectionCounter == 0 && selectedItemNo!=0) ui->startButton->setEnabled(true);
}

void QCstmBHWindow::on_saveButton_clicked()
{

}

void QCstmBHWindow::on_loadButton_clicked()
{
    dataOpened = true;
    emit openData();
    if(!correctionMap.contains(thicknessvctr.back())&&dataOpened)
    {
        correctionMap.insert(thicknessvctr.back(), *_mpx3gui->getDataset());
        emptyCorrectionCounter--;
    }

    if(emptyCorrectionCounter == 0) ui->startButton->setEnabled(true);
}

void QCstmBHWindow::on_optionsButton_clicked()
{

}

void QCstmBHWindow::on_startButton_clicked()
{
  _mpx3gui->getDataset()->applyBHCorrection(thicknessvctr, _mpx3gui->getOriginalDataset(), correctionMap);
  emit reload();
}

void QCstmBHWindow::on_list_itemClicked(QListWidgetItem *item)
{
	selectedItemNo = item->listWidget()->row(item);
    ui->clearButton->setEnabled(true);
    ui->dataButton->setEnabled(true);
    ui->saveButton->setEnabled(true);
    ui->loadButton->setEnabled(true);
}

void QCstmBHWindow::on_talkToForm(double thickness)
{
	QString description = ui->comboBox->currentText() + " ";
	description.append(QString("%1").arg(thickness));
	description += " um";
	ui->list->addItem(description);
    thicknessvctr.push_back(thickness);
    emptyCorrectionCounter++;
    ui->startButton->setEnabled(false);
}

void QCstmBHWindow::on_open_data_failed()
{
    dataOpened = false;
}


