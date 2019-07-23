#ifndef HANDLERFUNCTIONS_H
#define HANDLERFUNCTIONS_H

#include "commandhandler.h"
#include "mpx3gui.h"
#include "qcstmglvisualization.h"
#include "qcstmconfigmonitoring.h"
#include "MerlinInterface.h"

//handler functions
void helloHandler(CommandHandler* ch, Command* cmd)
{
    Q_UNUSED(ch);
    cmd->setData(_softwareVersion);
    cmd->setError(NO_ERROR);
}

void byeHandler(CommandHandler* ch, Command* cmd)
{
    Q_UNUSED(ch);
    QString tstData = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type ki";
    if(cmd->enoughArguments(1,"Bye"))
        cmd->setData(tstData);
}

void setReadoutModeHandler(CommandHandler* ch, Command* cmd)
{
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
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
    else if(cmd->arguments.at(0) == "24"){ //if it is not continous
        //here code to set depth
        if (Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 1)
        {
            cmd->setError(INVALID_ARG);
            return;
        }
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

void getCounterDepthHandler(CommandHandler* ch, Command* cmd) {
    Q_UNUSED(ch);
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

void getChipTemperatureHandler(CommandHandler* ch, Command* cmd) {
    Q_UNUSED(ch);
    SpidrController* spidrcontrol = Mpx3GUI::getInstance()->GetSpidrController();

    if (spidrcontrol) {
        int mdegrees;
        if (spidrcontrol->getRemoteTemp(&mdegrees)) {
            cmd->setData(QString::number((double(mdegrees)/1000.)));
            cmd->setError(NO_ERROR);
        }
    } else {
        cmd->setData(QString::number(-1));
        cmd->setError(UNKNOWN_ERROR);
    }
}

 void getBoardTemperatureHandler(CommandHandler* ch, Command* cmd) {
     Q_UNUSED(ch);
     SpidrController* spidrcontrol = Mpx3GUI::getInstance()->GetSpidrController();

     if (spidrcontrol) {
         int mdegrees;
         if (spidrcontrol->getLocalTemp( &mdegrees )) {
             cmd->setData(QString::number((double(mdegrees)/1000.)));
             cmd->setError(NO_ERROR);
         }
     } else {
         cmd->setData(QString::number(-1));
         cmd->setError(UNKNOWN_ERROR);
     }
 }

 void getFpgaTemperatureHandler(CommandHandler* ch, Command* cmd) {
     Q_UNUSED(ch);
     SpidrController* spidrcontrol = Mpx3GUI::getInstance()->GetSpidrController();

     if (spidrcontrol) {
         int mdegrees;
         if ( spidrcontrol->getFpgaTemp( &mdegrees )) {
             cmd->setData(QString::number((double(mdegrees)/1000.)));
             cmd->setError(NO_ERROR);
         }
     } else {
         cmd->setData(QString::number(-1));
         cmd->setError(UNKNOWN_ERROR);
     }
 }

 void getHumidityHandler(CommandHandler* ch, Command* cmd) {
     Q_UNUSED(ch);
     SpidrController* spidrcontrol = Mpx3GUI::getInstance()->GetSpidrController();

     if (spidrcontrol) {
         int humidityPercentage;
         if (spidrcontrol->getHumidity(&humidityPercentage)) {
             cmd->setData(QString::number(int((humidityPercentage))));
             cmd->setError(NO_ERROR);
         }
     } else {
         cmd->setData(QString::number(-1));
         cmd->setError(UNKNOWN_ERROR);
     }
 }



void setOperationalModeHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
    //here code to get the operational mode
    cmd->setData("Getting the operational mode....!");

}
void openHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
    Mpx3GUI::getInstance()->closeRemotely();
    cmd->setData("0");
    cmd->setError(NO_ERROR);
}
void snapHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);

    ch->startSnap();
    cmd->setData(QString::number(0));
    cmd->setError(NO_ERROR);

    cmd->setData(QString::number(-1));
    cmd->setError(UNKNOWN_ERROR);
}

void startHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    ch->startLiveCamera(true);
    cmd->setData(QString::number(0));
    cmd->setError(NO_ERROR);
}
void stopHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    ch->startLiveCamera(false);
    cmd->setData(QString::number(0));
    cmd->setError(NO_ERROR);
    ch->startSendingImage(false);
}

void setTriggerModeHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
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
    int val = cmd->arguments.at(0).toInt();
    if(val < 0 && val <= 5)
    {
        cmd->setError(INVALID_ARG);
        return;
    }
    auto config = Mpx3GUI::getInstance()->getConfig();
    if(config->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW && val != 0)
    {
        cmd->setError(INVALID_ARG);
        return;
    }
    ch->getGui()->getConfigMonitoring()->setTriggerModeByIndex(val);
    cmd->setError(NO_ERROR);
}
void getTriggerModeHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    int val = ch->getGui()->getConfigMonitoring()->getTriggerModeIndex();
    cmd->setError(NO_ERROR);
    cmd->setData(QString::number(val));
}

void setGainModeHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
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
    Q_UNUSED(ch);
    int freq = Mpx3GUI::getInstance()->getConfig()->getContRWFreq();
    cmd->setData(QString::number(freq));
    cmd->setError(NO_ERROR);
}

void setShutterLengthHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    if(!cmd->enoughArguments(2,"SetShutterLength"))  //this command comes with two argument {SetShutterLength;{open,down};value
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    auto openOrClose = cmd->arguments.at(0);
    auto length = cmd->arguments.at(1); //ms
    auto config = Mpx3GUI::getInstance()->getConfig();
    bool cont = (config->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW);
    if (openOrClose == "open"){
        //here code to set operational mode
        if (cont) {
            double freq = 1000./length.toDouble();
            if(freq >= 1 && freq <= 2034)
                config->setContRWFreq(int(freq));
            else
            {
               cmd->setError(INVALID_ARG);
               return;
            }
        } else {
            double val =  length.toDouble();
            if(val < 0.001 && int(val) != 0)
            {
                cmd->setError(INVALID_ARG);
                return;
            }
            config->setTriggerLength(val);    // us
        }
        cmd->setData("Shutter open length is set to " + length);
        cmd->setError(NO_ERROR);
    }
    else if (openOrClose == "down"){
        //here code to set operational mode
        if (cont) {
            cmd->setData("Shutter down length is 0");
        } else {
            config->setTriggerDowntime(int((1000. * length.toDouble())));  // us
            cmd->setData("Shutter down length is set to " + length);
        }
        cmd->setError(NO_ERROR);
    }
    else
    {
        cmd->setData("Invalid argument...!");
        cmd->setError(UNKNOWN_COMMAND);
    }
}

void setShutterPeriodHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    if (!cmd->enoughArguments(1,"SetShutterPeriod"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    auto config = Mpx3GUI::getInstance()->getConfig();
    bool cont = (config->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW);

    if (cont) {
        auto length = cmd->arguments.at(0); //ms;
        double freq = 1000./length.toDouble();
        //config->setContRWFreq(freq);
        qDebug() << "shutter period -> *not* set freq to " << freq;
        cmd->setError(INVALID_ARG);
        return;
    } else {
        auto length = cmd->arguments.at(0); //ms
        int open = int(config->getTriggerLength_64());
        config->setTriggerDowntime( length.toDouble() - double(open)/1000.0);   // ms
        cmd->setError(NO_ERROR);
    }
}

void getShutterPeriodHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    auto config = Mpx3GUI::getInstance()->getConfig();
    if (config->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW) {
        cmd->setData(QString::number(1000.0 / config->getContRWFreq())); //ms
    } else {
        cmd->setData(QString::number(config->getTriggerPeriod_ms())); //ms
    }
    cmd->setError(NO_ERROR);
}

void getShutterLengthHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    if (!cmd->enoughArguments(1,"GetShutterLength"))  //this command comes with two argument {GetShutterLength;{open,down}
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    auto openOrClose = cmd->arguments.at(0);
    auto config = Mpx3GUI::getInstance()->getConfig();
    bool cont = (config->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW);
    if(openOrClose == "open"){
       double trig = cont ? 1000.0 / config->getContRWFreq()
               : config->getTriggerLength_ms_64(); //ms
       cmd->setData(QString::number(trig));
       cmd->setError(NO_ERROR);
    }
    else if(openOrClose == "down"){
        double trig = cont ? 0.0
               : config->getTriggerDowntime_ms_64(); //ms
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
    Q_UNUSED(ch);

    if(!cmd->enoughArguments(1,"SetBothCounters"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    if (Mpx3GUI::getInstance()->getConfig()->getOperationMode() == 1)
    {
        cmd->setError(INVALID_ARG);
        return;
    }
    if(cmd->arguments.at(0) == "2"){ //we send the data of both counter
        Mpx3GUI::getInstance()->getConfig()->setReadBothCounters(true);
        cmd->setData("Both counters is enabled");
        cmd->setError(NO_ERROR);
    }
    else if(cmd->arguments.at(0) == "0" || cmd->arguments.at(0) == "1"){ //we send the data of low counter
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
    Q_UNUSED(ch);

    if(Mpx3GUI::getInstance()->getConfig()->getReadBothCounters())
        cmd->setData("1");
    else
        cmd->setData("0");
    cmd->setError(NO_ERROR);

}

void setColourModeHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);

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
    Q_UNUSED(ch);

    if(Mpx3GUI::getInstance()->getConfig()->getColourMode())
        cmd->setData("1");
    else
        cmd->setData("0");
    cmd->setError(NO_ERROR);
}
void setChargeSummingHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);

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
    Q_UNUSED(ch);

    if(Mpx3GUI::getInstance()->getConfig()->getCsmSpm())
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
    if(!ch->setRecordPath(cmd->arguments.at(0)))
    {
        cmd->setError(INVALID_ARG);
        return;
    }
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
    Q_UNUSED(ch);
    // TODO: this function is broken: it will not produce an image in an understandable format

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
        value |= im.at(i*4) | (im.at((i*4)+1)<<8) | (im.at((i*4)+2)<<16) | (im.at((i*4)+3)<<24);
        strData += QString::number(value) + ";";
    }
  //  cmd->setImage();

    QByteArray pixels = im.mid(20*4); //17*4 = 68 bytes header later this must become dynamic // 18*4 = 72 bytes for double counter
    qDebug()<<"pixel length = "<<pixels.length();

    cmd->setData(strData);
    cmd->setImage(pixels);

}

void setNumberOfFrameHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    if(!cmd->enoughArguments(1,"SetFrameNumber"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int val = cmd->arguments.at(0).toInt();
    if(val < 0 ){
        cmd->setError(INVALID_ARG);
        return;
    }

    Mpx3GUI::getInstance()->getConfig()->setNTriggers(val);
    cmd->setError(NO_ERROR);
}

void getNumberOfFrameHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    cmd->setData(QString::number(Mpx3GUI::getInstance()->getConfig()->getNTriggers()));
    cmd->setError(NO_ERROR);
}

void saturateThresholdDacValue(int *dacIn){
    if(*dacIn < 0)
    {
        *dacIn = 0;
        return;
    }
    if(*dacIn > 511)
    {
        *dacIn = 511;
        return;
    }
}

void setThresholdHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(2,"SetThreshold"))  //this command comes with two argument{SetThreshold;0;100}
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int idx = cmd->arguments.at(0).toInt();
    double val = cmd->arguments.at(1).toDouble(); //energy kev
   // qDebug() <<"Double: " << cmd->arguments.at(1).toDouble() << "..." << idx;
    //qDebug() << "Int : " << cmd->arguments.at(1).toInt();
