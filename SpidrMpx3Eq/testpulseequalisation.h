#ifndef TESTPULSEEQUALISATION_H
#define TESTPULSEEQUALISATION_H

#include <QDialog>
#include "mpx3gui.h"
#include "qcstmequalization.h"


namespace Ui {
class testPulseEqualisation;
}

class testPulseEqualisation : public QDialog
{
    Q_OBJECT

public:
    explicit testPulseEqualisation(Mpx3GUI *, QWidget *parent = 0);
    ~testPulseEqualisation();

    bool activate(int startPixelOffset = 0);
    bool deactivate();

private slots:

    void on_spinBox_injectionCharge_valueChanged(int arg1);
    void on_comboBox_injectionChargeUnits_currentIndexChanged(int index);
    void on_spinBox_testPulseLength_valueChanged(int arg1);
    void on_spinBox_testPulsePeriod_valueChanged(int arg1);
    void on_spinBox_pixelSpacing_valueChanged(int arg1);
    void on_comboBox_verbosity_currentIndexChanged(int index);

private:
    Mpx3GUI * _mpx3gui = nullptr;
    Ui::testPulseEqualisation *ui = nullptr;

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
        electrons,
        KeV_Si
    } injectionChargeUnits;

    enum {
        LOW,
        HIGH
    } verbosity;                     //! LOW for text only, HIGH for text + ASCII graph output

    bool estimate_V_TP_REF_AB(uint electrons);      //! This should fail if requested charge cannot be injected.
    void turnOffAllCTPRs(SpidrController *spidrcontrol, int chipID, bool submit);
};

#endif // TESTPULSEEQUALISATION_H
