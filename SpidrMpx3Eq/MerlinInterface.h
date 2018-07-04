#ifndef MERLININTERFACE_H
#define MERLININTERFACE_H

#include <QObject>
#include <QHash>
#include "merlinCommandsDef.h"

class MerlinInterface : public QObject
{
    Q_OBJECT
public:
    enum PSI_ARG_TYPES{N,N_INF,CONT_SEQ,CSM_SPM,ENABLE_DISABLE,STRING,OPEN_DOWN,HIGH_LOW};
    enum ERROR_TYPE{NO_ERROR = 0, UNKOWN_ERROR = 1, UNKOWN_COMMAND = 2, PARAM_OUT_OF_RANGE = 3};
    explicit MerlinInterface(QObject *parent = 0);
    char *parseCommand(char *); //get the merlin command and parse it to PSI command
                                //e.g MPX,0000000024,GET,GETSOFTWAREREVISION  ==> Hello
private:
    QString _cmdHeader = "";
    int     _cmdLength = 0;
    QString _cmdType   = "";
    QString _cmdName   = "";
    double  _cmdValue  = 0.0;
    int     _error     = NO_ERROR;
    QString _response  = "";
    QHash<QString,QString> setTable; //key => merlin's command name; value => PSI's command name
    QHash<QString,QString> getTable;
    QHash<QString,QString> cmdTable;
    void initializeTables(void);
    QString argParser(PSI_ARG_TYPES);

private: //constants
    const int SET_PARTS = 5;
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




signals:

public slots:
};

#endif // MERLININTERFACE_H
