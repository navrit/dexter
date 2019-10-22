#ifndef ZMQCONTROLLER_H
#define ZMQCONTROLLER_H

#include "../Qzmq/qzmqcontext.h"
#include "../Qzmq/qzmqsocket.h"

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QJsonDocument>

class Mpx3GUI;
class Mpx3Config;

//! Interface 'documentation' from XRE PDF: 'Acquila socket interface CWI.pdf'
//!
//! SEND events : broadcast a command to be executed by the responsible component
//! REPLY events : broadcasted by the component to give feedback on the progress of a command
//!
//! Example for SEND events:
//! {
//!     "component":“name",
//!     "comp_phys":“physical_name",
//!     "command":"your_command",
//!     "arg1":"",
//!     "arg2":"",
//!     "reply":"",
//!     "reply type":"",
//!     "comp_type":"other",
//!     "tick count":1380210404,
//!     "UUID":26481
//! }
//!
//! ****************************
//!   All fields are required!
//! ****************************
//!
//! --> component : medipix (only this in our case)
//! --> comp_phys : physical name of the addressed component (as described in the Acquila settings file)
//! --> comp_type : possible values are tube, motor, camera or other (other for us only, it's relatively arbitrary)
//! --> command : the command issued
//! --> arg1 : the first argument field (optional, empty if no arguments are needed)
//! --> arg2 : the second argument field (optional, empty if no arguments are needed)
//! --> reply : always empty for SEND events (this field is used for the content of replies, see below)
//! --> reply type :
//!         Possible values are:
//!             RCV (confirmation of reception of the command by the code that will execute the command)
//!             FDB (optional intermediate feedback on the progress of execution)
//!             ACK (confirmation of finishing execution of the command)
//!             ERR (notification that an error occurred while executing the command)
//!             Empty for SEND events
//! --> tick count : a tick count which holds the time when the command was issued by Acquila
//! --> UUID : a unique identifier number that is assigned by Acquila for each broadcasted command. When forwarding a command from the client to Acquila, set this value to 0.

//! When executing a command progress is reported back to the sender through REPLY events.
//! It is critical that both the tick count and UUID which arrived in the SEND event are copied to any REPLY event that relates to this particular command.
//!
//! SEND events:
//! + "reply type" is always empty
//!
//! REPLY events:
//! + When executing a command progress is reported back to the sender through REPLY events.
//! + It is critical that both the tick count and UUID which arrived in the SEND event are copied to any REPLY event that relates to this particular command.
//! + REPLY events should be issued as follows : first a RCV event (obligatory!), one or more FDB events (optional), and finally (obligatory!) either an ACK event or ERR in case of an error.

class zmqController : public QObject
{
    Q_OBJECT
public:
    explicit zmqController(Mpx3GUI *p, QObject *parent = nullptr);
    ~zmqController();

    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }

    //! Helper functions
    bool JsonContains(QJsonObject obj, QString key, QString string);
    bool folderExists(QString path);
    bool fileExists(QString path);

private:
    Mpx3GUI *_mpx3gui = nullptr;
    Mpx3Config *_config = nullptr;

    QString _PUB_addr = ""; //!tcp://127.0.0.1:50001"; //! Eg. "tcp://192.168.178.9:50001";
    QString _SUB_addr = ""; //! tcp://127.0.0.1:50000"; //! Eg. "tcp://192.168.178.9:50000";

    QZmq::Context *_QZmq_context = nullptr;
    QZmq::Socket *_QZmq_PUB_socket = nullptr;
    QZmq::Socket *_QZmq_SUB_socket = nullptr;

    QTimer *_timer = nullptr;

    QQueue<QJsonDocument> *_eventQueue = nullptr;

    QJsonDocument _JsonDocument; //! Unique current JSON document
    QString _currentUUID = "";

    void initialiseJsonResponse();

    void processEvents();
    bool _processingEvents = false;
    QTimer *_eventProcessTimer = nullptr;

    bool _isConnectedToSPIDR = false;

    void takeImage(QJsonObject obj);
    void takeAndSaveImageSequence(QJsonObject obj);
    void saveImage(QJsonObject obj);
    void setExposure(QJsonObject obj);
    void setNumberOfFrames(QJsonObject obj);
    void setThreshold(QJsonObject obj);
    void setGainMode(QJsonObject obj);
    void setCsm(QJsonObject obj);
    void loadDefaultEqualisation(QJsonObject obj);
    void loadEqualisationFromFolder(QJsonObject obj);
    void setReadoutMode(QJsonObject obj);
    void setReadoutFrequency(QJsonObject obj);
    void loadConfiguration(QJsonObject obj);
    void setIntegration(QJsonObject obj);

signals:
    void takeImage();
    void takeAndSaveImageSequence(QString folder);
    void saveImageSignal(QString filePath);
    void setExposure(int microseconds);
    void setNumberOfFrames(int number_of_frames);
    void setThreshold(int threshold, int value);
    void setGainMode(int mode);
    void setCSM(bool active);
    void loadDefaultEqualisation();
    void loadEqualisation(QString filePath);
    void setReadoutMode(QString mode);
    void setReadoutFrequency(int frequency); //! in Hz
    void loadConfiguration(QString filePath);
    void setIntegration(bool integrate);

public slots:
    void addressChanged_PUB(QString addr); //! Apparently you cannot change addresses for an existing socket or something ???
    void addressChanged_SUB(QString addr);
    void sendZmqMessage();
    void tryToProcessEvents();
    void tryToSendFeedback();

private slots:
    void sock_readyRead(); //! Just read all the events coming in, parse, caste and push to back of eventQueue
    void sock_messagesWritten(int count); //! Mainly for debugging purposes
    void someCommandHasFinished_Successfully();
    void someCommandHasFailed(QString reply="");
};

#endif
