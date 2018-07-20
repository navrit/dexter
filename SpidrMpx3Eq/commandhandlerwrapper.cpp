#include "commandhandlerwrapper.h"

CommandHandlerWrapper::CommandHandlerWrapper(QObject *parent) : QObject(parent)
{
    commandHandler = new CommandHandler;
    merlinInterface = new MerlinInterface;
    connect(commandHandler,SIGNAL(requestForDataTaking(void)),this,SLOT(on_requestForDataTaking(void)));
    connect(commandHandler,SIGNAL(imageIsReady(QByteArray)),this,SLOT(on_ImageIsReady(QByteArray)));
}

void CommandHandlerWrapper::on_dataRecieved(QString command)
{
    qDebug() << "This command recieved at commandhandlerWrapper : " << command;

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

void CommandHandlerWrapper::on_requestForDataTaking()
{
    QString acqHeader = commandHandler->getAcquisitionHeader();
    emit imageIsReady(acqHeader.toLatin1());
    commandHandler->getImage();
//    QByteArray image;
//    image = QString("This is an test imageeeeeeeeeeeeeee....!").toLatin1();

}

void CommandHandlerWrapper::on_ImageIsReady(QByteArray image)
{
    emit imageIsReady(image);
}
