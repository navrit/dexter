#include "profiledialog.h"
#include "ui_profiledialog.h"
#include "qcustomplot.h"
#include "qcstmglvisualization.h"

#include <QString>

ProfileDialog::ProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileDialog)
{
    ui->setupUi(this);

    makeEditsList();

    for(int i = 0; i < editsList.length(); i++)
        connect( editsList[i], SIGNAL(editingFinished()), this, SLOT(on_pointEdit_editingFinished()));

}

ProfileDialog::~ProfileDialog()
{
    delete ui;
}

void ProfileDialog::SetMpx3GUI(Mpx3GUI * p )
{
    _mpx3gui = p;

    connect( this, &ProfileDialog::user_accepted_profile,
             _mpx3gui->getVisualization(),
             &QCstmGLVisualization::on_user_accepted_profile );

    connect( ui->profilePlot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(mousePressEvent(QMouseEvent*)));
}



void ProfileDialog::on_buttonBox_accepted() {
    // talk back to the main gui
    emit user_accepted_profile();
}

void ProfileDialog::changeTitle(){
    QString T = "Profile in the " + _axis + " direction";
    T += QString(" from (%1, %2)-->(%3, %4)").arg(_begin.x()).arg(_begin.y()).arg(_end.x()).arg(_end.y());
    ui->groupBox->setTitle(T);
}

void ProfileDialog::changeText(QString text){
    ui->textBrowser->setText(text);
    //update();
}

void ProfileDialog::plotProfile()
{
    ui->profilePlot->clearGraphs();
    ui->profilePlot->xAxis->setLabel(_axis + "-axis");
    ui->profilePlot->yAxis->setLabel("Total pixel count");

    //Use the difference in X or Y between the begin and end point as range for the x-axis of the plot.
    if(_axis == "X") ui->profilePlot->xAxis->setRange(_begin.x(), _end.x());
    if(_axis == "Y") ui->profilePlot->xAxis->setRange(_begin.y(), _end.y());

    ui->profilePlot->addGraph();

    //Set up kdf graph
    ui->profilePlot->addGraph();
   //ui->profilePlot->graph(kdf_index)->setPen(QPen(Qt::blue));
    ui->profilePlot->graph(kdf_index)->setPen(Qt::DashLine);

    //Add all the datapoints of the map to the data that is to be plotted.
    QMap<int, int>::const_iterator i = _Axismap.constBegin();
    while( i != _Axismap.constEnd()){
        ui->profilePlot->graph(0)->addData(double(i.key()), double(i.value()));
        i++;
    }

    ui->profilePlot->rescaleAxes();
    ui->profilePlot->replot( QCustomPlot::rpQueued );

    int Ngraphs = editsList.length()+ N_maingraphs;

    //Add (6) dots for the regions of CNR calculation
    for(int i = N_maingraphs; i < Ngraphs; i++){
        ui->profilePlot->addGraph();
        ui->profilePlot->graph(i)->setPen(QPen(Qt::red));
        ui->profilePlot->graph(i)->setLineStyle(QCPGraph::lsNone);
        ui->profilePlot->graph(i)->setScatterStyle(QCPScatterStyle::ssDisc);
        //ui->profilePlot->graph(i)->addData(0, -1); //Unnecessary to initialize them.
    }

    //Add (3) graphs to represent the means of the areas.
    for(int i = 0; i < N_meangraphs; i++){
        ui->profilePlot->addGraph();
        ui->profilePlot->graph(Ngraphs + i)->setPen(QPen(Qt::red));
    }
    //Make sure profilepoints does not contain any points before they are indicated.
    _mpx3gui->getDataset()->clearProfilepoints();
}

