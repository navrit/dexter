#include "qcstmdqe.h"
#include "ui_qcstmdqe.h"
//#include "mpx3gui.h"
#include "ui_mpx3gui.h"
#include "dataset.h"
#include <boost/math/constants/constants.hpp>
#include <dlib/algs.h>
#include <complex>
#include <QtDataVisualization>
#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "dlib/statistics.h"

//using namespace boost::math::constants;

QCstmDQE::QCstmDQE(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QCstmDQE)
{
    ui->setupUi(this);

    ui->ESFplot->xAxis->setLabel("Distance (px)");
    ui->ESFplot->yAxis->setLabel("Normalised ESF");
    // add title layout element:(font is too big)
//    ui->ESFplot->plotLayout()->insertRow(0);
//    ui->ESFplot->plotLayout()->addElement(0, 0, new QCPPlotTitle(ui->ESFplot, "ESF"));

    ui->LSFplot->xAxis->setLabel("Distance (px)");
    ui->LSFplot->yAxis->setLabel("Normalised LSF");

    ui->MTFplot->xAxis->setLabel("Spatial frequency (F_Nyquist)");
    ui->MTFplot->yAxis->setLabel("Normalised Presampled MTF");

    ui->xNPSplot->xAxis->setLabel("Spatial frequency (1/x)");
    ui->xNPSplot->yAxis->setLabel("Normalised NPS X direction");
    ui->yNPSplot->xAxis->setLabel("Spatial frequency (1/y)");
    ui->yNPSplot->yAxis->setLabel("Normalised NPS Y direction");

//    connect( this, SIGNAL(start_takingData()), _mpx3gui->GetUI()->visualizationGL, SLOT(StartDataTaking()) );
//    connect( this, &QCstmDQE::open_data, _mpx3gui, &Mpx3GUI::open_data_with_path);


    connect( ui->ESFplot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(on_mouseMove_showPlotPoint(QMouseEvent*)) );
    connect( ui->LSFplot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(on_mouseMove_showPlotPoint(QMouseEvent*)) );
    connect( ui->MTFplot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(on_mouseMove_showPlotPoint(QMouseEvent*)) );


    //Tracer.. doesn't move. TO DO: fix
//    connect( ui->LSFplot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(on_mouseClick_showPlotPoint(QMouseEvent*)) );

//    tracer = new QCPItemTracer(ui->LSFplot);
//    tracer->setGraph(ui->LSFplot->graph(0));
//    tracer->setGraphKey(0.0);
//    tracer->setInterpolating(false);
//    tracer->setStyle(QCPItemTracer::tsCircle);
//    tracer->setPen(QPen(Qt::red));
//    tracer->setBrush(Qt::red);
//    tracer->setSize(7);
//    ui->LSFplot->addItem(tracer);

//    ui->windowLabel->setToolTip(tr("used for local fitting, must be an uneven number"));
//    ui->windowLineEdit->setToolTip(tr("must be an uneven number"));

    _optionsDialog = new optionsDialog(this);

    addNPSfile("icons/startupimage.bin"); //Set startupimage data in NPS files list.

}

QCstmDQE::~QCstmDQE()
{
    delete ui;
}
void QCstmDQE::SetMpx3GUI(Mpx3GUI *p){
    _mpx3gui = p;

    connect( this, SIGNAL(start_takingData()), _mpx3gui->GetUI()->visualizationGL, SLOT(StartDataTaking()) );
    connect( this, &QCstmDQE::open_data, _mpx3gui, &Mpx3GUI::open_data_with_path);
    connect( ui->comboBox, SIGNAL(currentIndexChanged(QString)), _mpx3gui->GetUI()->visualizationGL, SLOT(on_layerSelector_activated(QString)) );

    _optionsDialog->SetMpx3GUI(_mpx3gui);

//    connect( _mpx3gui,SIGNAL(returnFilename(QString)), this, SLOT(on_maindata_changed(QString)) );
//    connect( _mpx3gui, &Mpx3GUI::reload_all_layers, this, &QCstmDQE::on_maindata_changed ); //Both work, different syntax only.
    connect( _mpx3gui,SIGNAL(returnFilename(QString)), this, SLOT(addNPSfile(QString)) );
}

void QCstmDQE::setRegion(QPoint pixel_begin, QPoint pixel_end)
{
    _beginpix = pixel_begin; _endpix = pixel_end;
    //ui->regionLabel->text();
    ui->regionLabel->setText(QString("Region of interest: (%1, %2)-->(%3, %4)").arg(_beginpix.x()).arg(_beginpix.y()).arg(_endpix.x()).arg(_endpix.y()) );
}

void QCstmDQE::clearDataAndPlots(bool clearNPS)
{
//    emit ui->clearMTFpushButton->clicked();
//    if(clearNPS) emit ui->clearNPSpushButton->clicked();
//    emit ui->logClearPushButton->clicked(); //SLOW
    on_clearMTFpushButton_clicked();
    if(clearNPS) on_clearNPSpushButton_clicked();
    on_logClearPushButton_clicked();

    _beginpix   = QPoint(-1, -1);
    _endpix     = QPoint(-1, -1);

}

void QCstmDQE::refreshLog(bool emptylog){
    if(emptylog) ui->textBrowser->clear();
    else{
        ui->textBrowser->setText(_logtext);
        ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum()); //Scroll down.
    }
}

bool QCstmDQE::isValidRegionSelected()
{
    if( _beginpix.x()   < 0)
        return false;
    if( _beginpix.y()   < 0)
        return false;
    if( _endpix.x()     < 0)
        return false;
    if( _endpix.y()     < 0)
        return false;

    return true;
}

//------------------------MTF (Modulation Transfer Function)---------------------------------------------------------------------------------------

//!Plots the Edge Spread Function.
void QCstmDQE::plotESF()
{
    ui->ESFplot->clearGraphs();
    _params = 0; //Sets all parameters to zero.
    _ESFdata.clear();

    _ESFdata = _mpx3gui->getDataset()->calcESFdata();

    if(!_ESFdata.empty()){
        _ESFbinData = calcESFbinData();

        double min = _ESFdata[0][0], max = min;

        for(int i = 0; i < _ESFdata[0].length(); i++){
            if(_ESFdata[0][i] < min)
                min = _ESFdata[0][i];
            if(_ESFdata[0][i] > max)
                max = _ESFdata[0][i];
        }
        _xstart = min;
        _plotrange = max - min;

        ui->ESFplot->xAxis->setRange(_xstart - 1, _xstart + _plotrange + 1);

        //Plot data points and binvalues
        ui->ESFplot->addGraph();
        ui->ESFplot->graph(0)->setLineStyle(QCPGraph::lsNone);
        ui->ESFplot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::lightGray, Qt::white, 5));

        ui->ESFplot->addGraph();
        ui->ESFplot->graph(1)->setLineStyle(QCPGraph::lsNone);//QCPGraph::lsStepCenter);
        ui->ESFplot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::red, Qt::white, 5));
        ui->ESFplot->graph(1)->setErrorType(QCPGraph::etValue);

        ui->ESFplot->graph(0)->setData(_ESFdata[0], _ESFdata[1]);
        ui->ESFplot->graph(1)->setDataValueError(_ESFbinData[0], _ESFbinData[1], _ESFbinData[2]);

        ui->ESFplot->yAxis->rescale();
        ui->ESFplot->replot( QCustomPlot::rpQueued );

        ui->dataCheckbox->setEnabled(true);
        _optionsDialog->setDataRange(_plotrange);
        _logtext += QString("ESF data was binned using\n binsize = %1\n").arg(_binsize);
        refreshLog(false);
    }
    else  QMessageBox::warning ( this, tr("Error"), tr( "No data." ) );

}

