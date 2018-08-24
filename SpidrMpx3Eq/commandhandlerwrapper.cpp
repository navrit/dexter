#include "commandhandlerwrapper.h"

CommandHandlerWrapper::CommandHandlerWrapper(QObject *parent) : QObject(parent)
{
    commandHandler = new CommandHandler(parent);
    merlinInterface = new MerlinInterface;
    connect(commandHandler,SIGNAL(requestForDataTaking(void)),this,SLOT(on_requestForDataTaking(void)));
    connect(commandHandler,SIGNAL(imageIsReady(QByteArray,QByteArray)),this,SLOT(on_ImageIsReady(QByteArray,QByteArray)));
}

void CommandHandlerWrapper::on_dataRecieved(QString command)
{
    qDebug() << "This command recieved at commandhandlerWrapper : " << command;

    MerlinCommand merlinCmd (command, *merlinInterface);
    Command cmd(merlinCmd.parseResult);
    cmd.invoke(commandHandler);
    //get response
    QString response = "";
    if(merlinCmd.getCommandType() == "SET" || merlinCmd.getCommandType() == "CMD")
        response = merlinCmd.makeSetCmdResponse();
    else if(merlinCmd.getCommandType() == "GET")
        response = merlinCmd.makeGetResponse(cmd.getData());
    else
        response = "Invalid command type";

    emit responseIsReady(response); //must be passed to command socket in order to send it to the client
    qDebug() << "response from handler : " << response;


}

void CommandHandlerWrapper::on_requestForDataTaking()
{
    QString acqHeader = commandHandler->getAcquisitionHeader();
    QByteArray dummy;
    emit imageIsReady(dummy,acqHeader.toLatin1());
    commandHandler->getImage();

//    QByteArray image;
//    image = QString("This is an test imageeeeeeeeeeeeeee....!").toLatin1();

}

void CommandHandlerWrapper::on_ImageIsReady(QByteArray header,QByteArray image)
{
    emit imageIsReady(header,image);
}
