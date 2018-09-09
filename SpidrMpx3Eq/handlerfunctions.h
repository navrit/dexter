#ifndef HANDLERFUNCTIONS_H
#define HANDLERFUNCTIONS_H

#include "commandhandler.h"
#include "mpx3gui.h"
#include "qcstmglvisualization.h"
#include "MerlinInterface.h"


//handler functions
void helloHandler(CommandHandler* ch, Command* cmd)
{
    cmd->setData("Hello From Dexter Server");
    cmd->setError(NO_ERROR);
    QString hd = ch->generateMerlinFrameHeader(1);
    qDebug() <<"header is" << hd;
    qDebug() << "header size is : " << hd.length();
}

void byeHandler(CommandHandler* ch, Command* cmd)
{
    QString tstData = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type ki";
    if(cmd->enoughArguments(1,"Bye"))
        cmd->setData(tstData);
}

void setReadoutModeHandler(CommandHandler* ch, Command* cmd)
{
    if(!cmd->enoughArguments(1,"SetReadoutMode")) //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "seq"){ //set readout mode to sequential
        // here code for setting the readout mode to sequential
        Mpx3GUI::getInstance()->getConfig()->setOperationMode(0);
        cmd->setData("Readout mode is set to sequential");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "cont"){ //set readout mode to continuous
        // here code for setting the readout mode to continuous
        Mpx3GUI::getInstance()->getConfig()->setOperationMode(1);
        cmd->setData("Readout mode is set to continuous");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setData("Invalid argument...!");
        cmd->setError(UNKNOWN_COMMAND);
    }

}

