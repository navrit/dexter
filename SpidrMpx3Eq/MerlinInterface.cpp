#include "MerlinInterface.h"

MerlinInterface::MerlinInterface(QObject *parent) : QObject(parent)
{
    initializeTables();
}

char *MerlinInterface::parseCommand(char *command)
{
    QString stringCmd;
    stringCmd.sprintf("%s",command);
    QStringList items = stringCmd.split(",");
    //check the header
    if(items.at(HEADER_INDEX) != HEADER){
        _error = UNKOWN_ERROR;
        return "error";
    }
    //check the range of the command
    if(items.length() < CMD_GET_PARTS && items.length() > SET_PARTS)
    {
        _error = PARAM_OUT_OF_RANGE;
        return "error";
    }
    if(items.at(TYPE_INDEX) == SET_TYPE && items.length() != SET_PARTS){
        _error = PARAM_OUT_OF_RANGE;
        return "error";
    }
    if((items.at(TYPE_INDEX) == CMD_TYPE || items.at(TYPE_INDEX) == GET_TYPE) && items.length() != CMD_GET_PARTS){
        _error = PARAM_OUT_OF_RANGE;
        return "error";
    }

    //check the length of the following command
    if(items.at(LENGTH_INDEX).length() != 10){
        _error = PARAM_OUT_OF_RANGE;
        return "error";
    }
    _cmdLength = items.at(LENGTH_INDEX).toInt();
    int cmdLength = items.length() - 2;
    for(int i = TYPE_INDEX; i < items.length(); ++i){
        cmdLength += items.at(i).length();
    }
    if(cmdLength != _cmdLength)
    {
        _error = PARAM_OUT_OF_RANGE;
        return "error";
    }
    _cmdType = items.at(TYPE_INDEX);
    //convert the command
    //set commands
    if( _cmdType == SET_TYPE){
        if(setTable.contains(items.at(NAME_INDEX))){
            _cmdName = items.at(NAME_INDEX);
            _cmdValue = items.at(VALUE_INDEX).toDouble();
            QString psiCmd = setTable[_cmdName];
            QStringList psiCmdList = psiCmd.split(";");
            if(psiCmdList.length() > 1){
                PSI_ARG_TYPES psiType = (PSI_ARG_TYPES) psiCmdList.at(1).toInt();
                QString arg = argParser(psiType);
                psiCmd = psiCmdList.at(0) + ";" + arg;
                return psiCmd.toLatin1().data();
            }
            return psiCmdList.at(0).toLatin1().data();
        }
    }
    //get commands
    if( _cmdType == GET_TYPE){
        if(getTable.contains(items.at(NAME_INDEX))){
            _cmdName = items.at(NAME_INDEX);
            QString psiCmd = getTable[_cmdName];
            QStringList psiCmdList = psiCmd.split(";");
            if(psiCmdList.length() > 1){
                PSI_ARG_TYPES psiType = (PSI_ARG_TYPES) psiCmdList.at(1).toInt();
                QString arg = argParser(psiType);
                psiCmd = psiCmdList.at(0) + ";" + arg;
                return psiCmd.toLatin1().data();
            }
            return psiCmdList.at(0).toLatin1().data();
        }
    }

    return "";


}

void MerlinInterface::initializeTables()
{
    //initialize getTable
    getTable.insert(GETSOFTWAREREVISION,"Hello");
    getTable.insert(COLOURMODE,"GetColourMode");
    getTable.insert(CHARGESUMMING,"GetOperationalMode");
    getTable.insert(GAIN,"GetGainMode");
    getTable.insert(CONTINUOUSRW,"GetReadoutMode");
    //initialize setTable
    setTable.insert(COLOURMODE,"SetColourMode;" + QString::number(ENABLE_DISABLE));
    setTable.insert(CHARGESUMMING,"SetOperationalMode;" + QString::number(CSM_SPM));
    setTable.insert(GAIN,"SetGainMode;" + QString::number(HIGH_LOW));
    setTable.insert(CONTINUOUSRW,"SetReadoutMode;" + QString::number(CONT_SEQ));
    //initialize cmdTable
}

QString MerlinInterface::argParser(MerlinInterface::PSI_ARG_TYPES argType)
{
    switch (argType) {
    case N:
        return  QString::number((int)_cmdValue);
        break;
    case N_INF:
        return QString::number((int)_cmdValue);
        break;
    case ENABLE_DISABLE:
        if((int)_cmdValue == 1)
            return "enable";
        else {
            return "disable";
        }
        break;
    case CSM_SPM:
        if((int)_cmdValue == 1)
            return "csm";
        else
            return "spm";
        break;
    case HIGH_LOW:
        if((int)_cmdValue == 0)
            return "shigh";
        else if((int)_cmdValue == 1)
            return "low";
        else if((int)_cmdValue == 2)
            return "high";
        else if((int)_cmdValue == 3)
            return "slow";
        break;
    case CONT_SEQ:
        if((int)_cmdValue == 0)
            return "seq";
        if((int)_cmdValue == 1)
            return "cont";
        break;
    default:
        break;
    }
    return "";
}
