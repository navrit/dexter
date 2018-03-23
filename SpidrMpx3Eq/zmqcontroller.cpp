#include "zmqcontroller.h"
#include "QDebug"
#include "QMessageBox"
#include "mpx3config.h"
#include "qcstmglvisualization.h"
#include <QFileInfo>

#include "../Qzmq/qzmqcontext.h"
#include "../Qzmq/qzmqsocket.h"

#include "qjsonobject.h"
#include "qjsondocument.h"

zmqController::zmqController(Mpx3GUI * p, QObject *parent) : QObject(parent)
{
    SetMpx3GUI(p);
    config = _mpx3gui->getConfig();
    timer = new QTimer(this);
    eventProcessTimer = new QTimer(this);

    QZmq_context = new QZmq::Context();
    QZmq_PUB_socket = new QZmq::Socket(QZmq::Socket::Type::Pub, QZmq_context, this);
    QZmq_SUB_socket = new QZmq::Socket(QZmq::Socket::Type::Sub, QZmq_context, this);

    eventQueue = new QQueue<QJsonDocument>();

//    QZmq_PUB_socket->connectToAddress(PUB_addr);
//    qDebug() << "[INFO]\tZMQ Connected to PUB socket:" << PUB_addr;

//    QZmq_SUB_socket->connectToAddress(SUB_addr);
//    QZmq_SUB_socket->subscribe("");
//    qDebug() << "[INFO]\tZMQ Connected to SUB socket:" << SUB_addr;

    initialiseJsonResponse();

    // ---------------------- Internal class signals ---------------------
    connect(config, SIGNAL(IpZmqPubAddressChanged(QString)), this, SLOT(addressChanged_PUB(QString)));
    connect(config, SIGNAL(IpZmqSubAddressChanged(QString)), this, SLOT(addressChanged_SUB(QString)));

    connect(QZmq_PUB_socket, SIGNAL(messagesWritten(int)), SLOT(sock_messagesWritten(int)));
    connect(QZmq_SUB_socket, SIGNAL(readyRead()), SLOT(sock_readyRead()));

    connect(timer, SIGNAL(timeout()), this, SLOT(tryToProcessEvents()));
    //! Don't need to connect the failed attempts to change the address here. That's taken care of elsewhere
    connect(eventProcessTimer, SIGNAL(timeout()), this, SLOT(tryToSendFeedback()));
    // -------------------------------------------------------------------


    //  ---------------------- External class signals ---------------------
    //! This is logically connected to takeImage and takeAndSaveImageSequence
    connect(_mpx3gui->getVisualization(), SIGNAL(someCommandHasFinished_Successfully()),
            this, SLOT(someCommandHasFinished_Successfully()));
    connect(_mpx3gui->getVisualization(), SIGNAL(someCommandHasFailed()),
            this, SLOT(someCommandHasFailed()));

    //! Straight name mapping to appropriate slots in visualisation, could go anywhere
    connect(this, SIGNAL(takeImage()), _mpx3gui->getVisualization(), SLOT(takeImage()));
    connect(this, SIGNAL(takeAndSaveImageSequence()), _mpx3gui->getVisualization(), SLOT(takeAndSaveImageSequence()));
    connect(this, SIGNAL(saveImageSignal(QString)), _mpx3gui->getVisualization(), SLOT(saveImageSlot(QString)));
    connect(this, SIGNAL(setExposure(int)), _mpx3gui->getVisualization(), SLOT(setExposure(int)));
    connect(this, SIGNAL(setNumberOfFrames(uint64_t)), _mpx3gui->getVisualization(), SLOT(setNumberOfFrames(uint64_t)));
    connect(this, SIGNAL(setThreshold(uint16_t,uint16_t)), _mpx3gui->getVisualization(), SLOT(setThreshold(uint16_t,uint16_t)));
    connect(this, SIGNAL(setGainMode(QString)), _mpx3gui->getVisualization(), SLOT(setGainMode(QString)));
    connect(this, SIGNAL(setCSM(bool)), _mpx3gui->getVisualization(), SLOT(setCSM(bool)));
    connect(this, SIGNAL(loadDefaultEqualisation()), _mpx3gui->getVisualization(), SLOT(loadDefaultEqualisation()));
    connect(this, SIGNAL(loadEqualisation(QString)), _mpx3gui->getVisualization(), SLOT(loadEqualisation(QString)));
    connect(this, SIGNAL(setReadoutMode(QString)), _mpx3gui->getVisualization(), SLOT(setReadoutMode(QString)));
    connect(this, SIGNAL(setReadoutFrequency(uint16_t)), _mpx3gui->getVisualization(), SLOT(setReadoutFrequency(uint16_t)));
    connect(this, SIGNAL(loadConfiguration(QString)), _mpx3gui->getVisualization(), SLOT(loadConfiguration(QString)));
    connect(this, SIGNAL(setNumberOfAverages(uint64_t)), _mpx3gui->getVisualization(), SLOT(setNumberOfAverages(uint64_t)));
    // -------------------------------------------------------------------

}

