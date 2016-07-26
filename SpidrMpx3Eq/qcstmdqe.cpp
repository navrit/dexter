#include "qcstmdqe.h"
#include "ui_qcstmdqe.h"
//#include "mpx3gui.h"
#include "ui_mpx3gui.h"

QCstmDQE::QCstmDQE(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmDQE)
{
    ui->setupUi(this);
}

QCstmDQE::~QCstmDQE()
{
    delete ui;
}

void QCstmDQE::SetMpx3GUI(Mpx3GUI *p) {

    _mpx3gui = p;

    connect( this, SIGNAL(start_takingData()), _mpx3gui->GetUI()->visualizationGL, SLOT(StartDataTaking()) );

}

void QCstmDQE::setRegion(QPoint pixel_begin, QPoint pixel_end)
{
    _beginpix = pixel_begin; _endpix = pixel_end;
    //ui->regionLabel->text();
    ui->regionLabel->setText(QString("Region of interest: (%1, %2)-->(%3, %4)").arg(_beginpix.x()).arg(_beginpix.y()).arg(_endpix.x()).arg(_endpix.y()) );
}



void QCstmDQE::plotMTF()
{

    //_mpx3gui->getDataset()->determinePointsROI(_currentThreshold, _pixel_begin, _pixel_end);
}

void QCstmDQE::plotESF()
{
    if(ui->ESFplot->graphCount() != 0)
        ui->ESFplot->clearGraphs();

    _data.clear();
    _data = _mpx3gui->getDataset()->calcESFdata();

    if(!_data.empty()){
        ui->ESFplot->xAxis->setLabel("Distance (px)");
        ui->ESFplot->yAxis->setLabel("Counts");
        ui->ESFplot->xAxis->setRange(_beginpix.x(), _endpix.x());

        //Plot data points
        ui->ESFplot->addGraph();
        ui->ESFplot->graph(0)->setLineStyle(QCPGraph::lsNone);
        ui->ESFplot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 5));

        ui->ESFplot->graph(0)->setData(_data[0], _data[1]);

        ui->ESFplot->rescaleAxes();
        ui->ESFplot->replot( QCustomPlot::rpQueued );


    }
    else{
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
        msgbox.exec();
    }
}

void QCstmDQE::plotFitESF()
{
    //Add graph for the fit
    ui->ESFplot->addGraph();
    ui->ESFplot->graph(1)->setPen(QPen(Qt::red));

    //Params contains the scaling, offset and half-width a of the erfc, respectively.
    QVector<QVector<double> > fitdata = _mpx3gui->getDataset()->fitESF(_data);
    if(fitdata.empty()){
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No fitting data.",0);
        msgbox.exec();
    }
    else{
        QVector<double> x = fitdata[0];
        QVector<double> y = fitdata[1];

            ui->ESFplot->graph(1)->setData(x, y);
    //        ui->ESFplot->xAxis->setRange(-5., 5.);
    //        ui->ESFplot->yAxis->setRange(-0.2, 1.2);
            ui->ESFplot->rescaleAxes();
            ui->ESFplot->replot( QCustomPlot::rpQueued );

    }

}

void QCstmDQE::plotEdge(QPoint ab)
{   //Display the midline edge in the heatmap glplot...?

}


void QCstmDQE::on_takeDataPushButton_clicked()
{
   _mpx3gui->GetUI()->stackedWidget->setCurrentIndex(__visualization_page_Id);
   emit start_takingData();

}



void QCstmDQE::on_comboBox_currentIndexChanged(const QString &arg1)
{
//    QString s = arg1;
//    s.remove("Threshold", Qt::CaseInsensitive);
//    int layerIndex = s.toInt();
//    setLayer(layerIndex);
    QStringList split = arg1.split(' ');
    int threshold = split.last().toInt();
    int layerIndex = _mpx3gui->getDataset()->thresholdToIndex(threshold);
    setLayer(layerIndex);

    //Collect new dataset.
    _mpx3gui->getDataset()->collectPointsROI(layerIndex, _beginpix, _endpix);
    //And do everything again for this set..

}

void QCstmDQE::on_fitPushButton_clicked()
{
    plotFitESF();
}
