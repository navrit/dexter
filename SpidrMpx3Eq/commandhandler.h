#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H


#include <QObject>
#include <QHash>
#include <QVector>
#include "MerlinInterface.h"



struct cmd_struct
{
    void (*handler) (void);
    QVector<QString> args;

};


class CommandHandler : public QObject
{
    Q_OBJECT
public:
    enum ERROR_TYPE{NO_ERROR = 0, UNKWON_ERROR = -1, UNKWON_COMMAND = -2 , ARG_NUM_OUT_RANGE = -3, ARG_VAL_OUT_RANGE = -4};
    explicit CommandHandler(QObject *parent = nullptr);
    QString getData(void);
    void fetchCmd();
    void setCmd(QString);
    void setData(QString);
    void setImage(QByteArray);
    bool enoughArguments(int, QString);
    static CommandHandler* getInstance();
    ERROR_TYPE getError(void);
    void setError(ERROR_TYPE);
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
    int setThreholdScan(int);
    int getStartScan(void);
    int getStopScan(void);
    int getStepScan(void);
    int getThreholdScan(void);
    int startScan(int);
    int stopScan(int);
    void startSendingImage(bool);
    void merlinErrorToPslError(int errNum);
    QString generateMerlinFrameHeader(FrameHeaderDataStruct);
    QString getAcquisitionHeader(void);
    void getImage(void);
    //data
    void emitrequestForAnotherSocket(int);
    //test
    void print(void);
    QHash<QString,cmd_struct> cmdTable;


signals:
    //void commandIsDecoded(QString,QByteArray,bool);
    void imageIsReady(QByteArray);
    void requestForDataTaking(void);
    void requestForInfDataTracking(bool);
    void requestForSnap(void);
    void requestForAutoSave(bool);
    void requestForSettingSavePath(QString);
    void requestForSettingSaveFormat(int);
    void requestAnotherSocket(int);

public slots:
    void on_cmdRecieved(QString);
    void on_doneWithOneFrame(int);
    void on_someCommandHasFinished_Successfully(void);
private:
    QString cmd;         // core command
    QVector<QString> arguments; // command's arguments
    QString data;               // command data string to repond to commands excepts 'GetImage'
    QByteArray imageToSend;    // image to be sent when 'GetImage' recieved
    void initializeCmdTable(void);
    ERROR_TYPE _error = NO_ERROR;
    char* getTimeStamp();
    void setMerlinFrameHeader(FrameHeaderDataStruct&);
    bool _sendingImage = false;


};


#endif // COMMANDHANDLER_H
