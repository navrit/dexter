#include "qcstmBHWindow.h"
#include "ui_qcstmBHWindow.h"

#include "qcstmglvisualization.h" 

QCstmBHWindow::QCstmBHWindow(QWidget *parent) :
	QDialog(parent),
  ui(new Ui::QCstmBHWindow)
{
  ui->setupUi(this);
  connect(this,&QCstmBHWindow::loadSignal,this, &QCstmBHWindow::on_loadButton_clicked);
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
    connect(this, SIGNAL(reload()),_mpx3gui->getVisualization(),SLOT(reload_all_layers()));
    connect(_mpx3gui, SIGNAL(open_data_failed()),this,SLOT(on_open_data_failed()));
    connect(this, SIGNAL(updateProgressBar(int)),this, SLOT(on_progressBar_valueChanged(int)));
    connect(this,SIGNAL(applyCorrection()),this, SLOT(on_applyBHCorrection()));
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
    if(!correctionMap.contains(thicknessvctr.back())){
        correctionMap.insert(thicknessvctr.back(), *_mpx3gui->getDataset());
        emptyCorrectionCounter--;
        qDebug() << emptyCorrectionCounter ;
    }

    if(emptyCorrectionCounter == 0 && thicknessvctr.size()>2 )
        ui->startButton->setEnabled(true);

}

void QCstmBHWindow::on_clearButton_clicked()
{
    if(!correctionMap.contains(thicknessvctr[selectedItemNo])) emptyCorrectionCounter--;

	delete ui->list->item(selectedItemNo);
    correctionMap.remove(thicknessvctr[selectedItemNo]);
    thicknessvctr.erase (thicknessvctr.begin()+selectedItemNo);

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

  if(emptyCorrectionCounter == 0 && thicknessvctr.size()>2 )
        ui->startButton->setEnabled(true);
}

void QCstmBHWindow::on_saveButton_clicked()
{

}

void QCstmBHWindow::on_loadButton_clicked()
{
    //TODO sort yPlot based on xPlot.

    dataOpened = true;
    emit openData();

    if(!correctionMap.contains(thicknessvctr[selectedItemNo])&&dataOpened)
    {
        correctionMap.insert(thicknessvctr[selectedItemNo], *_mpx3gui->getDataset());
        emptyCorrectionCounter--;
        ui->loadButton->setEnabled(false);

        double count = 0;
        for(int j = 0; j< _mpx3gui->getDataset()->getPixelsPerLayer(); j++ )
        {
            count += _mpx3gui->getDataset()->getLayer(0)[j];
        }
        xPlot.push_back(thicknessvctr[selectedItemNo]);
        yPlot.push_back(count);

        // create graph and assign data to it:
        ui->plotWidget->addGraph();
        ui->plotWidget->graph(0)->setData(xPlot, yPlot);
        // give the axes some labels:
        ui->plotWidget->xAxis->setLabel("Thickness");
        ui->plotWidget->yAxis->setLabel("Signal");
        // set axes ranges, so we see all data:

        double minX = 0;
        double maxX = 0;
        double minY = 0;
        double maxY = 0;

        for(int i = 0; i < xPlot.size(); i++ )
        {
            minX = std::min(minX, xPlot[i]);
            maxX = std::max(maxX, xPlot[i]);
            minY = std::min(minY, yPlot[i]);
            maxY = std::max(maxY, yPlot[i]);
        }

        if(xPlot.size()>2)
        {
            tk::spline s;
            //sort(xPlot.begin(), xPlot.end());
            //sort(yPlot.begin(), yPlot.end());
            s.set_points(xPlot.toStdVector(),yPlot.toStdVector(),false);

            QVector<double> sx;
            QVector<double> sy;

            for(int i = 0; i < maxX; i++)
            {
                sx.push_back(i);
                sy.push_back(s(i));
            }

            ui->plotWidget->graph(1)->setData(sx, sy);
        }

        ui->plotWidget->xAxis->setRange(minX, maxX);
        ui->plotWidget->yAxis->setRange(minY, maxY);
        ui->plotWidget->replot();
    }

    if(emptyCorrectionCounter == 0 && thicknessvctr.size()>2 )
        ui->startButton->setEnabled(true);    


}

