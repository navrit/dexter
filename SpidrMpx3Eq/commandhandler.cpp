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
#include "dataconsumerthread.h"



CommandHandler *cmdInst;



CommandHandler::CommandHandler(QObject *parent) : QObject(parent)
{

    connect(this,SIGNAL(requestForDataTaking()),QCstmGLVisualization::getInstance(),SLOT(StartDataTaking()));
    connect(this,SIGNAL(requestForInfDataTracking(bool)),QCstmGLVisualization::getInstance(),SLOT(on_infDataTakingCheckBox_toggled(bool)));
    connect(this,SIGNAL(requestForSnap()),QCstmGLVisualization::getInstance(),SLOT(on_singleshotPushButton_clicked()));
    connect(this,SIGNAL(requestForAutoSave(bool)),QCstmGLVisualization::getInstance(),SLOT(onRequestForAutoSaveFromServer(bool)));
    connect(this,SIGNAL(requestForSettingSavePath(QString)),QCstmGLVisualization::getInstance(),SLOT(onRequestForSettingPathFromServer(QString)));
    connect(QCstmGLVisualization::getInstance(),SIGNAL(someCommandHasFinished_Successfully()),this,SLOT(on_someCommandHasFinished_Successfully()));
    //    connect(this,SIGNAL(requestForSettingSaveTag(int)),QCstmGLVisualization::getInstance(),SLOT(onRequestForSettingFormatFromServer(int)));
   //if(cmdInst == nullptr)
    cmdInst = this;
    initializeCmdTable();
}