zmqController::~zmqController()
{
    timer->stop();
    delete timer;
    timer = nullptr;

    eventProcessTimer->stop();
    delete eventProcessTimer;
    eventProcessTimer = nullptr;

    delete eventQueue;
    delete QZmq_PUB_socket;
    QZmq_SUB_socket->unsubscribe(""); //! This may not be necessary
    QZmq_SUB_socket->setTcpKeepAliveEnabled(false); //! This may not be necessary
    delete QZmq_SUB_socket;
    delete QZmq_context;
    QZmq_context = nullptr;
    QZmq_PUB_socket = nullptr;
    QZmq_SUB_socket = nullptr;
    eventQueue = nullptr;
}

void zmqController::addressChanged_PUB(QString addr)
{
    if ( addr == PUB_addr ) {
        return;
    } else {
        PUB_addr = addr;
        qDebug() << "[INFO]\tZMQ PUB address changed:" << addr;
    }

//    if (QZmq_PUB_socket != nullptr) {
//        delete QZmq_PUB_socket;
//        QZmq_PUB_socket = nullptr;
//    }

//    QZmq_PUB_socket = new QZmq::Socket(QZmq::Socket::Type::Pub, QZmq_context, this);
    QZmq_PUB_socket->connectToAddress(PUB_addr);
    qDebug() << "[INFO]\tZMQ Connected to PUB socket:" << PUB_addr;

    initialiseJsonResponse();

}

void zmqController::addressChanged_SUB(QString addr)
{
    if ( addr == SUB_addr ) {
        return;
    } else {
        SUB_addr = addr;
        qDebug() << "[INFO]\tZMQ SUB address changed:" << addr;
    }

    if (QZmq_SUB_socket != nullptr) {
        QZmq_SUB_socket->unsubscribe("");
//        QZmq_SUB_socket->setTcpKeepAliveEnabled(false);
//        delete QZmq_SUB_socket;
//        QZmq_SUB_socket = nullptr;
    }

//    QZmq_SUB_socket = new QZmq::Socket(QZmq::Socket::Type::Sub, QZmq_context, this);
    QZmq_SUB_socket->connectToAddress(SUB_addr);
    QZmq_SUB_socket->subscribe("");
    qDebug() << "[INFO]\tZMQ Connected to SUB socket:" << SUB_addr;

    initialiseJsonResponse();
}

void zmqController::initialiseJsonResponse()
{
    QJsonObject root_obj;
    root_obj.insert("component","medipix");
    root_obj.insert("comp_type","other");
    root_obj.insert("comp_phys","medipix");
    root_obj.insert("command","");
    root_obj.insert("arg1","");
    root_obj.insert("arg2","");
    root_obj.insert("reply","");
    root_obj.insert("reply type","");
    root_obj.insert("tick count",0);
    root_obj.insert("UUID",0);
    JsonDocument = QJsonDocument(root_obj);
}

