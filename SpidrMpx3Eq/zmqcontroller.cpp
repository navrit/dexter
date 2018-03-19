#include "zmqcontroller.h"
#include "QDebug"
#include "QMessageBox"
#include "mpx3config.h"

#include "../Qzmq/qzmqcontext.h"
#include "../Qzmq/qzmqsocket.h"

#include "qjsonobject.h"
#include "qjsondocument.h"

zmqController::zmqController(Mpx3GUI * p, QObject *parent) : QObject(parent)
{
    SetMpx3GUI(p);
    config = _mpx3gui->getConfig();
    timer = new QTimer(this);

    initialise();

    connect(config, SIGNAL(IpZmqPubAddressChanged(QString)), this, SLOT(addressChanged_PUB(QString)));
    connect(config, SIGNAL(IpZmqSubAddressChanged(QString)), this, SLOT(addressChanged_SUB(QString)));

    connect(QZmq_PUB_socket, SIGNAL(messagesWritten(int)), SLOT(sock_messagesWritten(int)));
    connect(QZmq_SUB_socket, SIGNAL(readyRead()), SLOT(sock_readyRead())); //! TODO This signal never seems to be triggered...

    connect(timer, SIGNAL(timeout()), this, SLOT(sendZmqMessage()));
    //! Don't need to connect the failed attempts to change the address here. That's taken care of elsewhere

}

zmqController::~zmqController()
{
    delete QZmq_context;
    delete QZmq_PUB_socket;
    delete QZmq_SUB_socket;
    QZmq_context = nullptr;
    QZmq_PUB_socket = nullptr;
    QZmq_SUB_socket = nullptr;
    timer->stop();
    delete timer;
    timer = nullptr;
}

QZmq::Context *zmqController::getZmqContext()
{
    if (QZmq_context != nullptr){
        return QZmq_context;
    }
    else {
        qDebug() << "[ERROR] Could not make a ZMQ Context, is the port bound by another program?";
        QMessageBox::warning ( _mpx3gui, tr("Error"), "Could not make a ZMQ Context, is the port bound by another program?" );
        return nullptr;
    }
}

QZmq::Socket *zmqController::getZmq_PUB_socket()
{
    if (QZmq_PUB_socket != nullptr){
        return QZmq_PUB_socket;
    }
    else {
        qDebug() << "[ERROR] ZMQ Could not make a ZMQ PUB socket, is the port bound by another program?";
        qDebug() << "[ERROR] ZMQ I cannot send any messages";
        return nullptr;
    }
}

QZmq::Socket *zmqController::getZmq_SUB_socket()
{
    if (QZmq_SUB_socket != nullptr){
        return QZmq_SUB_socket;
    }
    else {
        qDebug() << "[ERROR] Could not make a ZMQ SUB Socket, is the port bound by another program?";
        qDebug() << "[ERROR] ZMQ I cannot receive any messages";
        return nullptr;
    }
}

void zmqController::initialise()
{
    QZmq_context = new QZmq::Context();
    QZmq_PUB_socket = new QZmq::Socket(QZmq::Socket::Type::Pub, QZmq_context, this);
    QZmq_SUB_socket = new QZmq::Socket(QZmq::Socket::Type::Sub, QZmq_context, this);

    QZmq_PUB_socket->connectToAddress(PUB_addr);
    qDebug() << "[INFO]\tZMQ Connected to PUB socket:" << PUB_addr;

    QZmq_SUB_socket->connectToAddress(SUB_addr);
    QZmq_SUB_socket->subscribe("");
    qDebug() << "[INFO]\tZMQ Connected to SUB socket:" << SUB_addr;

    // msec
    timer->start(1000);
}

void zmqController::addressChanged_PUB(QString addr)
{
    PUB_addr = addr;
    qDebug() << "[INFO]\tZMQ PUB address changed:" << addr;
    addressChanged();
}

void zmqController::addressChanged_SUB(QString addr)
{
    SUB_addr = addr;
    qDebug() << "[INFO]\tZMQ SUB address changed:" << addr;
    addressChanged();
}

void zmqController::addressChanged()
{
    //! Reinitialise with new addresses
    initialise();

    qDebug() << "[INFO]\tZMQ address changed, context and sockets reinitialised";
}

void zmqController::sendZmqMessage()
{
    QJsonObject root_obj;
    root_obj.insert("component","medipix");
    root_obj.insert("comp_phys","medipix");
    root_obj.insert("command","take image");
    root_obj.insert("arg1","");
    root_obj.insert("arg2","");
    root_obj.insert("reply","got it");
    root_obj.insert("reply type","");
    root_obj.insert("comp_type","other");
    root_obj.insert("tick count",0);
    root_obj.insert("UUID",0);
    QJsonDocument json_doc(root_obj);
    QString json_string = json_doc.toJson();


#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Writing:" << json_string;

    if (json_doc.object().value("component").toString() == "medipix") {
        qDebug() << "[INFO]\tZMQ CHECK - JSON successfully encoded: component =" << json_doc.object().value("component").toString();
    } else {
        qDebug() << "[INFO]\tZMQ CHECK - JSON encoding FAILED";
    }
#endif

    const QList<QByteArray> outList = QList<QByteArray>() << json_string.toLocal8Bit();
    QZmq_PUB_socket->write(outList);
}

void zmqController::sock_readyRead()
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Reading a message from the ZMQ server";
#endif
    const QList<QByteArray> msg = QZmq_SUB_socket->read();
    if(msg.isEmpty()){
        qDebug() << "[ERROR]\tZMQ Received empty message\n";
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ First message read: " << msg.first();
    qDebug() << "[INFO]\tZMQ JSON document length:" << msg.length() << "\n";
#endif

    QJsonDocument json_doc = QJsonDocument::fromJson( msg.first() );

#ifdef QT_DEBUG
    if (json_doc.object().value("command").toString() == "take image") {
        qDebug() << "[INFO]\tZMQ CHECK - Received command to TAKE IMAGE!";
    } else {
        qDebug() << "[INFO]\tZMQ CHECK - DID NOT receive command to take image";
    }
#endif

    sendZmqMessage();
}

void zmqController::sock_messagesWritten(int count)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Messages written:" << count << "\n";
#endif
}
