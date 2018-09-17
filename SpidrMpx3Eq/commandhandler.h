#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H


#include <QObject>
#include <QHash>
#include <QVector>
#include "MerlinInterface.h"


class CommandHandler;
class Command;
class Mpx3GUI;

struct cmd_struct
{
    void (*handler) (CommandHandler*, Command*);
};

enum ERROR_TYPE{NO_ERROR = 0, UNKNOWN_ERROR = -1, UNKNOWN_COMMAND = -2 , ARG_NUM_OUT_RANGE = -3, ARG_VAL_OUT_RANGE = -4};
static const QString gainModeStrTable[] = {"shigh","high","low","slow"};

class CommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit CommandHandler(QObject *parent = nullptr);
    Mpx3GUI* getGui();
    void startLiveCamera(void);
    void startSnap(void);
    void setAutoSave(bool);
    void setRecordPath(QString);
    void setRecordFormat(int);
    int setThreshold(int,int);
    int getThreshold(int);
    int setStartScan(int);
    int setStopScan(int);
    int setStepScan(int);
    int setThresholdScan(int);
    int getStartScan(void);
    int getStopScan(void);
    int getStepScan(void);
    int getThresholdScan(void);
    int startScan(int);
    int stopScan(int);
    void startSendingImage(bool);
    QString generateMerlinFrameHeader(int frameid);
    QString getAcquisitionHeader(void);
    void getImage(void);
    int setPixelMask(int,int);
    int setPixelUnmask(int,int);
    void loadEqualizationRemotely(QString path);
    QString getEqualizationPath();

    //data
    void emitrequestForAnotherSocket(int);
    QHash<QString,cmd_struct> cmdTable;

signals:
    //void commandIsDecoded(QString,QByteArray,bool);
    void imageIsReady(QByteArray,QByteArray);
    void requestForDataTaking(void);
    void requestForInfDataTracking(bool);
    void requestForSnap(void);
    void requestForAutoSave(bool);
    void requestForSettingSavePath(QString);
    void requestForSettingSaveFormat(int);
    void requestAnotherSocket(int);
    void requestToMaskPixelRemotely(int,int);
    void requestToUnmaskPixelRemotely(int,int);
    void requestToLoadEqualizationRemotely(QString);

public slots:
    void on_doneWithOneFrame(int);
    void on_someCommandHasFinished_Successfully(void);
    void on_equalizationPathExported(QString path);
private:
    QByteArray imageToSend;    // image to be sent when 'GetImage' recieved
    void initializeCmdTable(void);
    char* getTimeStamp();
    bool _sendingImage = false;
    QString _equalizationPath = "";
};

class Command {
public:
    Command(QString command);
    void invoke(CommandHandler* ch);
    QString getData(void);
    void setData(QString);
    void setImage(QByteArray);
    bool enoughArguments(int, QString);
    ERROR_TYPE getError(void);
    void setError(ERROR_TYPE);
    void merlinErrorToPslError(int errNum);
    QVector<QString> arguments; // command's arguments
private:
    QString cmd;         // core command
    QString data;               // command data string to repond to commands excepts 'GetImage'
    QByteArray imageToSend;    // image to be sent when 'GetImage' recieved
    ERROR_TYPE _error = NO_ERROR;
    //test
    void print(void);
};

#endif // COMMANDHANDLER_H