void zmqController::processEvents()
{
    //!                     Overview
    //!
    //! Received a SEND event --> if reply type == ""
    //! Send REPLY event(s) in response to thats specific SEND event.
    //!    Do not process other events until the first is complete

    JsonDocument = eventQueue->dequeue(); //! This is checked to have at least one event otherwise this event will not be called
    processingEvents = true; //! Used as a controller level lock
    QJsonObject root_obj = JsonDocument.object();

    //! 1. Received a SEND event from server
    if (root_obj["reply type"].toString() == "") {

        //! 2. REPLY with RCV immediately
        root_obj["reply type"] = "RCV";
        JsonDocument.setObject( root_obj );
        sendZmqMessage();

        //! 3. If it's taking a while, REPLY with FDB events at least at 1Hz independently
        //!    Done eleswhere via a QTimer --> eventProcessTimer
        //!    It just checks to see if this is still processing events

        //! 4. Emit relevant signals to trigger whatever we want elsewhere
        //!    This must not be blocking
        if ( JsonContains(root_obj, "command", "take image") ) {
            qDebug() << "[INFO]\tZMQ TAKE IMAGE :"  << root_obj["command"].toString();
            emit takeImage();

        } else if ( JsonContains(root_obj, "command", "take and save image sequence") ) {
            qDebug() << "[INFO]\tZMQ TAKE AND SAVE IMAGE SEQUENCE :"  << root_obj["command"].toString();
            emit takeAndSaveImageSequence();

        } else if ( JsonContains(root_obj, "command", "save image") ) {
            qDebug() << "[INFO]\tZMQ SAVE IMAGE :"  << root_obj["command"].toString() << root_obj["arg1"].toString();
            emit saveImageSignal(root_obj["arg1"].toString());

        } else if ( JsonContains(root_obj, "command", "set exposure") ) {
            qDebug() << "[INFO]\tZMQ SET EXPOSURE :"  << root_obj["command"].toString() << root_obj["arg1"].toString().toInt();

            bool ok;
            int arg1 = root_obj["arg1"].toString().toInt(&ok);
            if (ok) {
                emit setExposure(arg1);
            }

            emit setExposure(arg1);

        } else if ( JsonContains(root_obj, "command", "set number of frames") ) {
            qDebug() << "[INFO]\tZMQ SET NUMBER OF FRAMES :"  << root_obj["command"].toString() << root_obj["arg1"].toString();
            bool ok;
            uint64_t arg1 = root_obj["arg1"].toString().toInt(&ok);
            if (ok) {
                emit setNumberOfFrames(arg1);
            }

        } else if ( JsonContains(root_obj, "command", "set threshold") ) {
            qDebug() << "[INFO]\tZMQ SET THRESHOLD :"  << root_obj["command"].toString();

            //! TODO

        } else if ( JsonContains(root_obj, "command", "set gain mode") ) {
            qDebug() << "[INFO]\tZMQ SET GAIN MODE :"  << root_obj["command"].toString() << root_obj["arg1"].toString();

            emit setGainMode(root_obj["arg1"].toString());

        } else if ( JsonContains(root_obj, "command", "set csm")) {
            qDebug() << "[INFO]\tZMQ SET CSM :"  << root_obj["command"].toString() << root_obj["arg1"].toString();

            QString arg1 = root_obj["arg1"].toString().toLower();
            if (arg1.contains("true") || arg1.contains("on")) {
                emit setCSM(true);
            } else if (arg1.contains("false") || arg1.contains("off")) {
                emit setCSM(false);
            } else {
                qDebug() << "[INFO]";
            }

        } else if ( JsonContains(root_obj, "command", "load default equalisation") ) {
            qDebug() << "[INFO]\tZMQ LOAD DEFAULT EQUALISATION :"  << root_obj["command"].toString();

            emit loadDefaultEqualisation();

        } else if ( JsonContains(root_obj, "command", "load equalisation from folder") ) {
            qDebug() << "[INFO]\tZMQ LOAD EQUALISATION FROM FOLDER:"  << root_obj["command"].toString() << root_obj["arg1"].toString();

            QString arg1 = root_obj["arg1"].toString(); //! You actually need to keep the case here, Linux filesystems are mostly case sensitive of course!

            if ( folderExists(arg1) && fileExists( QDir::cleanPath( QString(arg1 + QDir::separator() + "adj_0"))) && fileExists( QDir::cleanPath( QString(arg1 + QDir::separator() + "mask_0"))) ) {
                //! Probably fine to proceed to loading the equalisation
                emit loadEqualisation(arg1);
            } else {
                qDebug() << "[ERROR]\tZMQ failed to load non-default equalisation from :" << arg1;
            }

        } else if ( JsonContains(root_obj, "command", "set readout mode") ) {
            qDebug() << "[INFO]\tZMQ SET READOUT MODE :"  << root_obj["command"].toString();

        } else if ( JsonContains(root_obj, "command", "set readout frequency") ) {
            qDebug() << "[INFO]\tZMQ SET READOUT FREQUENCY :"  << root_obj["command"].toString();

        } else if ( JsonContains(root_obj, "command", "load configuration") ) {
            qDebug() << "[INFO]\tZMQ LOAD CONFIGURATION FILE :"  << root_obj["command"].toString();

        } else if ( JsonContains(root_obj, "command", "set number of averages") ) {
            qDebug() << "[INFO]\tZMQ SET NUMBER OF AVERAGES :" << root_obj["command"].toString();

        } else {
            qDebug() << "[ERROR]\tZMQ Unrecognised command... : " << root_obj["command"].toString();
            processingEvents = false;
        }
    }
    //! 5. when it's processed, REPLY with a ACK event --> see the SLOT dataTakingFinishedAndSaved()
}

