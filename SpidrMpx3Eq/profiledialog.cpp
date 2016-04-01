#include "profiledialog.h"
#include "ui_profiledialog.h"

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
