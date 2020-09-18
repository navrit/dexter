#ifndef TESTPULSEEQUALISATION_H
#define TESTPULSEEQUALISATION_H

#include <QDialog>
#include "mpx3gui.h"

class QCstmEqualization;

namespace Ui {
class testPulseEqualisation;
}

//! Only functions I want to access externally are public,
//! the rest are private by default
class testPulseEqualisation : public QDialog
{
    Q_OBJECT

public:
    explicit testPulseEqualisation(Mpx3GUI *, QWidget *parent = nullptr);
    ~testPulseEqualisation();

    bool activate(int startPixelOffset = 0);
    bool deactivate();

    uint getPixelSpacing() { return config.pixelSpacing; }
    uint getInjectionChargInElectrons() { return config.injectionChargeInElectrons; }
    uint getTestPulseLength() { return config.testPulseLength; }
    uint getTestPulsePeriod() { return config.testPulsePeriod; }
    uint getEqualisationTarget() { return config.equalisationTarget; }
    uint get_1st_DAC_DISC_val() { return config.DAC_DISC_1; }
    uint get_2nd_DAC_DISC_val() { return config.DAC_DISC_2; }
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

    void on_spinBox_equalisationTarget_valueChanged(int arg1);
    void on_spinBox_1st_DAC_DISC_valueChanged(int arg1);
    void on_spinBox_2nd_DAC_DISC_valueChanged(int arg1);

private:
    Mpx3GUI *_mpx3gui = nullptr;
    Ui::testPulseEqualisation *ui = nullptr;
    SpidrController *spidrcontrol = nullptr;
    QCstmEqualization *_equalisation = nullptr;

    const double maximumInjectionVoltage = 0.975; //! over linear range
    const int maximumInjectionElectrons = 30431; //! over linear range, assuming 5fF exactly and maximum voltage injection (over linear range)
    const double maximumInjectionKeV = maximumInjectionElectrons / 3.62; //! Assuming near room temperature for Si

    const double e = 1.6021766208e-19;
    const double c_test = 5e-15;
    const double e_dividedBy_c_test = 3.20435324e-5; //! Just so there aren't any weird overflow/underflow issues...

    bool setDACs = true;

    struct testPulseConfig {
        uint injectionChargeInElectrons = 2222;     //! Electrons by default
        uint testPulseLength = 400;                 //! DAC units, so n x 25 ns = m microseconds
                                                    //!    Length of the test pulses in 25 ns units
        uint testPulsePeriod = 100;                //! DAC units, so n x 25 ns = m ms.
                                                    //!    Period between TP Switch pulses in 25 ns units
        uint pixelSpacing = 5;                      //! Pixel spacing, where 1 is the minimum --> no gaps

                                                    //! These defaults give 2992 test pulses per pixel for 30 ms frames

        uint equalisationTarget = 10;
        uint DAC_DISC_1 = 100;
        uint DAC_DISC_2 = 150;
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
    } verbosity;                     //! LOW for text only or none?, HIGH for text + ASCII graph output

    QVector<int> activeChips;

    bool estimate_V_TP_REF_AB(uint electrons);      //! This should fail if requested charge cannot be injected.
    uint setDACToVoltage(uint chipID, int dacCode, double V);
    void SetDAC_propagateInGUI(uint devId, int dac_code, int dac_val );
};

#endif // TESTPULSEEQUALISATION_H
