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
        if ( ! _mpx3gui->setTestPulses() ) {
            QMessageBox::warning(this, tr("Test pulses error"),
                                 tr("You need to be connected to the detector"
                                    "\nand the Equalization needs to be loaded."));
            ui->activateCheckBox->setChecked( false );
        }

    }

}

void TestPulses::on_pushButtonSet_clicked()
{

    // Ext trigger
    int val = ui->spinBox->value();
    qDebug() << "Setting : " << val;

    _mpx3gui->GetSpidrController()->setSpidrReg(0x10BC, 200000, true); //

    _mpx3gui->GetSpidrController()->setSpidrReg(0x10C0, val, true);

    // Get the voltage reference
    int devId = 0, nSamples = 10;
    int adc_val;
    _mpx3gui->GetSpidrController()->setDac(
                devId,
                MPX3RX_DAC_RPZ,
                255);

    // The first measurement is never precise
    // Skip the first
    for ( int i = 0 ; i < 2 ; i++ ) {
        _mpx3gui->GetSpidrController()->setSenseDac(
                    devId,
                    MPX3RX_DAC_RPZ
                    );

        _mpx3gui->GetSpidrController()->getDacOut(
                    devId,
                    &adc_val,
                    nSamples);
    }

    // This can be used as the voltage reference
    double adc_volt = (__voltage_DACS_MAX/(double)__maxADCCounts) * (((double)adc_val)/nSamples);
    qDebug() << "RPZ[255] : " << adc_val << " | volts : " << adc_volt;

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