void ProfileDialog::addMeanLines(QString data){
    if(data != ""){
        //Split the CNRdata String in the mean values, if calculated.
        QStringList meanlist = data.split("Mean:");
        if(meanlist.length() > 1){
            meanlist = meanlist[1].split("\n");
            meanlist = meanlist[0].split("\t");
            if(meanlist.first() == "" ) meanlist.removeFirst();

            QList<int> profilevector = _mpx3gui->getDataset()->getProfilepoints();
            bool ok;

            //MyGraphs contains the profilegraph, 6 dots and then 3 graphs for the mean lines.
            //Start from the graph after the dots.
            int graphindex = editsList.length() + N_maingraphs;

            //Clear any existing graphdata
            for(int i = 0; i < N_meangraphs; i++)
                ui->profilePlot->graph(graphindex + i)->clearData();

            //Add horizontal lines at the level of the mean for every region.
            for(int i = 0; i < meanlist.length(); i ++){
                QString mean = meanlist.at(i);

                if(meanlist.length() == 2 && profilevector[0] == -1){//The left meanline is not drawn.
                    ui->profilePlot->graph(graphindex + i + 1)->addData(profilevector[i*2+2], mean.toDouble(&ok));
                    ui->profilePlot->graph(graphindex + i +1)->addData(profilevector[i*2+3], mean.toDouble(&ok));
                }
                else if(meanlist.length() == 2 && profilevector[2] == -1);
                else {
                    ui->profilePlot->graph(graphindex + i)->addData(profilevector[i*2], mean.toDouble(&ok));
                    ui->profilePlot->graph(graphindex + i)->addData(profilevector[i*2+1], mean.toDouble(&ok));
                }
            if(!ok)changeText("An error has occured with converting the means to integers.");
            }

            ui->profilePlot->replot(QCustomPlot::rpQueued);
        }
    }
}


void ProfileDialog::useKernelDensityFunction(double bandwidth)
{   //Determine the number of points to be used in the kdf
    int Npoints;
    int begin;
    if (_axis == "X") {
        Npoints = _end.x() - _begin.x() + 1;
        begin = _begin.x();
    }
    if (_axis == "Y"){
        Npoints = abs(_end.y() - _begin.y()) + 1;
        begin = min(_begin.y(), _end.y());
    }

    //Fill the histogram with the right values.
    QVector<double> hist;
    QVector<int>    points;
    QMap<int, int>::const_iterator i = _Axismap.constBegin();
    while( i != _Axismap.constEnd()){
        hist.append(i.value());
        points.append(i.key() - begin);
        i++;
    }

    createKernelDensityFunction(Npoints, hist, bandwidth);

    QVector<double> kdf;
    double value;
    ui->profilePlot->graph(kdf_index)->clearData();
    for(int i = 0; i < Npoints; i++){
        value = GausFuncAdd(points[i], par_v);
        kdf.append(value);
        ui->profilePlot->graph(kdf_index)->addData(i + begin, value);
    }

    ui->profilePlot->rescaleAxes();
    ui->profilePlot->replot(QCustomPlot::rpQueued);

    calcPoints(kdf);
}

void ProfileDialog::createKernelDensityFunction(int Npoints, QVector<double> hist, double bandwidth){
    // Set of parameters for the full kernel density function
    // Npars = m_nbins*( constants + sigmas + means ) + size
    // ex: for 100 bins we need 301 parameters
    par_v.clear();
    par_v.resize(Npoints*3 +1);

    //par = new double[Npoints * 3 + 1]; //We're going to use a QVector to prevent memory leakage
    par_v[0] = Npoints;
    //double distmax = *max_element(hist.begin(), hist.end());

    int i = 1;
    for ( ; i <= Npoints ; i++ ) {

        //par[i] = hist[i-1]; // constant
        //par[i] = hist[i-1] / distmax; // constant
        par_v[i] = hist[i-1] / bandwidth; // constant

        par_v[i + Npoints] = i-1; // mean

        par_v[i + 2*Npoints] = bandwidth; // sigma

    }

//	// Final kernel density
//	TString kernelname = "kernel_";
//	kernelname += m_calhandler->GetSourcename();
//	kernelname += "_";
//	kernelname += pix;
//	//cout << "Creating kernel function : " << kernelname << endl;
//	TF1 * fker = new TF1(kernelname, GausFuncAdd, 0, m_nbins, 3 * m_nbins + 1);
//	fker->SetParameters( par );

//	return fker;
}

double ProfileDialog::GausFuncAdd(double x, QVector<double> par) {

    double xx = x;

    // very first parameter is the nbins which is the
    // same range of the kernel density function
    int N = par[0];

    // C-array "par" contains 3N + 1 parameters
    // N = par[               0 ] : Number of entries per parameter
    // a = par[       1 -->   N ] : N constants
    // b = par[   N + 1 --> 2*N ] : N mean vaues
    // c = par[ 2*N + 1 --> 3*N ] : N sigma values

    // sum func
    double func = 0;
    int Nd = 2*N;
    for ( int i = 0 ; i < N ; i++ ) {
        func += par[ i + 1 ] * exp( - ( ( xx - par[ N + i + 1 ] )*( xx - par[ N + i + 1 ] ) ) / ( 2.*par[ Nd + i + 1 ]*par[ Nd + i + 1 ] ) );
    }

    func *= 0.4;

    return func;
}

