#include "commandhandlerwrapper.h"
#include "qcstmglvisualization.h"

CommandHandlerWrapper::CommandHandlerWrapper(QObject *parent) : QObject(parent)
{
    commandHandler = new CommandHandler(parent);
    merlinInterface = new MerlinInterface;
    connect(commandHandler,SIGNAL(requestForDataTaking(bool)),this,SLOT(on_requestForDataTaking(bool)));
    connect(commandHandler,SIGNAL(imageIsReady(QByteArray,std::pair<const char*,int>)),this,SLOT(on_ImageIsReady(QByteArray,std::pair<const char*,int>)));
    connect(QCstmGLVisualization::getInstance(),SIGNAL(busyByTakingData(bool)),this,SLOT(on_serverIsBusy(bool)));
}

void CommandHandlerWrapper::on_dataRecieved(QString command)
{

    MerlinCommand merlinCmd (command, *merlinInterface);
    qDebug() << "mapped : " << merlinCmd.parseResult;
    Command cmd(merlinCmd.parseResult);
    cmd.invoke(commandHandler,_serverBusy);
    //get response
    QString response = "";
    merlinCmd.setErrorExternally(cmd.getError());
    if(merlinCmd.getCommandType() == "SET" || merlinCmd.getCommandType() == "CMD")
        response = merlinCmd.makeSetCmdResponse();
    else if(merlinCmd.getCommandType() == "GET")
        response = merlinCmd.makeGetResponse(cmd.getData());
    else
        response = "Invalid command type";

    emit responseIsReady(response); //must be passed to command socket in order to send it to the client
    qDebug() << "response from handler : " << response;

}

void CommandHandlerWrapper::on_requestForDataTaking(bool)
{
    QString acqHeader = commandHandler->getAcquisitionHeader();
    const char* dummy="";
    emit imageIsReady(acqHeader.toLatin1(), std::pair<const char*,int>(dummy,0));
    commandHandler->getImage();

//    QByteArray image;
//    image = QString("This is a test imageeeeeeeeeeeeeee....!").toLatin1();

}

void CommandHandlerWrapper::on_ImageIsReady(QByteArray header,std::pair<const char*,int> image)
{
    emit imageIsReady(header,image);
}

void CommandHandlerWrapper::on_serverIsBusy(bool flag)
{
    _serverBusy = flag;
}
