#ifndef ENERGYCONFIGURATION_H
#define ENERGYCONFIGURATION_H

#include <QWidget>
#include "mpx3eq_common.h"

class Mpx3GUI;

namespace Ui {
    class EnergyConfiguration;
}

class EnergyConfiguration : public QWidget
{
    Q_OBJECT

public:
    explicit EnergyConfiguration(QWidget *parent = nullptr);
    virtual ~EnergyConfiguration() override;

    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
    void setWindowWidgetsStatus(win_status s = win_status::startup);

    void updateEnergyFromConfig();

public slots:
    void updateDACsFromConfig();
    void slot_colourModeChanged(bool);
    void slot_readBothCounters(bool);

private:
    Ui::EnergyConfiguration *ui = nullptr;
    Mpx3GUI * _mpx3gui = nullptr;

    void setDACs_all_chips_and_thresholds(int threshold, double energy);

    void setMinimumEnergy_sliders(int val);
    void setMaximumEnergy_sliders(int val);
    void setMinimumEnergy_spinBox(double val);
    void setMaximumEnergy_spinBox(double val);

    void setActualEnergiesLabel(int threshold, double E);
    void enablePossibleSlidersAndCheckboxes();

    std::array<double, __max_number_of_thresholds> targetEnergies = {{-1.0}};

    uint _maximumEnergy_keV = 500; /* This gets propagated to the sliders and spinboxes */
    uint _minimumEnergy_keV = 1; /* This gets propagated to the sliders and spinboxes */

private slots:
    void ConnectionStatusChanged(bool conn);

    void on_slider_th0_valueChanged(int value);
    void on_spinBox_th0_valueChanged(double energy);
    void on_slider_th1_valueChanged(int value);
    void on_spinBox_th1_valueChanged(double energy);
    void on_slider_th2_valueChanged(int value);
    void on_spinBox_th2_valueChanged(double energy);
    void on_slider_th3_valueChanged(int value);
    void on_spinBox_th3_valueChanged(double energy);
    void on_slider_th4_valueChanged(int value);
    void on_spinBox_th4_valueChanged(double energy);
    void on_slider_th5_valueChanged(int value);
    void on_spinBox_th5_valueChanged(double energy);
    void on_slider_th6_valueChanged(int value);
    void on_spinBox_th6_valueChanged(double energy);
    void on_slider_th7_valueChanged(int value);
    void on_spinBox_th7_valueChanged(double energy);
};


#endif // ENERGYCONFIGURATION_H