//!Plots the error function  or smoothed (locally fitted) function that is fitted to the ESF data.
void QCstmDQE::plotFitESF()
{

    int graphNr = ui->ESFplot->graphCount();
    if(graphNr <  4){
        //Add graph for the error function fit (if not available)
        ui->ESFplot->addGraph();
        ui->ESFplot->graph(graphNr)->setPen(QPen(Qt::blue));
        //And for the smoothing fit
        ui->ESFplot->addGraph();
        ui->ESFplot->graph(graphNr + 1)->setPen(QPen(Qt::magenta));
    }

    graphNr = ui->ESFplot->graphCount();

    QVector<QVector<double> > fitdata;
    QVector<QVector<double> > data;

    if(_usebins) data = _ESFbinData;
    else data = _ESFdata;

    std::vector<double> fity(data[0].length());
    std::vector<double> y(data[0].length());

    if(_useErrorFunc){
        fitESFparams(data);

        fitdata = calcESFfitData();

        if(fitdata[0].empty() || fitdata[1].empty())
            QMessageBox::warning ( this, tr("Error"), tr( "No fitting data could be generated." ) );
        else{
            _logtext += "Error function fitted to ";
            if(_usebins) _logtext += "BINNED ESF data ";
                else _logtext += "prebinned ESF data ";
            _logtext += QString("with parameters:\n scaling = %1\n offset = %2\n a = %3\n").arg(_params(0, 0)).arg(_params(1,0)).arg(_params(2,0));
        }
        if(fitdata[0].length() != fitdata[1].length())
            QMessageBox::warning ( this, tr("Error"), tr( "Something is wrong with the data. Input and output arrays are not the same size." ) );
        ui->ESFplot->graph(graphNr - 2)->setData(fitdata[0], fitdata[1]);

        //Calculate the values of the fit at the original data positions.
        input_vector input;
        for(int i = 0; i < fity.size(); i++){
            input(0) = data[0][i];
            fity.at(i) = model(input, _params);    //model value
            y.at(i) = data[1][i];                  //actual data value
        }
    }
    else{
        fitdata = calcSmoothedESFdata(data);
        _ESFsmoothData = fitdata;
//        _logtext += QString("Smoothing 4th order polynomial function fitted to binned ESF data with \n binsize = %1\n window width = %2").arg(_binsize).arg(_windowW);



        if(fitdata[0].length() != fitdata[1].length())
            QMessageBox::warning ( this, tr("Error"), tr( "Something is wrong with the data. Input and output arrays are not the same size." ) );
        else if(fitdata[0].empty() || fitdata[1].empty())
            QMessageBox::warning ( this, tr("Error"), tr( "No smoothing data could be generated." ) );
        else{
            _logtext += "Smoothing 4th order polynomial function fitted to ";
            if(_usebins) _logtext += "BINNED ESF data ";
                else _logtext += "prebinned ESF data ";
            _logtext += QString("with \n window width = %1\n").arg(_windowW);
        }

        ui->ESFplot->graph(graphNr - 1)->setData(fitdata[0], fitdata[1]);

        for(int i = 0; i < fitdata[0].length(); i++){
            fity.at(i) = fitdata[1][i];
            y.at(i) = data[1][i];
        }

    }

    //std::vectors required for dlib
//    int length = fitdata[0].length();
//    std::vector<double> fity(length);
//    std::vector<double> y(length);
//    for(int i = 0; i < length; i++){
//        fity.at(i) = fitdata[1][i];
//        y.at(i) = data[1][i];
//    }

    double R2 = dlib::r_squared(fity, y);
    _logtext += QString("R squared = %1\n").arg(R2);
    refreshLog(false);

    //        ui->ESFplot->xAxis->setRange(-5., 5.);
    //        ui->ESFplot->yAxis->setRange(-0.2, 1.2);
    ui->ESFplot->rescaleAxes();
    ui->ESFplot->replot( QCustomPlot::rpQueued );

    ui->clearFitsPushButton->setEnabled(true); //Enable button to clear fitplots
}

void QCstmDQE::plotLSF()
{
    //if(ui->LSFplot->graphCount() != 0)
    ui->LSFplot->clearGraphs();
    ui->LSFplot->addGraph();

    ui->LSFplot->xAxis->setRange(_beginpix.x(), _endpix.x());

    ui->LSFplot->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
//        ui->LSFplot->graph(0)->setScatterStyle(QCPGraph::);
    ui->LSFplot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 5));
        //Plot data points
        //ui->LSFplot->addGraph();

//        _useDerFit = ui->derivCheckBox->isChecked(); //let's use the numerical derivative.
    _LSFdata = calcLSFdata();

    if(!_LSFdata.empty()){
        ui->LSFplot->graph(0)->setData(_LSFdata[0], _LSFdata[1]);
        if (ui->logScaleCheckBox->isChecked()) ui->LSFplot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui->LSFplot->rescaleAxes();
        //ui->LSFplot->xAxis->setRange(-5, 5);
        ui->LSFplot->replot( QCustomPlot::rpQueued );
    }
    else  QMessageBox::warning ( this, tr("Error"), tr( "No LSF data!" ) );


}


void QCstmDQE::plotMTF()
{
    //if(ui->MTFplot->graphCount() != 0) //check necessary?
    ui->MTFplot->clearGraphs();
    ui->MTFplot->addGraph();

    QVector<QVector<double> > data = calcMTFdata();
    if(!data.empty()){
        ui->MTFplot->graph(0)->setData(data[0], data[1]);

        //ui->MTFplot->graph(0)->setScatterStyle( QCPScatterStyle(QCPScatterStyle::ssCross, Qt::red, 6) );

        ui->MTFplot->rescaleAxes();
        ui->MTFplot->xAxis->setRange(0, 1.1); //Plot until the Nyquist frequency
        ui->MTFplot->yAxis->rescale();//setRange(0, 1.1);
        ui->MTFplot->replot( QCustomPlot::rpQueued );
    }


    //_mpx3gui->getDataset()->determinePointsROI(_currentThreshold, _pixel_begin, _pixel_end);
}

