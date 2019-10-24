#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <QObject>
#include <QHash>
#include <QVector>
#include "MerlinInterface.h"
#include "ServerStatus.h"
#include "canvas.h"


class CommandHandler;
class Command;
class Mpx3GUI;

struct cmd_struct
{
    void (*handler) (CommandHandler*, Command*);
};

enum ERROR_TYPE{NO_ERROR = 0, UNKNOWN_ERROR = -1, UNKNOWN_COMMAND = -2 , ARG_NUM_OUT_RANGE = -3, ARG_VAL_OUT_RANGE = -4,
                SERVER_BUSY_DATA_TAKING = -5,SERVER_BUSY_EQUALIZATION = -6,SERVER_BUSY_DAC_SCAN = -7,
                SERVER_BUSY_THRESHOLD_SCAN = -8, INVALID_ARG = -9 };
static const QString gainModeStrTable[] = {"shigh","high","low","slow"};

class CommandHandler : public QObject
{
    Q_OBJECT

public:
    explicit CommandHandler(QObject *parent = nullptr);
    Mpx3GUI* getGui();
    void startLiveCamera(bool);
    void startSnap();
    void setAutoSave(bool);
    bool setRecordPath(QString);
    void setRecordFormat(int);
    int setThreshold(int threshold, int DAC_value);
    int setThreshold(int chip, int DAC_value, int t);
    int getThreshold(int threshold, double *DAC_value);
    int getThreshold(int threshold, int chipId, int *DAC_value);
    int setStartScan(int);
    int setStopScan(int);
    int setStepScan(int);
    int setThresholdScan(int);
    int setFramesPerScan(int);
    int setScanPath(QString);
    void startThrsholdScan();
    int getStartScan();
    int getStopScan();
    int getStepScan();
    int getThresholdScan();
    int getFramesPerScan();
    QString getScanPath();
    void startScan();
    void stopEqualization();

    void startSendingImage(bool);
    QString getAcquisitionHeader();
    void getImage();
    int setPixelMask(int,int);
    int setPixelUnmask(int,int);
    int loadEqualizationRemotely(QString path);
    QString getEqualizationPath();
    int loadConfigRemotely(QString path);
    QString getConfigPath();
    int saveConfigRemotely(QString);
    int doEqualizationRemotely(QString path);
    int setInhibitShutter(bool);
    bool getInhibitShutter();

    int setSlope(int chipNum,double val);
    double getSlope(int chipNum);

    int setOffset(int chipNum,double val);
    double getOffset(int chipNum);

    int resetSlopesAndOffsets();
    int getServerStatus();

    //data
    void emitrequestForAnotherSocket(int);
    QHash<QString,cmd_struct> cmdTable;

signals:
    void imageIsReady(QByteArray,Canvas);
    void requestForDataTaking(bool);
    void requestForInfDataTracking(bool);
    void requestForSnap();
    void requestForAutoSave(bool);
    void requestForSettingSavePath(QString);
    void requestForSettingSaveFormat(int);
    void requestAnotherSocket(int);
    void requestToMaskPixelRemotely(int,int);
    void requestToUnmaskPixelRemotely(int,int);
    void requestToLoadEqualizationRemotely(QString);
    void requestToStartStopThresholdScan();
    void requestToDoEqualizationRemotely(QString);
    void requestToSetInhibitShutterRemotely(bool);
    void requestToLoadConfigRemotely(QString);
    void requestToSaveConfigRemotely(QString);
    void requestToChangeGuiforThreshold(int);

public slots:
    void on_doneWithOneFrame(int);
    void on_someCommandHasFinished_Successfully();
    void on_equalisationPathExported(QString path);
    void setServerStatus(SERVER_BUSY_TYPE status);

private:
    void initializeCmdTable();
    char* getTimeStamp();
    bool _sendingImage = false;
    QString _equalisationPath = "";
    SERVER_BUSY_TYPE _serverStatus;
};

class Command {

public:
    Command(QString command);
    void invoke(CommandHandler* ch, SERVER_BUSY_TYPE serverStatus);
    QString getData();
    void setData(QString);
    void setImage(QByteArray);
    bool enoughArguments(int, QString);
    ERROR_TYPE getError();
    void setError(ERROR_TYPE);
    void merlinErrorToPslError(int errNum);
    QVector<QString> arguments; // command's arguments

private:
    QString cmd;                // core command
    QString data;               // command data string to repond to commands excepts 'GetImage'
    QByteArray imageToSend;     // image to be sent when 'GetImage' received
    ERROR_TYPE _error = NO_ERROR;
    void print(); //test


};

#endif // COMMANDHANDLER_H
