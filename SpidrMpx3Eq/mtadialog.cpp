#include "mtadialog.h"
#include "ui_mtadialog.h"

MTADialog::MTADialog(Mpx3GUI * mg, QWidget * parent) :
    _mpx3gui(mg),
    QDialog(parent),
    ui(new Ui::MTADialog)
{
    ui->setupUi(this);

    // The Logo
    QPixmap pix("icons/ASI_logo.png");
    //pix = pix.scaled(pix.width() / 10., pix.height() / 10., Qt::KeepAspectRatio);
    pix = pix.scaled(200., 70., Qt::KeepAspectRatio);
    qDebug() << pix.width() << " " << pix.height();
    ui->logoLabel->setPixmap( pix );

    // get the timer going
    this->startTimer( 1000 ); // ms

    ui->lcdNumber0->setPalette(Qt::darkGreen);
    ui->lcdNumber2->setPalette(Qt::darkGreen);
    ui->lcdNumber4->setPalette(Qt::darkGreen);
    ui->lcdNumber6->setPalette(Qt::darkGreen);

    // Histogram
    BarChartProperties cprop;
    cprop.xAxisLabel = "THL";
    cprop.yAxisLabel = "entries";
    cprop.min_x = 0;
    cprop.max_x = 7;
    cprop.nBins = 7;
    cprop.color_r = 0;
    cprop.color_g = 127;
    cprop.color_b = 0;
    ui->barChartHisto->AppendSet( cprop );
    ui->barChartHisto->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );

    this->setWindowTitle( tr("Color reconstruction input summary") );

}

MTADialog::~MTADialog()
{

    delete ui;
}

void MTADialog::timerEvent(QTimerEvent *)
{

    ui->lcdNumber0->display( QString::number( _mpx3gui->getDataset()->getActivePixels( 0 ), 'f', 0 ) );
    ui->lcdNumber2->display( QString::number( _mpx3gui->getDataset()->getActivePixels( 2 ), 'f', 0 ) );
    ui->lcdNumber4->display( QString::number( _mpx3gui->getDataset()->getActivePixels( 4 ), 'f', 0 ) );
    ui->lcdNumber6->display( QString::number( _mpx3gui->getDataset()->getActivePixels( 6 ), 'f', 0 ) );

    QString sE = "<font color=\"blue\">";
    sE += QString::number( 10.2, 'f', 1 );
    sE += " keV </font>";
    ui->thl0Label->setText( sE );

    sE.clear();
    sE = "<font color=\"blue\">";
    sE += QString::number( 20.4, 'f', 1 );
    sE += " keV </font>";
    ui->thl2Label->setText( sE );

    sE.clear();
    sE = "<font color=\"blue\">";
    sE += QString::number( 32.6, 'f', 1 );
    sE += " keV </font>";
    ui->thl4Label->setText( sE );

    sE.clear();
    sE = "<font color=\"blue\">";
    sE += QString::number( 44.1, 'f', 1 );
    sE += " keV </font>";
    ui->thl6Label->setText( sE );

    // Histogram
    ui->barChartHisto->SetValueInSetNonAcc( 0, 0, _mpx3gui->getDataset()->getActivePixels( 0 ) );
    ui->barChartHisto->SetValueInSetNonAcc( 0, 2, _mpx3gui->getDataset()->getActivePixels( 2 ) );
    ui->barChartHisto->SetValueInSetNonAcc( 0, 4, _mpx3gui->getDataset()->getActivePixels( 4 ) );
    ui->barChartHisto->SetValueInSetNonAcc( 0, 6, _mpx3gui->getDataset()->getActivePixels( 6 ) );


    ui->barChartHisto->fitToHeight();

    // Life feed
    //ui->timePlot

}