void QCstmDQE::fitESFparams(QVector<QVector<double> > esfdata)
{
    std::vector<std::pair<input_vector, double> > data; //vector of pairs of variable going in and the value coming out.

    int length = esfdata.length(); //esfdata[0].length();
    input_vector input;     //must be in dlib::matrix form...

    for(int i = 0; i < length; i++){
        input(0) = esfdata[0][i];           //x
        const double output = esfdata[1][i];//y

        data.push_back( make_pair(input, output) );     //std
    }

    parameter_vector params;
    double red_chi_squared = 0;
    double mse; //Mean Squared Error.

    try
    {   //Initialize parameters
        params(0,0) = 0.5;
        params(1,0) = 0.0;
        params(2,0) = 0.8;

        cout << "Use Levenberg-Marquardt, approximate derivatives" << endl;
        dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-7).be_verbose(),
                                     residual,
                                     dlib::derivative(residual),
                                     data,
                                     params);

        cout << "inferred parameters: "<< dlib::trans(params) << endl;

        _params = params;

        double res;
        double res2;
        double Res;
        std::vector<std::pair<input_vector, double>>::size_type i ;
        for(i = 0; i != data.size(); i++){

            res = residual( data.at(i), params );
            res2 = res * res;
            if(_usebins) Res = res2 / esfdata[2][i]; //divide by st.dev. -> Normalized residual..
                else Res = res2;
            red_chi_squared += Res;
            mse += res2;
        }

        cout << "Normalized residuals: " << red_chi_squared;

        red_chi_squared /= length - (int)params.size();

        cout << " 'Reduced Chi squared:' "<< red_chi_squared << endl;

        //TO DO: find a more appropriate way to represent the quality of the fit.
        //#dof is undefined for a non-linear model fit.

        mse /= length;
//        cout << "Mean Squared error: " << mse << endl;
        _logtext += QString("Mean Squared Error of the fit: %1\n").arg(mse) ;
    }
    catch (std::exception& e)
    {
        cout << e.what() << endl;
        QMessageBox::warning ( this, tr("An error has occurred"), tr( e.what() ) );

    }

}

double model(const input_vector &input, const parameter_vector &params){ //Types designed for the optimization algorithm.
    const double x = input(0);
    const double scaling = params(0);
    const double offset = params(1);
    const double a = params(2);
    double arg = 0;
    if (a!= 0)  arg = - (x - offset) / a;

    return scaling * erfc(arg);
}

double residual(const std::pair<input_vector, double>& data, const parameter_vector& params)
{
    return model(data.first, params) - data.second;
}

QString QCstmDQE::dataToString(QVector<QVector<double> > data)
{
    QString string;
    int length = data[0].length();
    int i;
    for(i = 0; i < length; i++){
        string += QString("%1").arg(data[0][i]);
        string += " ";
    }
    string += "\n\n";
    for(i = 0; i < length; i++){
        string += QString("%1").arg(data[1][i]);
        string += " ";
    }

    return string;
}

QVector<QVector<double> > QCstmDQE::calcESFbinData()
{   //TO DO Check/fix and use for other way of MTF.

    //QMap<double, QPair<double, double> > bindata;
    QVector<QVector<double> > esfbindata(3); //3rd for st.dev.
    QVector<int> Npoints;
    double dmin, dmax;
    dmin = _ESFdata[0][0];
    dmax = dmin;
    int i;

    //Find beginning and end of the data.
    for(i = 0; i < _ESFdata[0].length(); i++){
        double d = _ESFdata[0][i];
        if(d < dmin) dmin = d;
        if(d > dmax) dmax = d;
    }
    dmin = floor(dmin);
    dmax = ceil(dmax);
    int length = (dmax - dmin + 1) / _binsize;

    for(i = 0; i < esfbindata.length(); i++ )
        esfbindata[i].resize(length);


    for(i = 0; i < length; i++){
        esfbindata[0][i] = dmin + i*_binsize;
        //Npoints[i] = 0;
    }

    Npoints.resize(length);

    //Calculating the total pixelvalue and #points in each bin.
    //_ESFdata is not in order of distance...
    double d, binleft, dif;
    for(i = 0; i < _ESFdata[0].length(); i++){
        d = _ESFdata[0][i];
        for(int j = 0; j < length; j++){
            binleft = esfbindata[0][j];
            dif = d - binleft;
            if(dif >= 0 && dif < _binsize){
                esfbindata[1][j] += _ESFdata[1][i];
                Npoints[j]++;
                break; //A point can only be in one bin.
            }
        }
    }

    //Check for empty bins and remove.
    for(i = length - 1; i >= 0; i--){
        if(Npoints[i] == 0){
            Npoints.remove(i);
            for(int j=0; j<esfbindata.length(); j++)
                esfbindata[j].remove(i);
            length--;
        }
    }

    //Calculate mean of bins
    for(i = 0; i< length; i++)
        esfbindata[1][i] /= Npoints[i];

    //Calculating the standard deviation of each bin,
    //by looking again in which bin each point belongs...
    //TO DO: don't repeat..?
    double esfval, mean;
    for(i = 0; i < _ESFdata[0].length(); i++){
        d = _ESFdata[0][i];
        for(int j = 0; j < length; j++){
            binleft = esfbindata[0][j];
            dif = d - binleft;
            if(dif >= 0 && dif < _binsize){
                esfval = _ESFdata[1][i];
                mean = esfbindata[1][j];
                esfbindata[2][j] += (esfval - mean) * (esfval - mean);
                break;
            }
        }
    }

    for(i = 0; i< length; i++){
        esfbindata[2][i] /= Npoints[i];
        esfbindata[2][i] = sqrt(esfbindata[2][i]);
    }

    //Use the middle of the bin for plotting.
    for(i = 0; i < length; i++){
        //esfbindata[1][i] /= Npoints[i]; //already taken in stdev loop.
        esfbindata[0][i] += _binsize /2;
    }

    return esfbindata;
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

QVector<QVector<double> > QCstmDQE::calcSmoothedESFdata(QVector<QVector<double> > data)
{   int i;
//    _windowW = 11; //Set window width (TO DO user option).
    int offset = (_windowW - 1) / 2;
    std::vector<std::pair<input_vector, double> > windowData(_windowW); //vector of pairs of the variable going in and the value coming out.
    parameter_vector params;
    params = 1;
    params(5) = _windowW;
    input_vector input;
    QVector<QVector<double> > smoothData(2);
    //smoothData[0] = data[0]; //No.. smoothdata is 2*offset shorter...
    int lengthSmooth = data[0].length() - 2* offset;
    smoothData[0].resize(lengthSmooth);
    smoothData[1].resize(lengthSmooth);

//    for(i = 0; i < windowW; i++){
//        input(0) = data[0][i];       //x
//        input(1) = - 0.5*(windowW - 1) + i; //Position of datapoint in array wrt middle (for weighting)
//        double output = data[1][i];  //y
//        windowData.at(i) = make_pair(input, output);
//    }

    for(int j = 0; j < lengthSmooth; j++){//For every point(index) to be fitted and put in smoothdata.

//        if(i > offset){
//            //Shift window by one spot.
//            windowData.erase(windowData.begin());
//            input(0) = data[0][i + offset];
//            windowData.push_back( make_pair( input , data[1][i + offset] ) );
//        }
        int imiddle = j + offset;

        int begin   = - 0.5*(_windowW-1);
        int end     =   0.5*(_windowW-1);

        for(i = begin; i <= end; i++){
            input(0) = data[0][imiddle + i];                        //x
            input(1) = i;                                           //Position of datapoint in array wrt middle (for weighting)
            double output = data[1][imiddle + i];                   //y
            windowData.at(offset + i) = make_pair(input, output);
        }

        //Find parameters for the fit to this window of data.
        dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-7).be_verbose(),
                                     polyResidual,
                                     dlib::derivative(polyResidual),
                                     windowData,
                                     params);

        //Use fitted polynomial to find the value yi for point ximiddle and put in smoothData.
        input(0) = data[0][imiddle];
        smoothData[0][j] = data[0][imiddle];
        smoothData[1][j] = polyModel(input, params);
    }

    return smoothData;
}