//    for(int i = 0; i<NUMBER_OF_CHIPS; i++){
//        int dac = Mpx3GUI::getInstance()->getEnergyCalibrator()->calDac(i,val);

//        ch->setThreshold(idx,dac,i);
//        qDebug() << "converted to : " << dac;


//    }

    int dac = Mpx3GUI::getInstance()->getEnergyCalibrator()->calDac(0,val);
    saturateThresholdDacValue(&dac);
    ch->setThreshold(idx,dac,0);
    //ch->setThreshold(idx,dac); //for some strange reason does not take the set threshold value for chip 1
                                // this is why first we assign the correspondingthreshold  of chip 1 to all chips
                                //then we set the correspondig threshold of chip0-2-3 seprately


    qDebug() << "[INFO]\tThreshold "<< idx << " of chip 0 converted to : " << dac;

    dac = Mpx3GUI::getInstance()->getEnergyCalibrator()->calDac(1,val);
    saturateThresholdDacValue(&dac);
    ch->setThreshold(idx,dac,1);


    qDebug() << "[INFO]\tThreshold "<< idx << " of chip 1 converted to : " << dac;


    dac = Mpx3GUI::getInstance()->getEnergyCalibrator()->calDac(2,val);
    saturateThresholdDacValue(&dac);
    ch->setThreshold(idx,dac,2);

    qDebug() << "[INFO]\tThreshold "<< idx << " of chip 2 converted to : " << dac;
    dac = Mpx3GUI::getInstance()->getEnergyCalibrator()->calDac(3,val);
    saturateThresholdDacValue(&dac);
    ch->setThreshold(idx,dac,3);

    qDebug() << "[INFO]\tThreshold "<< idx << " of chip 3 converted to : " << dac;

    //ch->setThreshold(idx,val);
    cmd->setError(NO_ERROR);
}
void getThresholdHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"GetThreshold"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int idx = cmd->arguments.at(0).toInt();
    double thr = 0.0;
    int error = ch->getThreshold(idx,&thr);
    cmd->setData(QString::number(thr));
    cmd->setError(ERROR_TYPE(error));
}


void setThresholdPerChipHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(3,"SetThresholdPerChip"))  //this command comes with three argument{SetThresholdPerChip;th;chip;val}
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int idx = cmd->arguments.at(0).toInt();
    double val = cmd->arguments.at(2).toDouble(); //energy in Kev
    qDebug() << "[INFO]\tReceived [Dac value] : " << val;
    int chipId = cmd->arguments.at(1).toInt();

    // No need for the conversion here. Per chip we address the setting in DAC units.
    //int dac = Mpx3GUI::getInstance()->getEnergyCalibrator()->calDac(chipId,val);

    int dac = int(val);
    saturateThresholdDacValue(&dac);

    qDebug() << "[INFO]\tConverted to [Dac value] : " << dac;

    ch->setThreshold(idx,dac,chipId);
    cmd->setError(NO_ERROR);
}

void getThresholdPerChipHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(2,"GetThresholdPerChip"))  //this command comes with two argument{SetThresholdPerChip;th;chip}
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int idx = cmd->arguments.at(0).toInt();
    int chipId = cmd->arguments.at(1).toInt();
    int val =0;
    int error = ch->getThreshold(idx,chipId,&val);
    qDebug() << "[INFO]\tval [Dac value] : " << val;
    // In the per chip case there is no need for energy conversion.
    //double energy = Mpx3GUI::getInstance()->getEnergyCalibrator()->calcEnergy(chipId,val);
    //cmd->setData(QString::number(energy));
    cmd->setData(QString::number(val));
    cmd->setError(ERROR_TYPE(error));
}

