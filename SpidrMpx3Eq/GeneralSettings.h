#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QDir>

const static QString companyName = "ASI";
const static QString softwareName = "Dexter";
const static QString equalisationPath = "EqualizationPath";
const static QString configPath = "ConfigPath";
const static QString lastThresholdScanPath = "last_threshold_scan_path";

//! TODO REMOVE
#define NUMBER_OF_CHIPS 4
const QString SLOPE  = "slope";
const QString OFFSET = "Offset";

/**
 * @brief The GeneralSettings class purpose is to remember the last loaded equalisation, configuration, threshold scan path
 */
class GeneralSettings : public QObject
{
    Q_OBJECT

public:
    explicit GeneralSettings(QObject *parent = nullptr);
    void writeSetting();
    void readSetting();
    void setEqualisationPath(QString path);
    QString getEqualisationPath(void);
    void setConfigPath(QString path);
    QString getConfigPath(void);
    void setLastThresholdPath(QString folder);
    QString getLastThresholdPath();

    //! TODO REMOVE
    void setOffset(int chipNum, double val);
    double getOffset(int chipNum);
    void setSlope(int chipNum, double val);
    double getSlope(int chipNum);

private:
    QString _equalizationPath = "./config/"; // Default equalisation files should be here
    QString _configPath = "./config/mpx3.json";
    QString _lastThresholdScanPath = "";

    //! TODO REMOVE
    //energy calibration
    double _slopes[NUMBER_OF_CHIPS] = {1}; // set a default value for thl to energy calibration slope parameters
    double _offsets[NUMBER_OF_CHIPS] = {1}; // set a default value for thl to energy calibration offset parameters

};

#endif // GENERALSETTINGS_H
