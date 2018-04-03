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

#include <QRegExp>

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
    connect(this, SIGNAL(setNumberOfFrames(int)), _mpx3gui->getVisualization(), SLOT(setNumberOfFrames(int)));
    connect(this, SIGNAL(setThreshold(int,int)), _mpx3gui->getVisualization(), SLOT(setThreshold(int,int)));
    connect(this, SIGNAL(setGainMode(int)), _mpx3gui->getVisualization(), SLOT(setGainMode(int)));
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
    root_obj.insert("tick count",-1);
    root_obj.insert("UUID",-1);
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
    QJsonObject root_obj = JsonDocument.object();

    //! 1. Received a SEND event from server
    if (root_obj["reply type"].toString() == "") {

        processingEvents = true; //! Used as a controller level lock

        //! 2. REPLY with RCV immediately
        root_obj["reply type"] = "RCV";
        JsonDocument.setObject( root_obj );
        sendZmqMessage();

        //! If not connected to a SPIDR at this point, you should really connect
        //! However, not all commands need this obviously
        //! Blocking is acceptable because the other commands probably cannot run without being connected
        if (!_mpx3gui->getConfig()->isConnected()) {
            if ( _mpx3gui->establish_connection() ) {
                qDebug() << "[INFO]\tZMQ Connected to SPIDR via remote command";
            } else {
                qDebug() << "[ERROR]\tZMQ Failed to connect to a SPIDR via a remote command";
            }
        }

        //! 3. If it's taking a while, REPLY with FDB events at least at 1Hz independently
        //!    Done eleswhere via a QTimer --> eventProcessTimer
        //!    It just checks to see if this is still processing events

        //! 4. Emit relevant signals to trigger whatever we want elsewhere
        //!    This must not be blocking
        if ( JsonContains(root_obj, "command", "take image") ) {
            takeImage(root_obj);

        } else if ( JsonContains(root_obj, "command", "take and save image sequence") ) {
            takeAndSaveImageSequence(root_obj);

        } else if ( JsonContains(root_obj, "command", "save image") ) {
            saveImage(root_obj);

        } else if ( JsonContains(root_obj, "command", "set exposure") ) {
            setExposure(root_obj);

        } else if ( JsonContains(root_obj, "command", "set number of frames") ) {
            setNumberOfFrames(root_obj);

        } else if ( JsonContains(root_obj, "command", "set threshold") ) {
            setThreshold(root_obj);

        } else if ( JsonContains(root_obj, "command", "set gain mode") ) {
            setGainMode(root_obj);

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
                someCommandHasFailed();
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
            someCommandHasFailed();
            processingEvents = false;
        }
    }
    //! 5. when it's processed, REPLY with a ACK event --> see the SLOT dataTakingFinishedAndSaved()
}

void zmqController::takeImage(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ TAKE IMAGE :"  << obj["command"].toString();
#endif
    emit takeImage();
}

void zmqController::takeAndSaveImageSequence(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ TAKE AND SAVE IMAGE SEQUENCE :"  << obj["command"].toString();
#endif
    emit takeAndSaveImageSequence();
}

void zmqController::saveImage(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SAVE IMAGE :"  << obj["command"].toString() << obj["arg1"].toString();
#endif
    emit saveImageSignal(obj["arg1"].toString());
}

void zmqController::setExposure(QJsonObject obj)
{
#ifdef QT_DEBUG

    qDebug() << "[INFO]\tZMQ SET EXPOSURE :"  << obj["command"].toString() << obj["arg1"].toString().toInt();
#endif

    bool ok;
    int arg1 = obj["arg1"].toString().toInt(&ok);

    //! 1 microsecond is the shortest exposure time that's reasonable
    if (ok && arg1 >= 1) {
        obj["reply"] = QString::number( arg1 );
        JsonDocument = QJsonDocument(obj);
        emit setExposure(arg1);
    } else {
        someCommandHasFailed();
    }
}

void zmqController::setNumberOfFrames(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SET NUMBER OF FRAMES :"  << obj["command"].toString() << obj["arg1"].toString();
#endif

    bool ok;
    int arg1 = obj["arg1"].toString().toInt(&ok);
    if (ok && arg1 >= 0) {
        emit setNumberOfFrames(arg1);
    } else {
        someCommandHasFailed();
    }
}

void zmqController::setThreshold(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SET THRESHOLD :"  << obj["command"].toString() << obj["arg1"].toString() << obj["arg2"].toString();
#endif

    //! TODO Test it
    //! Arg1: Which threshold to change
    //! Arg2: Threshold value (DAC units)

    bool ok;
    int arg1 = obj["arg1"].toString().toInt(&ok);
    bool ok2;
    int arg2 = obj["arg2"].toString().toInt(&ok2);

    //! May want to handle the failures better

    if (ok && ok2) {
        if (arg1 >= 0 && arg1 <= 7) {
            if (arg2 >= 0 && arg2 <= 511) {
                //! SUCCESS, this is actually valid input
                emit setThreshold(arg1, arg2);
            } else {
                //! Threshold value set to a dumb value
                someCommandHasFailed();
            }
        } else {
            //! Specified threshold does not exist, what a fool
            someCommandHasFailed();
        }
    } else {
        //! Could not even parse the arguments, such epic failure!
        someCommandHasFailed();
    }
}

