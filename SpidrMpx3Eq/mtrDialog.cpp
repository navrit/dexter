#include "mtrDialog.h"
#include "ui_mtrDialog.h"

#include "qcstmplothistogram.h"

MTRDialog::MTRDialog(Mpx3GUI * mg, QWidget * parent) :
    _mpx3gui(mg),
    QDialog(parent),
    ui(new Ui::MTRDialog)
{
    ui->setupUi(this);

    // The Logo
    QPixmap pix("icons/ASI_logo.png");
    //pix = pix.scaled(pix.width() / 10., pix.height() / 10., Qt::KeepAspectRatio);
    pix = pix.scaled(200., 70., Qt::KeepAspectRatio);
    //qDebug() << pix.width() << " " << pix.height();
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
    ui->barChartHisto->AppendSet( cprop, false );
    ui->barChartHisto->setLocale( QLocale(QLocale::English, QLocale::UnitedKingdom) );

    this->setWindowTitle( tr("Color reconstruction input summary") );

    // Defaults
    ui->radioButtonSelPixelsON->setChecked( true );
    _displayMode = __pixelsON;

    //
    _lcds.push_back( ui->lcdNumber0 );
    _lcds.push_back( ui->lcdNumber1 );
    _lcds.push_back( ui->lcdNumber2 );
    _lcds.push_back( ui->lcdNumber3 );
    _lcds.push_back( ui->lcdNumber4 );
    _lcds.push_back( ui->lcdNumber5 );
    _lcds.push_back( ui->lcdNumber6 );
    _lcds.push_back( ui->lcdNumber7 );

    _labels.push_back( ui->thl0Label );
    _labels.push_back( ui->thl1Label );
    _labels.push_back( ui->thl2Label );
    _labels.push_back( ui->thl3Label );
    _labels.push_back( ui->thl4Label );
    _labels.push_back( ui->thl5Label );
    _labels.push_back( ui->thl6Label );
    _labels.push_back( ui->thl7Label );

}

MTRDialog::~MTRDialog()
{
    delete ui;
}

void MTRDialog::timerEvent(QTimerEvent *)
{
    int nLCDs = _lcds.size();
    for ( int i = 0 ; i < nLCDs ; i++ ) {
        QString lcdText = "<b><font color=\"black\" font-size=\"30px\">" + QString::number( _mpx3gui->getDataset()->getActivePixels( i ), 'f', 0) + " </font></b>";
        _lcds.at(i)->setText(lcdText);
    }

    QString sE = "<font color=\"blue\">";
    sE += QString::number( 10.2, 'f', 1 );
    sE += " keV </font>";
    ui->thl0Label->setText( sE );

    sE.clear();
    sE = "<font colour=\"blue\">";
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


    if ( _barCharLogY ) ui->barChartHisto->fitToHeight( __range_min_whenLog );
    else ui->barChartHisto->fitToHeight( );

    // Life feed
    //ui->timePlot
    //ui->barChartHisto->replot( QCustomPlot::rpQueued );

}

void MTRDialog::changePlotsProperties()
{

    switch ( _displayMode ) {
    case __counts:
        ui->barChartHisto->yAxis->setLabel( "Total counts" );
        break;
    case __mean:
        ui->barChartHisto->yAxis->setLabel( "Mean" );
        break;
    case __stdv:
        ui->barChartHisto->yAxis->setLabel( "Standard Deviation" );
        break;
    case __pixelsON:
        ui->barChartHisto->yAxis->setLabel( "Pixels ON" );
        break;
    case __NofClusters:
        ui->barChartHisto->yAxis->setLabel( "Number of clusters" );
        break;

    default:
        ui->barChartHisto->yAxis->setLabel( "Pixels ON" );
        break;
    }

    // replot
    ui->barChartHisto->replot( QCustomPlot::rpQueued );

}


// Oriented to imaging
void MTRDialog::on_radioButtonSelCounts_toggled(bool checked)
{
    if ( checked ) _displayMode = __counts;

    changePlotsProperties();
}

void MTRDialog::on_radioButtonSelMean_toggled(bool checked)
{
    if ( checked ) _displayMode = __mean;

    changePlotsProperties();
}

void MTRDialog::on_radioButtonSelStdv_toggled(bool checked)
{
    if ( checked ) _displayMode = __stdv;

    changePlotsProperties();
}

// Oriented to tracking
void MTRDialog::on_radioButtonSelPixelsON_toggled(bool checked)
{
   if ( checked ) _displayMode = __pixelsON;

   changePlotsProperties();
}

void MTRDialog::on_radioButtonSelNumberOfClusters_toggled(bool checked)
{
    if ( checked ) _displayMode = __NofClusters;

    changePlotsProperties();
}


void MTRDialog::on_barCharLogYCheckBox_clicked(bool checked)
{
    _barCharLogY = checked;

    if ( _barCharLogY ) {
        ui->barChartHisto->yAxis->setScaleType( QCPAxis::stLogarithmic );
        ui->barChartHisto->yAxis->setRangeLower( __range_min_whenLog );
    } else {
        ui->barChartHisto->yAxis->setScaleType( QCPAxis::stLinear );
        ui->barChartHisto->yAxis->setRangeLower( 0 );
    }

    ui->barChartHisto->replot( QCustomPlot::rpQueued );

}

void MTRDialog::on_timePlotLogYCheckBox_clicked(bool checked)
{
    _timePlotLogY = checked;
}