double polyModel(const input_vector &input, const parameter_vector &params){ //Types designed for the optimization algorithm.

    double x = input(0);

    return params(0)*x*x*x*x + params(1)*x*x*x + params(2)*x*x + params(3)*x + params(4);
}

double polyResidual(const std::pair<input_vector, double>& data, const parameter_vector& params)
{
    return (polyModel(data.first, params) - data.second)*polyWeightRoot((data.first), params(5)); //multiply by the square root of the weighting factor.
}

double polyWeightRoot(input_vector input, int windowW){
    //Gaussian weights, (Samei et al. (1998))
    double i = input(1);
    double arg = 4*i/( windowW - 1);
    arg *= - arg; //- arg2

    double f = exp(arg);
    return sqrt(f);
}

QVector<QVector<double> > QCstmDQE::calcNumDerivativeOfdata(QVector<QVector<double> > data){
    //!Calculates the numerical derivative of the ESF bindata, giving the LSF.
    int length = data[0].length();

    QVector<QVector<double> > derData(2);

//    double bw = _binsize;
//    int offset = 2*bw; //5point
    int offset = 1; //3point
    length -= 2*offset;

    for(int i = 0; i < derData.length(); i++){
        derData[i].resize(length);
    }

    for(int i = 0; i < length; i++){
        int j = i + offset;
        derData[0][i] = data[0][j];
        //derData[1][i] = FivePointsStencil(data[1], i, bw);
        derData[1][i] = (data[1][j+1] - data[1][j-1])/(2*_binsize); //3-point difference equation
    }

    return derData;
}


double QCstmDQE::FivePointsStencil(QVector<double> func, int x, double bw) {

    double der = 0.;

    der -=      func[ x + 2*bw ];
    der += 8. * func[ x +   bw ];
    der -= 8. * func[ x -   bw ];
    der +=      func[ x - 2*bw ];
    der /= 12.*bw;

    return der;
}



