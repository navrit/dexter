#include "testpulseequalisation.h"
#include "ui_testpulseequalisation.h"

#include "mpx3config.h"

testPulseEqualisation::testPulseEqualisation(Mpx3GUI * mg, QWidget *parent) :
    _mpx3gui(mg),
    QDialog(parent),
    ui(new Ui::testPulseEqualisation)
{
    ui->setupUi(this);

    this->setWindowTitle( tr("Test pulses configuration") );

    //! Set UI to whatever the header file initialises them as
    ui->spinBox_injectionCharge->setValue( config.injectionChargeInElectrons );
    ui->comboBox_injectionChargeUnits->setCurrentIndex( injectionChargeUnits );
    ui->spinBox_testPulseLength->setValue( config.testPulseLength );
    ui->spinBox_testPulsePeriod->setValue( config.testPulsePeriod );
    ui->spinBox_pixelSpacing->setValue( config.pixelSpacing );
    ui->comboBox_verbosity->setCurrentIndex( verbosity );

    //! Seems redundant but isn't because it's not relying on the signal slot joining up by name... this is more robust to name changing
    connect(ui->spinBox_injectionCharge, SIGNAL(valueChanged(int)), this, SLOT(on_spinBox_injectionCharge_valueChanged(int)));
    connect(ui->comboBox_injectionChargeUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBox_injectionChargeUnits_currentIndexChanged(int)));
    connect(ui->spinBox_testPulseLength, SIGNAL(valueChanged(int)), this, SLOT(on_spinBox_testPulseLength_valueChanged(int)));
    connect(ui->spinBox_testPulsePeriod, SIGNAL(valueChanged(int)), this, SLOT(on_spinBox_testPulsePeriod_valueChanged(int)));
    connect(ui->spinBox_pixelSpacing, SIGNAL(valueChanged(int)), this, SLOT(on_spinBox_pixelSpacing_valueChanged(int)));
    connect(ui->comboBox_verbosity, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBox_verbosity_currentIndexChanged(int)));
}

testPulseEqualisation::~testPulseEqualisation()
{
    delete ui;
}

bool testPulseEqualisation::activate(int startPixelOffset)
{
    if ( ! _mpx3gui->getConfig()->isConnected() ) {
        QMessageBox::warning ( this, tr("Activate Test Pulses"), tr( "No device connected." ) );
        return false;
    }

    QVector<int> activeChips = _mpx3gui->getConfig()->getActiveDevices();

    if ( _mpx3gui->equalizationLoaded() ) {

        SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
        QCstmEqualization * _equalisation = _mpx3gui->getEqualization();

        //! Very basic error check
        if (spidrcontrol == nullptr || _equalisation == nullptr) return false;

        //! Set some SPIDR registers, these should match the setTpFrequency call
        spidrcontrol->setSpidrReg(0x10C0, config.testPulseLength, true);
        spidrcontrol->setSpidrReg(0x10BC, config.testPulsePeriod, true);

        //! Set Test Pulse frequency (millihertz!) Eg. --> 40000 * 25 ns = 1 ms = 1000 Hz
        //! Set Pulse width: 400 --> 10 us default
        spidrcontrol->setTpFrequency(true, config.testPulsePeriod, config.testPulseLength );

        QMap<int, Mpx3EqualizationResults *>  eqMap_L = _equalisation->getEqMap();
        QMap<int, Mpx3EqualizationResults *>  eqMap_H = _equalisation->getEqMap();

        for ( int chipID = 0; chipID < activeChips.size(); chipID++ ) {
            pair<int, int> pix;
            bool testbit = false;
            int testBitsOn = 0;

            //! Get equalisation results because we have to submit these again
            Mpx3EqualizationResults * eqResults_L = eqMap_L[chipID];
            Mpx3EqualizationResults * eqResults_H = eqMap_H[chipID];

            //! Turn test pulse bit on for that chip
            spidrcontrol->setInternalTestPulse(chipID, true);

            turnOffAllCTPRs(spidrcontrol, chipID, false);

            for ( int i = startPixelOffset; i < __matrix_size; i++ ) {
                pix = _mpx3gui->XtoXY(i, __array_size_x);

                //! Unmask all pixels that we are going to inject test pulses into.
                //! --> mask all pixels that we aren't using

                if ( pix.first % config.pixelSpacing == 0 && pix.second % config.pixelSpacing == 0 ) {
                    testbit = true;
                    spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, false);
                    //qDebug() << "[TEST PULSES] Config CTPR on (x,y): (" << pix.first << "," << pix.second << ")";
                } else {
                    testbit = false;
                    spidrcontrol->setPixelMaskMpx3rx(pix.first, pix.second, true);
                }

                if ( testbit && pix.second == 0 ) {
                    spidrcontrol->configCtpr( chipID, pix.first, 1 );
                }

                spidrcontrol->configPixelMpx3rx(pix.first,
                                                pix.second,
                                                eqResults_L->GetPixelAdj(i),
                                                eqResults_H->GetPixelAdj(i, Mpx3EqualizationResults::__ADJ_H),
                                                testbit);
                if ( testbit ) testBitsOn++;
            }
            spidrcontrol->setCtpr( chipID );

            qDebug() << "[TEST PULSES] CTPRs set on dev" << chipID;
            qDebug() << "[TEST PULSES] Number of pixels testBit ON :"<< testBitsOn;

            spidrcontrol->setPixelConfigMpx3rx( chipID );
        }
        return true;

    } else {
        deactivate();
        return false;
    }
}

