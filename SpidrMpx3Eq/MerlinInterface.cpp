#include "commandhandler.h"
#include "MerlinInterface.h"
#include <QDebug>
#include <QRegularExpression>
MerlinInterface::MerlinInterface(QObject *parent) : QObject(parent)
{
    initializeTables();
}

void MerlinCommand::setErrorExternally(int error)
{
    //{NO_ERROR = 0, UNKNOWN_ERROR = -1, UNKNOWN_COMMAND = -2 , ARG_NUM_OUT_RANGE = -3, ARG_VAL_OUT_RANGE = -4};

    if(error == 0)
    {
        _error = NO_ERROR;
        return;
    }
    if(error == -1)
    {
        _error = UNKNOWN_ERROR;
        return;
    }
    if(error == -2)
    {
        _error = UNKNOWN_COMMAND;
        return;
    }
    if(error == -3)
    {
        _error = PARAM_OUT_OF_RANGE;
        return;
    }
    if(error == -4)
    {
        _error = UNKNOWN_COMMAND;
        return;
    }

}

MerlinCommand::MerlinCommand(QString command, MerlinInterface &mi)
{
    _cmdLength = 0;
    _cmdType = "";
    _cmdName = "";
    _cmdValue.clear();

    QStringList items = command.split(",");
//    QStringList itemsTemp ;

//    for(int i=0; i<items.size(); i++){
//        QString str = items.at(i);

//        //! Tolerate new lines sent from netcat
//        QStringList list = str.split(QRegularExpression("(\\n)"));
//        itemsTemp.push_back(list.join(""));
//    }

//    items.clear();
//    items = itemsTemp;


    //check the header
    if(items.at(HEADER_INDEX) != HEADER){
        _error = UNKNOWN_COMMAND;
        parseResult = QString::number(UNKNOWN_COMMAND); return;
    }
    //check the range of the command
    if(items.length() < CMD_GET_PARTS || items.length() > SET_PARTS)
    {
        _error = PARAM_OUT_OF_RANGE;
        parseResult = QString::number(PARAM_OUT_OF_RANGE); return;
    }
    //if(items.at(TYPE_INDEX) == SET_TYPE && items.length() != SET_PARTS){
        //_error = PARAM_OUT_OF_RANGE;
        //parseResult = QString::number(PARAM_OUT_OF_RANGE); return;
    //}
    bool isLengthCorrect = (items.length() == CMD_GET_PARTS || items.length() == CMD_GET_PARTS+1);
    if((items.at(TYPE_INDEX) == CMD_TYPE || items.at(TYPE_INDEX) == GET_TYPE) && !isLengthCorrect ){
        _error = PARAM_OUT_OF_RANGE;
        parseResult = QString::number(PARAM_OUT_OF_RANGE); return;
    }
    //copy to local variables
    _cmdLength = items.at(LENGTH_INDEX).toInt();
    _cmdType = items.at(TYPE_INDEX);
    _cmdName = items.at(NAME_INDEX);
    if(items.at(TYPE_INDEX) == SET_TYPE || (items.at(TYPE_INDEX) == GET_TYPE && items.length() == CMD_GET_PARTS + 1))
    {
         _cmdValue.push_back(items.at(VALUE_INDEX));
        if(items.length() == SET_PARTS) //means commands comes with two arguments
            _cmdValue.push_back(items.at(VALUE_INDEX + 1));
    }
    _ptrCmdValue = 0;
    //check the length of the following command
    if(items.at(LENGTH_INDEX).length() != 10){
        _error = PARAM_OUT_OF_RANGE;
        parseResult = QString::number(PARAM_OUT_OF_RANGE); return;
    }

    int cmdLength = items.length() - 2;
    for(int i = TYPE_INDEX; i < items.length(); ++i){
        cmdLength += items.at(i).length();
    }
    if(cmdLength != _cmdLength)
    {
        _error = PARAM_OUT_OF_RANGE;
        parseResult = QString::number(PARAM_OUT_OF_RANGE); return;
    }

    //convert the command
    //set commands
    if( _cmdType == SET_TYPE){
        if(mi.setTable.contains(items.at(NAME_INDEX))){


            QString pslCmd = mi.setTable[_cmdName];
            QStringList pslCmdList = pslCmd.split(";");
            QString sndCmd = pslCmdList.at(0);
            for (int i = 1; i < pslCmdList.length(); ++i) {
                PSL_ARG_TYPES pslType = (PSL_ARG_TYPES) pslCmdList.at(i).toInt();
                QString arg = argParser(pslType);
                //_ptrCmdValue++;
                sndCmd += ";" + arg;
            }
            _error = NO_ERROR;
            parseResult = sndCmd; return;
//            if(pslCmdList.length() > 1){
//                PSL_ARG_TYPES pslType = (PSL_ARG_TYPES) pslCmdList.at(1).toInt();
//                QString arg = argParser(pslType);
//                _error = NO_ERROR;
//                pslCmd = pslCmdList.at(0) + ";" + arg;
//                parseResult = pslCmd.toLatin1().data(); return;
//            }
            //_error = NO_ERROR;
           // parseResult = pslCmdList.at(0).toLatin1().data(); return;
        }
        _error = UNKNOWN_COMMAND;
        parseResult = ""; return;
    }
    //get commands
    if( _cmdType == GET_TYPE){
        if(mi.getTable.contains(items.at(NAME_INDEX))){
            QString pslCmd = mi.getTable[_cmdName];

            QStringList pslCmdList = pslCmd.split(";");
            if(pslCmdList.length() > 1){
                PSL_ARG_TYPES pslType = (PSL_ARG_TYPES) pslCmdList.at(1).toInt();
                QString arg = argParser(pslType);
                _error = NO_ERROR;
                pslCmd = pslCmdList.at(0) + ";" + arg;
                parseResult = pslCmd; return;
            }
            _error = NO_ERROR;
            parseResult = pslCmdList.at(0); return;
        }
        _error = UNKNOWN_COMMAND;
        parseResult = ""; return;
    }
    //cmd commands
    if( _cmdType == CMD_TYPE){
        if(mi.cmdTable.contains(items.at(NAME_INDEX))){
            QString pslCmd = mi.cmdTable[_cmdName];
            QStringList pslCmdList = pslCmd.split(";");
            if(pslCmdList.length() > 1){
                PSL_ARG_TYPES pslType = (PSL_ARG_TYPES) pslCmdList.at(1).toInt();
                QString arg = argParser(pslType);
                _error = NO_ERROR;
                pslCmd = pslCmdList.at(0) + ";" + arg;
                parseResult = pslCmd; return;
            }
            _error = NO_ERROR;
            parseResult = pslCmdList.at(0); return;
        }
        _error = UNKNOWN_COMMAND;
        parseResult = ""; return;
    }





    _error = UNKNOWN_ERROR;
    parseResult = "";


}