QVector<QVector<double> > QCstmDQE::calcLSFdata()
{   //Create function
    QVector<QVector<double> > data;
    int i;
    _histStep = _binsize; //?

    int fitlength = _plotrange / _histStep;
    QVector<double> x(fitlength);
    QVector<double> y(fitlength);

    if(_useErrorFunc && _useDerFit){
        double scaling  = _params(0, 0);
        double offset   = _params(1, 0);
        double a        = _params(2, 0);
        double xval=0, yval=0;

        if(a != 0){
            //Calculate the values for the derivative of the erfc function, given the parameters used for the fit.
            for(i = 0; i < fitlength; i++){
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
            //return data;
            _logtext += "LSF calculated using fitting parameters and derivative of corresponding Error function.\n";

        }
        else{
            QMessageBox::warning ( this, tr("Error"), tr( "Cannot divide by zero!" ) );
            //return data; //empty
        }
    }
    else if(_useDerFit){//Take derivative of smoothed datafit.
        data = calcNumDerivativeOfdata(_ESFsmoothData);
        if(!data.isEmpty()) _logtext += "LSF calculated using numerical derivative of the SMOOTHED ESF.\n";
        else QMessageBox::warning ( this, tr("Error"), tr( "Something went wrong while taking the numerical derivative." ) );
    }

    else { //Take derivative of the binned data..
        data = calcNumDerivativeOfdata(_ESFbinData);
        if(!data.isEmpty()) _logtext += "LSF calculated, using numerical derivative of the BINNED ESF.\n";
        else QMessageBox::warning ( this, tr("Error"), tr( "Something went wrong while taking the numerical derivative." ) );
    }

    //Calculate maximum value of the ESFdata and normalize to one.
    double max = 0;
    int length = data[1].length();
    for(i = 0; i < length; i++){
        double val = data[1][i];
        if( val > max) max = val;
    }
    for(i = 0; i < length; i++){
        data[1][i] /= max;
    }

    return data;
}

QVector<QVector<double> > QCstmDQE::calcMTFdata()
{

    int length = _LSFdata[0].length();

    //TO DO: pad with zeros instead of removing data.
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
//    dlib::matrix<complex<double> > mtfData(2, length);

    int offset = N / 2; //Take half of the removed length away from beginning and end.

    for(int i = 0; i < length; i++ ){
        cdata(0, i)     = {_LSFdata[1][offset + i], 0};     //imaginary part is zero
//        mtfData(1, i)   = {_LSFdata[1][offset + i], 0};     //imaginary part is zero
//        mtfData(0, i)   = 2* i / length;                    //Normalize x-axis (1/L) to Nyquist frequency: 1/2*1/L.
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

    for(double i = 0; i < length; i++ ){
        mtfdata[0][i]   = 2* i / length;    //Normalized to Nyquist frequency: Length/2.
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

    _MTFdata = mtfdata;
    return mtfdata;
    }



void QCstmDQE::plotEdge(QPoint ab)
{   //Display the midline/edge in the heatmap glplot...?

}

//-------------------------------------------------------------------------------------------------------------------------------------------------


//---------------------NPS (Noise Power Spectrum)------------------------------------------------------------------------------------------------



void QCstmDQE::calcNPSdata()
{
    QVector<QVector<double> > ft2Ddata;
    QVector<QVector<double> > ftROIdata;
//    QVector<double> npsdata;

    _NPSdata.clear();
    _NPSdata.resize(2);

    int Nfiles = ui->listWidget->count();
    if(Nfiles == 0){
        QMessageBox::warning ( this, tr("Error"), tr( "No data files." ) );
        _NPSdata.clear();
        return ;    //npsdata is empty;
    }

    if(_singleFileNPS){
        _logtext += "NPS calculated for a single file.\n";
        if(_useSelectedRoI){
            if(isValidRegionSelected())
                ft2Ddata = calcFTsquareRoI( _mpx3gui->getDataset()->collectPointsROI(_currentThreshold, _beginpix, _endpix) );

            _logtext += QString("Region (%1, %2)->(%3, %4) was used.\n").arg(_beginpix.x()).arg(_beginpix.y()).arg(_endpix.x()).arg(_endpix.y());

        }
        else if(_useFullimage){
            //Give the upper left and lower right points of the image to collectPointsROI
            QPoint dsize = _mpx3gui->getDataset()->getSize();
            int Nd = _mpx3gui->getDataset()->getFrameCount();
            int Nx = sqrt( double(Nd) ) * dsize.x();
            int Ny = sqrt( double(Nd) ) * dsize.y();

            ft2Ddata = calcFTsquareRoI( _mpx3gui->getDataset()->collectPointsROI( _currentThreshold, QPoint(0, Ny), QPoint(Nx, 0) ) );

                _logtext += QString("Region (%1, %2)->(%3, %4) was used.\n").arg(0).arg(Ny).arg(Nx).arg(0);
        }

        if(_showFT) plotFTsquare(ft2Ddata);

        calc1Dnps(ft2Ddata);
    }
    else{
        _logtext += "Mean NPS calculated for multiple files:\n";

        for( int i = 0; i < Nfiles; i++){
            QString filename = _NPSfilepaths[i];
            emit open_data(false, true, filename);
            _logtext += "  " + filename + "\n";

            ftROIdata = calcFTsquareRoI( _mpx3gui->getDataset()->collectPointsROI( _currentThreshold, _beginpix, _endpix));

            int xlength = ftROIdata[0].length();
            int ylength = ftROIdata.length();

            ft2Ddata.resize(ylength);

            for(int i = 0; i < ylength; i++){
                ft2Ddata[i].resize(xlength);
            }

            //Add to total of multiple files.
            for(int y = 0; y < ftROIdata.length(); y++){
                for(int x = 0; x < ftROIdata[0].length(); x++){ //assuming all rows are equal length
                    ft2Ddata[y][x] += ftROIdata[y][x];
                }
            }
        }
        //Calculate mean FT squared.
        if(Nfiles > 1){
            for(int y = 0; y < ftROIdata.length(); y++)
                for(int x = 0; x < ftROIdata[0].length(); x++){ //assuming all rows are equal length
                    ft2Ddata[y][x] /= Nfiles;
                }
        }

        if(_showFT) plotFTsquare(ft2Ddata);

        calc1Dnps(ft2Ddata);
    }

    //Now normalize the NPS to its maximum value.
    if(_normNPSmax) calcNormNPS();

}

void QCstmDQE::calcNormNPS(){
    //Normalize the NPS to its maximum value.
    if(_NPSdata.isEmpty()) return;

    int max = 0, i;
    double value;
    for(i = 0; i < _NPSdata[0].length(); i++){
        value = _NPSdata[0][i];
        if(value > max) max = value;
    }
    for(i = 0; i < _NPSdata[0].length(); i++)
        _NPSdata[0][i] /= max;

    max = 0;
    for(int i = 0; i < _NPSdata[1].length(); i++){
        value = _NPSdata[1][i];
        if(value > max) max = value;
    }
    for(i = 0; i < _NPSdata[1].length(); i++)
        _NPSdata[1][i] /= max;
}

void QCstmDQE::calc1Dnps(const QVector<QVector<double> > &ftdata)
{
    int i;
    int xlength = ftdata[0].length();
    int ylength = ftdata.length();
    _NPSdata[1].resize( ylength );

    //Should not be possible anymore.
//    if(!_useZeroFreq && _NlinesNPS == 0){
//        QMessageBox::warning ( this, tr("Nothing to use for 1D NPS"), tr( "Please specify whether the zero frequency axes and/or off-axis lines should be used." ) );
//        _NPSdata.clear();
//        return;
//    }
        if(_useZeroFreq){
            _NPSdata[0] = ftdata[0]; //x-axis (y=0)

            for( i = 0; i < ylength; i++){
                _NPSdata[1][i] = ftdata[i][0]; //y-axis (x=0)
            }
            _logtext += "Zero frequency included in 1D NPS calculation.\n";
        }
        else _NPSdata[0].resize(xlength);

        if(_NlinesNPS > 0){
            for(int n = 1; n <= _NlinesNPS; n++){
                for( i = 0; i < xlength; i++){
                    _NPSdata[0][i] += ftdata[n][i];
    //                _NPSdata[0][i] += ftdata[xlength - n][i];  //? include symmetric (/negative)
                }
                for ( i = 0; i < ylength; i++){
                    _NPSdata[1][i] += ftdata[i][n];
    //                _NPSdata[1][i] += ftdata[i][ylength - n];
                }
            }

            if(_NlinesNPS == 1) _logtext += "1 off-axis line was used in 1D NPS calculation.\n";
            else _logtext += QString("%1 off-axis lines were used in 1D NPS calculation.\n").arg(_NlinesNPS);
        }

}

QVector<QVector<double> > QCstmDQE::calcFTsquareRoI(QVector<QVector<int> > data )
{
    //The data is constructed as follows:
    //      - Each row represents a horizontal row of pixels, starting from the bottom of the selected RoI.
    //      - The elements in each row represent the pixels in the row, starting from the left.
    //The data can thus be seen as a normal cartesion system, where the left side of each pixel is the index.
    //To get datapoints in the middle of each pixel, a correction of +0.5 pixel has to be added in both the x and y direction.

    int xlength = data[0].length(); //Assuming the RoI is rectangular, i.e. every row has the same length.
    int ylength = data.length();

    //For test plotting:
    QtDataVisualization::QScatterDataArray data3D;


    if(_fitPlane){
        //Fit planar ramp.
        parameter_vector params = fitPlaneParams(data);
        input_vector input;
        double z;

        //Correct for the fitted plane (substract)
        for(int y = 0; y < ylength; y++){
                for(int x = 0; x < xlength; x++){
                    input(0) = x + 0.5;
                    input(1) = y + 0.5;
                    z = planeModel(input, params);
                    data[y][x] -= z;
                }
        }
    }

//    //Test pattern.
//    xlength = 8;
//    ylength = 8;

//    dlib::matrix<complex<double> > datamatrix(xlength, ylength);

//    //Make a stripy testpattern.
//    for(int y = 0; y < ylength; y++){
//        for(int x = 0; x < xlength; x++){
//            if( x <  2){
//                data3D.push_back(QVector3D(x, y, 10));
//                datamatrix(x, y) = { 10, 0};
//            }
//            else if( x <  4) {
//                data3D.push_back(QVector3D(x, y,  0));
//                datamatrix(x, y) = {  0, 0};
//            }
//            else if( x < 6) {
//                data3D.push_back(QVector3D(x, y, 10));
//                datamatrix(x, y) = { 10, 0};
//            }
//            else if( x < 8) {
//                data3D.push_back(QVector3D(x, y,  0));
//                datamatrix(x, y) = {  0, 0};
//            }
//        }
//    }


//    int end = xlength;
    for(int i = 0; i < 1000; i++){
        if( dlib::is_power_of_two( data[0].length() ) ) break;
        else{
            //Pad one zero in each row.
            for(int j = 0; j < ylength; j++)
                data[j].push_back(0);
            xlength++ ;
        }
    }
//    end = ylength;
    for(int i = 0; i < 1000; i++){
        if( dlib::is_power_of_two( data.length() ) ) break;
        else{
            //Pad one row of zeros.
            QVector<int> zeros(xlength, 0);
            data.push_back(zeros);
//            for(int j = 0; j < xlength; j++)
//                data[j].push_back(0);
            ylength++ ;
        }
    }

    //Put the data in a matrix with complex values for FFT calculation...
    dlib::matrix<complex<double> > datamatrix(xlength, ylength);
//    dlib::matrix<complex<double> > FTmatrix(ylength, xlength);
    dlib::matrix<complex<double> > FTmatrix(xlength, ylength); //right?

    for(int y = 0; y < ylength; y++){
            for(int x = 0; x < xlength; x++){
                datamatrix(x, y) = {double(data[y][x]), 0.0};
            }
    }

//    plotData3D(data3D);
    FTmatrix = dlib::fft(datamatrix);

    //Just to see data in Debugger :
    QVector<QVector<double> > ftdata(ylength);
    for(int y = 0; y < ylength; y++){
        ftdata[y].resize(xlength);
    }
    for(int y = 0; y < ylength; y++){
            for(int x = 0; x < xlength; x++){
                //double z = norm( FTmatrix(y, x) ); //Norm gives the squared magnitude of the complex number in the FTmatrix.
                double z = abs ( FTmatrix(x, y) );
//                colorMap->data()->setCell(x, y, z);
                ftdata[y][x] = z;
            }
    }

    return ftdata;
}


parameter_vector QCstmDQE::fitPlaneParams(QVector<QVector<int> > dataRoI) //Creates error onlt when running in debug mode...
{
    std::vector<std::pair<input_vector, double> > data; //vector of pairs of variable going in and the value coming out.

    int xlength = dataRoI[0].length();
    int ylength = dataRoI.length();
    input_vector input;
    parameter_vector params;
    QtDataVisualization::QScatterDataArray data3D;

    try{
        for(int y = 0; y < ylength; y++){
            for(int x = 0; x < xlength; x++){
                input(0) = x + 0.5;
                input(1) = y + 0.5;
                double output = dataRoI[y][x];

                data.push_back( make_pair(input, output) );
                data3D.push_back( QVector3D(input(0), input(1), output) );
            }
        }

        //Let's display the data in a 3Dplot using Qt data visualization (Qt 5.7 onwards only)
        //plotData3D(data3D);

        params(0,0) = 1;
        params(1,0) = 1;
        params(2,0) = dataRoI[0][0];
        //Now use the least-squares algorithm to fit a plane.
        dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-7).be_verbose(),
                                     planeResidual,
                                     dlib::derivative(planeResidual),
                                     data,
                                     params);

        //Calculate mean quared error:
        double res, mse;
        std::vector<std::pair<input_vector, double>>::size_type i ;
        for(i = 0; i != data.size(); i++){
            res = residual( data.at(i), params );
            mse += res*res;
        }
        mse /= data.size();
        _logtext += QString("Mean Squared Error of the plane fit: %1\n").arg(mse) ;
    }
    catch(std::exception& e){
        QMessageBox::warning ( this, tr("Error"), tr( e.what() ) );
        cout << e.what() << endl;
    }

    return params;
}

double planeModel(const input_vector &input, const parameter_vector &params){ //Types designed for the optimization algorithm.
   //Returns z = ax + by + c.
    return params(0)*input(0) + params(1)*input(1) + params(2);
}

double planeResidual(const std::pair<input_vector, double>& data, const parameter_vector& params)
{
    return planeModel(data.first, params) - data.second;
}

void QCstmDQE::plotData3D(QtDataVisualization::QScatterDataArray data3D)
{
    QtDataVisualization::Q3DScatter *scatter = new QtDataVisualization::Q3DScatter();
    QWidget *container = QWidget::createWindowContainer(scatter, _mpx3gui, Qt::Window );
    container->setParent(_mpx3gui);
    container->setAttribute( Qt::WA_DeleteOnClose );
    scatter->setFlags(scatter->flags() ^ Qt::FramelessWindowHint);
    QtDataVisualization::QScatter3DSeries *series = new QtDataVisualization::QScatter3DSeries;
    series->dataProxy()->addItems(data3D);
    scatter->addSeries(series);
    container->show();
}

void QCstmDQE::plotFTsquare(const QVector<QVector<double> > &ftdata)
{
    int xlength = ftdata[0].length();
    int ylength = ftdata.length();

    QCustomPlot *ftplot = new QCustomPlot;
    ftplot->setParent(_mpx3gui, Qt::Window);
    ftplot->setAttribute( Qt::WA_DeleteOnClose );
    ftplot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    ftplot->axisRect()->setupFullAxesBox(true);
    ftplot->xAxis->setLabel("x");
    ftplot->yAxis->setLabel("y");
    ftplot->setMinimumHeight(400);
    ftplot->setMinimumWidth(500);

    //Set up QCPColorMap:
    QCPColorMap *colorMap = new QCPColorMap(ftplot->xAxis, ftplot->yAxis);
    ftplot->addPlottable(colorMap);
    colorMap->data()->setSize(xlength, ylength); // we want the color map to have nx * ny data points
    colorMap->data()->setRange(QCPRange(0, 1), QCPRange(0, 1));
    colorMap->setInterpolate(false);

    for(int y = 0; y < ylength; y++){
            for(int x = 0; x < xlength; x++){
                double z = ftdata[y][x];
                colorMap->data()->setCell(x, y, z);
            }
    }

    //Add a color scale:
    QCPColorScale *colorScale = new QCPColorScale(ftplot);
    ftplot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    colorMap->setColorScale(colorScale); // associate the color map with the color scale
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();
    ftplot->rescaleAxes();
    ftplot->show();
}

void QCstmDQE::plotNPS(){

    calcNPSdata();

    int datalength = _NPSdata.length();
    if(datalength == 0){
        QMessageBox::warning ( this, tr("Error when plotting NPS"), tr( "No data!" ) );
        return ;
    }

    int xlength = _NPSdata[0].length();
    int ylength = _NPSdata[1].length();
    double i;
    double xstepsize = 1 / double( xlength );
    double ystepsize = 1 / double( ylength );
    ui->xNPSplot->clearGraphs();                                    ui->yNPSplot->clearGraphs();
    ui->xNPSplot->addGraph();                                       ui->yNPSplot->addGraph();
    ui->xNPSplot->graph(0)->setLineStyle(QCPGraph::lsImpulse);      ui->yNPSplot->graph(0)->setLineStyle(QCPGraph::lsImpulse);

    for(i = 0.0; i < xlength; i++)
        ui->xNPSplot->graph(0)->addData( i * xstepsize, _NPSdata[0][i] );    //Plot the values for fx (fy=0).

    for(i = 0.0; i < ylength; i++)
        ui->yNPSplot->graph(0)->addData( i * ystepsize, _NPSdata[1][i] );    //Plot the values for fy (fx=0)

    ui->xNPSplot->rescaleAxes();                                    ui->yNPSplot->rescaleAxes();
    ui->xNPSplot->xAxis->setRange(-0.01, 1.01);                     ui->yNPSplot->xAxis->setRange(-0.01, 1.01);
    ui->xNPSplot->replot( QCustomPlot::rpQueued );                  ui->yNPSplot->replot( QCustomPlot::rpQueued );

}


//-------------------------------------------------------------------------------------------------------------------------------------------------

void QCstmDQE::on_takeDataPushButton_clicked() {
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

//    _mpx3gui->GetUI()->visualizationGL->setThreshold(threshold); //connected..

    //Collect new dataset.
    _mpx3gui->getDataset()->collectPointsROI(threshold, _beginpix, _endpix);
    //And do everything again for this set..

}

//void QCstmDQE::on_fitESFpushButton_clicked()
//{
//    if(!_ESFdata.isEmpty() && !_ESFbinData.isEmpty())   plotFitESF();
//    else{
//        QMessageBox::warning ( this, tr("Error fitting parameters"), tr( "No data!" ) );
//    }
//}



//void QCstmDQE::on_fitLSFpushButton_clicked()
//{
//    if(_params(0,0) == 0 && _params(1,0)==0 && _params(2,0)==0){
//        QMessageBox msgbox(QMessageBox::Warning, "Error", "No fitting parameters.",0);
//        msgbox.exec();
//    }
//    else if(_ESFdata.empty()) {
//        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
//        msgbox.exec();
//    }

//    else plotLSF();
//}

void QCstmDQE::on_loadDataPushButton_clicked()
{
    _openingNPSfile = true;
    QStringList filepaths = QFileDialog::getOpenFileNames(this, tr("Read Data"), tr("."), tr("binary files (*.bin)") );
    QString filepath;

    for(int i = 0; i < filepaths.length(); i++){
        filepath = filepaths[i];
        if(!filepath.isNull()){

//            _NPSfilepaths += filepath;
            if(i==0) emit open_data(false, true, filepath);

            addNPSfile(filepath);

//            QStringList split = filepath.split('/');
//            QString filename = split.last();

//            ui->listWidget->addItem(filename);
        }
        else QMessageBox::warning ( this, tr("Error"), tr( "Something went wrong when trying to open a file.\nPath does not exist.") );

    }
//    int count = ui->listWidget->count();
//    if(ui->listWidget->count() != 0)  ui->listWidget->setCurrentRow(0);
    if(ui->listWidget->count() == 0)
        QMessageBox::warning ( this, tr("Error"), tr( "No files could be opened." ) );

    _openingNPSfile = false;
}

void QCstmDQE::addNPSfile(QString filepath){

    if(!_openingNPSfile) clearDataAndPlots(true);

    _NPSfilepaths += filepath;

    QStringList split = filepath.split('/');
    QString filename = split.last();

    ui->listWidget->addItem(filename);

    int count = ui->listWidget->count();
    if(count != 0)  ui->listWidget->setCurrentRow(count - 1);
        else ui->listWidget->setCurrentRow(0);
}

void QCstmDQE::on_listWidget_currentRowChanged(int currentRow)
{
    if(currentRow >= 0) emit open_data(false, true, _NPSfilepaths[currentRow]);
}

void QCstmDQE::on_removeDataFilePushButton_clicked()
{
    int index = ui->listWidget->currentRow();
    //int nr = ui->listWidget->count();


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
    if(_ESFdata.empty()) {
        QMessageBox msgbox(QMessageBox::Warning, "Error", "No data.",0);
        msgbox.exec();

        _logtext += "No data available, not able to calculate anything.\n"; //Is it necessary to put warnings in log as well as Qmessagebox??

    }
    else{
        plotFitESF();
//        if(_useErrorFunc)_logtext += QString("Function has been fitted to the binned ESF data with \n binsize = %1 pixels\n and parameters:\n scaling = %2\n offset = %3\n a = %4\n \n").arg(_binsize).arg(_params(0, 0)).arg(_params(1,0)).arg(_params(2,0));
//        else _logtext += QString("Smoothing 4th order polynomial function has been fitted locally to the binned ESF data with \n binsize = %1\n").arg(_binsize);

        if(_params(0,0) == 0 && _params(1,0)==0 && _params(2,0)==0 && _useErrorFunc && _useDerFit){
            QMessageBox::warning ( this, tr("Error"), tr( "No fitting parameters." ) );
        }
//        else if(_ESFdata.empty()) {
//            QMessageBox msgbox(QMessageBox::Warning, "Error", "No ESF data.",0);
//            msgbox.exec();
//        }

        else{
            plotLSF();
//            if(_useDerFit) _logtext += "A Line Spread Function has been calculated, using the fitting parameters and the derivative of the corresponding error function.\n";
//            else _logtext += "A Line Spread Function has been calculated, using a numerical derivative of the (smoothed) binned Edge Spread Function.\n";

            if(_LSFdata.empty()) {
                QMessageBox::warning ( this, tr("Error"), tr( "No LSF data." ) );
            }
            else{
                plotMTF();
                _logtext += "MTF calculated by Fast Fourier Transform of the calculated LSF.\n";
                //TO DO:..?
            }

        }

    }
    _logtext += "\n";
    refreshLog(false);
}

void QCstmDQE::on_npsPushButton_clicked()
{
    if(_useSelectedRoI && !isValidRegionSelected()){
        QMessageBox::warning ( this, tr("Cannot calculate NPS"), tr( "Please choose a region of interest." ) );
        _mpx3gui->GetUI()->stackedWidget->setCurrentIndex(__visualization_page_Id);
    }

    else{
        _logtext += "\n";
        refreshLog(false);
        plotNPS();
        refreshLog(false);
    }

//    calcNPSdata();

}

void QCstmDQE::on_saveMTFpushButton_clicked()
{ //FIX ME!! >>>>
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Data"), tr("."), tr("Text files (*.txt)"));
    QString pngfilename;
    // Force the .txt in the data filename
    if ( ! filename.contains(".txt") ) {
        pngfilename = filename.append(".png");
        filename.append(".txt");
    }
    else{ pngfilename = filename;
        pngfilename.remove(".txt");
        pngfilename.append(".png");
    }

    // And save
    QFile saveFile(filename);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        string messg = "Couldn't open: ";
        messg += filename.toStdString();
        messg += "\nNo output written!";
        QMessageBox::warning ( this, tr("Error saving data"), tr( messg.c_str() ) );
        return;
    }

    ui->MTFplot->savePng(pngfilename, 0, 0, 2);

    QTextStream out(&saveFile);
    if(!_MTFdata.isEmpty()) out << dataToString(_MTFdata);
    else QMessageBox::warning ( this, tr("Error"), tr( "No MTF data." ) );
//    saveFile.write(dataToString(_MTFdata));
    saveFile.close();
}

