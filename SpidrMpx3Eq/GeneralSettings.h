#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QDir>

#define COMPANY_NAME  "ASI"
#define SOFTWARE_NAME "Dexter"
#define EQUALIZATION_PATH "EqualizationPath"
#define CONFIG_PATH    "ConfigPath"
const static QString lastThresholdScanPath = "last_threshold_scan_path";
#define NUMBER_OF_CHIPS 4

const QString SLOPE   = "slope";
const QString  OFFSET = "Offset";

class GeneralSettings : public QObject
{
    Q_OBJECT
// purpose : load last equalization, config
public:
    explicit GeneralSettings(QObject *parent = nullptr);
    void setEqualizationPath(QString path);
    void setConfigPath(QString path);
    QString getEqualizationPath(void);
    QString getConfigPath(void);
    void setLastThresholdPath(QString folder);
    QString getLastThresholdPath();
    void setOffset(int chipNum, double val);
    double getOffset(int chipNum);
    void setSlope(int chipNum, double val);
    double getSlope(int chipNum);
    void writeSetting();
    void readSetting();

private:
    QString _equalizationPath = "nc"; //default (not connected)
    QString _configPath = "./config/mpx3.json";
    QString _lastThresholdScanPath = "";
    //energy calibration
    double _slopes[NUMBER_OF_CHIPS] = {1}; // set a default value for thl to energy calibration slope parameters
    double _offsets[NUMBER_OF_CHIPS] = {1}; // set a default value for thl to energy calibration offset parameters

};

#endif // GENERALSETTINGS_H
