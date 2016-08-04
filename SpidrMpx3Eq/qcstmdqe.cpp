#include "qcstmdqe.h"
#include "ui_qcstmdqe.h"
//#include "mpx3gui.h"
#include "ui_mpx3gui.h"
#include "dataset.h"
#include <boost/math/constants/constants.hpp>
#include <dlib/algs.h>
#include <complex>

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

void QCstmDQE::clearDataAndPlots()
{
    ui->ESFplot->clearGraphs();     ui->ESFplot->replot(QCustomPlot::rpQueued);
    ui->LSFplot->clearGraphs();     ui->LSFplot->replot(QCustomPlot::rpQueued);
    ui->MTFplot->clearGraphs();     ui->MTFplot->replot(QCustomPlot::rpQueued);

    _ESFdata.clear();
    _LSFdata.clear();
    _params = 0; //Sets all parameters to zero. (CHECK when using!)
    //_xstart = 0;
    //_plotrange = 0;
}



void QCstmDQE::plotESF()
{
    //if(ui->ESFplot->graphCount() != 0)
        ui->ESFplot->clearGraphs();

    _ESFdata.clear();
    _ESFdata = _mpx3gui->getDataset()->calcESFdata();

    _params = 0; //Sets all parameters to zero.

    if(!_ESFdata.empty()){
        double min = _ESFdata[0][0], max = min;

        for(int i = 0; i < _ESFdata[0].length(); i++){
            if(_ESFdata[0][i] < min)
                min = _ESFdata[0][i];
            if(_ESFdata[0][i] > max)
                max = _ESFdata[0][i];
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

        ui->ESFplot->graph(0)->setData(_ESFdata[0], _ESFdata[1]);

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
        //double stepsize = 0.2; //Specify the distance between datapoints of the plot in pixels.

        //Params contains the scaling, offset and half-width a of the erfc, respectively.
        //QVector<QVector<double> > fitdata = _mpx3gui->getDataset()->fitESF(_data);
        _params = _mpx3gui->getDataset()->fitESFparams(_ESFdata);

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

void QCstmDQE::plotLSF()
{
        //if(ui->LSFplot->graphCount() != 0)
            ui->LSFplot->clearGraphs();
        ui->LSFplot->addGraph();

        ui->LSFplot->xAxis->setLabel("Distance (px)");
        ui->LSFplot->yAxis->setLabel("Normalised signal (au)");
        ui->LSFplot->xAxis->setRange(_beginpix.x(), _endpix.x());

        ui->LSFplot->graph(0)->setLineStyle(QCPGraph::lsStepLeft);

        //Plot data points
        //ui->LSFplot->addGraph();
         _LSFdata = calcLSFdata();
        if(!_LSFdata.empty()){
            ui->LSFplot->graph(0)->setData(_LSFdata[0], _LSFdata[1]);

            ui->LSFplot->rescaleAxes();
            ui->LSFplot->xAxis->setRange(-5, 5);
            ui->LSFplot->replot( QCustomPlot::rpQueued );
        }
}


void QCstmDQE::plotMTF()
{
    //if(ui->MTFplot->graphCount() != 0) //check necessary?
    ui->MTFplot->clearGraphs();
    ui->MTFplot->addGraph();

    ui->MTFplot->xAxis->setLabel("Spatial frequency (1/px)");
    ui->MTFplot->yAxis->setLabel("Normalised response");

    QVector<QVector<double> > data = calcMTFdata();
    if(!data.empty()){
        ui->MTFplot->graph(0)->setData(data[0], data[1]);

        //ui->MTFplot->graph(0)->setScatterStyle( QCPScatterStyle(QCPScatterStyle::ssCross, Qt::red, 6) );

        ui->MTFplot->rescaleAxes();
        ui->MTFplot->xAxis->setRange(0, 0.50);
        ui->MTFplot->replot( QCustomPlot::rpQueued );
    }


    //_mpx3gui->getDataset()->determinePointsROI(_currentThreshold, _pixel_begin, _pixel_end);
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

QVector<QVector<double> > QCstmDQE::calcLSFdata()
{   //Create function
    QVector<QVector<double> > data;
    int fitlength = _plotrange / _histStep;
    QVector<double> x(fitlength);
    QVector<double> y(fitlength);

    double scaling  = _params(0, 0);
    double offset   = _params(1, 0);
    double a        = _params(2, 0);
    double xval=0, yval=0;

    if(a != 0){
        //Calculate the values for the derivative of the erfc function, given the parameters used for the fit.
        for(int i = 0; i < fitlength; i++){
            xval  = (int)_xstart + i*_histStep; //Begin on the left side of a pixel.
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

QVector<QVector<double> > QCstmDQE::calcMTFdata()
{

    int length = _LSFdata[0].length();

    //check if length is a power of two, or else adapt data
    int N = 0; //remember the number of corrections to the length..
    for(int i = 0; i < length; i++){
        if(dlib::is_power_of_two(length)) break;
        else{
            length--;
            N++;
        }
    }

    //Convert to type compatible with dlib function...
    dlib::matrix<complex<double> > cdata(1, length);
    dlib::matrix<complex<double> > fdata(1, length);

//    cdata(0, 1) = {5.7, 0};
//    cdata(0, 2) = {0, 1};
//    double a = cdata(0, 1).real();
//    double b = cdata(0, 2).real(); //testing.. works.. not easy in debug mode...

    int offset = N / 2; //Take half of the removed length away from beginning and end.

    for(int i = 0; i < length; i++ ){
        cdata(0, i) = {_LSFdata[1][offset + i], 0}; //imaginary part is zero
    }
    //The LSFdata should be equally spaced (bins).

//testen
//    N = 128;
//    int Nk = 256;
//    //testen
//    dlib::matrix<complex<double> > testdata(1, N); //Testen cos functie.
//    dlib::matrix<complex<double> > dftdata(1, N); //Testen cos functie met DFT.

//    for(int n = 0; n < N; n++){
//        double arg = 2*n*boost::math::constants::pi<double>();
//        arg /= 10;
//        testdata(0, n) = {cos( arg ), 0}; //{real, imaginary}
//    }

    fdata = dlib::fft(cdata);

    //To convert back to something plottable...
    QVector<QVector<double> > mtfdata(2);
    for(int i = 0; i < mtfdata.length(); i++) mtfdata[i].resize(length);
    double Norm = abs(fdata(0, 0)); //Normalization factor. Value at zero spatial frequency.

    for(double i = 0; i < length ; i++ ){ //Length/2 is Nyquist frequency...
        mtfdata[0][i]   = i / length;
        mtfdata[1][i]   = abs(fdata(0, i));
        mtfdata[1][i]  /= Norm;
    }

//testen
//    fdata = dlib::fft(testdata);

//    complex<double> im_i(0.0, 1.0);
//    for(int k = 0; k < N; k++)
//        for(int n = 0; n < N; n++){
//            complex<double> argu = -2.0*boost::math::constants::pi<double>()* im_i * double(k) * double(n);
//            argu /= N;
//            dftdata(0, k) += testdata(0, n) * exp(argu);
//        }
//    for(double i = 0; i < N; i++){
//        mtfdata[0][i]   = i / N;
//        mtfdata[1][i]   = abs(dftdata(0, i));
//    }

    return mtfdata;
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
    if(!_ESFdata.empty())   plotFitESF();
    else{
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
        msgbox.exec();
    }
}



void QCstmDQE::on_plotLSFpushButton_clicked()
{
    if(_params(0,0) == 0 && _params(1,0)==0 && _params(2,0)==0){
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No fitting parameters.",0);
        msgbox.exec();
    }
    else if(_ESFdata.empty()) {
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
        msgbox.exec();
    }

    else plotLSF();
}

void QCstmDQE::on_loadDataPushButton_clicked()
{
    _NPSfilepaths = QFileDialog::getOpenFileNames(this, tr("Read Data"), tr("."), tr("binary files (*.bin)") );
    QString filepath;

    for(int i = 0; i < _NPSfilepaths.length(); i++){
        filepath = _NPSfilepaths[i];
        if(!filepath.isNull()){
            if(i==0) emit open_data(false, true, filepath);

            QStringList split = filepath.split('/');
            QString filename = split.last();

            ui->listWidget->addItem(filename);
        }
        else{   QMessageBox msgbox(QMessageBox::Warning, "Error", QString("Something went wrong when trying to open a file.\nPath does not exist."),0); //path does not exist?
                msgbox.exec();
        }
    }
    if(ui->listWidget->count() != 0)  ui->listWidget->setCurrentRow(0);
    else{   QMessageBox msgbox(QMessageBox::Warning, "Error", QString("No files could be opened."),0);
            msgbox.exec();
    }

}

void QCstmDQE::on_listWidget_currentRowChanged(int currentRow)
{
    if(currentRow >= 0) emit open_data(false, true, _NPSfilepaths[currentRow]);
}

void QCstmDQE::on_removeDataFilePushButton_clicked()
{
    int index = ui->listWidget->currentRow();
    int nr = ui->listWidget->count();


    QString filename = ui->listWidget->currentItem()->text();
    delete ui->listWidget->item(index);

    for(int i = 0; i < _NPSfilepaths.length(); i++){
        QString filepath = _NPSfilepaths[i];
        if(filepath.contains(filename)) _NPSfilepaths.removeAt(i);
    }
}

void QCstmDQE::on_clearDataFilesPushButton_clicked()
{
    ui->listWidget->clear();
    _NPSfilepaths.clear();
}

void QCstmDQE::on_mtfPushButton_clicked()
{
    if(_LSFdata.empty()) {
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
        msgbox.exec();
    }
    else plotMTF();
}