void QCstmDQE::on_logClearPushButton_clicked()
{
    _logtext.clear();
    refreshLog(true);
}

void QCstmDQE::on_logSavePushButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Data"), tr("."), tr("Text files (*.txt)"));
    QFile savefile(filename);
    if (!savefile.open(QIODevice::WriteOnly)) {
        string messg = "Couldn't open: ";
        messg += filename.toStdString();
        messg += "\nNo output written!";
        QMessageBox::warning ( this, tr("Error saving data"), tr( messg.c_str() ) );
        return;
    }

    QTextStream out(&savefile);
    out << ui->textBrowser->document()->toPlainText();
}

//void QCstmDQE::on_binSizeLineEdit_editingFinished()
//{   ui->binSizeLineEdit->blockSignals(true); //Prevents double signals sent by pressing 'Enter' to interfere.

//    double binsize = ui->binSizeLineEdit->text().toDouble();

//    if(binsize <= 0){
//        QMessageBox::warning ( this, tr("Error"), tr( "Binsize cannot be zero or negative!" ) );
//        ui->binSizeLineEdit->setText( QString("%1").arg(_binsize) );
//    }
//    else _binsize = binsize;

//    if(!_ESFdata.isEmpty()) plotESF();
//    else QMessageBox::warning ( this, tr("Error"), tr( "No data!" ) );

