#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H


#include <QObject>
#include <QHash>
#include <QVector>



struct cmd_struct
{
    void (*handler) (void);
    QVector<QString> args;

};


class CommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit CommandHandler(QObject *parent = nullptr);
    QString getData(void);
    void fetchCmd();
    void setCmd(char*);
    void setData(QString);
    void setImage(QByteArray);
    bool enoughArguments(int, QString);
    static CommandHandler* getInstance();
    void startLiveCamera(void);
    void startSnap(void);
    void setAutoSave(bool);
    void setRecordPath(QString);
    void setRecordFormat(int);
    //test
    void print(void);
    QHash<QString,cmd_struct> cmdTable;


signals:
    void commandIsDecoded(QString,QByteArray,bool);
    void requestForDataTaking(void);
    void requestForInfDataTracking(bool);
    void requestForSnap(void);
    void requestForAutoSave(bool);
    void requestForSettingSavePath(QString);
    void requestForSettingSaveFormat(int);
public slots:
    void on_cmdRecieved(char*);

private:
    QString cmd;         //core command
    QVector<QString> arguments; //command's arguments
    QString data;               //data string to repond to commands excepts 'GetImage'
    QByteArray imageToSend;    // iamge to be sent when 'GetImage' recieved
    void initializeCmdTable(void);








};


#endif // COMMANDHANDLER_H
