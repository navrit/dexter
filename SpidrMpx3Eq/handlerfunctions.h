#ifndef HANDLERFUNCTIONS_H
#define HANDLERFUNCTIONS_H

#include "commandhandler.h"
#include "mpx3gui.h"
#include "qcstmglvisualization.h"
#include "MerlinInterface.h"


//handler functions
void helloHandler()
{
    CommandHandler::getInstance()->setData("Hello From Dexter Server");
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    FrameHeaderDataStruct frameHeader;
    QString hd = CommandHandler::getInstance()->generateMerlinFrameHeader(frameHeader);
    qDebug() <<"header is" << hd;
    qDebug() << "header size is : " << hd.length();
}

void byeHandler()
{
    QString tstData = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type ki";
    if(CommandHandler::getInstance()->enoughArguments(1,"Bye"))
        CommandHandler::getInstance()->setData(tstData);
}

void setReadoutModeHandler()
{
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetReadoutMode")) //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetReadoutMode"].args.at(0) == "seq"){ //set readout mode to sequential
        // here code for setting the readout mode to sequential
        Mpx3GUI::getInstance()->getConfig()->setOperationMode(0);
        CommandHandler::getInstance()->setData("Readout mode is set to sequential");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetReadoutMode"].args.at(0) == "cont"){ //set readout mode to continuous
        // here code for setting the readout mode to continuous
        Mpx3GUI::getInstance()->getConfig()->setOperationMode(1);
        CommandHandler::getInstance()->setData("Readout mode is set to continuous");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setData("Invalid argument...!");
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
    }

}

void getReadoutModeHandler()
{
    //here code to get the readout mode
    if(Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 0)
    {
        CommandHandler::getInstance()->setData("0");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 1)
    {
        CommandHandler::getInstance()->setData("1");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        return;
    }

    CommandHandler::getInstance()->setData("-1");
    CommandHandler::getInstance()->setError(CommandHandler::UNKWON_ERROR);


}

void setCounterDepthHandler()
{
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetCounterDepth"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetCounterDepth"].args.at(0) == "1"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(1);
        CommandHandler::getInstance()->setData("Counter depth is set to 1 bit...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetCounterDepth"].args.at(0) == "6"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(6);
        CommandHandler::getInstance()->setData("Counter depth is set to 6 bit...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetCounterDepth"].args.at(0) == "12"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(12);
        CommandHandler::getInstance()->setData("Counter depth is set to 12 bit...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetCounterDepth"].args.at(0) == "24"){
        //here code to set depth

        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(24);
        CommandHandler::getInstance()->setData("Counter depth is set to 24 bit...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setData("Invalid argument...!");
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
    }

}

void getCounterDepthHandler(){
    //here code to get the readout mode
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 1)
    {
        CommandHandler::getInstance()->setData("1");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 6)
    {
        CommandHandler::getInstance()->setData("6");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 12)
    {
        CommandHandler::getInstance()->setData("12");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 24)
    {
        CommandHandler::getInstance()->setData("24");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        return;
    }
    CommandHandler::getInstance()->setData("-1");//-1
    CommandHandler::getInstance()->setError(CommandHandler::UNKWON_ERROR);
}

void setOperationalModeHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetOperationalMode"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetOperationalMode"].args.at(0) == "spm"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setCsmSpm(0);
        CommandHandler::getInstance()->setData("Operational mode is set to spm...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetOperationalMode"].args.at(0) == "csm"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setCsmSpm(1);
        CommandHandler::getInstance()->setData("Operational mode is set to csm...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("Invalid argument...!");
    }
}

void getOperationalModeHandler(){
    //here code to get the operational mode
    CommandHandler::getInstance()->setData("Getting the operational mode....!");

}
void openHandler(){
    if(Mpx3GUI::getInstance()->establish_connection()){
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        CommandHandler::getInstance()->setData("0");
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("-1");
    }
}
void closeHandler(){
    Mpx3GUI::getInstance()->closeRemotely();
    CommandHandler::getInstance()->setData("0");
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
bool isStarted = false;
void snapHandler(){
    if(!isStarted){
       CommandHandler::getInstance()->startSnap();
       CommandHandler::getInstance()->setData(QString::number(0));
       CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
       return;
    }
    CommandHandler::getInstance()->setData(QString::number(-1));
    CommandHandler::getInstance()->setError(CommandHandler::UNKWON_ERROR);
}

void startHandler(){

    if(!isStarted){
        CommandHandler::getInstance()->startLiveCamera();
        isStarted = true;
    }
    int d = (isStarted) ? 0 : -1;
    CommandHandler::getInstance()->setData(QString::number(d));
    if(d == 0)
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    else
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_ERROR);
}
void stopHandler(){
    if(isStarted){
        CommandHandler::getInstance()->startLiveCamera();
        isStarted = false;
    }
    int d = (isStarted) ? -1 : 0;
    CommandHandler::getInstance()->setData(QString::number(d));
    if(d == 0)
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    else
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_ERROR);
}

