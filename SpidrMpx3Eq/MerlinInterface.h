#ifndef MERLININTERFACE_H
#define MERLININTERFACE_H

#include <QObject>
#include <QHash>
#include <QVector>
#include "merlinCommandsDef.h"

class Mpx3GUI;

class FrameHeaderDataStruct{
public:
    FrameHeaderDataStruct(Mpx3GUI *gui);
    QString toQString(int frameid);

    uint16_t dataOffset = 0;
    uint8_t numberOfChips = 0;
    uint32_t xDim = 0;
    uint32_t yDim = 0;
    QString pixelDepth = "U32";
    const QString sensorLayout = "   2x2"; //! The whitespaces intentional
    uint8_t chipSelect = 0;
    QString timeStamp;
    double shutterOpen = 0.0;
    uint8_t counter = 0;
    uint8_t colorMode = 0;
    uint8_t gainMode = 0;
    double threshold0 = 0; //in kev
    double threshold1 = 0; //in kev
    double threshold2 = 0; //in kev
    double threshold3 = 0; //in kev
    double threshold4 = 0; //in kev
    double threshold5 = 0; //in kev
    double threshold6 = 0; //in kev
    double threshold7 = 0; //in kev
    //here comes DAC fields
    const QString dacFormat = "3RX";
    uint16_t dacThreshold0 = 0; //9 bit DAC
    uint16_t dacThreshold1 = 0; //9 bit DAC
    uint16_t dacThreshold2 = 0; //9 bit DAC
    uint16_t dacThreshold3 = 0; //9 bit DAC
    uint16_t dacThreshold4 = 0; //9 bit DAC
    uint16_t dacThreshold5 = 0; //9 bit DAC
    uint16_t dacThreshold6 = 0; //9 bit DAC
    uint16_t dacThreshold7 = 0; //9 bit DAC
    uint8_t preamp = 0;
    uint8_t ikrum = 0;
    uint8_t shaper = 0;
    uint8_t disc = 0;
    uint8_t discLs = 0;
    uint8_t shaperTest = 0;
    uint8_t dacDiscL = 0;
    uint8_t dacTest = 0;
    uint8_t dacDiscH = 0;
    uint8_t delay = 0;
    uint8_t tpBuffIn = 0;
    uint8_t tpBuffOut = 0;
    uint8_t rpz = 0;
    uint8_t gnd = 0;
    uint8_t tpRef = 0;
    uint8_t fpk = 0;
    uint8_t cas = 0;
    uint16_t tpRefA = 0;
    uint16_t tpRefB = 0;
};

enum PSL_ARG_TYPES{N,N_INF,CONT_SEQ,CSM_SPM,ENABLE_DISABLE,STRING,OPEN,DOWN,HIGH_LOW,TH0,TH1,TH2,TH3,TH4,TH5,TH6,TH7};

class MerlinInterface : public QObject
{
    Q_OBJECT

public:
    explicit MerlinInterface(QObject *parent = nullptr);
private:
    QHash<QString,QString> setTable; //key => merlin's command name; value => PSL's command name
    QHash<QString,QString> getTable;
    QHash<QString,QString> cmdTable;
    void initializeTables(void);

    const int CHIPS_NUM = 4;
    const int FRAME_HEADER_SIZE = 256 + (128*CHIPS_NUM);

    friend class MerlinCommand;
};

class MerlinCommand {

    enum ERROR_TYPE{NO_ERROR = 0, UNKNOWN_ERROR = 1, UNKNOWN_COMMAND = 2, PARAM_OUT_OF_RANGE = 3, SB_DATA_TAKING = 101
                   ,SB_EQUALIZATION = 102,SB_DAC_SCAN = 103,SB_THRESHOLD_SCAN = 104,INVALID_ARG = 200};

public:
    MerlinCommand(QString, MerlinInterface&); //get the merlin command and parse it to PSL command
                                //e.g MPX,0000000024,GET,SOFTWAREVERSION  ==> Hello
    QString argParser(PSL_ARG_TYPES);
    QString makeSetCmdResponse(void);
    QString makeGetResponse(QString);
    QString getCommandType(void);
    void setErrorExternally(int);

    int     _cmdLength = 0;
    QString parseResult;
    QString _cmdType   = "";
    QString _cmdName   = "";
    QVector<QString>  _cmdValue;
    int     _error     = NO_ERROR;
    QString _response  = "";
    int _ptrCmdValue = 0;

private: //constants
    const int SET_PARTS = 6;
    const int CMD_GET_PARTS = 4;
    const QString SET_TYPE = "SET";
    const QString GET_TYPE = "GET";
    const QString CMD_TYPE = "CMD";
    const int HEADER_INDEX = 0;
    const int LENGTH_INDEX = 1;
    const int TYPE_INDEX = 2;
    const int NAME_INDEX = 3;
    const int VALUE_INDEX = 4;
    const QString HEADER = "MPX";
};


#endif // MERLININTERFACE_H