//    ui->binSizeLineEdit->blockSignals(false);
//}

void QCstmDQE::ConnectionStatusChanged(bool connected)
{
    ui->takeDataPushButton->setEnabled( connected );
}

void QCstmDQE::on_logScaleCheckBox_toggled(bool checked)
{
    if(checked) ui->LSFplot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    else        ui->LSFplot->yAxis->setScaleType(QCPAxis::stLinear);
    ui->LSFplot->replot( QCustomPlot::rpQueued );

}

void QCstmDQE::on_derivCheckBox_toggled(bool checked)
{
    _useDerFit = checked;
}

//void QCstmDQE::on_errorFuncCheckBox_toggled(bool checked)
//{
//    _useErrorFunc = checked;
//}

//void QCstmDQE::on_mouseClick_showPlotPoint(QMouseEvent *event)
//{
//    double x = ui->LSFplot->xAxis->pixelToCoord(event->pos().x());
//    tracer->setGraphKey(x);

//    tracer->updatePosition();
//    ui->LSFplot->replot( QCustomPlot::rpQueued );
//}

void QCstmDQE::on_mouseMove_showPlotPoint(QMouseEvent *event)
{
    QCustomPlot* plot = qobject_cast<QCustomPlot*>(sender());
//    int x = ui->LSFplot->xAxis->pixelToCoord(event->pos().x());
//    int y = ui->LSFplot->yAxis->pixelToCoord(event->pos().y());
    double x = plot->xAxis->pixelToCoord(event->pos().x());
    double y = plot->yAxis->pixelToCoord(event->pos().y());

    ui->pointLabel->setText(QString("(%1 , %2)").arg(x).arg(y));
}