bool testPulseEqualisation::deactivate()
{
    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    for ( int chipID = 0; chipID < _mpx3gui->getConfig()->getActiveDevices().size(); chipID++ ) {

        //! 1. Turn off test pulses
        //! 2. Unmask all pixels
        //! 3. Turn off all CTPRs
        //! 4. Set Pixel config

        spidrcontrol->setInternalTestPulse(chipID, false);
        for (int i = 0; i < __array_size_x; i++ ) {
            for(int j = 0; j < __array_size_y; j++ ) {
                spidrcontrol->setPixelMaskMpx3rx(i, j, false);
            }
        }
        turnOffAllCTPRs(spidrcontrol, chipID, true);
        spidrcontrol->setPixelConfigMpx3rx( chipID );
    }
}

void testPulseEqualisation::on_spinBox_injectionCharge_valueChanged(int arg1)
{
    config.injectionChargeInElectrons = arg1;
}

void testPulseEqualisation::on_comboBox_injectionChargeUnits_currentIndexChanged(int index)
{
    if (index == 0 ){
        injectionChargeUnits = electrons;
    } else if (index == 1 ){
        injectionChargeUnits = KeV_Si;
    } else {
        injectionChargeUnits = electrons;
    }
    ui->spinBox_injectionCharge->setValue( config.injectionChargeInElectrons );
}

void testPulseEqualisation::on_spinBox_testPulseLength_valueChanged(int arg1)
{
    config.testPulseLength = arg1;
}

void testPulseEqualisation::on_spinBox_testPulsePeriod_valueChanged(int arg1)
{
    config.testPulsePeriod = arg1;
}

void testPulseEqualisation::on_spinBox_pixelSpacing_valueChanged(int arg1)
{
    config.pixelSpacing = arg1;
}

void testPulseEqualisation::on_comboBox_verbosity_currentIndexChanged(int index)
{
    qDebug() << "[INFO]\tChanged test pulse verbosity to :" << index << "--> 0 is LOW, 1 is HIGH";
    if ( index == 0 ){
        verbosity = LOW;
    } else if ( index == 1) {
        verbosity = HIGH;
    } else {
        verbosity = LOW;
    }
}

bool testPulseEqualisation::estimate_V_TP_REF_AB(uint electrons)
{

}

void testPulseEqualisation::turnOffAllCTPRs(SpidrController *spidrcontrol, int chipID, bool submit)
{
    //! Turn off all CTPRs by default and submit to chip for a clean start
    for (int column = 0; column < __array_size_x; column++ ) {
        spidrcontrol->configCtpr( chipID, column, 0 );
    }

    if (submit) {
        spidrcontrol->setCtpr( chipID );
    }
}
