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
        connect( editsList[i], SIGNAL(editingFinished()), this, SLOT(onpointEdit_editingFinished()));

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
    QString T = "Profile ";
    T += QString("of (%1, %2) --> (%3, %4)").arg(_begin.x()).arg(_begin.y()).arg(_end.x()).arg(_end.y());
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
                    ui->profilePlot->graph(graphindex + i + 1)->addData(profilevector[i*N_left + N_left], mean.toDouble(&ok));
                    ui->profilePlot->graph(graphindex + i +1)->addData(profilevector[i*N_left + N_left + 1], mean.toDouble(&ok));
                }
                else if(meanlist.length() == 2 && profilevector[N_left] == -1); //Two backgrounds...
//                else if(meanlist.length() == 2){
//                    ui->profilePlot->graph(graphindex + i + 1)->addData(profilevector[i*N_left + N_left], mean.toDouble(&ok));
//                    ui->profilePlot->graph(graphindex + i +1)->addData(profilevector[i*N_left + N_left + 1], mean.toDouble(&ok));
//                }
                else if(meanlist.length() >= 2){
                    ui->profilePlot->graph(graphindex + i)->addData(profilevector[i*N_left], mean.toDouble(&ok));
                    ui->profilePlot->graph(graphindex + i)->addData(profilevector[i*N_left + 1], mean.toDouble(&ok));
                }
                else {
                    //Do nothing if there is only one mean.
                }

            if(!ok)
                changeText("An error has occured with converting the means to integers.");
            }

            ui->profilePlot->replot(QCustomPlot::rpQueued);
        }
    }
}


