#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QDir>
#define COMPANY_NAME  "ASI"
#define SOFTWARE_NAME "Dexter"
#define EQUALIZATION_PATH "EqualizationPath"
#define CONFIG_PATH    "ConfigPath"
#define NUMBER_OF_CHIPS 4

//#define SLOPE        "Slope"
const QString SLOPE = "slope";

const QString  OFFSET =      "Offset";
//#define SLOPE0       "SLOPE0"
//#define SLOPE1       "SLOPE1"
//#define SLOPE2       "SLOPE2"
//#define SLOPE3       "SLOPE3"
//#define OFFSET0      "OFFSET0"
//#define OFFSET1      "OFFSET1"
//#define OFFSET2      "OFFSET2"
//#define OFFSET3      "OFFSET3"


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
    void setOffset(int chipNum,double val);
    double getOffset(int chipNum);
    void setSlope(int chipNum,double val);
    double getSlope(int chipNum);
    void writeSetting();
    void readSetting();


private:
    QString _equalizationPath = "nc"; //default (not connected)
    QString _configPath = "nc";
    //energy calibration
    double _slopes[NUMBER_OF_CHIPS];
    double _offsets[NUMBER_OF_CHIPS];
//    double _slope0   = 0;
//    double _slope1   = 0;
//    double _slope2   = 0;
//    double _slope3   = 0;
//    double _offset0  = 0;
//    double _offset1 = 0;
//    double _offset2 = 0;
//    double _offset3 = 0;



signals:

public slots:
};

#endif // GENERALSETTINGS_H