void setTriggerModeHandler(){
    /*
    #define SHUTTERMODE_POS_EXT        0
    #define SHUTTERMODE_NEG_EXT        1
    #define SHUTTERMODE_POS_EXT_TIMER  2
    #define SHUTTERMODE_NEG_EXT_TIMER  3
    #define SHUTTERMODE_AUTO           4
    #define SHUTTERTRIG_POS_EXT_CNTR   5
     */
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetTriggerMode"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetTriggerMode"].args.at(0) == "0")
    {
        Mpx3GUI::getInstance()->getConfig()->setTriggerMode(4);
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("-1");
    }
}
void getTriggerModeHandler(){
    int val = Mpx3GUI::getInstance()->getConfig()->getTriggerMode();
    if(val == 4){
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        CommandHandler::getInstance()->setData("0");
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("-1");
    }

}
void setGainModeHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetGainMode"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetGainMode"].args.at(0) == "high"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setGainMode(1);
        CommandHandler::getInstance()->setData("Gain mode is set to high...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetGainMode"].args.at(0) == "shigh"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setGainMode(0);
        CommandHandler::getInstance()->setData("Gain mode is set to super high...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetGainMode"].args.at(0) == "low"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setGainMode(2);
        CommandHandler::getInstance()->setData("Gain mode is set to low...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetGainMode"].args.at(0) == "slow"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setGainMode(3);
        CommandHandler::getInstance()->setData("Gain mode is set to super low...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setData("Invalid argument...!");
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
    }
}
void getGainModeHandler(){
    int gainMode = Mpx3GUI::getInstance()->getConfig()->getGainMode();
    //QString gainModeStrTable[] = {"SuperHigh","High","Low","SuperLow"};
    if(gainMode == 0){
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        CommandHandler::getInstance()->setData("3");
        return;
    }
    if(gainMode == 1){
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        CommandHandler::getInstance()->setData("2");
        return;
    }
    if(gainMode == 2){
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        CommandHandler::getInstance()->setData("1");
        return;
    }
    if(gainMode == 3){
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        CommandHandler::getInstance()->setData("0");
        return;
    }
    CommandHandler::getInstance()->setError(CommandHandler::UNKWON_ERROR);
    CommandHandler::getInstance()->setData("-1");
    return;

}
void setPolarityHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetPolarity"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetPolarity"].args.at(0) == "neg"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setPolarity(1);
        CommandHandler::getInstance()->setData("Polarity is set to negative...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetPolarity"].args.at(0) == "pos"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setPolarity(0);
        CommandHandler::getInstance()->setData("Polarity is set to positive...!");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("Invalid argument...!");
    }
}
void getPolarityHandler(){
    if(Mpx3GUI::getInstance()->getConfig()->getPolarity())
    {
        CommandHandler::getInstance()->setData("1");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
        CommandHandler::getInstance()->setData("0");
    }
}
void setCounterSelectFrequencyHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetCounterSelectFrequency"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    int freq = CommandHandler::getInstance()->cmdTable["SetCounterSelectFrequency"].args.at(0).toInt();
    Mpx3GUI::getInstance()->getConfig()->setContRWFreq(freq);
    CommandHandler::getInstance()->setData("CRW frequency is set to " + QString::number(freq));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);

}
void getCounterSelectFrequencyHandler(){
    int freq = Mpx3GUI::getInstance()->getConfig()->getContRWFreq();
    CommandHandler::getInstance()->setData(QString::number(freq));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void setShutterLengthHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(2,"SetShutterLength"))  //this command comes with two argument {SetShutterLength;{open,down};value
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(0) == "open"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setTriggerLength(CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(1).toInt());
        CommandHandler::getInstance()->setData("Shutter open length is set to " + CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(1));
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(0) == "down"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setTriggerDowntime(CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(1).toInt());
        CommandHandler::getInstance()->setData("Shutter down length is set to " + CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(1));
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setData("Invalid argument...!");
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
    }

}
void setShutterPeriodHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetShutterPeriod"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    int open = Mpx3GUI::getInstance()->getConfig()->getTriggerLength();
    Mpx3GUI::getInstance()->getConfig()->setTriggerDowntime(CommandHandler::getInstance()->cmdTable["SetShutterPeriod"].args.at(0).toInt() - open);
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void getShutterPeriodHandler(){
   CommandHandler::getInstance()->setData(QString::number(Mpx3GUI::getInstance()->getConfig()->getTriggerDowntime() + Mpx3GUI::getInstance()->getConfig()->getTriggerLength()));
   CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}

void getShutterLengthHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"GetShutterLength"))  //this command comes with two argument {GetShutterLength;{open,down}
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["GetShutterLength"].args.at(0) == "open"){
       int trig =  Mpx3GUI::getInstance()->getConfig()->getTriggerLength();
       CommandHandler::getInstance()->setData(QString::number(trig));
       CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["GetShutterLength"].args.at(0) == "down"){
        int trig = Mpx3GUI::getInstance()->getConfig()->getTriggerDowntime();
        CommandHandler::getInstance()->setData(QString::number(trig));
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("Invalid argument...!");
    }

}


void setBothCountersHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetBothCounters"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetBothCounters"].args.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setReadBothCounters(true);
        CommandHandler::getInstance()->setData("Both counters is enabled");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetBothCounters"].args.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setReadBothCounters(false);
        CommandHandler::getInstance()->setData("Both counters is disabled");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("Invalid argument...!");
    }

}
void getBothCountersHandler(){
    if(Mpx3GUI::getInstance()->getConfig()->getReadBothCounters())
        CommandHandler::getInstance()->setData("1");
    else
        CommandHandler::getInstance()->setData("0");
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);

}

void setColourModeHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetColourMode"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetColourMode"].args.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setColourMode(true);
        CommandHandler::getInstance()->setData("Colour mode is enabled");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetColourMode"].args.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setColourMode(false);
        CommandHandler::getInstance()->setData("Colour mode is disabled");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else{
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("Invalid argument...!");
    }
}
void getColourModeHandler(){
    if(Mpx3GUI::getInstance()->getConfig()->getColourMode())
        CommandHandler::getInstance()->setData("1");
    else
        CommandHandler::getInstance()->setData("0");
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void setLutTableHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetLutTable"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetLutTable"].args.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setLUTEnable(true);
        CommandHandler::getInstance()->setData("LUT table is enabled");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetLutTable"].args.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setLUTEnable(false);
        CommandHandler::getInstance()->setData("LUT table is disabled");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("Invalid argument...!");
    }
}
void getLutTableHandler(){
    if(Mpx3GUI::getInstance()->getConfig()->getLUTEnable())
        CommandHandler::getInstance()->setData("1");
    else
        CommandHandler::getInstance()->setData("0");
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}

void setAutoSaveHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetAutoSave"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetAutoSave"].args.at(0) == "enable"){
        CommandHandler::getInstance()->setAutoSave(true);
        CommandHandler::getInstance()->setData("enabled");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetAutoSave"].args.at(0) == "disable"){
        CommandHandler::getInstance()->setAutoSave(false);
        CommandHandler::getInstance()->setData("disabled");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("Invalid argument...!");
    }

}

void setRecordPathHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetRecordPath"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    CommandHandler::getInstance()->setRecordPath(CommandHandler::getInstance()->cmdTable["SetRecordPath"].args.at(0));
    CommandHandler::getInstance()->setData("Path is set");
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}

void setRecordFormatHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetRecordFormat"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    if(CommandHandler::getInstance()->cmdTable["SetRecordFormat"].args.at(0) == "txt"){
        CommandHandler::getInstance()->setRecordFormat(2);
        CommandHandler::getInstance()->setData("Format is set to txt");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetRecordFormat"].args.at(0) == "tiff"){
        CommandHandler::getInstance()->setRecordFormat(0);
        CommandHandler::getInstance()->setData("Format is set to tiff");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else if(CommandHandler::getInstance()->cmdTable["SetRecordFormat"].args.at(0) == "rawtiff"){
        CommandHandler::getInstance()->setRecordFormat(1);
        CommandHandler::getInstance()->setData("Format is set to rawtiff");
        CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
    }
    else
    {
        CommandHandler::getInstance()->setError(CommandHandler::UNKWON_COMMAND);
        CommandHandler::getInstance()->setData("Invalid argument...!");
    }
}