QString MerlinCommand::makeSetCmdResponse()
{
    QString detail = "," + _cmdType + "," + _cmdName + "," +  QString::number(_error);
    QString len = QString::number(detail.length());
    //add leading zeros to len
    QString zeros ="";
    for (int i = 0; i < 10 - len.length(); ++i) {
        zeros += "0";
    }
    len = zeros + len;
    return HEADER + "," + len + detail +"\n";
}

QString MerlinCommand::makeGetResponse(QString val)
{
    QString detail = "," + _cmdType + "," + _cmdName + "," + val +"," +QString::number(_error);
    QString len = QString::number(detail.length());
    //add leading zeros to len
    QString zeros ="";
    for (int i = 0; i < 10 - len.length(); ++i) {
        zeros += "0";
    }
    len = zeros + len;
    return HEADER + "," + len + detail +"\n";

}

QString MerlinCommand::getCommandType()
{
    return _cmdType;
}

void MerlinInterface::initializeTables()
{
    //initialize getTable
    getTable.insert(SOFTWAREVERSION,"Hello");
    getTable.insert(COLOURMODE,"GetColourMode");
    getTable.insert(CHARGESUMMING,"GetChargeSumming");
    getTable.insert(GAIN,"GetGainMode");
    getTable.insert(CONTINUOUSRW,"GetReadoutMode");
    getTable.insert(ENABLECOUNTER1,"GetBothCounters");
    getTable.insert(COUNTERDEPTH,"GetCounterDepth");
    getTable.insert(TEMPERATURE,"GetTemperature");
    getTable.insert(ACQUISITIONTIME,"GetShutterLength;"+QString::number(OPEN));
    getTable.insert(ACQUISITIONPERIOD,"GetShutterPeriod");
    getTable.insert(NUMFRAMESTOACQUIRE,"GetFrameNumber");
    getTable.insert(THRESHOLD0,"GetThreshold;"+QString::number(TH0));
    getTable.insert(THRESHOLD1,"GetThreshold;"+QString::number(TH1));
    getTable.insert(THRESHOLD2,"GetThreshold;"+QString::number(TH2));
    getTable.insert(THRESHOLD3,"GetThreshold;"+QString::number(TH3));
    getTable.insert(THRESHOLD4,"GetThreshold;"+QString::number(TH4));
    getTable.insert(THRESHOLD5,"GetThreshold;"+QString::number(TH5));
    getTable.insert(THRESHOLD6,"GetThreshold;"+QString::number(TH6));
    getTable.insert(THRESHOLD7,"GetThreshold;"+QString::number(TH7));
    getTable.insert(THSTART,"GetStartScan");
    getTable.insert(THSTOP,"GetStopScan");
    getTable.insert(THSTEP,"GetStepScan");
    getTable.insert(THSCAN,"GetThresholdScan");
    getTable.insert(THFRAMES,"GetFramesPerScan;");
    getTable.insert(THPATH,"GetScanPath;");
    getTable.insert(OPERATINGENERGY,"GetOperatingEnergy");
    getTable.insert(PROFILES,"GetProfile");
    getTable.insert(TRIGGERMODE,"GetTriggerMode");
    getTable.insert(FILEDIRECTORY,"GetRecordPath");
    getTable.insert(EQUALIZATIONFILES,"GetEqualizationPath");
    getTable.insert(CONFIGFILE,"GetConfig");
    getTable.insert(INHIBITSHUTTER,"GetInhibitShutter");
    getTable.insert(SLOPES,"GetSlope;" +  QString::number(N));
    getTable.insert(OFFSETS,"GetOffset;" +  QString::number(N));

    //initialize setTable
    setTable.insert(COLOURMODE,"SetColourMode;" + QString::number(ENABLE_DISABLE));
    setTable.insert(CHARGESUMMING,"SetChargeSumming;" + QString::number(CSM_SPM));
    setTable.insert(GAIN,"SetGainMode;" + QString::number(HIGH_LOW));
    setTable.insert(CONTINUOUSRW,"SetReadoutMode;" + QString::number(CONT_SEQ));
    setTable.insert(ENABLECOUNTER1,"SetBothCounters;" + QString::number(ENABLE_DISABLE));
    setTable.insert(COUNTERDEPTH,"SetCounterDepth;" + QString::number(N));
    setTable.insert(ACQUISITIONTIME,"SetShutterLength;"+ QString::number(OPEN) + ";" + QString::number(N));
    setTable.insert(ACQUISITIONPERIOD,"SetShutterPeriod;" + QString::number(N));
    setTable.insert(NUMFRAMESTOACQUIRE,"SetFrameNumber;" + QString::number(N));
    setTable.insert(THRESHOLD0,"SetThreshold;"+QString::number(TH0)+";" +QString::number(N));
    setTable.insert(THRESHOLD1,"SetThreshold;"+QString::number(TH1)+";" +QString::number(N));
    setTable.insert(THRESHOLD2,"SetThreshold;"+QString::number(TH2)+";" +QString::number(N));
    setTable.insert(THRESHOLD3,"SetThreshold;"+QString::number(TH3)+";" +QString::number(N));
    setTable.insert(THRESHOLD4,"SetThreshold;"+QString::number(TH4)+";" +QString::number(N));
    setTable.insert(THRESHOLD5,"SetThreshold;"+QString::number(TH5)+";" +QString::number(N));
    setTable.insert(THRESHOLD6,"SetThreshold;"+QString::number(TH6)+";" +QString::number(N));
    setTable.insert(THRESHOLD7,"SetThreshold;"+QString::number(TH7)+";" +QString::number(N));

    setTable.insert(THRESHOLD0CHIP,"SetThresholdPerChip;"+QString::number(TH0)+";" +QString::number(N)+";"+QString::number(N));
    setTable.insert(THRESHOLD1CHIP,"SetThresholdPerChip;"+QString::number(TH1)+";" +QString::number(N)+";"+QString::number(N));
    setTable.insert(THRESHOLD2CHIP,"SetThresholdPerChip;"+QString::number(TH2)+";" +QString::number(N)+";"+QString::number(N));
    setTable.insert(THRESHOLD3CHIP,"SetThresholdPerChip;"+QString::number(TH3)+";" +QString::number(N)+";"+QString::number(N));
    setTable.insert(THRESHOLD4CHIP,"SetThresholdPerChip;"+QString::number(TH4)+";" +QString::number(N)+";"+QString::number(N));
    setTable.insert(THRESHOLD5CHIP,"SetThresholdPerChip;"+QString::number(TH5)+";" +QString::number(N)+";"+QString::number(N));
    setTable.insert(THRESHOLD6CHIP,"SetThresholdPerChip;"+QString::number(TH6)+";" +QString::number(N)+";"+QString::number(N));
    setTable.insert(THRESHOLD7CHIP,"SetThresholdPerChip;"+QString::number(TH7)+";" +QString::number(N) +";"+QString::number(N));


    setTable.insert(THSTART,"SetStartScan;" + QString::number(N));
    setTable.insert(THSTOP,"SetStopScan;" + QString::number(N));
    setTable.insert(THSTEP,"SetStepScan;" + QString::number(N));
    setTable.insert(THSCAN,"SetThresholdScan;" + QString::number(N));
    setTable.insert(THFRAMES,"SetFramesPerScan;" + QString::number(N));
    setTable.insert(THPATH,"SetScanPath;" + QString::number(STRING));
    setTable.insert(OPERATINGENERGY,"SetOperatingEnergy;" +  QString::number(N));
    setTable.insert(PROFILES,"SetProfile;" +  QString::number(N) );
    setTable.insert(TRIGGERMODE,"SetTriggerMode;"  +  QString::number(N));
    setTable.insert(FILEDIRECTORY,"SetRecordPath;" + QString::number(STRING));
    setTable.insert(FILEENABLE,"SetAutoSave;" + QString::number(ENABLE_DISABLE));
    setTable.insert(MASKPIXEL,"SetMaskPixel;"+QString::number(N)+";"+QString::number(N));
    setTable.insert(UNMASKPIXEL,"SetUnmaskPixel;"+QString::number(N)+";"+QString::number(N));
    setTable.insert(EQUALIZATIONFILES,"SetEqualizationPath;" + QString::number(STRING));
    setTable.insert(CONFIGFILE,"SetConfig;" + QString::number(STRING));
    setTable.insert(SAVEGONFIGS,"SaveConfig;" + QString::number(STRING) );
    setTable.insert(DOEQUALIZATION,"SetDoEqualization;" + QString::number(STRING));
    setTable.insert(INHIBITSHUTTER,"SetInhibitShutter;" + QString::number(ENABLE_DISABLE));
    setTable.insert(SLOPES,"SetSlope;"+ QString::number(N)+";"+QString::number(N));
    setTable.insert(OFFSETS,"SetOffset;"+ QString::number(N)+";"+QString::number(N));



    //initialize cmdTable
    cmdTable.insert(STOPACQUISITION,"Stop");
    cmdTable.insert(STARTACQUISITION,"Start");
    cmdTable.insert(THSCAN,"StarStoptScan");
}

