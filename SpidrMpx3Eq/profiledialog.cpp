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

void ProfileDialog::changeText(QString axis, QPoint pixel_begin, QPoint pixel_end){
    QString T = "Profile in the " + axis + " direction";
    T += QString(" from (%1, %2)-->(%3, %4)").arg(pixel_begin.x()).arg(pixel_begin.y()).arg(pixel_end.x()).arg(pixel_end.y());
    ui->groupBox->setTitle(T);
}

void ProfileDialog::plotProfileX(QPoint pixel_begin, QPoint pixel_end, QMap<int, int> Xmap)
{
    ui->profilePlot->xAxis->setLabel("x-axis");
    ui->profilePlot->yAxis->setLabel("Total pixel count");
    ui->profilePlot->xAxis->setRange(pixel_begin.x(), pixel_end.x());
    ui->profilePlot->addGraph();

    //Add all the datapoints of the map to the data that is to be plotted.
    QMap<int, int>::const_iterator i = Xmap.constBegin();
    while( i != Xmap.constEnd()){
        ui->profilePlot->graph(0)->addData(double(i.key()), double(i.value()));
        i++;
    }

    ui->profilePlot->rescaleAxes();
    ui->profilePlot->replot( QCustomPlot::rpQueued );
}

void ProfileDialog::plotProfileY(QPoint pixel_begin, QPoint pixel_end, QMap<int, int> Ymap)
{
    ui->profilePlot->xAxis->setLabel("y-axis");
    ui->profilePlot->yAxis->setLabel("Total pixel count");
    ui->profilePlot->xAxis->setRange(pixel_begin.y(), pixel_end.y());
    ui->profilePlot->addGraph();

    //Add all the datapoints of the map to the data that is to be plotted.
    QMap<int, int>::const_iterator i = Ymap.constBegin();
    while( i != Ymap.constEnd()){
        ui->profilePlot->graph(0)->addData(double(i.key()), double(i.value()));
        i++;
    }

    ui->profilePlot->rescaleAxes();
    ui->profilePlot->replot( QCustomPlot::rpQueued );
}
