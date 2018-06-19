#include "commandhandler.h"
#include <QDebug>
#include "handlerfunctions.h"
//#include "qcstmconfigmonitoring.h"
#include "qcstmglvisualization.h"


CommandHandler *cmdInst;



CommandHandler::CommandHandler(QObject *parent) : QObject(parent)
{
    connect(this,SIGNAL(requestForDataTaking()),QCstmGLVisualization::getInstance(),SLOT(StartDataTaking()));
    connect(this,SIGNAL(requestForInfDataTracking(bool)),QCstmGLVisualization::getInstance(),SLOT(on_infDataTakingCheckBox_toggled(bool)));
    connect(this,SIGNAL(requestForSnap()),QCstmGLVisualization::getInstance(),SLOT(on_singleshotPushButton_clicked()));
    connect(this,SIGNAL(requestForAutoSave(bool)),QCstmGLVisualization::getInstance(),SLOT(onRequestForAutoSaveFromServer(bool)));
    connect(this,SIGNAL(requestForSettingSavePath(QString)),QCstmGLVisualization::getInstance(),SLOT(onRequestForSettingPathFromServer(QString)));
    connect(this,SIGNAL(requestForSettingSaveTag(int)),QCstmGLVisualization::getInstance(),SLOT(onRequestForSettingFormatFromServer(int)));
    cmdInst = this;
    initializeCmdTable();
}

void CommandHandler::on_cmdRecieved(char *command)
{
//    QString str;
//    str.sprintf("%s",command);
//    cmd = str;
//    qDebug()<<"from de :"<<cmd;
//    fetchCmd();

}




void CommandHandler::initializeCmdTable()
{
    cmd_struct hello {helloHandler};
    cmdTable.insert("Hello",hello);
    cmd_struct bye{byeHandler};
    cmdTable.insert("Bye",bye);
    cmd_struct setReadoutMode {setReadoutModeHandler};
    cmdTable.insert("SetReadoutMode",setReadoutMode);
    cmd_struct getReadoutMode {getReadoutModeHandler};
    cmdTable.insert("GetReadoutMode",getReadoutMode);
    cmd_struct setCounterDepth{setCounterDepthHandler};
    cmdTable.insert("SetCounterDepth",setCounterDepth);
    cmd_struct getCounterDepth{getCounterDepthHandler};
    cmdTable.insert("GetCounterDepth",getCounterDepth);
    cmd_struct setOperationalMode{setOperationalModeHandler};
    cmdTable.insert("SetOperationalMode",setOperationalMode);
    cmd_struct getOperationalMode{getOperationalModeHandler};
    cmdTable.insert("GetOperationalMode",getOperationalMode);
    cmd_struct open {openHandler};
    cmdTable.insert("Open",open);
    cmd_struct close {closeHandler};
    cmdTable.insert("Close",close);
    cmd_struct start{startHandler};
    cmdTable.insert("Start",start);
    cmd_struct stop{stopHandler};
    cmdTable.insert("Stop",stop);
    cmd_struct snap{snapHandler};
    cmdTable.insert("Snap",snap);
    cmd_struct setAutoSave { setAutoSaveHandler};
    cmdTable.insert("SetAutoSave",setAutoSave);
    cmd_struct setRecordPath{setRecordPathHandler};
    cmdTable.insert("SetRecordPath",setRecordPath);
    cmd_struct setRecordFormat {setRecordFormatHandler};
    cmdTable.insert("SetRecordFormat",setRecordFormat);
    cmd_struct getImage{getImageHandler};
    cmdTable.insert("GetImage",getImage);
    cmd_struct setGainMode {setGainModeHandler};
    cmdTable.insert("SetGainMode",setGainMode);
    cmd_struct getGainMode {getGainModeHandler};
    cmdTable.insert("GetGainMode",getGainMode);
    cmd_struct setPolarity {setPolarityHandler};
    cmdTable.insert("SetPolarity",setPolarity);
    cmd_struct getPolarity { getPolarityHandler};
    cmdTable.insert("GetPolarity",getPolarity);
    cmd_struct setCounterSelectFrequency {setCounterSelectFrequencyHandler};
    cmdTable.insert("SetCounterSelectFrequency",setCounterSelectFrequency);
    cmd_struct getCounterSelectFrequency = {getCounterSelectFrequencyHandler};
    cmdTable.insert("GetCounterSelectFrequency",getCounterSelectFrequency);
    cmd_struct setShutterLength{setShutterLengthHandler};
    cmdTable.insert("SetShutterLength",setShutterLength);
    cmd_struct getShutterLength{getShutterLengthHandler};
    cmdTable.insert("GetShutterLength",getShutterLength);
    cmd_struct setBothCounters{setBothCountersHandler};
    cmdTable.insert("SetBothCounters",setBothCounters);
    cmd_struct getBothCounters{getBothCountersHandler};
    cmdTable.insert("GetBothCounters",getBothCounters);
    cmd_struct setColourMode{setColourModeHandler};
    cmdTable.insert("SetColourMode",setColourMode);
    cmd_struct getColourMode{getColourModeHandler};
    cmdTable.insert("GetColourMode",getColourMode);
    cmd_struct setLutTable{setLutTableHandler};
    cmdTable.insert("SetLutTable",setLutTable);
    cmd_struct getLutTable{getLutTableHandler};
    cmdTable.insert("GetLutTable",getLutTable);
}

bool CommandHandler::enoughArguments(int argsNum,QString command)
{
    if(!cmdTable.contains(command)){
        data = "Command does not exist...!";
        return false;
    }
    if(argsNum != cmdTable[command].args.size())
    {
        data = "Too many or too few arguments...";
        return false;
    }
    return true;
}

CommandHandler *CommandHandler::getInstance()
{
    return cmdInst;
}

void CommandHandler::startLiveCamera()
{
    emit requestForInfDataTracking(true);
    emit requestForDataTaking();
    //QCstmGLVisualization::getInstance()->setInfDataTaking(true);
}

void CommandHandler::startSnap()
{
    emit requestForSnap();
}

void CommandHandler::setAutoSave(bool val)
{
    emit requestForAutoSave(val);
}

void CommandHandler::setRecordPath(QString path)
{
    emit requestForSettingSavePath(path);
}

void CommandHandler::setRecordFormat(int idx)
{
    emit requestForSettingSaveFormat(idx);
}





QString CommandHandler::getData()
{
    return data;
}

void CommandHandler::fetchCmd()
{
    if(cmdTable.contains(cmd))
    {

        cmdTable[cmd].args = arguments;
        cmdTable[cmd].handler();
    }
    else
    {
        data = "Command does not exist...!";
        return;
    }
    if(cmd == "GetImage")
        emit commandIsDecoded(data,imageToSend,true);
    else
        emit commandIsDecoded(data,imageToSend,false);
}

void CommandHandler::setCmd(char* command)
{
   arguments.clear();
   QString stringCmd;
   stringCmd.sprintf("%s",command);
   QStringList cmdList = stringCmd.split(";");
   cmd = cmdList.at(0); //core command

   for(int i=1; i<cmdList.size(); i++){
       QString str = cmdList.at(i);

       //! Tolerate new lines sent from netcat
       QStringList list = str.split(QRegularExpression("(\\n)"));
       arguments.push_back(list.join(""));
   }
}

void CommandHandler::setData(QString d)
{
    data = d;
}

void CommandHandler::setImage(QByteArray im)
{
    imageToSend = im;
}

void CommandHandler::print()
{
    qDebug() << "Core command: " << cmd;
    for(int i=0; i<arguments.size(); i++){
        qDebug() << "argument: " << arguments.at(i);
    }
}