void ProfileDialog::calcPoints(QVector<double> function){

    QVector<double> stencil;

    for(int x = 2; x < function.length()-2; x++)
           stencil.append(FivePointsStencil(function, x, 1));
}

double ProfileDialog::FivePointsStencil(QVector<double> func, int x, double bw) {

    double der = 0.;

    der -=      func[ x + 2*bw ];
    der += 8. * func[ x +   bw ];
    der -= 8. * func[ x -   bw ];
    der +=      func[ x - 2*bw ];
    der /= 12.*bw;

    return der;

//	double sten = 0.;

//	sten  =      q.front();     q.pop(); // get oldest element and remove
//	sten -= 8. * q.front();     q.pop();

//	q.pop();                             // middle element not needed in the stencil

//	sten += 8. * q.front();     q.pop();
//	sten -=      q.front();     q.pop();

//	return sten/12.;
}

void ProfileDialog::on_checkBox_toggled(bool checked)
{   //Logarithmic scale
    if(checked){
        ui->profilePlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui->profilePlot->yAxis->setScaleLogBase(10.0);
    }
    else ui->profilePlot->yAxis->setScaleType(QCPAxis::stLinear);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
}

void ProfileDialog::on_CNRbutton_clicked()
{   //CALC CNR CLICKED
    //Assign boundary Profilepoints
    bool inRange = true;
    bool inOrder = true;
    int i;

    for(i = 0 ; i < editsList.length(); i++){
        //For every QLineEdit the value is extracted
        //and passed to ProfilePoints only if it lies within the selected region.
       QString text = editsList[i]->text();
       if(text!= "")
            if(!valueinRange(text.toInt())) inRange = false;

       _mpx3gui->getDataset()->setProfilepoint(i, text);
    }

    QList<int> profilepoints = _mpx3gui->getDataset()->getProfilepoints();
    for(i = 1; i < profilepoints.length(); i++)
            if(profilepoints[i] < profilepoints[i-1]) inOrder = false;

    //And calculate the CNR or display error message.
    if (inOrder && inRange){
    QString CNRdata = _mpx3gui->getDataset()->calcCNR(_Axismap);
    changeText(CNRdata);
    addMeanLines(CNRdata);
    }
    else if(!inOrder) changeText("Please choose points in increasing order.");
    else changeText("At least one boundary point is out of Range or the axis is not properly defined.");
}

void ProfileDialog::on_KDFbutton_clicked()
{   //USE KDF CLICKED

    double bw = ui->bandwidth_edit->text().toDouble();
    if(bw == 0) bw = 1;
    ui->bandwidth_edit->text() = "1";
    changeText("A bandwidth of 0.0 is not allowed, a default value of 1 has been used.");
    useKernelDensityFunction(bw);
}

bool ProfileDialog::valueinRange(int value){
    if(_axis == "X"){
        if(_begin.x() <= _end.x())
            if(value >= _begin.x() && value <= _end.x()) return true;
        else if(value >= _end.x() && value <= _begin.x()) return true;

        else return false;
    }
    if(_axis == "Y"){
        if(_begin.y() <= _end.y())
            if(value >= _begin.y() && value <= _end.y()) return true;
        else if(value >= _end.y() && value <= _begin.y()) return true;
        else return false;
    }
    else changeText("No axis defined.");
}

void ProfileDialog::on_pointEdit_editingFinished(){

    int x;
    QString txt = sender()->objectName().split("_")[1];
    int index = txt.toInt();

    ui->profilePlot->graph(N_maingraphs + index)->clearData();

    QString text = ((QLineEdit *)sender())->text();
    if(! text.isEmpty()){
        x = text.toInt();
        ui->profilePlot->graph(N_maingraphs + index)->addData(x, _Axismap[x]);
        ui->profilePlot->replot(QCustomPlot::rpQueued);
    }
}

//void ProfileDialog::on_pointEdit_0_editingFinished()
//{
//    int x;
//    ui->profilePlot->graph(N_maingraphs)->clearData();

