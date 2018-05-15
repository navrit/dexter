#include "testpulseequalisation.h"
#include "ui_testpulseequalisation.h"

#include "ui_mpx3gui.h"
#include "mpx3config.h"
#include "testpulseequalisation.h"
#include "testpulseequalisation.h"

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
    ui->checkBox_setDACs->setChecked( setDACs );

    ui->spinBox_injectionCharge->setMaximum( maximumInjectionElectrons );

    //! Seems redundant but isn't because it's not relying on the signal slot joining up by name... this is more robust to name changing
    connect(ui->spinBox_injectionCharge, SIGNAL(valueChanged(int)), this, SLOT(on_spinBox_injectionCharge_valueChanged(int)));
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
    if (_mpx3gui->equalizationLoaded()) {

        if (!initialise()) {
            qDebug() << "[FAIL]\tCould not initialise test pulses";
            return false;
        }

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

                if ( uint(pix.first) % config.pixelSpacing == 0 && uint(pix.second) % config.pixelSpacing == 0 ) {
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

            qDebug() << "[TEST PULSES] CTPRs set on chip" << chipID;
            qDebug() << "[TEST PULSES] Number of pixels testBit ON :"<< testBitsOn;

            spidrcontrol->setPixelConfigMpx3rx( chipID );
        }
        return true;

    } else {
        deactivate();
        return false;
    }
}

