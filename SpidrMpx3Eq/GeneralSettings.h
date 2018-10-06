#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QDir>
#define COMPANY_NAME  "ASI"
#define SOFTWARE_NAME "Dexter"
#define EQUALIZATION_PATH "EqualizationPath"
#define CONFIG_PATH    "ConfigPath"




class GeneralSettings : public QObject
{
    Q_OBJECT
// purpose : load last equalization,config
public:
    explicit GeneralSettings(QObject *parent = 0);
    void setEqualizationPath(QString path);
    void setConfigPath(QString path);
    QString getEqualizationPath(void);
    QString getConfigPath(void);
    void writeSetting(void);
    void readSetting(void);

private:
    QString _equalizationPath = "nc"; //default (not connected)
    QString _configPath = "nc";

signals:

public slots:
};

#endif // GENERALSETTINGS_H