//    QString text = ui->pointEdit_0->text();
//    if(! text.isEmpty()){
//        x = text.toInt();
//        ui->profilePlot->graph(N_maingraphs)->addData(x, _Axismap[x]);
//        ui->profilePlot->replot(QCustomPlot::rpQueued);
//    }

//    int x = ui->pointEdit_0->text().toInt();
//    ui->profilePlot->graph(N_maingraphs)->clearData();
//    ui->profilePlot->graph(N_maingraphs)->addData(x, _Axismap[x]);

//    ui->profilePlot->replot(QCustomPlot::rpQueued);
//}

//void ProfileDialog::on_pointEdit_1_editingFinished()
//{
//    int x = ui->pointEdit_1->text().toInt();
//    ui->profilePlot->graph(N_maingraphs + 1)->clearData();
//    ui->profilePlot->graph(N_maingraphs + 1)->addData(x, _Axismap[x]);

//    ui->profilePlot->replot(QCustomPlot::rpQueued);
//}

//void ProfileDialog::on_pointEdit_2_editingFinished()
//{
//    int x = ui->pointEdit_2->text().toInt();
//    ui->profilePlot->graph(N_maingraphs + 2)->clearData();
//    ui->profilePlot->graph(N_maingraphs + 2)->addData(x, _Axismap[x]);

//    ui->profilePlot->replot(QCustomPlot::rpQueued);
//}

//void ProfileDialog::on_pointEdit_3_editingFinished()
//{
//    int x = ui->pointEdit_3->text().toInt();
//    ui->profilePlot->graph(N_maingraphs + 3)->clearData();
//    ui->profilePlot->graph(N_maingraphs + 3)->addData(x, _Axismap[x]);

//    ui->profilePlot->replot(QCustomPlot::rpQueued);

//}

//void ProfileDialog::on_pointEdit_4_editingFinished()
//{

//    int x = ui->pointEdit_4->text().toInt();
//    ui->profilePlot->graph(N_maingraphs + 4)->clearData();
//    ui->profilePlot->graph(N_maingraphs + 4)->addData(x, _Axismap[x]);

//    ui->profilePlot->replot(QCustomPlot::rpQueued);
//}

//void ProfileDialog::on_pointEdit_5_editingFinished()
//{
//    int x = ui->pointEdit_5->text().toInt();
//    ui->profilePlot->graph(N_maingraphs + 5)->clearData();
//    ui->profilePlot->graph(N_maingraphs + 5)->addData(x, _Axismap[x]);

//    ui->profilePlot->replot(QCustomPlot::rpQueued);
//}


void ProfileDialog::on_comboBox_currentIndexChanged(const QString &arg1)
{
    QString s = arg1;
    s.remove("Threshold", Qt::CaseInsensitive);
    int layerIndex = s.toInt();
    setLayer(layerIndex);
    setAxisMap(_mpx3gui->getDataset()->calcProfile(_axis, layerIndex, _begin, _end));
    plotProfile();
    show();
}

void ProfileDialog::mousePressEvent(QMouseEvent *event)
{   int i;

    //i = _mpx3gui->getDataset()->countProfilepoints();
    i = _mpx3gui->getDataset()->getProfilepoints().indexOf(-1);

    if(event->button() == Qt::LeftButton){
        if(i == -1) {
            changeText("There are already 6 boundary points indicated.");
        }
        else{
            int x = ui->profilePlot->xAxis->pixelToCoord(event->x());
            ui->profilePlot->graph(N_maingraphs + i)->addData(x, _Axismap[x]);
            editsList[i]->setText(QString("%1").arg(x));
            _mpx3gui->getDataset()->setProfilepoint(i, x); //Needed for setting i.
        }
    }
}

void ProfileDialog::closeEvent(QCloseEvent *event)
{
    emit user_accepted_profile();
}

void ProfileDialog::makeEditsList(){

    QList<QLineEdit *> list = ui->groupBox->findChildren<QLineEdit *>();
    for(int i = 0; i < list.length(); i++)
        if(list[i]->objectName().contains("pointEdit")) editsList.append(list[i]);
}

void ProfileDialog::on_clearbutton_clicked()
{
    _mpx3gui->getDataset()->clearProfilepoints();
    for(int i = 0; i < editsList.length(); i++)
        editsList[i]->setText("");
}

