#include "statsdialog.h"
#include "ui_statsdialog.h"
#include "mpx3gui.h"
#include "qcstmglvisualization.h"


StatsDialog::StatsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StatsDialog)
{
    ui->setupUi(this);
}

StatsDialog::~StatsDialog()
{
    delete ui;
}

void StatsDialog::SetMpx3GUI(Mpx3GUI * p )
{
    _mpx3gui = p;

    connect( this, &StatsDialog::user_accepted_stats,
             _mpx3gui->getVisualization(),
             &QCstmGLVisualization::on_user_accepted_stats );
}

void StatsDialog::on_buttonBox_accepted() {
    // talk back to the main gui
    emit user_accepted_stats();
}



void StatsDialog::changeText(){

    QString s = " Threshold: \t";

    QList<int> keys = _mpx3gui->getDataset()->getThresholds();

    for(int i =0; i<keys.length(); i++){
        s += QString("\t %1").arg(keys[i]);
    }
    s += "\n Mean: \t ";
    for(int i =0; i<keys.length(); i++){
        s += QString("\t %1").arg(_mpx3gui->getDataset()->bstats.mean_v[i]);
    }
    s += "\n Standard deviation: ";

    for(int i =0; i<keys.length(); i++){
        s += QString("\t %1").arg(_mpx3gui->getDataset()->bstats.stdev_v[i]);
    }
    ui->textBrowser->setText( s );

    QString T = "Statistics on region ";
    T += QString(" (%1, %2)-->(%3, %4)").arg(_mpx3gui->getDataset()->bstats.init_pixel.x()).arg(_mpx3gui->getDataset()->bstats.init_pixel.y()).arg(_mpx3gui->getDataset()->bstats.end_pixel.x()).arg(_mpx3gui->getDataset()->bstats.end_pixel.y());
    ui->groupBox->setTitle(T);
}
