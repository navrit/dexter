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

void TestPulses::on_pushButtonSet_clicked()
{

    // Ext trigger
    //int val = ui->spinBox->value();
    //qDebug() << "Setting : " << val;

    // Good configuration for external shutter
    unsigned int val1 = 0x5; // External shutter IN
    val1 = val1;
    unsigned int val2 = 0x4; // Debug shutter (read back)
    val2 = val2 << 8;
    // mask
    unsigned int val = val1 | val2;
    qDebug() << "Setting : " << val;
    _mpx3gui->GetSpidrController()->setSpidrReg(0x0810, val, true);

    // IDELAY
    /*
    int val = ui->spinBox->value();
    qDebug() << "Setting : " << val;
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10A0, val, true);
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10A4, val, true);
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10A8, val, true);
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10AC, val, true);
    */

}
