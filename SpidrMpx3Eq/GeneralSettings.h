#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QDir>
#include <QtDebug>
#include "mpx3eq_common.h"

const static QString companyName = "ASI";
const static QString softwareName = "Dexter";
const static QString equalisationPath = "EqualizationPath";
const static QString configPath = "ConfigPath";
const static QString lastThresholdScanPath = "last_threshold_scan_path";

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
    QString getEqualisationPath();
    void setConfigPath(QString path);
    QString getConfigPath();
    void setLastThresholdPath(QString folder);
    QString getLastThresholdPath();

    //! TODO REMOVE
    void setOffset(int chipNum, double val);
    double getOffset(int chipNum);
    void setSlope(int chipNum, double val);
    double getSlope(int chipNum);

private:
    QString _equalisationPath = "./config/"; // Default equalisation files should be here
    QString _configPath = "./config/mpx3.json";
    QString _lastThresholdScanPath = "";

    const QString SLOPE = "slope";
    const QString OFFSET = "offset";

    double _slopes[__max_number_of_chips] = {-1}; // set a default value for thl to energy calibration slope parameters
    double _offsets[__max_number_of_chips] = {-1}; // set a default value for thl to energy calibration offset parameters

};

#endif // GENERALSETTINGS_H