void zmqController::setGainMode(QJsonObject obj)
{
    qDebug() << "[INFO]\tZMQ SET GAIN MODE :"  << obj["command"].toString() << obj["arg1"].toString();

    int val = -1;
    QString mode = obj["arg1"].toString().toLower();
    if (mode == "super high") {
        val = 0;
    } else if (mode == "high") {
        val = 1;
    } else if (mode == "low") {
        val = 2;
    } else if (mode == "super low") {
        val = 3;
    }

    emit setGainMode(val);
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
    QJsonObject root_obj = JsonDocument.object();
    root_obj["UUID"] = currentUUID;
    JsonDocument = QJsonDocument(root_obj);

#ifdef QT_DEBUG
    QString doc = QString(JsonDocument.toJson()).toLocal8Bit().replace("\n","").replace("    ","").replace(": ",":");
    qDebug() << "[INFO]\tZMQ Writing:" <<  doc;

//    if (JsonDocument.object()["component"].toString() == "medipix") {
//        qDebug() << "[INFO]\tZMQ CHECK - JSON successfully encoded: component =" << json_doc.object().value("component").toString();
//    } else {
//        qDebug() << "[INFO]\tZMQ CHECK - JSON encoding FAILED";
//    }
#endif

    //! This is just how you construct the QList of QByteArrays, super weird
    const QList<QByteArray> outList = QList<QByteArray>() << QString(JsonDocument.toJson()).toLocal8Bit().replace("\n","").replace("    ","").replace(": ",":");
    QZmq_PUB_socket->setWriteQueueEnabled(false); //! This probably isn't necessary
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
        root_obj["UUID"] = currentUUID;
        JsonDocument = QJsonDocument(root_obj);
        sendZmqMessage();
#ifdef QT_DEBUG
        qDebug() << "[INFO]\tZMQ Sent FDB";
#endif
    }
}

void zmqController::sock_readyRead()
{
#ifdef QT_DEBUG
    //qDebug() << "[INFO]\tZMQ Reading a message from the ZMQ server";
#endif
    //! This is just the format that you need to use with QZmq
    const QList<QByteArray> msg = QZmq_SUB_socket->read();
    if (msg.isEmpty()){
        qDebug() << "[ERROR]\tZMQ Received empty message\n";
        return;
    }

    QJsonDocument tmp = QJsonDocument::fromJson(msg.first());
    if (tmp.object()["component"].toString() != "medipix") {
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ RECEIVED message 1 /" << msg.length() << "\n\t --> " << msg.first() << "\n";
#endif

    if (tmp.object()["reply type"].toString() != "") {
        return;
    }

    //! Extract current UUID before QJson screws this up...
    QTextCodec *codec = QTextCodec::codecForName("KOI8-R");
    QString str = codec->toUnicode( msg.first());
    QRegExp rx("(\\\"UUID\\\":)(\\d+)");

    int pos = 0;
    while ((pos = rx.indexIn(str, pos)) != -1) {
        if ( rx.cap(1) != "\"UUID\":") {
            qDebug() << "[ERROR]\tZMQ FAIL \t Could not find the UUID based on regex: (\\\"UUID\\\":)(\\d+) \t Found content =" << rx.cap(1) << "\n";
            return;
        }
#ifdef QT_DEBUG
        else {
            qDebug() << "[INFO]\tZMQ SUCCESS \t Got the UUID :" << rx.cap(1) << "," << rx.cap(2);
        }
#endif
        currentUUID = rx.cap(2);
        pos += rx.matchedLength();
    }

    tmp.object()["UUID"] = currentUUID;
    eventQueue->push_back( tmp );

    if (!timer->isActive() && !eventProcessTimer->isActive()) {
        timer->start(10); //! milliseconds
        eventProcessTimer->start(1000); //! milliseconds
    }

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
    root_obj["UUID"] = currentUUID;
    JsonDocument = QJsonDocument(root_obj);
    sendZmqMessage();
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ Sent ACK";
#endif
    processingEvents = false;
}

void zmqController::someCommandHasFailed()
{
    QJsonObject root_obj = JsonDocument.object();
    root_obj["reply type"] = QString("ERR");
    root_obj["UUID"] = currentUUID;
    JsonDocument = QJsonDocument(root_obj);
    sendZmqMessage();
    qDebug() << "[INFO]\tZMQ Sent ERR";
    processingEvents = false;
}
