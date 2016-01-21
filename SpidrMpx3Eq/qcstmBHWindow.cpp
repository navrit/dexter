#include "qcstmBHWindow.h"
#include "ui_qcstmBHWindow.h"

#include "qcstmglvisualization.h"

#include "qcstmcorrectionsdialog.h"

QCstmBHWindow::QCstmBHWindow(QWidget *parent) :
	QDialog(parent),
  ui(new Ui::QCstmBHWindow)
{
  ui->setupUi(this);
  connect(this,&QCstmBHWindow::loadSignal,this, &QCstmBHWindow::on_loadButton_clicked);
  _corr = dynamic_cast<QCstmCorrectionsDialog*>(parent);
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
    connect(_corr, SIGNAL(applyBHCorrection()), this, SLOT(on_applyBHCorrection()));
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

        connect(_bhdialog, SIGNAL(talkToForm(double, QString)), this, SLOT(on_talkToForm(double, QString)));
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
    if(correctionMap.contains(thicknessvctr[selectedItemNo])) return;

    dataOpened = true;
    // Load BH layers, but don't save the data set.  The original dataset
    //  has already been saved when loaded.  Otherwise we end up correcting
    //  with respect to a correction layer
    emit openData(false);

    if(!correctionMap.contains(thicknessvctr[selectedItemNo])&&dataOpened)
    {
        correctionMap.insert(thicknessvctr[selectedItemNo], *_mpx3gui->getDataset());
        emptyCorrectionCounter--;
        ui->loadButton->setEnabled(false);
        ui->list->currentItem()->setBackground(QBrush(Qt::cyan));

//Plot

        QVector<double> yPlot, xPlot;
        QMap<double, double> plotMap;
        for(int i = 0; i<thicknessvctr.size(); i++)
        {
            if(correctionMap.contains(thicknessvctr[i])) xPlot.push_back(thicknessvctr[i]);
        }

        for(int i = 0; i< xPlot.size(); i++)
        {
            double count = 0;
            for(int j = 0; j< _mpx3gui->getDataset()->getPixelsPerLayer(); j++ )
            {
                count += correctionMap[xPlot[i]].getLayer(0)[j];
            }
            count /= _mpx3gui->getDataset()->getPixelsPerLayer(); //average of threshold 0
            plotMap.insert(xPlot[i], count);
        }

        std::sort(xPlot.begin(), xPlot.end());

        for(int i = 0; i<xPlot.size(); i++)
        {
            yPlot.push_back(plotMap[xPlot[i]]);
        }

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

        ui->plotWidget->addGraph();
        ui->plotWidget->graph(0)->setData(xPlot, yPlot);
        ui->plotWidget->xAxis->setLabel("Thickness");
        ui->plotWidget->yAxis->setLabel("Signal");
        ui->plotWidget->xAxis->setRange(minX, maxX);
        ui->plotWidget->yAxis->setRange(minY, maxY);
        ui->plotWidget->replot();

        /* Do interpolation - Disabled because QPlot automatically does linear interpolation and spline cubic does not fit well
        if(xPlot.size()>2)
        {
            tk::spline s;
            //sort(xPlot.begin(), xPlot.end());
            //sort(yPlot.begin(), yPlot.end());
            //s.set_points(xPlot.toStdVector(),yPlot.toStdVector(),false);
            s.set_points(xPlot.toStdVector(),yPlot.toStdVector());

            QVector<double> sx;
            QVector<double> sy;

            for(int i = 0; i < maxX; i++)
            {
                sx.push_back(i);
                sy.push_back(s(i));
            }

            ui->plotWidget->graph(1)->setData(sx, sy);
            ui->plotWidget->graph(1)->setPen(QPen(Qt::red));
        }
        */

    }

    if(emptyCorrectionCounter == 0 && thicknessvctr.size()>2 )
        ui->startButton->setEnabled(true);    
}

void QCstmBHWindow::on_optionsButton_clicked()
{

}

void QCstmBHWindow::on_startButton_clicked()
{
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

void QCstmBHWindow::on_talkToForm(double thickness, QString material)
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
        //QString description = ui->comboBox->currentText() + " ";
        material += " ";
        material.append(QString("%1").arg(thickness));
        material += " um";
        ui->list->addItem(material);

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
    if(emptyCorrectionCounter != 0 || thicknessvctr.size()<3 ) return;

    QList<int> keys = _mpx3gui->getDataset()->getThresholds();
    if(m_spline==nullptr) m_spline = new tk::spline;  // instantiate spline if not defined

    //Loop over layers
    for (int i = 0; i < keys.length(); i++)
    {
        //Create data structure
        QVector<QVector<double>> bhData(_mpx3gui->getDataset()->getPixelsPerLayer());
        std::sort(thicknessvctr.begin(), thicknessvctr.end(), cstmSortStruct);
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

            bool ascending = true;
            for(int q = 0; q<temp.size()-1; q++)
            {
                if(temp[q]>=temp[q+1]) ascending = false;
            }

            int a = currentLayer[j];

            if(temp[0]!= 0 && temp[0] < 50000 && ascending)
            {
                 m_spline->set_points(temp.toStdVector(),thicknessvctr.toStdVector(), false);
                 currentLayer[j] = (*m_spline)(currentLayer[j]); //Do the interpolation
            }

            if(j% 1000 == 0) qDebug()<<temp << thicknessvctr << a << currentLayer[j];

            if(a == currentLayer[j]) currentLayer[j] = 0;

            if(j % (_mpx3gui->getDataset()->getPixelsPerLayer() / 1000) == 0)
            {
                emit updateProgressBar( (100 / keys.size()) * (i) + j * (100 / keys.size()) / _mpx3gui->getDataset()->getPixelsPerLayer() );
            }
        }

        for(unsigned int j = 0; j< _mpx3gui->getDataset()->getPixelsPerLayer(); j++)
        {
            _mpx3gui->getDataset()->getLayer(keys[i])[j] = currentLayer[j];
        }

    }

    emit updateProgressBar(100);

}


void QCstmBHWindow::on_okButton_clicked()
{
    if(emptyCorrectionCounter != 0 || thicknessvctr.size()<3 )
    {
        QMessageBox msgBox;
        msgBox.setText("You haven't loaded all / enough corrections. The beam hardening will not operate. Please load more corrections.");
        msgBox.exec();
    }

    this->close();
}