bool zmqController::JsonContains(QJsonObject obj, QString key, QString string)
{
    if ( obj[key].toString().toLower().contains(string) ) {
        return true;
    } else {
        return false;
    }
}

bool zmqController::folderExists(QString path)
{
    QFileInfo check_file(path);
    return check_file.exists() && check_file.isReadable();
}

bool zmqController::fileExists(QString path)
{
    QFileInfo check_file(path);
    return check_file.exists() && check_file.isFile() && check_file.isReadable();
}

void zmqController::sendZmqMessage()
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Writing:" <<  QString(JsonDocument.toJson());

//    if (JsonDocument.object()["component"].toString() == "medipix") {
//        qDebug() << "[INFO]\tZMQ CHECK - JSON successfully encoded: component =" << json_doc.object().value("component").toString();
//    } else {
//        qDebug() << "[INFO]\tZMQ CHECK - JSON encoding FAILED";
//    }
#endif

    //! This is just how you construct the QList of QByteArrays, super weird
    const QList<QByteArray> outList = QList<QByteArray>() << QString(JsonDocument.toJson()).toLocal8Bit();
    QZmq_PUB_socket->write(outList);
}

void zmqController::tryToProcessEvents()
{
    //! Only try to process events if not already processing an event and if there are events to process
    if (!processingEvents && !eventQueue->isEmpty()) {
        processEvents();
    }
}

void zmqController::tryToSendFeedback()
{
    //! Doesn't matter if the eventQueue is empty
    if(processingEvents) {
        QJsonObject root_obj = JsonDocument.object();
        root_obj["reply type"] = QString("FDB");
        JsonDocument = QJsonDocument(root_obj);
        qDebug() << "[INFO]\tZMQ Trying to send FDB";
        sendZmqMessage();
    }
}

void zmqController::sock_readyRead()
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Reading a message from the ZMQ server";
#endif
    //! This is just the format that you need to use with QZmq
    const QList<QByteArray> msg = QZmq_SUB_socket->read();
    if (msg.isEmpty()){
        qDebug() << "[ERROR]\tZMQ Received empty message\n";
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ First message read: " << msg.first();
    qDebug() << "[INFO]\tZMQ JSON document length:" << msg.length() << "\n";
#endif

    QJsonDocument tmp = QJsonDocument::fromJson(msg.first());
    if (tmp.object()["component"].toString() != "medipix") {
        return;
    }

    eventQueue->push_back( tmp );

    if (!timer->isActive() && !eventProcessTimer->isActive()) {
        timer->start(10); //! milliseconds
        eventProcessTimer->start(1000); //! milliseconds
    }

#ifdef QT_DEBUG
    QJsonDocument json_doc = QJsonDocument::fromJson( msg.first() );
    if (json_doc.object().value("command").toString() == "take image") {
        qDebug() << "[INFO]\tZMQ CHECK - Received command to TAKE IMAGE!";
    } else {
        qDebug() << "[INFO]\tZMQ CHECK - DID NOT receive command to take image";
    }
#endif
}

void zmqController::sock_messagesWritten(int count)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Messages written:" << count << "\n";
#endif
}

void zmqController::someCommandHasFinished_Successfully()
{
    QJsonObject root_obj = JsonDocument.object();
    root_obj["reply type"] = QString("ACK");
    JsonDocument = QJsonDocument(root_obj);
    sendZmqMessage();
    qDebug() << "[INFO]\tZMQ Sent ACK";
    processingEvents = false;
}

void zmqController::someCommandHasFailed()
{
    QJsonObject root_obj = JsonDocument.object();
    root_obj["reply type"] = QString("ERR");
    JsonDocument = QJsonDocument(root_obj);
    sendZmqMessage();
    qDebug() << "[INFO]\tZMQ Sent ERR";
    processingEvents = false;
}
