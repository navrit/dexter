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

    //Add all the datapoints of the map to the data that is to be plotted.
    QMap<int, int>::const_iterator i = _Axismap.constBegin();
    while( i != _Axismap.constEnd()){
        ui->profilePlot->graph(0)->addData(double(i.key()), double(i.value()));
        i++;
    }

    ui->profilePlot->rescaleAxes();
    ui->profilePlot->replot( QCustomPlot::rpQueued );

    //Add 6 dots for the regions of CNR calculation
    for(int i = 1; i <= 6; i++){
        ui->profilePlot->addGraph();
        ui->profilePlot->graph(i)->setPen(QPen(Qt::red));
        ui->profilePlot->graph(i)->setLineStyle(QCPGraph::lsNone);
        ui->profilePlot->graph(i)->setScatterStyle(QCPScatterStyle::ssDisc);
        //ui->profilePlot->graph(i)->addData(0, -1); //Unnecessary to initialize them.
    }

    //Add 3 graphs to represent the means of the 3 areas.
    for(int i = 1; i <= 3; i++){
        ui->profilePlot->addGraph();
        ui->profilePlot->graph(6+i)->setPen(QPen(Qt::red));
    }

}

void ProfileDialog::addMeanLines(QString data){
    if(data != ""){
        QStringList meanlist = data.split("\n");
        meanlist = meanlist[1].split("\t");
        meanlist.removeFirst();

        QList<int> profilevector = _mpx3gui->getDataset()->getProfilepoints();
        bool ok;

        //Clear any existing graphdata
        //MyGraphs contains the profilegraph, 6 dots and then 3 graphs for the mean lines.
        for(int i = 7; i < 10; i++)
            ui->profilePlot->graph(i)->clearData();

        //Add horizontal lines at the level of the mean for every region.
        for(int i = 0; i < meanlist.length(); i ++){
            QString mean = meanlist.at(i);

            if(meanlist.length() == 2 && profilevector[0] == -1){//The left meanline is not drawn.
                ui->profilePlot->graph(8+i)->addData(profilevector[i*2+2], mean.toDouble(&ok));
                ui->profilePlot->graph(8+i)->addData(profilevector[i*2+3], mean.toDouble(&ok));
            }
            else {
                ui->profilePlot->graph(7+i)->addData(profilevector[i*2], mean.toDouble(&ok));
                ui->profilePlot->graph(7+i)->addData(profilevector[i*2+1], mean.toDouble(&ok));
            }
        if(!ok)changeText("An error has occured with converting the means to integers.");
        }

        ui->profilePlot->replot(QCustomPlot::rpQueued);
    }
    else ; //TODO
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

void ProfileDialog::on_pushButton_clicked()
{   //Assign boundary Profilepoints
    bool inRange = true;

    for(int i = 0 ; i < editsList.length(); i++){
        //For every QLineEdit the value is extracted
        //and passed to ProfilePoints only if it lies within the selected region.
       QString text = editsList[i]->text();
       if(text!= "")
            if(!valueinRange(text.toInt())) inRange = false;

       _mpx3gui->getDataset()->setProfilepoint(i, text);
    }

    //And calculate the CNR or display error message.
    if (inRange){
    QString CNRdata = _mpx3gui->getDataset()->calcCNR(_Axismap);
    changeText(CNRdata);
    addMeanLines(CNRdata);
    }
    else changeText("At least one boundary point is out of Range or the axis is not properly defined.");
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

void ProfileDialog::on_comboBox_currentIndexChanged(int index)
{
    //TO DO: Change the layer of which the profile is made, make new plot.

}

void ProfileDialog::on_lineEdit_editingFinished()
{
    int x = ui->lineEdit->text().toInt();
    ui->profilePlot->graph(1)->clearData();
    ui->profilePlot->graph(1)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
}

void ProfileDialog::on_lineEdit_2_editingFinished()
{
    int x = ui->lineEdit_2->text().toInt();
    ui->profilePlot->graph(2)->clearData();
    ui->profilePlot->graph(2)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
}

void ProfileDialog::on_lineEdit_3_editingFinished()
{
    int x = ui->lineEdit_3->text().toInt();
    ui->profilePlot->graph(3)->clearData();
    ui->profilePlot->graph(3)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
}

void ProfileDialog::on_lineEdit_4_editingFinished()
{
    int x = ui->lineEdit_4->text().toInt();
    ui->profilePlot->graph(4)->clearData();
    ui->profilePlot->graph(4)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);

}

void ProfileDialog::on_lineEdit_5_editingFinished()
{
    int x = ui->lineEdit_5->text().toInt();
    ui->profilePlot->graph(5)->clearData();
    ui->profilePlot->graph(5)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
}

void ProfileDialog::on_lineEdit_6_editingFinished()
{
    int x = ui->lineEdit_6->text().toInt();
    ui->profilePlot->graph(6)->clearData();
    ui->profilePlot->graph(6)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
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

    //i = _mpx3gui->getDataset()->countProfilepoints();
    i = _mpx3gui->getDataset()->getProfilepoints().indexOf(-1);

    if(event->button() == Qt::LeftButton){
        if(i == 6) {
            //Do nothing for now.
//            i = 1;
//            for(int j = 1; j <= 6; j++)
//                ui->profilePlot->graph(j)->clearData();
        }
        else{
            int x = ui->profilePlot->xAxis->pixelToCoord(event->x());
            ui->profilePlot->graph(i+1)->addData(x, _Axismap[x]);
            editsList[i]->setText(QString("%1").arg(x));
            _mpx3gui->getDataset()->setProfilepoint(i, x); //Get's done by on_pushbutton_clicked()

        }
    }
}

void ProfileDialog::closeEvent(QCloseEvent *event)
{
    emit user_accepted_profile();
}

void ProfileDialog::makeEditsList(){ //Depends on the ui layoutstructure...
    QList<QObject*> list = ui->groupBox_2->children();
    for(int i = 0; i < list.length(); i++){
        if(qobject_cast<QLineEdit*>(list[i]) != 0) editsList.append( qobject_cast<QLineEdit*>(list[i]) );
    }
    list = ui->groupBox_3->children();
    for(int i = 0; i < list.length(); i++){
        if(qobject_cast<QLineEdit*>(list[i]) != 0) editsList.append( qobject_cast<QLineEdit*>(list[i]) );
    }
    list = ui->groupBox_4->children();
    for(int i = 0; i < list.length(); i++){
        if(qobject_cast<QLineEdit*>(list[i]) != 0) editsList.append( qobject_cast<QLineEdit*>(list[i]) );
    }
}
