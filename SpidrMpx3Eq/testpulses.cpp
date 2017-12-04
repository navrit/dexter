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

        //! TODO Probably remove this.
//        // 1) Configure pixels with testbit
//        Mpx3Config::testpulses_config_parameters tp_conf;
//        tp_conf

        //! See if there is an equalization present
        //! I have no idea why...
        if ( ! _mpx3gui->setTestPulses() ) {
            QMessageBox::warning(this, tr("Test pulses error"),
                                 tr("You need to be connected to the detector"
                                    "\n and the Equalization needs to be loaded."));
            ui->pushButton_setLength->setChecked(false);


        }
        _mpx3gui->GetSpidrController()->setSpidrReg(0x10C0, 40, true); //! Default 1 us
        _mpx3gui->GetSpidrController()->setSpidrReg(0x10BC, TP_PERIOD, true);

        qDebug() << "[TEST PULSES] SET Default 1 us length, 5 ms period (200 Hz)";
    }

}

void TestPulses::on_pushButton_setPeriod_clicked()
{
    int val = ui->spinBox_period->value();
    qDebug() << "TP Period:" << val *  (25.0/1000000000.0*1000.0) << "ms";
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10BC, val, true); // 200000 x 25ns = 5ms default. Period between TP Switch pulses in 25ns
}

void TestPulses::on_pushButton_setLength_clicked()
{
    int val = ui->spinBox_length->value();
    qDebug() << "TP Length :" << val * (25.0/1000000000.0*1000.0) << "ms";
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
    qDebug() << "ADC value - RPZ[255] : " << adc_val << " | ADC Voltage : " << adc_volt;
}


void TestPulses::on_pushButton_setIDELAY_clicked()
{
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10A0, ui->spinBox_idelay_chip0->value(), true);
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10A4, ui->spinBox_idelay_chip1->value(), true);
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10A8, ui->spinBox_idelay_chip2->value(), true);
    _mpx3gui->GetSpidrController()->setSpidrReg(0x10AC, ui->spinBox_idelay_chip3->value(), true);
}