QString MerlinCommand::argParser(PSL_ARG_TYPES argType)
{
    switch (argType) {
    case N:
    {
        QString ret =  _cmdValue.at(0);
         _cmdValue.removeFirst();
        return ret;
    }

    case STRING:
    {
        QString ret =  _cmdValue.at(0);
         _cmdValue.removeFirst();
        return ret;
    }

    case N_INF:
    {
        QString ret =  _cmdValue.at(0);
         _cmdValue.removeFirst();
        return ret;
    }
    case ENABLE_DISABLE:
    {
        if(_cmdValue.at(0).toInt() == 1)
        {
            _cmdValue.removeFirst();
            return "enable";
        }
        else {
            _cmdValue.removeFirst();
            return "disable";
        }

    }
    case CSM_SPM:
        if(_cmdValue.at(0).toInt() == 1)
        {
              _cmdValue.removeFirst();
            return "csm";
        }
        else{
              _cmdValue.removeFirst();
            return "spm";
       }
        break;
    case HIGH_LOW:
        { auto ival = _cmdValue.at(0).toInt();
            if (ival >= 0 && ival <= 3){
                  _cmdValue.removeFirst();
                return gainModeStrTable[3-ival];
            }
              _cmdValue.removeFirst();
        }
        break;
    case CONT_SEQ:
        if(_cmdValue.at(0).toInt()== 0){
              _cmdValue.removeFirst();
            return "seq";
        }
        if(_cmdValue.at(0).toInt()== 1){
              _cmdValue.removeFirst();
            return "cont";
        }
        break;
    case OPEN:
        return "open";
        break;
    case TH0:
        return "0";
        break;
    case TH1:
        return "1";
        break;
    case TH2:
        return "2";
        break;
    case TH3:
        return "3";
        break;
    case TH4:
        return "4";
        break;
    case TH5:
        return "5";
        break;
    case TH6:
        return "6";
        break;
    case TH7:
        return "7";
        break;
    default:
        break;
    }
    return "";
}
