#ifndef COMMANDHANDLERWRAPPER_H
#define COMMANDHANDLERWRAPPER_H

#include <QObject>
#include <QDebug>
#include "commandhandler.h"
#include "MerlinInterface.h"

class CommandHandlerWrapper : public QObject
{
    Q_OBJECT
public:
    explicit CommandHandlerWrapper(QObject *parent = 0);

private:
    CommandHandler *commandHandler;
    MerlinInterface *merlinInterface;

signals:
    //pass response to command server
    void responseIsReady(QString);
    //pass image to data server
    void imageIsReady(QByteArray);

public slots:
    //recive commands from command socket(server)
    void on_dataRecieved(QString);
    //provide image data for image socket(server) when is requested.
    void on_requestForDataTaking(void);
    //recive image from commandHandler
    void on_ImageIsReady(QByteArray);
};

#endif // COMMANDHANDLERWRAPPER_H