void getReadoutModeHandler(CommandHandler* ch, Command* cmd)
{
    //here code to get the readout mode
    if(Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 0)
    {
        cmd->setData("0");
        cmd->setError(NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 1)
    {
        cmd->setData("1");
        cmd->setError(NO_ERROR);
        return;
    }

    cmd->setData("-1");
    cmd->setError(UNKNOWN_ERROR);


}

void setCounterDepthHandler(CommandHandler* ch, Command* cmd)
{
    if(!cmd->enoughArguments(1,"SetCounterDepth"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "1"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(1);
        cmd->setData("Counter depth is set to 1 bit...!");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "6"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(6);
        cmd->setData("Counter depth is set to 6 bit...!");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "12"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(12);
        cmd->setData("Counter depth is set to 12 bit...!");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "24"){
        //here code to set depth

        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(24);
        cmd->setData("Counter depth is set to 24 bit...!");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setData("Invalid argument...!");
        cmd->setError(UNKNOWN_COMMAND);
    }

}

void getCounterDepthHandler(CommandHandler* ch, Command* cmd){
    //here code to get the readout mode
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 1)
    {
        cmd->setData("1");
        cmd->setError(NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 6)
    {
        cmd->setData("6");
        cmd->setError(NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 12)
    {
        cmd->setData("12");
        cmd->setError(NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 24)
    {
        cmd->setData("24");
        cmd->setError(NO_ERROR);
        return;
    }
    cmd->setData("-1");//-1
    cmd->setError(UNKNOWN_ERROR);
}

void getTemperatureHandler(CommandHandler* ch, Command* cmd) {
    SpidrController* spidrcontrol = Mpx3GUI::getInstance()->GetSpidrController();

    if (spidrcontrol) {
        int mdegrees;
        if (spidrcontrol->getRemoteTemp(&mdegrees)) {
            cmd->setData(QString::number((int) (mdegrees/1000)));
            cmd->setError(NO_ERROR);
        }
    } else {
        cmd->setData(QString::number(-1));
        cmd->setError(UNKNOWN_ERROR);
    }
}

void setOperationalModeHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetOperationalMode"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "spm"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setCsmSpm(0);
        cmd->setData("Operational mode is set to spm...!");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "csm"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setCsmSpm(1);
        cmd->setData("Operational mode is set to csm...!");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }
}

void getOperationalModeHandler(CommandHandler* ch, Command* cmd){
    //here code to get the operational mode
    cmd->setData("Getting the operational mode....!");

}
void openHandler(CommandHandler* ch, Command* cmd){
    if(Mpx3GUI::getInstance()->establish_connection()){
        cmd->setError(NO_ERROR);
        cmd->setData("0");
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("-1");
    }
}
void closeHandler(CommandHandler* ch, Command* cmd){
    Mpx3GUI::getInstance()->closeRemotely();
    cmd->setData("0");
    cmd->setError(NO_ERROR);
}
bool isStarted = false;
void snapHandler(CommandHandler* ch, Command* cmd){
    if(!isStarted){
       ch->startSnap();
       cmd->setData(QString::number(0));
       cmd->setError(NO_ERROR);
       return;
    }
    cmd->setData(QString::number(-1));
    cmd->setError(UNKNOWN_ERROR);
}

void startHandler(CommandHandler* ch, Command* cmd){

    if(!isStarted){

        ch->startLiveCamera();
        isStarted = true;

    }
    int d = (isStarted) ? 0 : -1;
    cmd->setData(QString::number(d));
    if(d == 0)
    {
        cmd->setError(NO_ERROR);
//        QThread::usleep(20000);
////        ch->startSendingImage(true);
   //    ch-> sendMerlinImage();
//        ch->emitrequestForAnotherSocket(6352);
    }
    else
        cmd->setError(UNKNOWN_ERROR);


}
void stopHandler(CommandHandler* ch, Command* cmd){
    if(isStarted){
        ch->startLiveCamera();
        isStarted = false;
    }
    int d = (isStarted) ? -1 : 0;
    cmd->setData(QString::number(d));
    if(d == 0)
    {
        cmd->setError(NO_ERROR);
        ch->startSendingImage(false);
    }
    else
        cmd->setError(UNKNOWN_ERROR);
}

void setTriggerModeHandler(CommandHandler* ch, Command* cmd){
    /*
    #define SHUTTERMODE_POS_EXT        0
    #define SHUTTERMODE_NEG_EXT        1
    #define SHUTTERMODE_POS_EXT_TIMER  2
    #define SHUTTERMODE_NEG_EXT_TIMER  3
    #define SHUTTERMODE_AUTO           4
    #define SHUTTERTRIG_POS_EXT_CNTR   5
     */
    if(!cmd->enoughArguments(1,"SetTriggerMode"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "0")
    {
        Mpx3GUI::getInstance()->getConfig()->setTriggerMode(4);
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("-1");
    }
}
void getTriggerModeHandler(CommandHandler* ch, Command* cmd){
    int val = Mpx3GUI::getInstance()->getConfig()->getTriggerMode();
    if(val == 4){
        cmd->setError(NO_ERROR);
        cmd->setData("0");
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("-1");
    }
}

void setGainModeHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetGainMode"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    auto arg = cmd->arguments.at(0);
    for (int i = 0; i < 3; i++) {
        if (arg == gainModeStrTable[i]) {
            Mpx3GUI::getInstance()->getConfig()->setGainMode(i);
            cmd->setData("Gain mode is set to " + arg + "...!");
            cmd->setError(NO_ERROR);
            return;
        }
    }
    cmd->setData("Invalid argument...!");
    cmd->setError(UNKNOWN_COMMAND);
}

void getGainModeHandler(CommandHandler* ch, Command* cmd){
    int gainMode = Mpx3GUI::getInstance()->getConfig()->getGainMode();
    //QString gainModeStrTable[] = {"SuperHigh","High","Low","SuperLow"};
    if(gainMode >= 0 && gainMode <= 3){
        cmd->setError(NO_ERROR);
        cmd->setData(QString::number(3-gainMode));
        return;
    }
    cmd->setError(UNKNOWN_ERROR);
    cmd->setData("-1");
    return;
}

void setPolarityHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetPolarity"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "neg"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setPolarity(1);
        cmd->setData("Polarity is set to negative...!");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "pos"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setPolarity(0);
        cmd->setData("Polarity is set to positive...!");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }
}
void getPolarityHandler(CommandHandler* ch, Command* cmd){
    if(Mpx3GUI::getInstance()->getConfig()->getPolarity())
    {
        cmd->setData("1");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(NO_ERROR);
        cmd->setData("0");
    }
}
void setCounterSelectFrequencyHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetCounterSelectFrequency"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int freq = cmd->arguments.at(0).toInt();
    Mpx3GUI::getInstance()->getConfig()->setContRWFreq(freq);
    cmd->setData("CRW frequency is set to " + QString::number(freq));
    cmd->setError(NO_ERROR);

}
void getCounterSelectFrequencyHandler(CommandHandler* ch, Command* cmd){
    int freq = Mpx3GUI::getInstance()->getConfig()->getContRWFreq();
    cmd->setData(QString::number(freq));
    cmd->setError(NO_ERROR);
}

void setShutterLengthHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(2,"SetShutterLength"))  //this command comes with two argument {SetShutterLength;{open,down};value
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    auto openOrClose = cmd->arguments.at(0);
    auto length = cmd->arguments.at(1); //ms
    auto config = Mpx3GUI::getInstance()->getConfig();
    if (openOrClose == "open"){
        //here code to set operational mode
        config->setTriggerLength((int) (1000. * length.toDouble()));    // us
        cmd->setData("Shutter open length is set to " + length);
        cmd->setError(NO_ERROR);
    }
    else if(openOrClose == "down"){
        //here code to set operational mode
        config->setTriggerDowntime((int) (1000. * length.toDouble()));  // us
        cmd->setData("Shutter down length is set to " + length);
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setData("Invalid argument...!");
        cmd->setError(UNKNOWN_COMMAND);
    }
}

void setShutterPeriodHandler(CommandHandler* ch, Command* cmd){
    if (!cmd->enoughArguments(1,"SetShutterPeriod"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 0)//sequential mode
    {
        auto config = Mpx3GUI::getInstance()->getConfig();
        auto length = cmd->arguments.at(0); //ms
        int open = config->getTriggerLength();
        config->setTriggerDowntime(1000. * length.toDouble() - open);   // us
        //qDebug()<<"dodododod:" << length.toDouble();
        cmd->setError(NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 1)//continous mode
    {
        auto length = cmd->arguments.at(0); //ms;
        double freq = 1./(length.toDouble()*2) * 1000.;
        Mpx3GUI::getInstance()->getConfig()->setContRWFreq(freq);
        cmd->setError(NO_ERROR);
        return;
    }
    cmd->setError(UNKNOWN_ERROR);
    return;

}

void getShutterPeriodHandler(CommandHandler* ch, Command* cmd){
    auto config = Mpx3GUI::getInstance()->getConfig();
    cmd->setData(QString::number((config->getTriggerDowntime() + config->getTriggerLength()) * 0.001)); //ms
    cmd->setError(NO_ERROR);
}

void getShutterLengthHandler(CommandHandler* ch, Command* cmd){
    if (!cmd->enoughArguments(1,"GetShutterLength"))  //this command comes with two argument {GetShutterLength;{open,down}
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    auto openOrClose = cmd->arguments.at(0);
    auto config = Mpx3GUI::getInstance()->getConfig();
    if(openOrClose == "open"){
       double trig =  config->getTriggerLength() * 0.001; //ms
       cmd->setData(QString::number(trig));
       cmd->setError(NO_ERROR);
    }
    else if(openOrClose == "down"){
        double trig = config->getTriggerDowntime() * 0.001; //ms
        cmd->setData(QString::number(trig));
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }
}

void setBothCountersHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetBothCounters"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setReadBothCounters(true);
        cmd->setData("Both counters is enabled");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setReadBothCounters(false);
        cmd->setData("Both counters is disabled");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }

}
void getBothCountersHandler(CommandHandler* ch, Command* cmd){
    if(Mpx3GUI::getInstance()->getConfig()->getReadBothCounters())
        cmd->setData("1");
    else
        cmd->setData("0");
    cmd->setError(NO_ERROR);

}

void setColourModeHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetColourMode"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setColourMode(true);
        cmd->setData("Colour mode is enabled");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setColourMode(false);
        cmd->setData("Colour mode is disabled");
        cmd->setError(NO_ERROR);
    }
    else{
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }
}
void getColourModeHandler(CommandHandler* ch, Command* cmd){
    if(Mpx3GUI::getInstance()->getConfig()->getColourMode())
        cmd->setData("1");
    else
        cmd->setData("0");
    cmd->setError(NO_ERROR);
}
void setChargeSummingHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetChargeSumming"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "csm"){
        Mpx3GUI::getInstance()->getConfig()->setCsmSpm(true);
        cmd->setData("Charge summing is enabled");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "spm"){
        Mpx3GUI::getInstance()->getConfig()->setCsmSpm(false);
        cmd->setData("Charge summing is disabled");
        cmd->setError(NO_ERROR);
    }
    else{
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }
}
void getChargeSummingHandler(CommandHandler* ch, Command* cmd){
    if(Mpx3GUI::getInstance()->getConfig()->getCsmSpm())
        cmd->setData("1");
    else
        cmd->setData("0");
    cmd->setError(NO_ERROR);
}
void setLutTableHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetLutTable"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setLUTEnable(true);
        cmd->setData("LUT table is enabled");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setLUTEnable(false);
        cmd->setData("LUT table is disabled");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }
}
void getLutTableHandler(CommandHandler* ch, Command* cmd){
    if(Mpx3GUI::getInstance()->getConfig()->getLUTEnable())
        cmd->setData("1");
    else
        cmd->setData("0");
    cmd->setError(NO_ERROR);
}

void setAutoSaveHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetAutoSave"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "enable"){
        ch->setAutoSave(true);
        cmd->setData("enabled");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "disable"){
        ch->setAutoSave(false);
        cmd->setData("disabled");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }

}

void setRecordPathHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetRecordPath"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    ch->setRecordPath(cmd->arguments.at(0));
    cmd->setData("Path is set");
    cmd->setError(NO_ERROR);
}

void setRecordFormatHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetRecordFormat"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if(cmd->arguments.at(0) == "txt"){
        ch->setRecordFormat(2);
        cmd->setData("Format is set to txt");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "tiff"){
        ch->setRecordFormat(0);
        cmd->setData("Format is set to tiff");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "rawtiff"){
        ch->setRecordFormat(1);
        cmd->setData("Format is set to rawtiff");
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }
}

void getImageHandler(CommandHandler* ch, Command* cmd){
//    QVector<int> frame0 = Mpx3GUI::getInstance()->getDataset()->makeFrameForSaving(0,false);
//   // QVector<int> frame0 = Mpx3GUI::getInstance()->getDataset()->getActualData();

//    QString strData ="";
//    for(int x = 0; x<frame0.length();x++){
//        if(x != frame0.length() - 1)
//            strData+=QString::number(frame0.at(x))+",";
//        else
//           strData+=QString::number(frame0.at(x));
//    }
    QByteArray im = Mpx3GUI::getInstance()->getDataset()->toByteArray();
    QString strData = "";
    for(int i = 0; i<20; i++){
        int value = 0;
        value |= im.at(i*4) | (im.at((i*4)+1)<<8) | (im.at((i*4)+2)<<16) | (im.at((i*4)+3)<<32);
        strData += QString::number(value) + ";";
    }
  //  cmd->setImage();

    QByteArray pixels = im.mid(20*4); //17*4 = 68 bytes header later this must become dynamic // 18*4 = 72 bytes for double counter
    qDebug()<<"pixel length = "<<pixels.length();

    cmd->setData(strData);
    cmd->setImage(pixels);

}

void setNumberOfFrameHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetFrameNumber"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    Mpx3GUI::getInstance()->getConfig()->setNTriggers(cmd->arguments.at(0).toInt() );
    cmd->setError(NO_ERROR);
}

void getNumberOfFrameHandler(CommandHandler* ch, Command* cmd){
    cmd->setData(QString::number(Mpx3GUI::getInstance()->getConfig()->getNTriggers()));
    cmd->setError(NO_ERROR);
}

void setThresholdHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(2,"SetThreshold"))  //this command comes with two argument{SetThreshold;0;100}
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int idx = cmd->arguments.at(0).toInt();
    int val = cmd->arguments.at(1).toDouble();
   // qDebug() <<"Double: " << cmd->arguments.at(1).toDouble() << "..." << idx;
    //qDebug() << "Int : " << cmd->arguments.at(1).toInt();
    ch->setThreshold(idx,val);
    cmd->setError(NO_ERROR);
}
void getThresholdHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"GetThreshold"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int idx = cmd->arguments.at(0).toInt();
    int thr = ch->getThreshold(idx);
    cmd->setData(QString::number(thr));
    cmd->setError(NO_ERROR);
}


void setStartScanHandler(CommandHandler* ch, Command* cmd)
{
    if(!cmd->enoughArguments(1,"SetStartScan"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int val = cmd->arguments.at(0).toInt();
    int ret = ch->setStartScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError((ERROR_TYPE)ret);
}

void getStartScanHandler(CommandHandler* ch, Command* cmd)
{
    int ret = ch->getStartScan();
    cmd->setData(QString::number(ret));
    cmd->setError(NO_ERROR);
}
void setStopScanHandler(CommandHandler* ch, Command* cmd)
{
    if(!cmd->enoughArguments(1,"SetStopScan"))  //this command comes with one argument
        return;
    int val = cmd->arguments.at(0).toInt();
    int ret = ch->setStopScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError((ERROR_TYPE)ret);
}
void getStopScanHandler(CommandHandler* ch, Command* cmd)
{
    int ret = ch->getStopScan();
    cmd->setData(QString::number(ret));
    cmd->setError(NO_ERROR);
}
void setStepScanHandler(CommandHandler* ch, Command* cmd)
{
    if(!cmd->enoughArguments(1,"SetStepScan"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int val = cmd->arguments.at(0).toInt();
    int ret = ch->setStepScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError((ERROR_TYPE)ret);
}
void getStepScanHandler(CommandHandler* ch, Command* cmd)
{
    int ret = ch->getStepScan();
    cmd->setData(QString::number(ret));
    cmd->setError(NO_ERROR);
}
void setThresholdScanHandler(CommandHandler* ch, Command* cmd)
{
    if(!cmd->enoughArguments(1,"SetThresholdScan"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int val = cmd->arguments.at(0).toInt();
    int ret = ch->setThresholdScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError((ERROR_TYPE)ret);
}
void getThresholdScanHandler(CommandHandler* ch, Command* cmd)
{
    int ret = ch->getThresholdScan();
    cmd->setData(QString::number(ret));
    cmd->setError(NO_ERROR);
}
void startScanHandler(CommandHandler* ch, Command* cmd){

}
void stopScanHandler(CommandHandler* ch, Command* cmd){

}

void setOperatingEnergyHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetOperatingEnergy"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    cmd->setData(QString::number(0));
    cmd->setError(NO_ERROR);
}

void getOperatingEnergyHandler(CommandHandler* ch, Command* cmd){
    cmd->setData(QString::number(20));
    cmd->setError(NO_ERROR);
}

void setProfileHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetProfile"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    cmd->setData(QString::number(0));
    cmd->setError(NO_ERROR);
}
void getProfileHandler(CommandHandler* ch, Command* cmd){
    cmd->setData(QString::number(1));
    cmd->setError(NO_ERROR);
}

void setMaskPixelHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(2,"SetMaskPixel"))  //this command comes with two argument x,y
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int x = cmd->arguments.at(0).toInt();
    int y = cmd->arguments.at(1).toInt();
    if( x < 0 || x > 511 || y < 0 || y > 511 ){
        cmd->setError(ARG_VAL_OUT_RANGE);
        return;
    }
    ch->setPixelMask(x,y);
}

void setUnmaskPixelHandler(CommandHandler* ch, Command* cmd){

}

//end of handler functions






#endif // HANDLERFUNCTIONS_H
