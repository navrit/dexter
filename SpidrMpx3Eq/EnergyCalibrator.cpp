#include "EnergyCalibrator.h"
#include <QDebug>

EnergyCalibrator::EnergyCalibrator(QObject *parent) : QObject(parent)
{

}

void EnergyCalibrator::setSlope(int idx, double value)
{
    if(_indexChecker(idx))
        _slopes[idx] = value;
}

void EnergyCalibrator::setOffset(int idx, double value)
{
    if(_indexChecker(idx))
        _offsets[idx] = value;
}

double EnergyCalibrator::getSlope(int idx)
{
    if(_indexChecker(idx))
        return _slopes[idx];
    return 0.0;
}

double EnergyCalibrator::getOffset(int idx)
{
    if(_indexChecker(idx))
        return _offsets[idx];
    return 0.0;
}

double EnergyCalibrator::getEnergy(int idx)
{
    if(_indexChecker(idx))
        return _energies[idx];
    return 0.0;
}

int EnergyCalibrator::getDac(int idx)
{
    if(_indexChecker(idx))
        return _dacs[idx];
    return 0;
}

double EnergyCalibrator::calcEnergy(int idx, int dac)
{
    if(!_indexChecker(idx)) {
        return 0.0;
    }

    _energies[idx] = double(_slopes[idx] * dac + _offsets[idx]);

    return _energies[idx];
}

int EnergyCalibrator::calDac(int idx, double energy)
{
    if(!_indexChecker(idx)) {
        return 0;
    }

    /* This is how you rearrange a simple straight line linear equation */

    /* 1. Starting equation:    energy = _slopes[idx] * _dacs[idx] + _offsets[idx] */
    /* 2. Subtract offset:      energy - _offsets[idx] = _slopes[idx] * _dacs[idx] */
    /* 3. Divide by slope:      _dacs[idx] = (energy - _offsets[idx])/_slopes[idx] */

    _dacs[idx] = int(((energy - _offsets[idx]) / _slopes[idx]));

    return _dacs[idx];
}


bool EnergyCalibrator::_indexChecker(int idx)
{
    if(idx >= 0 && idx < NUMBER_OF_CHIPS)
        return true;
    return false;
}
