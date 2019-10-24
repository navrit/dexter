#ifndef COMMANDHANDLERWRAPPER_H
#define COMMANDHANDLERWRAPPER_H

#include <QObject>
#include <QDebug>
#include "commandhandler.h"
#include "MerlinInterface.h"
#include "ServerStatus.h"


class CommandHandlerWrapper : public QObject
{
    Q_OBJECT
public:

    explicit CommandHandlerWrapper(QObject *parent = 0);
    CommandHandler* getCommandHandler();


private:
    CommandHandler *commandHandler;
    MerlinInterface *merlinInterface;
    SERVER_BUSY_TYPE _serverStatus = FREE;

signals:
    //pass response to command server
    void responseIsReady(QString);
    //pass image to data server
    void imageIsReady(QByteArray, Canvas);

public slots:
    //recive commands from command socket(server)
    void on_dataReceived(QString);
    //provide image data for image socket(server) when is requested.
    void on_requestForDataTaking(bool);
    //recive image from commandHandler
    void on_ImageIsReady(QByteArray, Canvas);
    //get notified if server is busy or free e.g = data acquiring, equalization and etc.
    void on_serverStatusChanged(SERVER_BUSY_TYPE);
};

#endif // COMMANDHANDLERWRAPPER_H