void ProfileDialog::useKernelDensityFunction(double bandwidth)
{   //Determine the number of points to be used in the kdf
    int Npoints;
    int begin;
    int bw = bandwidth;

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

    //Fill parameter vector.
    createKernelDensityFunction(Npoints, hist, bandwidth);

    ui->profilePlot->graph(kdf_index)->clearData();

    QVector<double> kdf;
    double value;
    int kdf_offset = 10;

    //Calculating kdf values and adding them to a graph and vector.
    for(int i = kdf_offset; i < Npoints - kdf_offset ; i++){
        value = GausFuncAdd(points[i], par_v);
        kdf.append(value);
            ui->profilePlot->graph(kdf_index)->addData(i + begin, value);
    }

    int offset = begin + kdf_offset + bw*2;  //bw*2 because of the 5-point stencil.

    //First derivative for calculating the max (or min).
    int maxpt = 0;
    QVector<int> maxpts;
    QVector<double> stencil = calcPoints(kdf, 1, bw);

    for(int i = 1; i < stencil.length(); i++){
        if(stencil[i] > 0 && stencil[i-1] < 0) maxpts.append(offset + i);
        if(stencil[i] < 0 && stencil[i-1] > 0) maxpts.append(offset + i);
    }
    if(maxpts.length() == 1) maxpt = maxpts[0];

    else if(maxpts.length() >= 3){//Take the extremum closest to the middle, left or right, respectively.
        if(_left && _right) maxpt = maxpts[ closestToAt( maxpts, begin + Npoints/2 ) ];
        else if(_left) maxpt = maxpts[ closestToAt( maxpts, begin ) ];
        else if(_right) maxpt = maxpts [ closestToAt( maxpts, begin + Npoints ) ];
    }

    else {  //= 0 maxima. Just take the middle, beginning or end of the profile.
        if (_left && _right) maxpt = begin + Npoints/2;
        else if (_left) maxpt = begin;
        else if (_right) maxpt = begin + Npoints;
    }


    //second derivative with seperate five-point-stencil calculation
    QVector<double> stencil3 = calcPoints(kdf, 2, bw);
    //Draw
    QVector<int> infls; //Save inflection points
    for(int i = 1; i < stencil3.length(); i++){
        //ui->profilePlot->graph(11)->addData(i + offset, stencil3[i]*10 + kdf[kdf.length()/2]); //* +kdf[middle] for visualization
        if(stencil3[i] > 0 && stencil3[i-1] < 0)
            infls.append(i + offset);
        if(stencil3[i] < 0 && stencil3[i-1] > 0)
            infls.append(i + offset);
    }

    setInflectionPoints(infls, begin, Npoints, maxpt);

    ui->profilePlot->rescaleAxes();
    ui->profilePlot->replot(QCustomPlot::rpQueued);
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

QVector<double> ProfileDialog::calcPoints(QVector<double> function, int Nder, int bw){

    QVector<double> stencil;
    if(Nder == 1)
        for(int x = 2*bw; x < function.length()- 2*bw; x++)
           stencil.append(FivePointsStencil(function, x, bw));

    if(Nder == 2)
        for(int x = 2*bw; x < function.length()- 2*bw; x++)
           stencil.append(secderFivePointsStencil(function, x, bw));

    return stencil;
}

double ProfileDialog::FivePointsStencil(QVector<double> func, int x, double bw) {

    double der = 0.;

    der -=      func[ x + 2*bw ];
    der += 8. * func[ x +   bw ];
    der -= 8. * func[ x -   bw ];
    der +=      func[ x - 2*bw ];
    der /= 12.*bw;

    return der;
}

double ProfileDialog::secderFivePointsStencil(QVector<double> func, int x, double bw) {

    double der = 0.;

    der -=       func[ x + 2*bw ];
    der += 16. * func[ x +   bw ];
    der -= 30. * func[ x        ];
    der += 16. * func[ x -   bw ];
    der -=       func[ x - 2*bw ];
    der /= 12.*bw*bw;

    return der;
}

void ProfileDialog::setInflectionPoints(QVector<int> infls, int begin, int Npoints, int maxpt)
{   int i;
    int stepin_division = 3;
    for(i = 0; i < 6; i++) ui->profilePlot->graph(N_maingraphs + i)->clearData();

    QVector<int> points;

    if(infls.length() == 4){
        //Remove the point furthest from the top. (Do this twice.)
        //Might give faulty points, but better than doing nothing.
        int dmax = 0;
        int imax = 0;
        for(i = 0; i < infls.length(); i++){
            int d = abs(maxpt - infls[i]);
            if(d > dmax){
                dmax = d;
                imax = i;
            }
        }
        infls.removeAt(imax);
        changeText("You might want to adjust one or more of the boundary points.");
    }

    if(infls.length() == 3){    //Remove the point furthest from the top.
        int dmax = 0;
        int imax = 0;
        for(i = 0; i < infls.length(); i++){
            int d = abs(maxpt - infls[i]);
            if(d > dmax){
                dmax = d;
                imax = i;
            }
        }
        infls.removeAt(imax);
        changeText("You might want to adjust one or more of the boundary points.");
    }

    if(infls.length() == 2){

        int offset = ((maxpt - infls[0]) + (infls[1] - maxpt)) / (2*stepin_division); //Divide and take the mean (divide by 2).

        points.append(               begin);
        points.append(   infls[0] - offset);
        points.append(   infls[0] + offset);
        points.append(   infls[1] - offset);
        points.append(   infls[1] + offset);
        points.append( begin + Npoints - 1);

//        for(i = 0; i < 6; i++){
//            ui->profilePlot->graph(N_maingraphs + i)->addData( points[i], _Axismap.value(points[i]) );
//            //_mpx3gui->getDataset()->setProfilepoint(i, points.at(i)); //unnecessary. Done by on_CNRbutton_clicked.
//            editsList[i]->setText(QString::number(points.at(i)));
//        }
    }
    if(infls.length() == 1){//Try using only two regions
        int offset = abs(maxpt - infls[0])/stepin_division;

        points.append(               begin);
        points.append(   infls[0] - offset);
        points.append(   infls[0] + offset);
        //
        points.append( begin + Npoints - 1);

        changeText("You might want to adjust one or more of the boundary points.");

        if(!_left)
            for(i = 0; i < points.length(); i++){
                ui->profilePlot->graph(N_maingraphs + i + N_left)->addData( points[i], _Axismap.value(points[i]) );
                editsList[i + N_left]->setText(QString::number(points.at(i)));
            }
    }

    if (_left)
        for(i = 0; i < points.length(); i++){
        ui->profilePlot->graph(N_maingraphs + i)->addData( points[i], _Axismap.value(points[i]) );
        //_mpx3gui->getDataset()->setProfilepoint(i, points.at(i)); //unnecessary. Done by on_CNRbutton_clicked.
        editsList[i]->setText(QString::number(points.at(i)));
        }
}

int ProfileDialog::farthestFromAt(QVector<int> list, int x)
{
    int dmax = 0;
    int imax;
    int d;
    for(int i = 0; i < list.length(); i++){
        d = abs(x - list[i]);
        if(d > dmax){
            dmax = d;
            imax = i;
        }
    }
    return imax;
}

int ProfileDialog::closestToAt(QVector<int> list, int x)
{
    int dmin = x;
    int imin;
    int d;
        for(int i = 0; i < list.length(); i++){
            d = abs(x - list[i]);
            if(d < dmin){
                dmin = d;
                imin = i;
            }
        }
    return imin;
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
            if(profilepoints[i] != -1 && profilepoints[i] < profilepoints[i-1]) inOrder = false;

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
    if(bw == 0){ bw = 1;
        ui->bandwidth_edit->setText("1");
        changeText("A bandwidth of 0.0 is not allowed, a default value of 1 has been used.");
    }
    useKernelDensityFunction(bw);
}

void ProfileDialog::on_clearbutton_clicked()
{   int i;
    _mpx3gui->getDataset()->clearProfilepoints();
    for(i = 0; i < editsList.length(); i++){
        editsList[i]->setText("");
        ui->profilePlot->graph(N_maingraphs + i)->clearData();
    }
    for(i = 0; i < N_meangraphs; i++)
        ui->profilePlot->graph(N_maingraphs + editsList.length() + i)->clearData();

    ui->profilePlot->replot(QCustomPlot::rpQueued);

    ui->textBrowser->clear();
}

bool ProfileDialog::valueinRange(int value){
    if(_axis == "X"){
        if(_begin.x() <= _end.x()) {
            if(value >= _begin.x() && value <= _end.x()) return true;
        }
        else if(value >= _end.x() && value <= _begin.x()) return true;

        return false;
    }
    if(_axis == "Y"){
        if(_begin.y() <= _end.y()) {
            if(value >= _begin.y() && value <= _end.y()) return true;
        }
        else if(value >= _end.y() && value <= _begin.y()) return true;

        return false;
    }
    changeText("No axis defined.");
    return false;
}

void ProfileDialog::onpointEdit_editingFinished(){

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

    i = _mpx3gui->getDataset()->getProfilepoints().indexOf(-1);

    if(event->button() == Qt::LeftButton){
        if(i == -1) {
            changeText("There are already 6 boundary points indicated.");
        }
        else{
            int x = ui->profilePlot->xAxis->pixelToCoord(event->x());
            ui->profilePlot->graph(N_maingraphs + i)->addData(x, _Axismap[x]);
            editsList[i]->setText(QString("%1").arg(x));
            _mpx3gui->getDataset()->setProfilepoint(i, x); //Needed for setting i on next mousepress-event.
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


void ProfileDialog::on_checkBox_left_toggled(bool checked)
{
    _left = checked;
}

void ProfileDialog::on_checkBox_right_toggled(bool checked)
{
    _right = checked;

}

void ProfileDialog::on_select_xy_currentIndexChanged(int index)
{

    if (index==0) _axis = "X";
    if (index==1) _axis = "Y";

    QString s = ui->comboBox->currentText();
    s.remove("Threshold", Qt::CaseInsensitive);
    int layerIndex = s.toInt();
    setLayer(layerIndex);
    setAxisMap(_mpx3gui->getDataset()->calcProfile(_axis, layerIndex, _begin, _end));
    plotProfile();

    on_clearbutton_clicked();

    show();
}
