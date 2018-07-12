#include "commandhandler.h"
#include <QDebug>
#include <QTime>
#include "handlerfunctions.h"
//#include "qcstmconfigmonitoring.h"
#include "qcstmglvisualization.h"
#include "qcstmdacs.h"
#include "thresholdscan.h"
#include "ui_thresholdscan.h"
#include "mpx3gui.h"



CommandHandler *cmdInst;



CommandHandler::CommandHandler(QObject *parent) : QObject(parent)
{

    connect(this,SIGNAL(requestForDataTaking()),QCstmGLVisualization::getInstance(),SLOT(StartDataTaking()));
    connect(this,SIGNAL(requestForInfDataTracking(bool)),QCstmGLVisualization::getInstance(),SLOT(on_infDataTakingCheckBox_toggled(bool)));
    connect(this,SIGNAL(requestForSnap()),QCstmGLVisualization::getInstance(),SLOT(on_singleshotPushButton_clicked()));
    connect(this,SIGNAL(requestForAutoSave(bool)),QCstmGLVisualization::getInstance(),SLOT(onRequestForAutoSaveFromServer(bool)));
    connect(this,SIGNAL(requestForSettingSavePath(QString)),QCstmGLVisualization::getInstance(),SLOT(onRequestForSettingPathFromServer(QString)));
//    connect(this,SIGNAL(requestForSettingSaveTag(int)),QCstmGLVisualization::getInstance(),SLOT(onRequestForSettingFormatFromServer(int)));
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
    cmd_struct setShutterPeriod{setShutterPeriodHandler};
    cmdTable.insert("SetShutterPeriod",setShutterPeriod);
    cmd_struct getShutterPeriod{getShutterPeriodHandler};
    cmdTable.insert("GetShutterPeriod",getShutterPeriod);
    cmd_struct setNumberOfFrame {setNumberOfFrameHandler};
    cmdTable.insert("SetNumberOfFrame",setNumberOfFrame);
    cmd_struct getNumberOfFrame {getNumberOfFrameHandler};
    cmdTable.insert("GetNumberOfFrame",getNumberOfFrame);
    cmd_struct setThreshold {setThresholdHandler};
    cmdTable.insert("SetThreshold",setThreshold);
    cmd_struct getThreshold {getThresholdHandler};
    cmdTable.insert("GetThreshold",getThreshold);
    cmd_struct setStartScan {setStartScanHandler};
    cmdTable.insert("SetStartScan",setStartScan);
    cmd_struct getStartScan {getStartScanHandler};
    cmdTable.insert("GetStartScan",getStartScan);
    cmd_struct setStopScan {setStopScanHandler};
    cmdTable.insert("SetStopScan",setStopScan);
    cmd_struct getStopScan {getStopScanHandler};
    cmdTable.insert("GetStopScan",getStopScan);
    cmd_struct setStepScan {setStepScanHandler};
    cmdTable.insert("SetStepScan",setStepScan);
    cmd_struct getStepScan {getStepScanHandler};
    cmdTable.insert("GetStepScan",getStepScan);
    cmd_struct setThresholdScan{setThresholdScanHandler};
    cmdTable.insert("SetThresholdScan",setThresholdScan);
    cmd_struct startScan {startScanHandler};
    cmdTable.insert("StartScan",startScan);
    cmd_struct stopScan{stopScanHandler};
    cmdTable.insert("StopScan",stopScan);
    cmd_struct setOperatingEnergy {setOperatingEnergyHandler};
    cmdTable.insert("SetOperatingEnergy",setOperatingEnergy);
    cmd_struct getOperatingEnergy{getOperatingEnergyHandler};
    cmdTable.insert("GetOperatingEnergy",getOperatingEnergy);
    cmd_struct setProfile {setProfileHandler};
    cmdTable.insert("SetProfile",setProfile);
    cmd_struct getProfile {getProfileHandler};
    cmdTable.insert("GetProfile",getProfile);
    cmd_struct setTriggerMode{setTriggerModeHandler};
    cmdTable.insert("SetTriggerMode",setTriggerMode);
    cmd_struct getTriggerMode{getTriggerModeHandler};
    cmdTable.insert("GetTriggerMode",getTriggerMode);
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

CommandHandler::ERROR_TYPE CommandHandler::getError()
{
    return _error;
}

void CommandHandler::setError(CommandHandler::ERROR_TYPE et)
{
    _error = et;
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

int CommandHandler::setThreshold(int idx, int val)
{
    if(idx >= 0 && idx <= 7){
        QCstmDacs::getInstance()->GetCheckBoxList()[idx]->setChecked(true);
        QCstmDacs::getInstance()->GetSpinBoxList()[idx]->setValue(val);
        return NO_ERROR;
    }
    return UNKWON_ERROR;
}

int CommandHandler::getThreshold(int idx)
{
    if(idx >= 0 && idx <= 7){
        if(QCstmDacs::getInstance()->GetCheckBoxList()[idx]->isChecked())
            return QCstmDacs::getInstance()->GetSpinBoxList()[idx]->value();
        return NO_ERROR;
    }
    return UNKWON_ERROR;
}

int CommandHandler::setStartScan(int val)
{
    if(val < 0 || val > 511)
        return ARG_VAL_OUT_RANGE;
    thresholdScan::getInstance()->GetUI()->spinBox_minimum->setValue(val);
    return NO_ERROR;
}

int CommandHandler::setStopScan(int val)
{
    if(val < 1 || val > 512)
        return ARG_VAL_OUT_RANGE;
    thresholdScan::getInstance()->GetUI()->spinBox_maximum->setValue(val);
    return NO_ERROR;
}

int CommandHandler::setStepScan(int val)
{
    if(val < 1 || val > 510)
        return ARG_VAL_OUT_RANGE;
    thresholdScan::getInstance()->GetUI()->spinBox_spacing->setValue(val);
    return NO_ERROR;
}

int CommandHandler::setThreholdScan(int val)
{
    if(val == 0)
    {
       thresholdScan::getInstance()->GetUI()->comboBox_thresholdToScan->setCurrentIndex(0);
       return NO_ERROR;
    }
    if(val == 1)
    {
       thresholdScan::getInstance()->GetUI()->comboBox_thresholdToScan->setCurrentIndex(1);
       return NO_ERROR;
    }
    return ARG_VAL_OUT_RANGE;

}

int CommandHandler::getStartScan()
{
    return thresholdScan::getInstance()->GetUI()->spinBox_minimum->value();
}

int CommandHandler::getStopScan()
{
    return thresholdScan::getInstance()->GetUI()->spinBox_maximum->value();
}

int CommandHandler::getStepScan()
{
    return thresholdScan::getInstance()->GetUI()->spinBox_spacing->value();
}

int CommandHandler::getThreholdScan()
{
    return thresholdScan::getInstance()->GetUI()->comboBox_thresholdToScan->currentIndex();
}

void CommandHandler::merlinErrorToPslError(int errNum)
{
    //ERROR_TYPE{NO_ERROR = 0, UNKOWN_ERROR = 1, UNKOWN_COMMAND = 2, PARAM_OUT_OF_RANGE = 3};
    //ERROR_TYPE{NO_ERROR = 0, UNKWON_ERROR = -1, UNKWON_COMMAND = -2 , ARG_NUM_OUT_RANGE = -3, ARG_VAL_OUT_RANGE = -4};
    switch (errNum) {
    case 0:
        _error = NO_ERROR;
        break;
    case 1:
        _error = UNKWON_ERROR;
        break;
    case 2:
        _error = UNKWON_COMMAND;
        break;
    case 3:
        _error = ARG_NUM_OUT_RANGE;
        break;
    default:
        _error = UNKWON_ERROR;
        break;
    }
}

void CommandHandler::setMerlinFrameHeader(FrameHeaderDataStruct &frameHeader)
{
    frameHeader.colorMode = (uint8_t) Mpx3GUI::getInstance()->getConfig()->getColourMode();
    frameHeader.counter = 0;/// to be set
    frameHeader.dataOffset = 256 + (128 * 4);
    frameHeader.frameNumbers = (uint32_t)Mpx3GUI::getInstance()->getConfig()->getNTriggers();
    uint8_t  gainMap[] = {3,2,1,0};
    frameHeader.gainMode =  gainMap[Mpx3GUI::getInstance()->getConfig()->getGainMode()];
    frameHeader.numberOfChips = (uint32_t)Mpx3GUI::getInstance()->getConfig()->getActiveDevices().count();
    for (int i = 0; i < frameHeader.numberOfChips; ++i) {
        frameHeader.chipSelect |= 1 << i;
    }
    frameHeader.shutterOpen = (double)  Mpx3GUI::getInstance()->getConfig()-> getTriggerLength_ms() / (double)1000;
    QPoint pnt = Mpx3GUI::getInstance()->getDataset()->getSize();
    frameHeader.xDim = (uint32_t)pnt.x();
    frameHeader.yDim =(uint32_t) pnt.y();
    QDateTime time = QDateTime::currentDateTime();
    char *d = time.toString("yyyy-dd-MM hh:mm:ss.ssssss").toLatin1().data();
    for (int i = 0; i < 25; ++i) {
        frameHeader.timeStamp[i] = d[i];
    }
    frameHeader.threshold0 = QCstmDacs::getInstance()->GetSpinBoxList()[0]->value();
    frameHeader.threshold1 = QCstmDacs::getInstance()->GetSpinBoxList()[1]->value();
    frameHeader.threshold2 = QCstmDacs::getInstance()->GetSpinBoxList()[2]->value();
    frameHeader.threshold3 = QCstmDacs::getInstance()->GetSpinBoxList()[3]->value();
    frameHeader.threshold4 = QCstmDacs::getInstance()->GetSpinBoxList()[4]->value();
    frameHeader.threshold5 = QCstmDacs::getInstance()->GetSpinBoxList()[5]->value();
    frameHeader.threshold6 = QCstmDacs::getInstance()->GetSpinBoxList()[6]->value();
    frameHeader.threshold7 = QCstmDacs::getInstance()->GetSpinBoxList()[7]->value();

    //dac
    frameHeader.dacThreshold0 = QCstmDacs::getInstance()->GetSpinBoxList()[0]->value();
    frameHeader.dacThreshold1 = QCstmDacs::getInstance()->GetSpinBoxList()[1]->value();
    frameHeader.dacThreshold2 = QCstmDacs::getInstance()->GetSpinBoxList()[2]->value();
    frameHeader.dacThreshold3 = QCstmDacs::getInstance()->GetSpinBoxList()[3]->value();
    frameHeader.dacThreshold4 = QCstmDacs::getInstance()->GetSpinBoxList()[4]->value();
    frameHeader.dacThreshold5 = QCstmDacs::getInstance()->GetSpinBoxList()[5]->value();
    frameHeader.dacThreshold6 = QCstmDacs::getInstance()->GetSpinBoxList()[6]->value();
    frameHeader.dacThreshold7 = QCstmDacs::getInstance()->GetSpinBoxList()[7]->value();
    frameHeader.preamp = QCstmDacs::getInstance()->GetSpinBoxList()[8]->value();
    frameHeader.ikrum = QCstmDacs::getInstance()->GetSpinBoxList()[9]->value();
    frameHeader.shaper = QCstmDacs::getInstance()->GetSpinBoxList()[10]->value();
    frameHeader.disc = QCstmDacs::getInstance()->GetSpinBoxList()[11]->value();
    frameHeader.discLs = QCstmDacs::getInstance()->GetSpinBoxList()[12]->value();
    frameHeader.shaperTest = QCstmDacs::getInstance()->GetSpinBoxList()[13]->value();
    frameHeader.dacDiscL = QCstmDacs::getInstance()->GetSpinBoxList()[14]->value();
    frameHeader.dacTest = QCstmDacs::getInstance()->GetSpinBoxList()[15]->value();
    frameHeader.dacDiscH = QCstmDacs::getInstance()->GetSpinBoxList()[16]->value();
    frameHeader.delay = QCstmDacs::getInstance()->GetSpinBoxList()[17]->value();
    frameHeader.tpBuffIn = QCstmDacs::getInstance()->GetSpinBoxList()[18]->value();
    frameHeader.tpBuffOut = QCstmDacs::getInstance()->GetSpinBoxList()[19]->value();
    frameHeader.rpz = QCstmDacs::getInstance()->GetSpinBoxList()[20]->value();
    frameHeader.gnd = QCstmDacs::getInstance()->GetSpinBoxList()[21]->value();
    frameHeader.tpRef = QCstmDacs::getInstance()->GetSpinBoxList()[22]->value();
    frameHeader.fpk = QCstmDacs::getInstance()->GetSpinBoxList()[23]->value();
    frameHeader.cas = QCstmDacs::getInstance()->GetSpinBoxList()[24]->value();
    frameHeader.tpRefA = QCstmDacs::getInstance()->GetSpinBoxList()[25]->value();
    frameHeader.tpRefB = QCstmDacs::getInstance()->GetSpinBoxList()[26]->value();
}

QString CommandHandler::generateMerlinFrameHeader(FrameHeaderDataStruct frameHeader)
{
    setMerlinFrameHeader(frameHeader);
    QString header = frameHeader.headerID + "," + QString::number(frameHeader.frameNumbers) + "," + QString::number(frameHeader.dataOffset) + "," +
             QString::number(frameHeader.numberOfChips) +","+ QString::number(frameHeader.xDim) +","+ QString::number(frameHeader.yDim) +","+ frameHeader.pixelDepth
            + "," + frameHeader.sensorLayout + "," + QString::number(frameHeader.chipSelect) + "," + frameHeader.timeStamp + "," +
            QString::number(frameHeader.shutterOpen) + "," + QString::number(frameHeader.counter) + "," + QString::number(frameHeader.colorMode) + "," +
            QString::number(frameHeader.gainMode) + "," + QString::number(frameHeader.threshold0) + "," +QString::number(frameHeader.threshold1) +
            "," + QString::number(frameHeader.threshold2) + "," + QString::number(frameHeader.threshold3) + "," + QString::number(frameHeader.threshold4)
            + "," + QString::number(frameHeader.threshold5) + "," + QString::number(frameHeader.threshold6) + "," + QString::number(frameHeader.threshold7)+","+
            frameHeader.dacFormat + "," + QString::number(frameHeader.dacThreshold0) + "," + QString::number(frameHeader.dacThreshold1)
            + "," + QString::number(frameHeader.dacThreshold2) + "," + QString::number(frameHeader.dacThreshold3) + "," + QString::number(frameHeader.dacThreshold4)
            + "," + QString::number(frameHeader.dacThreshold5)+ "," + QString::number(frameHeader.dacThreshold6)+ "," + QString::number(frameHeader.dacThreshold7)
            + "," + QString::number(frameHeader.preamp) + "," + QString::number(frameHeader.ikrum) + "," + QString::number(frameHeader.shaper)
            + "," + QString::number(frameHeader.disc) + "," + QString::number(frameHeader.discLs) + "," + QString::number(frameHeader.shaperTest)
            + "," + QString::number(frameHeader.dacDiscL) + "," + QString::number(frameHeader.dacTest) + "," + QString::number(frameHeader.dacDiscH)
            + "," + QString::number(frameHeader.delay) + "," + QString::number(frameHeader.tpBuffIn) + "," + QString::number(frameHeader.tpBuffOut)
            + "," + QString::number(frameHeader.rpz) + "," + QString::number(frameHeader.gnd) + "," + QString::number(frameHeader.tpRef)
            + "," + QString::number(frameHeader.fpk) + "," + QString::number(frameHeader.cas) + "," + QString::number(frameHeader.tpRefA)
            + "," + QString::number(frameHeader.tpRefB);
    if(header.length() > frameHeader.dataOffset)
        header = header.mid(0,frameHeader.dataOffset);
    else if(header.length() < frameHeader.dataOffset)
    {
        int diff = frameHeader.dataOffset - header.length();
        for (int i = 0; i < diff; ++i) {
            header.append(' ');
        }
    }
    return header;
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

        data = cmd; //"Command does not exist...!";
        merlinErrorToPslError(cmd.toInt());
        qDebug()<<"No Matched...."<<cmd.toInt();
        emit commandIsDecoded(data,imageToSend,false);
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





