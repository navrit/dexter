#include "profiledialog.h"
#include "ui_profiledialog.h"
#include "qcustomplot.h"

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

//    connect( this, &StatsDialog::user_accepted_stats,
//             _mpx3gui->getVisualization(),
//             &QCstmGLVisualization::on_user_accepted_stats );
}


void ProfileDialog::on_buttonBox_accepted() {
    // talk back to the main gui
    emit user_accepted_profile();
}

void ProfileDialog::changeTitle(QString axis){
    QString T = "Profile in the " + axis + " direction";
    T += QString(" from (%1, %2)-->(%3, %4)").arg(_begin.x()).arg(_begin.y()).arg(_end.x()).arg(_end.y());
    ui->groupBox->setTitle(T);
}

void ProfileDialog::changeText(QString text){
    ui->textBrowser->setText(text);
    //update();
}

void ProfileDialog::plotProfileX()
{
    ui->profilePlot->clearGraphs();
    ui->profilePlot->xAxis->setLabel("x-axis");
    ui->profilePlot->yAxis->setLabel("Total pixel count");
    ui->profilePlot->xAxis->setRange(_begin.x(), _end.x());
    ui->profilePlot->addGraph();

    //Add all the datapoints of the map to the data that is to be plotted.
    QMap<int, int>::const_iterator i = _Axismap.constBegin();
    while( i != _Axismap.constEnd()){
        ui->profilePlot->graph(0)->addData(double(i.key()), double(i.value()));
        i++;
    }

    ui->profilePlot->rescaleAxes();
    ui->profilePlot->replot( QCustomPlot::rpQueued );
}

void ProfileDialog::plotProfileY()
{
    ui->profilePlot->clearGraphs();
    ui->profilePlot->xAxis->setLabel("y-axis");
    ui->profilePlot->yAxis->setLabel("Total pixel count");
    ui->profilePlot->xAxis->setRange(_begin.y(), _end.y());
    ui->profilePlot->addGraph();

    //Add all the datapoints of the map to the data that is to be plotted.
    QMap<int, int>::const_iterator i = _Axismap.constBegin();
    while( i != _Axismap.constEnd()){
        ui->profilePlot->graph(0)->addData(double(i.key()), double(i.value()));
        i++;
    }

    //ui->profilePlot->yAxis->setNumberFormat("b"); //does nothing?
    ui->profilePlot->rescaleAxes();
    ui->profilePlot->replot( QCustomPlot::rpQueued );
}

void ProfileDialog::on_checkBox_toggled(bool checked)
{   //Logarithmic scale
    if(checked){
        ui->profilePlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        ui->profilePlot->yAxis->setScaleLogBase(10.0);
    }
    else ui->profilePlot->yAxis->setScaleType(QCPAxis::stLinear);

    ui->profilePlot->replot();
}

void ProfileDialog::on_pushButton_clicked()
{   //Calc CNR

    QString CNRdata = _mpx3gui->getDataset()->calcCNR(_Axismap);
    changeText(CNRdata);
}

void ProfileDialog::on_comboBox_currentIndexChanged(int index)
{
    //TO DO: Change the layer of which the profile is made, make new plot.

}

void ProfileDialog::on_lineEdit_editingFinished()
{
    _mpx3gui->getDataset()->setProfilepoint(0, ui->lineEdit->text().toInt());
}

void ProfileDialog::on_lineEdit_2_cursorPositionChanged(int arg1, int arg2)
{
    _mpx3gui->getDataset()->setProfilepoint(1, ui->lineEdit_2->text().toInt());
}

void ProfileDialog::on_lineEdit_3_editingFinished()
{
    _mpx3gui->getDataset()->setProfilepoint(2, ui->lineEdit_3->text().toInt());
}

void ProfileDialog::on_lineEdit_4_editingFinished()
{
    _mpx3gui->getDataset()->setProfilepoint(3, ui->lineEdit_4->text().toInt());
}
