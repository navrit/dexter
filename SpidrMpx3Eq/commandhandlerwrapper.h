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
    void responseIsReady(QString);

public slots:
    void on_dataRecieved(QString);
};

#endif // COMMANDHANDLERWRAPPER_H
