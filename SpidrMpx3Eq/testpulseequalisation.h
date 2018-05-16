#ifndef TESTPULSEEQUALISATION_H
#define TESTPULSEEQUALISATION_H

#include <QDialog>
#include "mpx3gui.h"
#include "qcstmequalization.h"


namespace Ui {
class testPulseEqualisation;
}

//! Only functions I want to access externally are public,
//! the rest are private by default
class testPulseEqualisation : public QDialog
{
    Q_OBJECT

public:
    explicit testPulseEqualisation(Mpx3GUI *, QWidget *parent = 0);
    ~testPulseEqualisation();

    bool activate(int startPixelOffset = 0);
    bool deactivate();

    uint getPixelSpacing() { return config.pixelSpacing; }
    uint getInjectionChargInElectrons() { return config.injectionChargeInElectrons; }
    uint getTestPulseLength() { return config.testPulseLength; }
    uint getTestPulsePeriod() { return config.testPulsePeriod; }
    void turnOffAllCTPRs(SpidrController *spidrcontrol, int chipID, bool submit);

signals:
    void slideAndSpin(int, int);

private slots:

    void on_spinBox_injectionCharge_valueChanged(int arg1);
    void on_spinBox_testPulseLength_valueChanged(int arg1);
    void on_spinBox_testPulsePeriod_valueChanged(int arg1);
    void on_spinBox_pixelSpacing_valueChanged(int arg1);
    void on_comboBox_verbosity_currentIndexChanged(int index);

    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    void on_checkBox_setDACs_toggled(bool checked);
    void on_pushButton_activate_clicked();
    void on_pushButton_deactivate_clicked();

private:
    Mpx3GUI * _mpx3gui = nullptr;
    Ui::testPulseEqualisation *ui = nullptr;
    SpidrController * spidrcontrol = nullptr;
    QCstmEqualization * _equalisation = nullptr;

    const float maximumInjectionVoltage = 0.975; //! over linear range
    const int maximumInjectionElectrons = 30431; //! over linear range, assuming 5fF exactly and maximum voltage injection (over linear range)
    const int maximumInjectionKeV = maximumInjectionElectrons / 3.62; //! Assuming near room temperature

    const double e = 1.6021766208e-19;
    const double c_test = 5e-15;
    const double e_dividedBy_c_test = 3.20435324e-5; //! Just so there aren't any weird overflow/underflow issues...

    bool setDACs = true;

    struct testPulseConfig {
        uint injectionChargeInElectrons = 3000;     //! Electrons by default
        uint testPulseLength = 400;                 //! DAC units, so 40 x 25 ns = 10 microseconds
                                                    //!    Length of the test pulses in 25 ns units
        uint testPulsePeriod = 40000;               //! DAC units, so 40000 x 25ns = 1ms.
                                                    //!    Period between TP Switch pulses in 25 ns units
        uint pixelSpacing = 4;                      //! Pixel spacing, where 1 is the minimum --> no gaps
    } config;

    struct testPulseDACconfig {
        uint V_TP_REF;
        uint V_TP_REF_A;
        uint V_TP_REF_B;
        uint V_TP_BufferIn = 128;
        uint V_TP_BufferOut = 4;
    } dacConfig;                                    //! All Test Pulse related DACs are in here

    enum {
        electrons
    } injectionChargeUnits; //! Could add units more here

    enum {
        LOW,
        HIGH
    } verbosity;                     //! LOW for text only, HIGH for text + ASCII graph output

    QVector<int> activeChips;

    bool estimate_V_TP_REF_AB(uint electrons, bool makeDialog);      //! This should fail if requested charge cannot be injected.
    uint setDACToVoltage(int chipID, int dacCode, double V);
    void SetDAC_propagateInGUI(int devId, int dac_code, int dac_val );

};

#endif // TESTPULSEEQUALISATION_H
