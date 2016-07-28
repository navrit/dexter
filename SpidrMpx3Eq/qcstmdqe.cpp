#include "qcstmdqe.h"
#include "ui_qcstmdqe.h"
//#include "mpx3gui.h"
#include "ui_mpx3gui.h"
#include "dataset.h"
#include <boost/math/constants/constants.hpp>

//using namespace boost::math::constants;

QCstmDQE::QCstmDQE(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmDQE)
{
    ui->setupUi(this);    

//    connect( this, SIGNAL(start_takingData()), _mpx3gui->GetUI()->visualizationGL, SLOT(StartDataTaking()) );
//    connect( this, &QCstmDQE::open_data, _mpx3gui, &Mpx3GUI::open_data_with_path);
}

QCstmDQE::~QCstmDQE()
{
    delete ui;
}
void QCstmDQE::SetMpx3GUI(Mpx3GUI *p){
    _mpx3gui = p;

    connect( this, SIGNAL(start_takingData()), _mpx3gui->GetUI()->visualizationGL, SLOT(StartDataTaking()) );
    connect( this, &QCstmDQE::open_data, _mpx3gui, &Mpx3GUI::open_data_with_path);
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

    _params = 0; //Sets all parameters to zero.

    if(!_data.empty()){
        double min = _data[0][0], max = min;

        for(int i = 0; i < _data[0].length(); i++){
            if(_data[0][i] < min)
                min = _data[0][i];
            if(_data[0][i] > max)
                max = _data[0][i];
        }
        _xstart = min;
        _plotrange = max - min;

        //Plot data points
        ui->ESFplot->addGraph();
        ui->ESFplot->graph(0)->setLineStyle(QCPGraph::lsNone);
        ui->ESFplot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 5));
        ui->ESFplot->xAxis->setLabel("Distance (px)");
        ui->ESFplot->yAxis->setLabel("Normalised signal (au)");
        ui->ESFplot->xAxis->setRange(_xstart, _xstart + _plotrange);

        ui->ESFplot->graph(0)->setData(_data[0], _data[1]);

        ui->ESFplot->rescaleAxes();
        ui->ESFplot->replot( QCustomPlot::rpQueued );
    }
    else{
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
        msgbox.exec();
    }
}

void QCstmDQE::clearDataAndPlots()
{
    ui->ESFplot->clearGraphs();     ui->ESFplot->replot(QCustomPlot::rpQueued);
    ui->PSFplot->clearGraphs();     ui->PSFplot->replot(QCustomPlot::rpQueued);
    ui->MTFplot->clearGraphs();     ui->MTFplot->replot(QCustomPlot::rpQueued);

    _data.clear();
    _params = 0; //Sets all parameters to zero. (CHECK when using!)
    //_xstart = 0;
    //_plotrange = 0;
}

void QCstmDQE::plotFitESF()
{
    if(!_data.empty()){
        //Add graph for the fit
        ui->ESFplot->addGraph();
        ui->ESFplot->graph(1)->setPen(QPen(Qt::red));
        //double stepsize = 0.2; //Specify the distance between datapoints of the plot in pixels.

        //Params contains the scaling, offset and half-width a of the erfc, respectively.
        //QVector<QVector<double> > fitdata = _mpx3gui->getDataset()->fitESF(_data);
        _params = _mpx3gui->getDataset()->fitESFparams(_data);

        QVector<QVector<double> > fitdata = calcESFfitData();

        if(fitdata[0].empty() || fitdata[1].empty()){
            QMessageBox msgbox(QMessageBox::Warning, "Error", "No fitting data could be generated.",0);
            msgbox.exec();
        }
        ui->ESFplot->graph(1)->setData(fitdata[0], fitdata[1]);
        //        ui->ESFplot->xAxis->setRange(-5., 5.);
        //        ui->ESFplot->yAxis->setRange(-0.2, 1.2);
        ui->ESFplot->rescaleAxes();
        ui->ESFplot->replot( QCustomPlot::rpQueued );
    }
    else{
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
        msgbox.exec();
    }
}

