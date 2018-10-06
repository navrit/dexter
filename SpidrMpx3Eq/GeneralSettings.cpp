#include "GeneralSettings.h"

GeneralSettings::GeneralSettings(QObject *parent) : QObject(parent)
{

}

void GeneralSettings::setEqualizationPath(QString path)
{
    _equalizationPath = path;
    writeSetting();
}

void GeneralSettings::setConfigPath(QString path)
{
    _configPath = path;
    writeSetting();
}

QString GeneralSettings::getEqualizationPath()
{
    readSetting();
    return _equalizationPath;
}

QString GeneralSettings::getConfigPath()
{
    readSetting();
    return _configPath;
}

void GeneralSettings::writeSetting()
{
    QSettings settings(COMPANY_NAME, SOFTWARE_NAME);
    settings.setValue(EQUALIZATION_PATH,_equalizationPath);
    settings.setValue(CONFIG_PATH,_configPath);
}

void GeneralSettings::readSetting()
{
    QSettings settings(COMPANY_NAME, SOFTWARE_NAME);
    if(settings.contains(EQUALIZATION_PATH))
        _equalizationPath = settings.value(EQUALIZATION_PATH).toString();

    if(settings.contains(CONFIG_PATH))
        _configPath = settings.value(CONFIG_PATH).toString();

}