void getImageHandler(){
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
  //  CommandHandler::getInstance()->setImage();

    QByteArray pixels = im.mid(20*4); //17*4 = 68 bytes header later this must become dynamic // 18*4 = 72 bytes for double counter
    qDebug()<<"pixel length = "<<pixels.length();

    CommandHandler::getInstance()->setData(strData);
    CommandHandler::getInstance()->setImage(pixels);

}

void setNumberOfFrameHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetNumberOfFrame"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    Mpx3GUI::getInstance()->getConfig()->setNTriggers(CommandHandler::getInstance()->cmdTable["SetNumberOfFrame"].args.at(0).toInt() );
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}

void getNumberOfFrameHandler(){
    CommandHandler::getInstance()->setData(QString::number(Mpx3GUI::getInstance()->getConfig()->getNTriggers()));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}

void setThresholdHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(2,"SetThreshold"))  //this command comes with two argument{SetThreshold;0;100}
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    int idx = CommandHandler::getInstance()->cmdTable["SetThreshold"].args.at(0).toInt();
    int val = CommandHandler::getInstance()->cmdTable["SetThreshold"].args.at(1).toInt();
    CommandHandler::getInstance()->setThreshold(idx,val);
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void getThresholdHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"GetThreshold"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    int idx = CommandHandler::getInstance()->cmdTable["GetThreshold"].args.at(0).toInt();
    int thr = CommandHandler::getInstance()->getThreshold(idx);
    CommandHandler::getInstance()->setData(QString::number(thr));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}


void setStartScanHandler()
{
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetStartScan"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    int val = CommandHandler::getInstance()->cmdTable["SetStartScan"].args.at(0).toInt();
    int ret = CommandHandler::getInstance()->setStartScan(val);
    CommandHandler::getInstance()->setData(QString::number(ret));
    CommandHandler::getInstance()->setError((CommandHandler::ERROR_TYPE)ret);
}

void getStartScanHandler()
{
    int ret = CommandHandler::getInstance()->getStartScan();
    CommandHandler::getInstance()->setData(QString::number(ret));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void setStopScanHandler()
{
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetStopScan"))  //this command comes with one argument
        return;
    int val = CommandHandler::getInstance()->cmdTable["SetStopScan"].args.at(0).toInt();
    int ret = CommandHandler::getInstance()->setStopScan(val);
    CommandHandler::getInstance()->setData(QString::number(ret));
    CommandHandler::getInstance()->setError((CommandHandler::ERROR_TYPE)ret);
}
void getStopScanHandler()
{
    int ret = CommandHandler::getInstance()->getStopScan();
    CommandHandler::getInstance()->setData(QString::number(ret));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void setStepScanHandler()
{
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetStepScan"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    int val = CommandHandler::getInstance()->cmdTable["SetStepScan"].args.at(0).toInt();
    int ret = CommandHandler::getInstance()->setStepScan(val);
    CommandHandler::getInstance()->setData(QString::number(ret));
    CommandHandler::getInstance()->setError((CommandHandler::ERROR_TYPE)ret);
}
void getStepScanHandler()
{
    int ret = CommandHandler::getInstance()->getStepScan();
    CommandHandler::getInstance()->setData(QString::number(ret));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void setThresholdScanHandler()
{
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetThresholdScan"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    int val = CommandHandler::getInstance()->cmdTable["SetThresholdScan"].args.at(0).toInt();
    int ret = CommandHandler::getInstance()->setThreholdScan(val);
    CommandHandler::getInstance()->setData(QString::number(ret));
    CommandHandler::getInstance()->setError((CommandHandler::ERROR_TYPE)ret);
}
void getThresholdScanHandler()
{
    int ret = CommandHandler::getInstance()->getThreholdScan();
    CommandHandler::getInstance()->setData(QString::number(ret));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void startScanHandler(){

}
void stopScanHandler(){

}

void setOperatingEnergyHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetOperatingEnergy"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    CommandHandler::getInstance()->setData(QString::number(0));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}

void getOperatingEnergyHandler(){
    CommandHandler::getInstance()->setData(QString::number(20));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}

void setProfileHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetProfile"))  //this command comes with one argument
    {
        CommandHandler::getInstance()->setError(CommandHandler::ARG_NUM_OUT_RANGE);
        return;
    }
    CommandHandler::getInstance()->setData(QString::number(0));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}
void getProfileHandler(){
    CommandHandler::getInstance()->setData(QString::number(1));
    CommandHandler::getInstance()->setError(CommandHandler::NO_ERROR);
}

//end of handler functions






#endif // HANDLERFUNCTIONS_H