void QCstmBHWindow::on_optionsButton_clicked()
{

}

void QCstmBHWindow::on_startButton_clicked()
{
  //_mpx3gui->getDataset()->applyBHCorrection(thicknessvctr, _mpx3gui->getOriginalDataset(), correctionMap);
  emit applyCorrection();
  emit reload();
}

void QCstmBHWindow::on_list_itemClicked(QListWidgetItem *item)
{
	selectedItemNo = item->listWidget()->row(item);
    ui->clearButton->setEnabled(true);
    ui->dataButton->setEnabled(true);
    ui->saveButton->setEnabled(true);
    if(!correctionMap.contains(thicknessvctr[selectedItemNo])){
        ui->loadButton->setEnabled(true);
    }else{
        ui->loadButton->setEnabled(false);
    }
}

void QCstmBHWindow::on_talkToForm(double thickness)
{
    bool contained = false;

    for(int i = 0; i<thicknessvctr.size(); i++)
    {
        if(thicknessvctr[i]==thickness)
        {
            contained = true;
            QMessageBox msgBox;
            msgBox.setText("Please do not use duplicate thicknesses.");
            msgBox.exec();
            break;
        }
    }

    if(!contained)
    {
        QString description = ui->comboBox->currentText() + " ";
        description.append(QString("%1").arg(thickness));
        description += " um";
        ui->list->addItem(description);

        thicknessvctr.push_back(thickness);
        emptyCorrectionCounter++;

        ui->startButton->setEnabled(false);
    }
}

void QCstmBHWindow::on_open_data_failed()
{
    dataOpened = false;
}

void QCstmBHWindow::on_list_doubleClicked(const QModelIndex &index)
{
    emit loadSignal();
}


void QCstmBHWindow::on_progressBar_valueChanged(int value)
{
    ui->progressBar->setValue(value);
    ui->progressBar->update();
}

void QCstmBHWindow::on_applyBHCorrection()
//Makes signal to thickness conversion
{
    QList<int> keys = _mpx3gui->getDataset()->getThresholds();
    if(m_spline==nullptr) m_spline = new tk::spline;  // instantiate spline if not defined

    //Loop over layers
    for (int i = 0; i < keys.length(); i++)
    {
        //Create data structure
        QVector<QVector<double>> bhData(_mpx3gui->getDataset()->getPixelsPerLayer());
        std::sort(thicknessvctr.begin(), thicknessvctr.end(), cstmSortStruct);
        qDebug() << thicknessvctr;
        for(int j = 0; j<thicknessvctr.size(); j++)
        {
                int * layer = correctionMap[thicknessvctr[j]].getLayer(keys[i]);
                for(unsigned int k = 0; k<_mpx3gui->getDataset()->getPixelsPerLayer(); k++){ bhData[k].push_back(layer[k]); }
        }
        //Apply correction
        int * currentLayer = _mpx3gui->getOriginalDataset()->getLayer(keys[i]);
        for(unsigned int j = 0; j< _mpx3gui->getDataset()->getPixelsPerLayer(); j++)
        {
            QVector<double> temp = bhData[j];
            qDebug()<<temp;

            bool ascending = true;
            for(int q = 0; q<temp.size()-1; q++)
            {
                if(temp[q]<temp[q+1]) ascending = false;
            }

            if(temp[0]!= 0 && temp[0] < 50000 && ascending)
            {
                 m_spline->set_points(temp.toStdVector(),thicknessvctr.toStdVector(), false);
                 currentLayer[j] = (*m_spline)(currentLayer[j]); //Do the interpolation
            }
            if(j % (_mpx3gui->getDataset()->getPixelsPerLayer() / 100) == 0)
            {
                emit updateProgressBar( (100 / keys.size()) * (i+1) * j / _mpx3gui->getDataset()->getPixelsPerLayer() );
            }
        }

        for(unsigned int j = 0; j< _mpx3gui->getDataset()->getPixelsPerLayer(); j++)
        {
            _mpx3gui->getDataset()->getLayer(keys[i])[j] = currentLayer[j];
        }

    }

    emit updateProgressBar(100);

}
