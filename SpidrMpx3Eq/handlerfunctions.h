#ifndef HANDLERFUNCTIONS_H
#define HANDLERFUNCTIONS_H

#include "commandhandler.h"
#include "mpx3gui.h"
#include "qcstmglvisualization.h"


//handler functions
void helloHandler()
{
    CommandHandler::getInstance()->setData("Hello From Dexter Server");
    CommandHandler::getInstance();
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
        return;
    if(CommandHandler::getInstance()->cmdTable["SetReadoutMode"].args.at(0) == "seq"){ //set readout mode to sequential
        // here code for setting the readout mode to sequential
        Mpx3GUI::getInstance()->getConfig()->setOperationMode(0);
        CommandHandler::getInstance()->setData("Readout mode is set to sequential");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetReadoutMode"].args.at(0) == "cont"){ //set readout mode to continuous
        // here code for setting the readout mode to continuous
        Mpx3GUI::getInstance()->getConfig()->setOperationMode(1);
        CommandHandler::getInstance()->setData("Readout mode is set to continuous");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");

}

void getReadoutModeHandler()
{
    //here code to get the readout mode
    if(Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 0)
    {
        CommandHandler::getInstance()->setData("sequential");
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 1)
    {
        CommandHandler::getInstance()->setData("continuous");
        return;
    }

    CommandHandler::getInstance()->setData("invalid");


}

void setCounterDepthHandler()
{
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetCounterDepth"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetCounterDepth"].args.at(0) == "1"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(1);
        CommandHandler::getInstance()->setData("Counter depth is set to 1 bit...!");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetCounterDepth"].args.at(0) == "6"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(6);
        CommandHandler::getInstance()->setData("Counter depth is set to 6 bit...!");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetCounterDepth"].args.at(0) == "12"){
        //here code to set depth
        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(12);
        CommandHandler::getInstance()->setData("Counter depth is set to 12 bit...!");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetCounterDepth"].args.at(0) == "24"){
        //here code to set depth

        Mpx3GUI::getInstance()->getConfig()->setPixelDepth(24);
        CommandHandler::getInstance()->setData("Counter depth is set to 24 bit...!");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");

}

void getCounterDepthHandler(){
    //here code to get the readout mode
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 1)
    {
        CommandHandler::getInstance()->setData("1");
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 6)
    {
        CommandHandler::getInstance()->setData("6");
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 12)
    {
        CommandHandler::getInstance()->setData("12");
        return;
    }
    if(Mpx3GUI::getInstance()->getConfig()->getPixelDepth() == 24)
    {
        CommandHandler::getInstance()->setData("24");
        return;
    }
    CommandHandler::getInstance()->setData("invalid");//-1
}

void setOperationalModeHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetOperationalMode"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetOperationalMode"].args.at(0) == "spm"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setCsmSpm(0);
        CommandHandler::getInstance()->setData("Operational mode is set to spm...!");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetOperationalMode"].args.at(0) == "csm"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setCsmSpm(1);
        CommandHandler::getInstance()->setData("Operational mode is set to csm...!");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");

}

void getOperationalModeHandler(){
    //here code to get the operational mode
    CommandHandler::getInstance()->setData("Getting the operational mode....!");

}
void openHandler(){
    if(Mpx3GUI::getInstance()->establish_connection())
        CommandHandler::getInstance()->setData("1");
    else
        CommandHandler::getInstance()->setData("0");
}
void closeHandler(){
    Mpx3GUI::getInstance()->closeRemotely();
    CommandHandler::getInstance()->setData("1");
}
bool isStarted = false;
void snapHandler(){
    if(!isStarted){
       CommandHandler::getInstance()->startSnap();
       CommandHandler::getInstance()->setData(QString::number(1));
       return;
    }
    CommandHandler::getInstance()->setData(QString::number(0));
}