void CommandHandler::on_cmdRecieved(QString command)
{
//    qDebug() << "This command recieved at commandhndler : " << command;
//    this->setCmd(command);
//    qDebug() << "after : " << cmd;
//    this->fetchCmd();

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
    cmdTable.insert("SetFrameNumber",setNumberOfFrame);
    cmd_struct getNumberOfFrame {getNumberOfFrameHandler};
    cmdTable.insert("GetFrameNumber",getNumberOfFrame);
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
    //emit requestForInfDataTracking(true);
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

void CommandHandler::startSendingImage(bool send)
{
    _sendingImage = send;
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

void CommandHandler::fillMerlinFrameHeader(FrameHeaderDataStruct &frameHeader)
{
    Mpx3GUI* gui = (Mpx3GUI*) parent();
    if(gui->getConfig()->getPixelDepth() == 24)
        frameHeader.pixelDepth = "U32";
    else
        frameHeader.pixelDepth = "U16";

    frameHeader.colorMode = (uint8_t) gui->getConfig()->getColourMode();
    frameHeader.counter = 0;/// to be set
    frameHeader.dataOffset = 256 + (128 * 4);
    uint8_t  gainMap[] = {3,2,1,0};
    frameHeader.gainMode =  gainMap[gui->getConfig()->getGainMode()];
    frameHeader.numberOfChips = (uint32_t)gui->getConfig()->getActiveDevices().count();
    for (int i = 0; i < frameHeader.numberOfChips; ++i) {
        frameHeader.chipSelect |= 1 << i;
    }
    frameHeader.shutterOpen = (double)  gui->getConfig()-> getTriggerLength_ms() / (double)1000;
    QPoint pnt = gui->getDataset()->getSize();
    frameHeader.xDim = (uint32_t)pnt.x()*2;
    frameHeader.yDim =(uint32_t) pnt.y()*2;
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

QString CommandHandler::generateMerlinFrameHeader(int frameid)
{
    FrameHeaderDataStruct frameHeader;
    fillMerlinFrameHeader(frameHeader);
    QString header = "MQ1," % QString::number(frameid) % "," % QString::number(frameHeader.dataOffset) % "," %
             QString::number(frameHeader.numberOfChips) %","% QString::number(frameHeader.xDim) %","% QString::number(frameHeader.yDim) %","% frameHeader.pixelDepth
            % "," % frameHeader.sensorLayout % "," % QString::number(frameHeader.chipSelect) % "," % frameHeader.timeStamp % "," %
            QString::number(frameHeader.shutterOpen) % "," % QString::number(frameHeader.counter) % "," % QString::number(frameHeader.colorMode) % "," %
            QString::number(frameHeader.gainMode) % "," % QString::number(frameHeader.threshold0) % "," %QString::number(frameHeader.threshold1) %
            "," % QString::number(frameHeader.threshold2) % "," % QString::number(frameHeader.threshold3) % "," % QString::number(frameHeader.threshold4)
            % "," % QString::number(frameHeader.threshold5) % "," % QString::number(frameHeader.threshold6) % "," % QString::number(frameHeader.threshold7)%","%
            frameHeader.dacFormat % "," % QString::number(frameHeader.dacThreshold0) % "," % QString::number(frameHeader.dacThreshold1)
            % "," % QString::number(frameHeader.dacThreshold2) % "," % QString::number(frameHeader.dacThreshold3) % "," % QString::number(frameHeader.dacThreshold4)
            % "," % QString::number(frameHeader.dacThreshold5)% "," % QString::number(frameHeader.dacThreshold6)% "," % QString::number(frameHeader.dacThreshold7)
            % "," % QString::number(frameHeader.preamp) % "," % QString::number(frameHeader.ikrum) % "," % QString::number(frameHeader.shaper)
            % "," % QString::number(frameHeader.disc) % "," % QString::number(frameHeader.discLs) % "," % QString::number(frameHeader.shaperTest)
            % "," % QString::number(frameHeader.dacDiscL) % "," % QString::number(frameHeader.dacTest) % "," % QString::number(frameHeader.dacDiscH)
            % "," % QString::number(frameHeader.delay) % "," % QString::number(frameHeader.tpBuffIn) % "," % QString::number(frameHeader.tpBuffOut)
            % "," % QString::number(frameHeader.rpz) % "," % QString::number(frameHeader.gnd) % "," % QString::number(frameHeader.tpRef)
            % "," % QString::number(frameHeader.fpk) % "," % QString::number(frameHeader.cas) % "," % QString::number(frameHeader.tpRefA)
            % "," % QString::number(frameHeader.tpRefB);
    /*
    if(header.length() > frameHeader.dataOffset)
        header = header.mid(0,frameHeader.dataOffset);
    else if(header.length() < frameHeader.dataOffset)
    {
        int diff = frameHeader.dataOffset - header.length();
        for (int i = 0; i < diff; ++i) {
            header.append(' ');
        }
    }*/
    return header.leftJustified(frameHeader.dataOffset, ' ', true);
}

QString CommandHandler::getAcquisitionHeader()
{
    Mpx3GUI* gui = (Mpx3GUI*) parent();
    Mpx3Config* config = gui->getConfig();
    QString len = QString::number(2044+3+2);
    QString file = "HDR,\nChip ID:	" % config->getDeviceWaferId(0) % "," % config->getDeviceWaferId(1) % "," % config->getDeviceWaferId(2) % "," % config->getDeviceWaferId(3) % ","
            % "\nChip Type (Medipix 3.0, Medipix 3.1, Medipix RX):	Medipix3RXAssembly Size (1X1, 2X2):	   2x2Chip Mode  (SPM, CSM, CM, CSCM):	SPM Counter Depth (number):	12Gain:	HGMActive Counters:	Counter 0Thresholds (keV):	0.000000E+0,1.000000E+1,1.500000E+1,2.000000E+1,2.500000E+1,3.000000E+1,3.500000E+1,4.000000E+1DACs:	030,056,083,111,139,167,194,222,100,010,125,125,100,100,080,100,090,050,128,004,255,148,128,203,189,417,417; 030,056,083,111,139,167,194,222,100,010,125,125,100,100,080,100,090,050,128,004,255,142,128,192,180,417,417; 030,056,083,111,139,167,194,222,100,010,125,125,100,100,080,100,090,050,128,004,255,151,128,205,191,417,417; 030,056,083,111,139,167,194,222,100,010,125,125,100,100,080,100,090,050,128,004,255,138,128,189,181,417,417bpc File:	c:\MERLIN Quad Host\Config\W117_E7\W117_E7_SPM.bpc,c:\MERLIN Quad Host\Config\W117_H7\W117_H7_SPM.bpc,c:\MERLIN Quad Host\Config\W117_I7\W117_I7_SPM.bpc,c:\MERLIN Quad Host\Config\W117_G7\W117_G7_SPM.bpcDAC File:	c:\MERLIN Quad Host\Config\W117_E7\W117_E7_SPM.dacs,c:\MERLIN Quad Host\Config\W117_H7\W117_H7_SPM.dacs,c:\MERLIN Quad Host\Config\W117_I7\W117_I7_SPM.dacs,c:\MERLIN Quad Host\Config\W117_G7\W117_G7_SPM.dacsGap Fill Mode:	NoneFlat Field File:	Dummy (C:\<NUL>\Temp.ffc)Dead Time File:	Dummy (C:\<NUL>\Temp.dtc)Acquisition Type (Normal, Th_scan, Config):	NormalFrames in Acquisition (Number):	  1000Trigger Start (Positive, Negative, Internal):	InternalTrigger Stop (Positive, Negative, Internal):	InternalFrames per Trigger (Number):	1Time and Date Stamp (yr, mnth, day, hr, min, s):	10/12/2013 17:36:32Sensor Bias (V, µA)	20 VSensor Polarity (Positive, Negative):	PositiveTemperature (C):	FPGA Temp 37.250000 Deg CMedipix Clock (MHz):	120MHzReadout System:	Merlin QuadSoftware Version:	DevelopmentEnd";
    int fileLen = file.length();
    if(fileLen < 2044){
        int s = 2044 - fileLen;
        for (int i = 0; i <s ; ++i) {
            file.append(' ');
        }
    }
    //add leading zeros to len
    QString zeros ="";
    for (int i = 0; i < 10 - len.length(); ++i) {
        zeros += "0";
    }
    len = zeros + len;
    QString acqHeader = "MPX,"+len+file + "\n";
    return acqHeader;
}

void CommandHandler::getImage()
{
    connect(QCstmGLVisualization::getInstance()->getDataConsumerThread(),SIGNAL(doneWithOneFrame(int)),this,SLOT(on_doneWithOneFrame(int)));
//    int nFrames = Mpx3GUI::getInstance()->getConfig()->getNTriggers();
//    int frameCounter = 0;
//    int size = 5;
//    QString len = "";

//    while(frameCounter <nFrames){

//        FrameHeaderDataStruct frameHeader;
//        QByteArray ba;
//        QString hd = generateMerlinFrameHeader(frameHeader);
//        QByteArray frame = Mpx3GUI::getInstance()->getDataset()->toSocketData();
//        size = hd.length();
//        size += frame.length();
//        len = QString::number(size);

//        QString zeros ="";
//        for (int i = 0; i < 10 - len.length(); ++i) {
//            zeros += "0";
//        }
//        len = zeros + len;
//        QString firstPart = "MPX,"+ len + ",MQ1," + hd;
//        ba += firstPart.toLatin1();
//        ba += frame;
//        emit imageIsReady(ba);
//        frameCounter++;

//    }
//    FrameHeaderDataStruct frameHeader;
//    QByteArray ba = generateMerlinFrameHeader(frameHeader).toLatin1().data();
//    ba += Mpx3GUI::getInstance()->getDataset()->toSocketData();
//    commandIsDecoded("",ba,true);
    return;
}


void CommandHandler::on_doneWithOneFrame(int frameid)
{
    Mpx3GUI* gui = (Mpx3GUI*) parent();
    QElapsedTimer tim; tim.start();
    QString hd = generateMerlinFrameHeader(frameid);
    qint64 nano1 = tim.nsecsElapsed();
    QByteArray frame = gui->getDataset()->toSocketData();
    qint64 nano2 = tim.nsecsElapsed();
    auto size = hd.length() + frame.length();
    auto len = QString("%1").arg(size, 10, 10, QChar('0'));
    QString firstPart = "MPX," + len + "," + hd;
    qint64 nano3 = tim.nsecsElapsed();
    emit imageIsReady(firstPart.toLatin1(),frame);

    qint64 nanos = tim.nsecsElapsed();
    if (nanos > 600000)
        qDebug() << "How Long " << nanos << " " << (nano1) << " " << (nano2 - nano1) << " " << (nano3 - nano2) << " " << (nanos - nano3);

}

void CommandHandler::on_someCommandHasFinished_Successfully()
{
    disconnect(QCstmGLVisualization::getInstance()->getDataConsumerThread(),SIGNAL(doneWithOneFrame(int)),this,SLOT(on_doneWithOneFrame(int)));
}

void CommandHandler::emitrequestForAnotherSocket(int port)
{
    emit requestAnotherSocket(port);
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
       // emit commandIsDecoded(data,nullptr,false);
        return;
    }
   // if(cmd == "GetImage")
        //emit commandIsDecoded(data,nullptr,true);
    //else
        //emit commandIsDecoded(data,nullptr,false);
}

void CommandHandler::setCmd(QString command)
{
   arguments.clear();
   QStringList cmdList = command.split(";");
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