void setStartScanHandler(CommandHandler* ch, Command* cmd)
{
    if(!cmd->enoughArguments(1,"SetStartScan"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int val = int(cmd->arguments.at(0).toDouble());
    int ret = ch->setStartScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError(ERROR_TYPE(ret));
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
    int val = int(cmd->arguments.at(0).toDouble());
    int ret = ch->setStopScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError(ERROR_TYPE(ret));
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
    int val = int(cmd->arguments.at(0).toDouble());
    int ret = ch->setStepScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError(ERROR_TYPE(ret));
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
    int val = int(cmd->arguments.at(0).toDouble());
    int ret = ch->setThresholdScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError(ERROR_TYPE(ret));
}
void getThresholdScanHandler(CommandHandler* ch, Command* cmd)
{
    int ret = ch->getThresholdScan();
    cmd->setData(QString::number(ret));
    cmd->setError(NO_ERROR);
}
void setFramesPerScanHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetFramesPerScan"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int val = int(cmd->arguments.at(0).toDouble());
    int ret = ch->setFramesPerScan(val);
    cmd->setData(QString::number(ret));
    cmd->setError(ERROR_TYPE(ret));
}

void getFramesPerScanHandler(CommandHandler* ch, Command* cmd)
{

    int ret = ch->getFramesPerScan();
    cmd->setData(QString::number(ret));
    cmd->setError(NO_ERROR);
}


void setScanPathHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetScanPath"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    QString path = cmd->arguments.at(0);
    int ret = ch->setScanPath(path);
    cmd->setData(QString::number(ret));
    cmd->setError(ERROR_TYPE(ret));
}

void getScanPathHandler(CommandHandler* ch, Command* cmd)
{
    QString path = ch->getScanPath();
    cmd->setData(path);
    cmd->setError(NO_ERROR);
}



void startScanHandler(CommandHandler* ch, Command* cmd){
    ch->startScan();
    cmd->setData(QString::number(0));
    cmd->setError(NO_ERROR);
}


void setOperatingEnergyHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    if(!cmd->enoughArguments(1,"SetOperatingEnergy"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    cmd->setData(QString::number(0));
    cmd->setError(NO_ERROR);
}

void getOperatingEnergyHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    cmd->setData(QString::number(20));
    cmd->setError(NO_ERROR);
}

void setProfileHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
    if(!cmd->enoughArguments(1,"SetProfile"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    cmd->setData(QString::number(0));
    cmd->setError(NO_ERROR);
}
void getProfileHandler(CommandHandler* ch, Command* cmd){
    Q_UNUSED(ch);
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
    const int COLUMN = 511 , ROW = 511;
    if( x < 0 || x > COLUMN || y < 0 || y > ROW ){
        cmd->setError(ARG_VAL_OUT_RANGE);
        return;
    }
    ch->setPixelMask(x,y);
}

void setUnmaskPixelHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(2,"SetUnmaskPixel"))  //this command comes with two argument x,y
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int x = cmd->arguments.at(0).toInt();
    int y = cmd->arguments.at(1).toInt();
    const int COLUMN = 511 , ROW = 511;
    if( x < 0 || x > COLUMN || y < 0 || y > ROW ){
        cmd->setError(ARG_VAL_OUT_RANGE);
        return;
    }
    ch->setPixelUnmask(x,y);
}


void setEqualizationHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetEqualization"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    QString path = cmd->arguments.at(0);
    int error = ch->loadEqualizationRemotely(path);
    cmd->setError(ERROR_TYPE(error));
}

void getEqualizationHandler(CommandHandler* ch, Command* cmd){
    cmd->setData(ch->getEqualizationPath());
    cmd->setError(NO_ERROR);
}

void setDoEqualizationHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetDoEqualization"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    QString path = cmd->arguments.at(0);
    cmd->setError(ERROR_TYPE(ch->doEqualizationRemotely(path)));

}

void stopEqualizationHandler(CommandHandler *ch, Command *cmd){
    Q_UNUSED(cmd);
    ch->stopEqualization();
}

void setConfigHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetConfig"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    QString path = cmd->arguments.at(0);
    int error = ch->loadConfigRemotely(path);
    cmd->setError((ERROR_TYPE(error)));

}

void getConfigHandler(CommandHandler* ch, Command* cmd){
    cmd->setData(ch->getConfigPath());
    cmd->setError(NO_ERROR);
}


void saveConfigHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SaveConfig"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    QString path = cmd->arguments.at(0);
    int error = ch->saveConfigRemotely(path);
    cmd->setError((ERROR_TYPE(error)));

}




void setInhibitShutterHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"SetInhibitShutter"))  //this command comes with one argument
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    auto config = Mpx3GUI::getInstance()->getConfig();
    if(config->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW)
    {
        cmd->setError(INVALID_ARG);
        return;
    }
    if(cmd->arguments.at(0) == "enable"){
        int error = ch->setInhibitShutter(true);
        cmd->setData("Inhibit_shutter is enabled");
        cmd->setError(ERROR_TYPE(error));
    }
    else if(cmd->arguments.at(0) == "disable"){
        int error = ch->setInhibitShutter(false);
        cmd->setData("Inhibit_shutter is disabled");
        cmd->setError(ERROR_TYPE( error));
    }
    else
    {
        cmd->setError(UNKNOWN_COMMAND);
        cmd->setData("Invalid argument...!");
    }

}

void getInhibitShutterHandler(CommandHandler* ch, Command* cmd){
    if(ch->getInhibitShutter()){
        cmd->setData("1");
        cmd->setError(NO_ERROR);
        return;
    }
    cmd->setData("0");
    cmd->setError(NO_ERROR);
    return;
}


void setSlopeHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(2,"SetSlope"))  //this command comes with two arguments (chipNum,val)
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int error = ch->setSlope(cmd->arguments.at(0).toInt(),cmd->arguments.at(1).toDouble());
    cmd->setData("1");
    cmd->setError(ERROR_TYPE( error));
}

void getSlopeHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"GetSlope"))  //this command comes with one argument (chipNum)
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int chipNum = int(cmd->arguments.at(0).toDouble());
    if(chipNum < 0 || chipNum >= NUMBER_OF_CHIPS){
        cmd->setError(ARG_VAL_OUT_RANGE );
        return;
    }
    cmd->setData(QString::number(ch->getSlope(chipNum)));
    cmd->setError(NO_ERROR);

}


void setOffsetHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(2,"SetOffset"))  //this command comes with two arguments (chipNum,val)
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int error = ch->setOffset(cmd->arguments.at(0).toInt(),cmd->arguments.at(1).toDouble());
    cmd->setData("1");
    cmd->setError(ERROR_TYPE( error));
}

void getOffsetHandler(CommandHandler* ch, Command* cmd){
    if(!cmd->enoughArguments(1,"GetSlope"))  //this command comes with one argument (chipNum)
    {
        cmd->setError(ARG_NUM_OUT_RANGE);
        return;
    }
    int chipNum = int(cmd->arguments.at(0).toDouble());
    if(chipNum < 0 || chipNum >= NUMBER_OF_CHIPS){
        cmd->setError(ARG_VAL_OUT_RANGE );
        return;
    }
    cmd->setData(QString::number(ch->getOffset(chipNum)));
    cmd->setError(NO_ERROR);

}

void resetSlopesAndOffsetsHandler(CommandHandler* ch, Command* cmd){
    int error = ch->resetSlopesAndOffsets();
    cmd->setData("1");
    cmd->setError(ERROR_TYPE(error));
}

void getServerStatusHandler(CommandHandler* ch, Command* cmd){
    cmd->setData(QString::number(ch->getServerStatus()));
    cmd->setError(NO_ERROR);
}

//end of handler functions


#endif // HANDLERFUNCTIONS_H