void startHandler(){

    if(!isStarted){
        CommandHandler::getInstance()->startLiveCamera();
        isStarted = true;
    }
    int d = (isStarted) ? 1 : 0;
    CommandHandler::getInstance()->setData(QString::number(d));

}
void stopHandler(){
    if(isStarted){
        CommandHandler::getInstance()->startLiveCamera();
        isStarted = false;
    }
    int d = (isStarted) ? 0 : 1;
    CommandHandler::getInstance()->setData(QString::number(d));
}

void setTriggerModeHandler(){

}
void getTriggerModeHandler(){

}
void setGainModeHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetGainMode"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetGainMode"].args.at(0) == "high"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setGainMode(1);
        CommandHandler::getInstance()->setData("Gain mode is set to high...!");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetGainMode"].args.at(0) == "shigh"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setGainMode(0);
        CommandHandler::getInstance()->setData("Gain mode is set to super high...!");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetGainMode"].args.at(0) == "low"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setGainMode(2);
        CommandHandler::getInstance()->setData("Gain mode is set to low...!");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetGainMode"].args.at(0) == "slow"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setGainMode(3);
        CommandHandler::getInstance()->setData("Gain mode is set to super low...!");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");
}
void getGainModeHandler(){
    int gainMode = Mpx3GUI::getInstance()->getConfig()->getGainMode();
    QString gainModeStrTable[] = {"SuperHigh","High","Low","SuperLow"};
    if(gainMode < 4 && gainMode >= 0)
        CommandHandler::getInstance()->setData(gainModeStrTable[gainMode]);
    else
        CommandHandler::getInstance()->setData("Invalid");
}
void setPolarityHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetPolarity"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetPolarity"].args.at(0) == "neg"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setPolarity(1);
        CommandHandler::getInstance()->setData("Polarity is set to negative...!");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetPolarity"].args.at(0) == "pos"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setPolarity(0);
        CommandHandler::getInstance()->setData("Polarity is set to positive...!");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");
}
void getPolarityHandler(){
    if(Mpx3GUI::getInstance()->getConfig()->getPolarity())
        CommandHandler::getInstance()->setData("Positive");
    else
        CommandHandler::getInstance()->setData("Negative");
}
void setCounterSelectFrequencyHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetCounterSelectFrequency"))  //this command comes with one argument
        return;
    int freq = CommandHandler::getInstance()->cmdTable["SetCounterSelectFrequency"].args.at(0).toInt();
    Mpx3GUI::getInstance()->getConfig()->setContRWFreq(freq);
    CommandHandler::getInstance()->setData("CRW frequency is set to " + QString::number(freq));

}
void getCounterSelectFrequencyHandler(){
    int freq = Mpx3GUI::getInstance()->getConfig()->getContRWFreq();
    CommandHandler::getInstance()->setData(QString::number(freq));
}
void setShutterLengthHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(2,"SetShutterLength"))  //this command comes with two argument {SetShutterLength;{open,down};value
        return;
    if(CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(0) == "open"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setTriggerLength(CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(1).toInt());
        CommandHandler::getInstance()->setData("Shutter open length is set to " + CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(1));
    }
    else if(CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(0) == "down"){
        //here code to set operational mode
        Mpx3GUI::getInstance()->getConfig()->setTriggerDowntime(CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(1).toInt());
        CommandHandler::getInstance()->setData("Shutter down length is set to " + CommandHandler::getInstance()->cmdTable["SetShutterLength"].args.at(1));
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");

}
void getShutterLengthHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"GetShutterLength"))  //this command comes with two argument {GetShutterLength;{open,down}
        return;
    if(CommandHandler::getInstance()->cmdTable["GetShutterLength"].args.at(0) == "open"){
       int trig =  Mpx3GUI::getInstance()->getConfig()->getTriggerLength();
       CommandHandler::getInstance()->setData(QString::number(trig));
    }
    else if(CommandHandler::getInstance()->cmdTable["GetShutterLength"].args.at(0) == "down"){
        int trig = Mpx3GUI::getInstance()->getConfig()->getTriggerDowntime();
        CommandHandler::getInstance()->setData(QString::number(trig));
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");

}


void setBothCountersHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetBothCounters"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetBothCounters"].args.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setReadBothCounters(true);
        CommandHandler::getInstance()->setData("Both counters is enabled");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetBothCounters"].args.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setReadBothCounters(false);
        CommandHandler::getInstance()->setData("Both counters is disabled");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");

}
void getBothCountersHandler(){
    if(Mpx3GUI::getInstance()->getConfig()->getReadBothCounters())
        CommandHandler::getInstance()->setData("1");
    else
        CommandHandler::getInstance()->setData("0");

}

void setColourModeHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetColourMode"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetColourMode"].args.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setColourMode(true);
        CommandHandler::getInstance()->setData("Colour mode is enabled");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetColourMode"].args.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setColourMode(false);
        CommandHandler::getInstance()->setData("Colour mode is disabled");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");
}
void getColourModeHandler(){
    if(Mpx3GUI::getInstance()->getConfig()->getColourMode())
        CommandHandler::getInstance()->setData("1");
    else
        CommandHandler::getInstance()->setData("0");
}
void setLutTableHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetLutTable"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetLutTable"].args.at(0) == "enable"){
        Mpx3GUI::getInstance()->getConfig()->setLUTEnable(true);
        CommandHandler::getInstance()->setData("LUT table is enabled");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetLutTable"].args.at(0) == "disable"){
        Mpx3GUI::getInstance()->getConfig()->setLUTEnable(false);
        CommandHandler::getInstance()->setData("LUT table is disabled");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");
}
void getLutTableHandler(){
    if(Mpx3GUI::getInstance()->getConfig()->getLUTEnable())
        CommandHandler::getInstance()->setData("1");
    else
        CommandHandler::getInstance()->setData("0");
}

void setAutoSaveHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetAutoSave"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetAutoSave"].args.at(0) == "enable"){
        CommandHandler::getInstance()->setAutoSave(true);
        CommandHandler::getInstance()->setData("enabled");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetAutoSave"].args.at(0) == "disable"){
        CommandHandler::getInstance()->setAutoSave(false);
        CommandHandler::getInstance()->setData("disabled");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");

}

void setRecordPathHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetRecordPath"))  //this command comes with one argument
        return;
    CommandHandler::getInstance()->setRecordPath(CommandHandler::getInstance()->cmdTable["SetRecordPath"].args.at(0));
    CommandHandler::getInstance()->setData("Path is set");
}

void setRecordFormatHandler(){
    if(!CommandHandler::getInstance()->enoughArguments(1,"SetRecordFormat"))  //this command comes with one argument
        return;
    if(CommandHandler::getInstance()->cmdTable["SetRecordFormat"].args.at(0) == "txt"){
        CommandHandler::getInstance()->setRecordFormat(2);
        CommandHandler::getInstance()->setData("Format is set to txt");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetRecordFormat"].args.at(0) == "tiff"){
        CommandHandler::getInstance()->setRecordFormat(0);
        CommandHandler::getInstance()->setData("Format is set to tiff");
    }
    else if(CommandHandler::getInstance()->cmdTable["SetRecordFormat"].args.at(0) == "rawtiff"){
        CommandHandler::getInstance()->setRecordFormat(1);
        CommandHandler::getInstance()->setData("Format is set to rawtiff");
    }
    else
        CommandHandler::getInstance()->setData("Invalid argument...!");
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
    for(int i = 0; i<17; i++){
        int value = 0;
        value |= im.at(i*4) | (im.at((i*4)+1)<<8) | (im.at((i*4)+2)<<16) | (im.at((i*4)+3)<<32);
        strData += QString::number(value) + ";";
    }
  //  CommandHandler::getInstance()->setImage();

    QByteArray pixels = im.mid(17*4); //17*4 = 68 bytes header later this must become dynamic // 18*4 = 72 bytes for double counter
   // qDebug()<<"pixel length = "<<pixels.length();

    CommandHandler::getInstance()->setData(strData);
    CommandHandler::getInstance()->setImage(pixels);

}


//end of handler functions






#endif // HANDLERFUNCTIONS_H