bool testPulseEqualisation::initialise()
{
    if ( ! _mpx3gui->getConfig()->isConnected() ) {
        QMessageBox::warning ( this, tr("Activate Test Pulses"), tr( "No device connected." ) );
        return false;
    }

    spidrcontrol = _mpx3gui->GetSpidrController();
    _equalisation = _mpx3gui->getEqualization();

    //! Very basic error check
    //if (spidrcontrol == nullptr || _equalisation == nullptr) return false;

    if ( !estimate_V_TP_REF_AB( config.injectionChargeInElectrons, true ) ) {
        qDebug() << "[FAIL]\tCould not set TP DAC values by voltage by scanning";
        return false;
    }

    activeChips = _mpx3gui->getConfig()->getActiveDevices();

    //! Set some SPIDR registers, these should match the setTpFrequency call
    spidrcontrol->setSpidrReg(0x10C0, int(config.testPulseLength), true);
    spidrcontrol->setSpidrReg(0x10BC, int(config.testPulsePeriod), true);

    //! Set Test Pulse frequency (millihertz!) Eg. --> 40000 * 25 ns = 1 ms = 1000 Hz
    //! Set Pulse width: 400 --> 10 us default
    spidrcontrol->setTpFrequency(true, int(config.testPulsePeriod), int(config.testPulseLength));

    return true;
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

bool testPulseEqualisation::estimate_V_TP_REF_AB(uint electrons, bool makeDialog)
{
    //! Set V_TP_REF and V_TP_REF(A/B) based on measurements
    //!
    //! Fail if it cannot be set to the requested injection charge
    //!
    //!    I should limit the GUI input so you can't enter a value above the maximum injection voltage for the linear range of V_TP_REF (300 mV - 1275 mV)
    //!    Ie. 1.275 V (max value) - 0.300 V = 0.975 V is the maximum injection voltage.
    //!
    //!       Checked this, it seems like ~880 mV is the practical limit for real chips
    //!
    //!    Design specification claims C_test = ~5fF.
    //!         Rafa: Typical tolerances are ~20% for this.
    //!         Speculation: Actual value should be very similar over the whole chip. Don't know how to confim this...
    //!
    //!    Electron to V conversion --> Q = CV
    //!                                 # of electrons * e = 5 fF * V
    //!
    //!    Maximum number of electrons is therefore (for the chips):
    //!         # of electrons = 5 fF * 0.975 V / e
    //!                        = 30430.7...
    //!                        = 30431 (rounded up)
    //!    If you check this, the SPIDR only allows you to set up to ~28000, so that's the maximum you can set.
    //!

    //! Set V_TP_REF   to 300 mV
    //!     V_TP_REF_A to 300 mV + (# of electrons * e / 5fF)
    //!     V_TP_REF_B to 300 mV + (# of electrons * e / 5fF)


    const double requestedInjectionVoltage = 0.3 + (electrons * e_dividedBy_c_test);

    if ( requestedInjectionVoltage < 0.3 || requestedInjectionVoltage >= 1.20 ) {
        qDebug() << "[FAIL]\tRequested injection voltage out of range";
        return false;
    }
    if ( requestedInjectionVoltage >= 1.15 ) {
        qDebug() << "[WARN]\tThis could fail, try setting a lower value";
    }

    qDebug() << "[INFO]\tTest pulse equalisation --> Requested injection voltage for TP_REF_A and TP_REF_B:" << requestedInjectionVoltage;

    //! Make a modal progress bar as well for visual updates
    int totalDACsToSet = _mpx3gui->getConfig()->getNActiveDevices() * 3;
    int numberOfDACsSet = 0;
    QProgressDialog progress("Setting test pulse DACs...", QString(), 0, totalDACsToSet, this);
    progress.setModal(true);
    progress.setMinimum(0);
    progress.setMaximum( totalDACsToSet );
    progress.show();

    //! We need to scan for these
    for (int chipID=0; chipID < _mpx3gui->getConfig()->getNActiveDevices(); chipID++) {
        qApp->processEvents();

        setDACToVoltage(chipID, MPX3RX_DAC_TP_REF, float(0.3));
        numberOfDACsSet++;
        progress.setValue( numberOfDACsSet );
        qApp->processEvents();

        setDACToVoltage(chipID, MPX3RX_DAC_TP_REF_A, requestedInjectionVoltage);
        numberOfDACsSet++;
        progress.setValue( numberOfDACsSet );
        qApp->processEvents();

        setDACToVoltage(chipID, MPX3RX_DAC_TP_REF_B, requestedInjectionVoltage);
        numberOfDACsSet++;
        progress.setValue( numberOfDACsSet );
        qApp->processEvents();

        //! Always just set these to defaults
        SetDAC_propagateInGUI(chipID, MPX3RX_DAC_TP_BUF_IN, int(dacConfig.V_TP_BufferIn));
        SetDAC_propagateInGUI(chipID, MPX3RX_DAC_TP_BUF_OUT, int(dacConfig.V_TP_BufferOut));
    }
    progress.setValue( totalDACsToSet );

    return true;
}

uint testPulseEqualisation::setDACToVoltage(int chipID, int dacCode, double V)
{
    uint dac_val = 0;
    bool foundTarget = false;
    int adc_val = 0;
    double adc_volt = 0;
    int nSamples = 1;
    double lowerVboundary = V * 0.99;
    double upperVboundary = V * 1.01;

    if ( !spidrcontrol ) {
        spidrcontrol = _mpx3gui->GetSpidrController();
    }

    //! Give them some reasonable starting values to save time
    //! Get these from this existing values in the config unless I know better ;)
    if (dacCode == MPX3RX_DAC_TP_REF)   dac_val = 60;
    if (dacCode == MPX3RX_DAC_TP_REF_A) dac_val = _mpx3gui->getConfig()->getDACValue(chipID, MPX3RX_DAC_TP_REF_A-1);
    if (dacCode == MPX3RX_DAC_TP_REF_B) dac_val = _mpx3gui->getConfig()->getDACValue(chipID, MPX3RX_DAC_TP_REF_B-1);

    while (!foundTarget) {
        if (dac_val >= 511) {
            qDebug() << "[FAIL]\tReached maximum DAC value, cannot find target. Set to maximum";
            dac_val = 511;
            foundTarget = true;

        }

        spidrcontrol->setDac(chipID, dacCode, int(dac_val));
        spidrcontrol->setSenseDac(chipID, dacCode);
        spidrcontrol->getDacOut(chipID, &adc_val, nSamples);

        adc_val /= nSamples;
        adc_volt = (__voltage_DACS_MAX/(double)__maxADCCounts) * (double)adc_val;

        //qDebug() << "adc_volt :" << adc_volt;

        if ( adc_volt <= lowerVboundary ) {
            if ( adc_volt < V*0.8 ) {
                dac_val += 5;
            } else {
                dac_val += 1;
            }
        } else if (adc_volt >= upperVboundary) {
            if ( adc_volt > V*1.2 ) {
                dac_val -= 5;
            } else {
                dac_val -= 1;
            }
        } else if ( adc_volt >= lowerVboundary && adc_volt <= upperVboundary ) {
            foundTarget = true;
        } else {
            qDebug() << "[FAIL]\tFix this... Could not find dac value to match given voltage within 1%" << dac_val << V;

            qDebug() << "\t Increasing tolerance to 5% and scanning again";
            lowerVboundary = V * 0.95;
            upperVboundary = V * 1.05;
        }
    }

    SetDAC_propagateInGUI(chipID, dacCode, int(dac_val));

    qDebug() << "[INFO]\tTest pulse equalisation --> found DAC value with voltage: " << dacCode << dac_val << adc_volt;

    return dac_val;
}

void testPulseEqualisation::SetDAC_propagateInGUI(int devId, int dac_code, int dac_val ) {

    // Set Dac
    spidrcontrol->setDac( devId, dac_code, dac_val );
    // Adjust the sliders and the SpinBoxes to the new value
    connect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );
    // Get the DAC back just to be sure and then slide&spin
    //int dacVal = 0;
    //spidrcontrol->getDac( devId,  dac_code, &dacVal);
    // SlideAndSpin works with the DAC index, no the code.
    int dacIndex = _mpx3gui->getDACs()->GetDACIndex( dac_code );
    //slideAndSpin( dacIndex,  dacVal );
    emit slideAndSpin( dacIndex,  dac_val );
    disconnect( this, SIGNAL( slideAndSpin(int, int) ), _mpx3gui->GetUI()->DACsWidget, SLOT( slideAndSpin(int, int) ) );

    // Set in the local config.  This function also takes the dac_index and not the dac_code
    _mpx3gui->getDACs()->SetDACValueLocalConfig( devId, dacIndex, dac_val);

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

void testPulseEqualisation::on_buttonBox_accepted()
{   
    if (setDACs) {
        if ( estimate_V_TP_REF_AB( config.injectionChargeInElectrons, true ) ) {
            qDebug() << "[INFO]\tDACs set according to test pulse equalisation GUI";
        } else {
            qDebug() << "[FAIL]\tCould not set DACs according to test pulse equalisation GUI";
        }
    }

    _mpx3gui->getEqualization()->setTestPulseMode(true);
    qDebug() << "[INFO]\tTest pulse mode is ON, may or may not be activated";
}

void testPulseEqualisation::on_buttonBox_rejected()
{
    _mpx3gui->getEqualization()->setTestPulseMode(false);
    qDebug() << "[INFO]\tTest pulse mode is OFF, may or may not be activated";
}

void testPulseEqualisation::on_checkBox_setDACs_toggled(bool checked)
{
    setDACs = checked;
}
