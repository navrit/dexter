#ifndef ZMQCONTROLLER_H
#define ZMQCONTROLLER_H

#include "../Qzmq/qzmqcontext.h"
#include "../Qzmq/qzmqsocket.h"

#include <QObject>
#include "mpx3gui.h"
#include "QTimer"

class zmqController : public QObject
{
    Q_OBJECT
public:
    explicit zmqController(Mpx3GUI *, QObject *parent = nullptr);
    ~zmqController();

    void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }

    //! Currently unused
    QZmq::Context *getZmqContext();
    QZmq::Socket *getZmq_PUB_socket();
    QZmq::Socket *getZmq_SUB_socket();


private:
    Mpx3GUI * _mpx3gui;

    const QString PUB_addr = "tcp://127.0.0.1:5557";
    const QString SUB_addr = "tcp://127.0.0.1:5555";

    QZmq::Context * QZmq_context;
    QZmq::Socket * QZmq_PUB_socket;
    QZmq::Socket * QZmq_SUB_socket;

    QTimer *timer;

signals:

public slots:
    void sendZmqMessage();

private slots:
    void sock_readyRead();
    void sock_messagesWritten(int count);
};

#endif ZMQCONTROLLER_H
