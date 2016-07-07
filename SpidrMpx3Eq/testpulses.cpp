#include "testpulses.h"
#include "ui_testpulses.h"

#include "mpx3config.h"

TestPulses::TestPulses(Mpx3GUI * mg,QWidget *parent) :
    _mpx3gui(mg),
    QDialog(parent),
    ui(new Ui::TestPulses)
{
    ui->setupUi(this);

    this->setWindowTitle( tr("Test pulses configuration") );
}

TestPulses::~TestPulses()
{
    delete ui;
}

void TestPulses::on_activateCheckBox_clicked(bool checked)
{

    if ( ! _mpx3gui->getConfig()->isConnected() ) {
        QMessageBox::warning ( this, tr("Activate Test Pulses"), tr( "No device connected." ) );
        ui->activateCheckBox->setChecked( false );
        return;
    }

    if ( checked ) {

        // 1) Configure pixels with testbit
        Mpx3Config::testpulses_config_parameters tp_conf;
        //tp_conf

        // See if there is an equalization present
        _mpx3gui->setTestPulses();



    }

}
