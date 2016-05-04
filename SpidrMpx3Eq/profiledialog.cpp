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
        QStringList datalist = data.split("\n");
        QStringList meanlist = datalist[1].split("\t");
        meanlist.removeFirst();

        QList<int> profilevector = _mpx3gui->getDataset()->getProfilepoints();
        bool ok;

        //Clear any existing graphdata
        for(int i = 7; i < 10; i++)
            ui->profilePlot->graph(i)->clearData();

        //Add horizontal lines at the level of the mean for every region.
        for(int i = 0; i < meanlist.length(); i ++){
            QString mean = meanlist.at(i);
            ui->profilePlot->graph(7+i)->addData(profilevector[i*2], mean.toDouble(&ok)); //MyGraphs contains the profilegraph, 6 dots and then 3 graphs for the mean lines.
            ui->profilePlot->graph(7+i)->addData(profilevector[i*2+1], mean.toDouble(&ok));
        if(!ok)changeText("An error has occured with converting the means to integers.");
        }

        ui->profilePlot->replot(QCustomPlot::rpQueued);
    }
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

    QList<QObject*> list = ui->groupBox->children();
    QList<QLineEdit*> editsList;
    for(int i = 0; i < list.length(); i++){
        if(qobject_cast<QLineEdit*>(list[i]) !=0) editsList.append( qobject_cast<QLineEdit*>(list[i]) );
    }

    for(int i = 0 ; i < editsList.length(); i++){
        //If the child object is a LineEdit, the value is extraced
        //and passed to ProfilePoints only if it lies within the selected region.
       QString text = editsList[i]->text();
       if(text!= ""){
            int value = text.toInt();
            if(valueinRange(value)){
                _mpx3gui->getDataset()->setProfilepoint(i, text);
            }
            else inRange = false;
       }
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
   // _mpx3gui->getDataset()->setProfilepoint(0, ui->lineEdit->text().toInt());

    int x = ui->lineEdit->text().toInt();
    ui->profilePlot->graph(1)->clearData();
    ui->profilePlot->graph(1)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
}

void ProfileDialog::on_lineEdit_2_editingFinished()
{
    //_mpx3gui->getDataset()->setProfilepoint(1, ui->lineEdit_2->text().toInt());

    int x = ui->lineEdit_2->text().toInt();
    ui->profilePlot->graph(2)->clearData();
    ui->profilePlot->graph(2)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
}

void ProfileDialog::on_lineEdit_3_editingFinished()
{
    //_mpx3gui->getDataset()->setProfilepoint(2, ui->lineEdit_3->text().toInt());

    int x = ui->lineEdit_3->text().toInt();
    ui->profilePlot->graph(3)->clearData();
    ui->profilePlot->graph(3)->addData(x, _Axismap[x]);

    ui->profilePlot->replot(QCustomPlot::rpQueued);
}

void ProfileDialog::on_lineEdit_4_editingFinished()
{
    //_mpx3gui->getDataset()->setProfilepoint(3, ui->lineEdit_4->text().toInt());

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