void QCstmDQE::plotPSF()
{
    if(!_data.empty()){
        if(ui->PSFplot->graphCount() != 0)
            ui->PSFplot->clearGraphs();
        ui->PSFplot->addGraph();

        ui->PSFplot->xAxis->setLabel("Distance (px)");
        ui->PSFplot->yAxis->setLabel("Normalised signal (au)");
        ui->PSFplot->xAxis->setRange(_beginpix.x(), _endpix.x());

        //Plot data points
        //ui->PSFplot->addGraph();
        QVector<QVector<double> > data = calcPSFdata();
        if(!data.empty()){
            ui->PSFplot->graph(0)->setData(data[0], data[1]);

            ui->PSFplot->rescaleAxes();
            ui->PSFplot->xAxis->setRange(-5, 5);
            ui->PSFplot->replot( QCustomPlot::rpQueued );
        }
    }
    else{
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
        msgbox.exec();
    }
}

QVector<QVector<double> > QCstmDQE::calcESFfitData()
{
    int fitlength = _plotrange/_stepsize;
    QVector<QVector<double> > fitdata;
    //parameter_vector params;
    input_vector fitxv;
    QVector<double> fitxdata(fitlength);
    QVector<double> fitydata(fitlength);

    //Setup plotdata to return
    double scaling = _params(0,0);
    double a = _params(2,0);

    if(scaling != 0 && a != 0){

        //Make x (distance) input_vector to put in model
        //and a QVector of all distances and a QVector of all the fitted curve values to return.
        for(int j = 0; j < fitlength; j++){
            double distance = _xstart + j*_stepsize;
            fitxv(0) = distance;
            fitxdata[j] = distance;
            fitydata[j] = model(fitxv, _params);
        }

        fitdata.push_back(fitxdata);
        fitdata.push_back(fitydata);
    }
        return fitdata;
}

QVector<QVector<double> > QCstmDQE::calcPSFdata()
{   //Create function
    QVector<QVector<double> > data;
    int fitlength = _plotrange / _stepsize;
    QVector<double> x(fitlength);
    QVector<double> y(fitlength);

    double scaling  = _params(0, 0);
    double offset   = _params(1, 0);
    double a        = _params(2, 0);
    double xval=0, yval=0;

    if(a != 0){
        //Calculate the values for the derivative of the erfc function, given the parameters used for the fit.
        for(int i = 0; i < fitlength; i++){
            xval  = _xstart + i*_stepsize;
            double arg = (xval - offset) / a;
            yval  = -arg*arg;
            yval  = exp(yval);
            yval *= 2 * scaling / a;
            yval /= boost::math::constants::root_pi<double>();

            x[i]  = xval;
            y[i]  = yval;
        }

        data.push_back(x);
        data.push_back(y);
        return data;
    }
    else{
        QMessageBox msgbox(QMessageBox::Warning, "Error", "Cannot divide by zero",0);
        msgbox.exec();
        return data;
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
    //int layerIndex = _mpx3gui->getDataset()->thresholdToIndex(threshold);
    setSelectedThreshold(threshold);

    //Collect new dataset.
    _mpx3gui->getDataset()->collectPointsROI(threshold, _beginpix, _endpix);
    //And do everything again for this set..

}

void QCstmDQE::on_fitPushButton_clicked()
{
    plotFitESF();
}



void QCstmDQE::on_plotPSFpushButton_clicked()
{
    if(_params(0,0) == 0 && _params(1,0)==0 && _params(2,0)==0){
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No fitting parameters.",0);
        msgbox.exec();
    }

    else plotPSF();
}

void QCstmDQE::on_loadDataPushButton_clicked()
{
    QString filepath = QFileDialog::getOpenFileName(this, tr("Read Data"), tr("."), tr("binary files (*.bin)") );

    if(!filepath.isNull()){
        emit open_data(false, true, filepath);

        QStringList split = filepath.split('/');
        QString filename = split.last();

        //TODO: Add listitem with appropriate name.
        ui->listWidget->addItem(filename);
        ui->listWidget->setCurrentRow(0);

        //TODO: Load and add multiple files.
    }
}
