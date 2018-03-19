#ifndef ZMQCONTROLLER_H
#define ZMQCONTROLLER_H

#include "../Qzmq/qzmqcontext.h"
#include "../Qzmq/qzmqsocket.h"

#include <QObject>
#include "mpx3gui.h"
#include "QTimer"

//! Interface 'documentation' from XRE PDF: 'Acquila socket interface CWI.pdf'
//!
//! SEND events : broadcast a command to be executed by the responsible component
//! REPLY events : broadcasted by the component to give feedback on the progress of a command
//!
//! Example for SEND events:
//!    {"component":“name","comp_phys":“physical_name","command":"your_command", "arg1":"","arg2":"","reply":"","reply type":"","comp_type":"other","tick count":1380210404,"UUID":26481}
//!
//! All fields are required!
//!
//! --> component : medipix
//! --> comp_type : possible values are tube, motor, camera or other (medipix for us)
//! --> command : the command issued
//! --> arg1 : the first argument field (optional, empty if no arguments are needed)
//! --> arg2 : the second argument field (optional, empty if no arguments are needed)
//! --> tick count : a tick count which holds the time when the command was issued by Acquila
//! --> a unique identifier number that is assigned by Acquila for each broadcasted command. When forwarding a command from the client to Acquila, set this value to 0.

//! Empty for SEND events.
//! For REPLY events the possible values are:
//!    RCV (confirmation of reception of the command by the code that will execute the command)
//!    FDB (optional intermediate feedback on the progress of execution)
//!    ACK (confirmation of finishing execution of the command)
//!    ERR (notification that an error occurred while executing the command)
//!
//! REPLY events:
//! When executing a command progress is reported back to the sender through REPLY events.
//! It is critical that both the tick count and UUID which arrived in the SEND event are copied to any REPLY event that relates to this particular command.
//! REPLY events should be issued as follows : first a RCV event (obligatory!), one or more FDB events (optional), and finally (obligatory!) either an ACK event or
//! ERR in case of an error.

class zmqController : public QObject
{
    Q_OBJECT
public:
    explicit zmqController(Mpx3GUI *p, QObject *parent = nullptr);
    ~zmqController();

    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }

    //! Currently unused
    QZmq::Context *getZmqContext();
    QZmq::Socket *getZmq_PUB_socket();
    QZmq::Socket *getZmq_SUB_socket();


private:
    Mpx3GUI * _mpx3gui = nullptr;
    Mpx3Config * config = nullptr;

    QString PUB_addr = "tcp://127.0.0.1:5556";
    QString SUB_addr = "tcp://127.0.0.1:5555";

    QZmq::Context * QZmq_context;
    QZmq::Socket * QZmq_PUB_socket;
    QZmq::Socket * QZmq_SUB_socket;

    QTimer *timer;

    enum reply_type {
        RCV,
        FDB,
        ACK,
        ERR
    };

    void initialise();
    void addressChanged();

signals:

public slots:
    void addressChanged_PUB(QString addr);
    void addressChanged_SUB(QString addr);
    void sendZmqMessage();

private slots:
    void sock_readyRead();
    void sock_messagesWritten(int count);
};

#endif ZMQCONTROLLER_H
