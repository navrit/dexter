#include "EnergyCalibrator.h"
#include <QDebug>

EnergyCalibrator::EnergyCalibrator(QObject *parent) : QObject(parent)
{

}

void EnergyCalibrator::setSlope(int chip, int threshold, double value)
{
    if (_indexChecker(chip, threshold)) {
        _slopes[chip][threshold] = value;
    }
}

void EnergyCalibrator::setOffset(int chip, int threshold, double value)
{
    if (_indexChecker(chip, threshold)) {
        _offsets[chip][threshold] = value;
    }
}

double EnergyCalibrator::getSlope(int chip, int threshold)
{
    if (_indexChecker(chip, threshold)) {
        return _slopes[chip][threshold];
    }
    return 0.0;
}

double EnergyCalibrator::getOffset(int chip, int threshold)
{
    if (_indexChecker(chip, threshold)) {
        return _offsets[chip][threshold];
    }
    return 0.0;
}

double EnergyCalibrator::getEnergy(int chip, int threshold)
{
    if (_indexChecker(chip, threshold)) {
        return _energies[chip][threshold];
    }
    return 0.0;
}

double EnergyCalibrator::getDac(int chip, int threshold)
{
    if (_indexChecker(chip, threshold)) {
        return _dacs[chip][threshold];
    }
    return 0;
}

double EnergyCalibrator::calcEnergy(int chip, int threshold, double DAC_value)
{
    if (!_indexChecker(chip, threshold)) {
        return 0.0;
    }

    _energies[chip][threshold] = double(_slopes[chip][threshold] * DAC_value + _offsets[chip][threshold]);

    return _energies[chip][threshold];
}

double EnergyCalibrator::calcDac(int chip, int threshold, double energy)
{
    if (!_indexChecker(chip, threshold)) {
        return 0;
    }

    /* This is how you rearrange a simple straight line linear equation */

    /* 1. Starting equation:    energy = _slopes[idx] * _dacs[idx] + _offsets[idx] */
    /* 2. Subtract offset:      energy - _offsets[idx] = _slopes[idx] * _dacs[idx] */
    /* 3. Divide by slope:      _dacs[idx] = (energy - _offsets[idx])/_slopes[idx] */

    _dacs[chip][threshold] = int(((energy - _offsets[chip][threshold]) / _slopes[chip][threshold]));

    if (_dacs[chip][threshold] >= 0 && _dacs[chip][threshold] < __max_DAC_range) {
        // Valid DAC!
        return _dacs[chip][threshold];
    } else {
        // Invalid DAC, return error code. Ie. don't set this DAC
        return -1;
    }
}


bool EnergyCalibrator::_indexChecker(int chip, int threshold)
{
    if (chip >= 0 && chip < __max_number_of_chips && threshold >= 0 && threshold < __max_number_of_thresholds ) {
        return true;
    }

    return false;
}
