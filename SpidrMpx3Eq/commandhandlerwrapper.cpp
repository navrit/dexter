#include "commandhandlerwrapper.h"
#include "qcstmglvisualization.h"
#include "qcstmdacs.h"
#include "qcstmequalization.h"

CommandHandlerWrapper::CommandHandlerWrapper(QObject *parent) : QObject(parent)
{
    commandHandler = new CommandHandler(parent);
    merlinInterface = new MerlinInterface;
    connect(commandHandler,SIGNAL(requestForDataTaking(bool)),this,SLOT(on_requestForDataTaking(bool)));
    connect(commandHandler,SIGNAL(imageIsReady(QByteArray,Canvas)),this,SLOT(on_ImageIsReady(QByteArray,Canvas)));
    connect(QCstmGLVisualization::getInstance(),SIGNAL(busy(SERVER_BUSY_TYPE)),this,SLOT(on_serverStatusChanged(SERVER_BUSY_TYPE)));
    connect(thresholdScan::getInstance(),SIGNAL(busy(SERVER_BUSY_TYPE)),this,SLOT(on_serverStatusChanged(SERVER_BUSY_TYPE)));
    connect(QCstmDacs::getInstance(),SIGNAL(busy(SERVER_BUSY_TYPE)),this,SLOT(on_serverStatusChanged(SERVER_BUSY_TYPE)));
    connect(QCstmEqualization::getInstance(),SIGNAL(busy(SERVER_BUSY_TYPE)),this,SLOT(on_serverStatusChanged(SERVER_BUSY_TYPE)));
}

CommandHandler *CommandHandlerWrapper::getCommandHandler()
{
    return commandHandler;
}

void CommandHandlerWrapper::on_dataReceived(QString command)
{
    MerlinCommand merlinCmd (command, *merlinInterface);
    qDebug() << "[INFO]\tMapped : " << merlinCmd.parseResult;
    Command cmd(merlinCmd.parseResult);
    cmd.invoke(commandHandler,_serverStatus);
    //get response
    QString response = "";
    merlinCmd.setErrorExternally(cmd.getError());
    if(merlinCmd.getCommandType() == "SET" || merlinCmd.getCommandType() == "CMD")
        response = merlinCmd.makeSetCmdResponse();
    else if(merlinCmd.getCommandType() == "GET")
        response = merlinCmd.makeGetResponse(cmd.getData());
    else
        response = "[INFO]\tInvalid command type";

    emit responseIsReady(response); //must be passed to command socket in order to send it to the client
    qDebug() << "[INFO]\tResponse from handler : " << response;
}

void CommandHandlerWrapper::on_requestForDataTaking(bool)
{
    QString acqHeader = commandHandler->getAcquisitionHeader();
    const char* dummy="";
    emit imageIsReady(acqHeader.toLatin1(), Canvas());
    commandHandler->getImage();
}

void CommandHandlerWrapper::on_ImageIsReady(QByteArray header, Canvas image)
{
    emit imageIsReady(header,image);
}

void CommandHandlerWrapper::on_serverStatusChanged(SERVER_BUSY_TYPE flag)
{
    _serverStatus = flag;
    //qDebug() << "[DEBUG]\tBusy state : " << flag;
}
