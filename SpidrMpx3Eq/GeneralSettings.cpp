#include "GeneralSettings.h"

GeneralSettings::GeneralSettings(QObject *parent) : QObject(parent)
{
    // set a default value for thl to energy calibration slope parameters
    for(int i=0; i<NUMBER_OF_CHIPS; i++) {
        _slopes[i] = 1;
    }

}

void GeneralSettings::setEqualisationPath(QString path)
{
    _equalisationPath = path;
    writeSetting();
}

void GeneralSettings::setConfigPath(QString path)
{
    _configPath = path;
    writeSetting();
}

QString GeneralSettings::getEqualisationPath()
{
    readSetting();
    return _equalisationPath;
}

QString GeneralSettings::getConfigPath()
{
    readSetting();
    return _configPath;
}

void GeneralSettings::setOffset(int chipNum, double val)
{
    if(chipNum >= 0 && chipNum < NUMBER_OF_CHIPS){
        _offsets[chipNum] = val;
        writeSetting();
    }

}

double GeneralSettings::getOffset(int chipNum)
{
    if(chipNum >= 0 && chipNum < NUMBER_OF_CHIPS){
        readSetting();
        return _offsets[chipNum];
    }
    return 0.0;
}

void GeneralSettings::setSlope(int chipNum, double val)
{
    if(chipNum >= 0 && chipNum < NUMBER_OF_CHIPS){
        _slopes[chipNum] = val;
        writeSetting();
    }
}

double GeneralSettings::getSlope(int chipNum)
{
    if(chipNum >= 0 && chipNum < NUMBER_OF_CHIPS){
        readSetting();
        return _slopes[chipNum];
    }
    return 0.0;
}

void GeneralSettings::setLastThresholdPath(QString folder)
{
    const QFileInfo info = QFileInfo(folder);
    if (info.isDir() && info.isAbsolute() && info.isWritable()) {
        _lastThresholdScanPath = folder;
        writeSetting();
        qDebug() << "[INFO]\tWritten last threshold path to file: " << _lastThresholdScanPath;
    } else {
        qDebug() << "[ERROR]\tCould not set last threshold path to General Settings";
    }
}

QString GeneralSettings::getLastThresholdPath()
{
    readSetting();
    qDebug() << "[INFO]\tRead last threshold path from file: " << _lastThresholdScanPath;
    return _lastThresholdScanPath;
}

void GeneralSettings::writeSetting()
{
    QSettings settings(companyName, softwareName);
    settings.setValue(equalisationPath, _equalisationPath);
    settings.setValue(configPath, _configPath);
    settings.setValue(lastThresholdScanPath, _lastThresholdScanPath);

    settings.beginGroup(SLOPE);
    settings.setValue("chip_0", _slopes[0]);
    settings.setValue("chip_1", _slopes[1]);
    settings.setValue("chip_2", _slopes[2]);
    settings.setValue("chip_3", _slopes[3]);
    settings.endGroup();

    settings.beginGroup(OFFSET);
    settings.setValue("chip_0",_offsets[0]);
    settings.setValue("chip_1",_offsets[1]);
    settings.setValue("chip_2",_offsets[2]);
    settings.setValue("chip_3",_offsets[3]);
    settings.endGroup();
}

void GeneralSettings::readSetting()
{
    QSettings settings(companyName, softwareName);


    if(settings.contains(equalisationPath))
        _equalisationPath = settings.value(equalisationPath).toString();

    if(settings.contains(configPath))
        _configPath = settings.value(configPath).toString();

    if ( settings.contains(lastThresholdScanPath) ) {
        _lastThresholdScanPath = settings.value(lastThresholdScanPath).toString();
    }

    if(settings.contains(SLOPE+"/"+"chip_0"))
        _slopes[0] = settings.value(SLOPE+"/"+"chip_0").toDouble();

    if(settings.contains(SLOPE+"/"+"chip_1"))
        _slopes[1] = settings.value(SLOPE+"/"+"chip_1").toDouble();

    if(settings.contains(SLOPE+"/"+"chip_2"))
        _slopes[2] = settings.value(SLOPE+"/"+"chip_2").toDouble();

    if(settings.contains(SLOPE+"/"+"chip_3"))
        _slopes[3]= settings.value(SLOPE+"/"+"chip_3").toDouble();

    if(settings.contains(OFFSET+"/"+"chip_0"))
        _offsets[0] = settings.value(OFFSET+"/"+"chip_0").toDouble();

    if(settings.contains(OFFSET+"/"+"chip_1"))
        _offsets[1] = settings.value(OFFSET+"/"+"chip_1").toDouble();

    if(settings.contains(OFFSET+"/"+"chip_2"))
        _offsets[2] = settings.value(OFFSET+"/"+"chip_2").toDouble();

    if(settings.contains(OFFSET+"/"+"chip_3"))
        _offsets[3] = settings.value(OFFSET+"/"+"chip_3").toDouble();
}
