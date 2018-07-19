#include "commandhandlerwrapper.h"

CommandHandlerWrapper::CommandHandlerWrapper(QObject *parent) : QObject(parent)
{
    commandHandler = new CommandHandler;
    merlinInterface = new MerlinInterface;
}

void CommandHandlerWrapper::on_dataRecieved(QString command)
{
    qDebug() << "This command recieved at commandhndlerWrapper : " << command;

    QString merlinCmd = merlinInterface->parseCommand(command);
    commandHandler->setCmd(merlinCmd);
    commandHandler->fetchCmd();
    //get response
    QString response = "";
    if(merlinInterface->getCommandType() == "SET" || merlinInterface->getCommandType() == "CMD")
        response = merlinInterface->makeSetCmdResponse();
    else if(merlinInterface->getCommandType() == "GET")
        response = merlinInterface->makeGetResponse(commandHandler->getData());
    else
        response = "Invalid command type";

    emit responseIsReady(response); //must be passed to command socket in order to send it to the client
    qDebug() << "response from handler : " << response;

}
