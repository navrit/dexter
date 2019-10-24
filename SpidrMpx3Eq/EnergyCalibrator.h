#ifndef ENERGYCALIBRATOR_H
#define ENERGYCALIBRATOR_H

#include <QObject>
#include "mpx3eq_common.h"

/* y = ax + b
 * energy = slope * dac_value + offset */

class EnergyCalibrator : public QObject
{
    Q_OBJECT

public:
    explicit EnergyCalibrator(QObject *parent = nullptr);
    void setSlope(int chip, int threshold, double value);
    void setOffset(int chip, int threshold, double value);
    double getSlope(int chip, int threshold);
    double getOffset(int chip, int threshold);
    double getEnergy(int chip, int threshold);
    double getDac(int chip, int threshold);
    double calcEnergy(int chip, int threshold, double DAC_value);
    double calcDac(int chip, int threshold, double energy);

private:
    double _slopes[__max_number_of_chips][__max_number_of_thresholds] = {{-1}};
    double _offsets[__max_number_of_chips][__max_number_of_thresholds] = {{-1}};
    double _energies[__max_number_of_chips][__max_number_of_thresholds] = {{-1}};
    int    _dacs[__max_number_of_chips][__max_number_of_thresholds] = {{-1}};
    bool   _indexChecker(int chip, int threshold);
};

#endif // ENERGYCALIBRATOR_H
