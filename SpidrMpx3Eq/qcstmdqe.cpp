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



void QCstmDQE::plotMTF()
{

    //_mpx3gui->getDataset()->determinePointsROI(_currentThreshold, _pixel_begin, _pixel_end);
}

void QCstmDQE::plotESF()
{

    QVector<QVector<double> > data = _mpx3gui->getDataset()->calcESFdata();

    ui->ESFplot->xAxis->setLabel("Distance (px)");
    ui->ESFplot->yAxis->setLabel("Counts");
    ui->ESFplot->xAxis->setRange(_beginpix.x(), _endpix.x());
    ui->ESFplot->addGraph(); //scatters..
    ui->ESFplot->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui->ESFplot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 5));

    ui->ESFplot->graph(0)->setData(data[0], data[1]);

    ui->ESFplot->rescaleAxes();
    ui->ESFplot->replot( QCustomPlot::rpQueued );

    //FitESF(data);
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