void QCstmDQE::on_dataCheckbox_toggled(bool checked)
{
    if(checked)
        ui->ESFplot->graph(0)->setData(_ESFdata[0], _ESFdata[1]);
    else
        ui->ESFplot->graph(0)->clearData();

    ui->ESFplot->replot( QCustomPlot::rpQueued );

}

//void QCstmDQE::on_fitComboBox_currentIndexChanged(const QString &arg1)
//{
//    if(arg1.contains("Error")){
//        _useErrorFunc = true;
//        ui->windowLabel->setEnabled(false);
//        ui->windowLineEdit->setEnabled(false);
//    }
//    if(arg1.contains("Smoothing")){
//        _useErrorFunc = false;
//        ui->windowLabel->setEnabled(true);
//        ui->windowLineEdit->setEnabled(true);
//    }
//}

//void QCstmDQE::on_windowLineEdit_editingFinished()
//{
//    int width = ui->windowLineEdit->text().toInt();
////    int width = _optionsDialog->ui->lineEdit();
//    if(width <= 2){
//        width = 3;
//        ui->windowLineEdit->setText(QString("%1").arg(width));
//        QMessageBox::warning ( this, tr("Warning"), tr( "The window width must be bigger than 2." ) );
//    }
//    if(width % 2 == 0){
//        width ++; //The window width must be an uneven number.
//        ui->windowLineEdit->setText(QString("%1").arg(width));
//        QMessageBox::warning ( this, tr("Warning"), tr( "The window width must be an uneven number." ) );
//    }
//    if(width > _ESFbinData[0].length()){
//        width = _ESFbinData[0].length();
//        ui->windowLineEdit->setText(QString("%1").arg(width));
//        QMessageBox::warning ( this, tr("Warning"), tr( "The window width can not be larger than the number of data points." ) );
//    }

//    _windowW = width;
//}

void QCstmDQE::on_clearFitsPushButton_clicked()
{
    ui->ESFplot->graph(i_esfFitgraph)->clearData();
    ui->ESFplot->graph(i_esfFitgraph + 1)->clearData();

    ui->ESFplot->replot( QCustomPlot::rpQueued );
}

void QCstmDQE::on_optionsPushButton_clicked()
{
    _optionsDialog->show();
//    _optionsDialog->setCurrentSettings();
}

void QCstmDQE::on_apply_options(QHash<QString, int> options)
{
    //Set all options values in variables.

    //MTF
    //    if(options.value("edge")    == 0);
    if(options.value("error")   == 0)
         _useErrorFunc = false;
    else _useErrorFunc = true;
    if(options.value("fitder")  == 0)
         _useDerFit = false;
    else _useDerFit = true;

    _windowW = options.value("windowW");

    if(options.value("bindata") == 0)
         _usebins = false;
    else _usebins = true;

    if(_binsize != options.value("binsize")){
        _binsize = options.value("binsize");
        plotESF();      //TO DO: only replot the BINNED data. Seperate functions?
    }

    //NPS
    _useFullimage   = options.value("fullimage");
    _useSelectedRoI = options.value("selectedroi");
    _useManualRoI   = options.value("manualroi");

    _nRoI = options.value("roinumber");
    _sizeRoI = QPoint( options.value("roixsize"), options.value("roiysize"));
    _nPixEdge = options.value("npixedge");

    if(options.value("fitplane") == 0)
         _fitPlane = false;
    else _fitPlane = true;

    if(options.value("zerofreq") == 0)
         _useZeroFreq = false;
    else _useZeroFreq = true;
    if(options.value("showft") == 0)
         _showFT = false;
    else _showFT = true;

    _NlinesNPS = options.value("nlines");
    _normNPSmax = options.value("normmaxnps");
}

void QCstmDQE::on_maindata_changed(QString /*filename*/)
{
    if(!_openingNPSfile){
        clearDataAndPlots(true);
//        QStringList list = filename.split("/");
//        filename = list.last();
//        ui->listWidget->addItem( filename );

//        int count = ui->listWidget->count();
//        if(count != 0) ui->listWidget->setCurrentRow(0);
    }
}

void QCstmDQE::on_close_optionsDialog()
{
//    if(_optionsDialog){
//        delete _optionsDialog;
//        _optionsDialog = nullptr;
//    }
    _optionsDialog->close();

}

void QCstmDQE::on_singleFileCheckBox_toggled(bool checked)
{
    _singleFileNPS = checked;
}

void QCstmDQE::on_clearNPSpushButton_clicked()
{
    ui->xNPSplot->clearGraphs();
    ui->xNPSplot->replot(QCustomPlot::rpQueued);
    ui->yNPSplot->clearGraphs();
    ui->yNPSplot->replot(QCustomPlot::rpQueued);
    _NPSdata.clear();

    //TODO: Close 3D plotting windows, if they are kept in final program.
}

void QCstmDQE::on_clearMTFpushButton_clicked()
{
//    clearDataAndPlots();
    ui->ESFplot->clearGraphs();     ui->ESFplot->replot(QCustomPlot::rpQueued);
    ui->LSFplot->clearGraphs();     ui->LSFplot->replot(QCustomPlot::rpQueued);
    ui->MTFplot->clearGraphs();     ui->MTFplot->replot(QCustomPlot::rpQueued);

    _ESFdata.clear();
    _ESFbinData.clear();
    _LSFdata.clear();
    _params = 0; //Sets all parameters to zero. (CHECK when using!)
    //_xstart = 0;
    //_plotrange = 0;
}

void QCstmDQE::on_clearAllPushButton_clicked()
{
    clearDataAndPlots(true);
    ui->regionLabel->setText("Choose a region of interest");
}

void QCstmDQE::on_toolButton_clicked()
{
    //Open an explaining dialog.
}
