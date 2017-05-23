#include "thresholdscan.h"
#include "ui_thresholdscan.h"

#include "ui_mpx3gui.h"

#include <QElapsedTimer>


thresholdScan::thresholdScan(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::thresholdScan)
{
    ui->setupUi(this);
}

thresholdScan::~thresholdScan()
{
    delete ui;
}

void thresholdScan::SetMpx3GUI(Mpx3GUI *p)
{
    _mpx3gui = p;

}

void thresholdScan::startScan()
{
    //! Use acquisition settings from other view
    resetScan();

    //! TODO Test this disable/enable spinbox behaviour - offline attempts are not trivial
    disableSpinBoxes();

    //! Initialise variables
    //!
    //! Get values from GUI
    minTH = ui->spinBox_minimum->value();
    maxTH = ui->spinBox_maximum->value();
    thresholdSpacing = ui->spinBox_spacing->value();
    framesPerStep = ui->spinBox_framesPerStep->value();

    //! Print header to log box and start timer
    ui->textEdit_log->append("------------------------------------------");
    auto startTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    ui->textEdit_log->append("Starting Threshold scan @ " +
                             startTime);
    QElapsedTimer timer;
    timer.start();

    //! Do the number of frames loop within the threshold scan
    for (uint i = minTH; i < maxTH; i += 1 + thresholdSpacing){
        for (uint j = 0; j < framesPerStep; j++){

            //! GUI interrupt - stop ASAP
            //! Cleanup code in stopScan()
            if (_stop){
                return;
            }

            //! TODO Take Double counter behaviour into account
            ui->textEdit_log->append("TH:" + QString::number(i) + " frame:" + QString::number(j));

            //! TODO uncomment
            //_mpx3gui->getDataset()->clear();
            startDataTakingThread();
        }
    }

    enableSpinBoxes();

    _running = false;
    ui->button_startStop->setText(tr("Start"));

    //! Print footer to log box and end timer with relevant units
    auto stopTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    ui->textEdit_log->append("Finished Threshold scan @ " +
                             stopTime);

    auto elapsed = timer.elapsed();
    if (elapsed < 1000){
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()) + "ms");
    } else {
        ui->textEdit_log->append("Elapsed time: " + QString::number(timer.elapsed()/1000) + "s");
    }

}

void thresholdScan::stopScan()
{
    enableSpinBoxes();

    //! Print interrupt footer
    ui->textEdit_log->append("GUI Interrupt: Stopping Threshold scan at: " +
                            QDateTime::currentDateTimeUtc().toString(Qt::ISODate) +
                             "\n------------------------------------------");
    _stop = true;
    _running = false;

}

void thresholdScan::resetScan()
{
    _stop = false;
}

void thresholdScan::resumeScan()
{
    //! TODO Figure out how this fits in
}

void thresholdScan::startDataTakingThread()
{
    return;
    //! TODO uncomment
    //_mpx3gui->getVisualization()->StartDataTaking(true);

    //! Note: MUST end function here to return back to Qt event loop
}

void thresholdScan::enableSpinBoxes()
{
    ui->spinBox_minimum->setEnabled(1);
    ui->spinBox_maximum->setEnabled(1);
    ui->spinBox_spacing->setEnabled(1);
    ui->spinBox_framesPerStep->setEnabled(1);
}

void thresholdScan::disableSpinBoxes()
{
    ui->spinBox_minimum->setDisabled(1);
    ui->spinBox_maximum->setDisabled(1);
    ui->spinBox_spacing->setDisabled(1);
    ui->spinBox_framesPerStep->setDisabled(1);
}

void thresholdScan::on_button_startStop_clicked()
{
    if (_running) {
        ui->button_startStop->setText("Start");
        stopScan();
    } else {
        ui->button_startStop->setText("Stop");
        startScan();
    }
}
