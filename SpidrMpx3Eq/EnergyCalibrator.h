#ifndef ENERGYCALIBRATOR_H
#define ENERGYCALIBRATOR_H

#include <QObject>
#define NUMBER_OF_CHIPS 4


// y = ax + b
// _energy = _slop X _dacs + _offset

class EnergyCalibrator : public QObject
{
    Q_OBJECT
public:
    explicit EnergyCalibrator(QObject *parent = 0);
    void setSlope(int idx,double value);
    void setOffset(int idx,double value);
    double getSlope(int idx);
    double getOffset(int idx);
    double getEnergy(int idx);
    int getDac(int idx);
    double calcEnergy(int idx,int dac);
    int calDac(int idx,double energy);


private:
    double _slopes[NUMBER_OF_CHIPS];
    double _offsets[NUMBER_OF_CHIPS];
    double _energies[NUMBER_OF_CHIPS];
    int    _dacs[NUMBER_OF_CHIPS];
    bool   _indexChecker(int idx);

signals:

public slots:
};

#endif // ENERGYCALIBRATOR_H
