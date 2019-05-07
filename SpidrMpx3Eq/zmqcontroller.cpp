#include "zmqcontroller.h"
#include "QDebug"
#include "QMessageBox"
#include "mpx3config.h"
#include "qcstmglvisualization.h"
#include <QFileInfo>

#include "qjsonobject.h"
#include "qjsondocument.h"

#include <QRegExp>

zmqController::zmqController(Mpx3GUI * p, QObject *parent) : QObject(parent)
{
    SetMpx3GUI(p);
    _config = _mpx3gui->getConfig();
    _timer = new QTimer(this);
    _eventProcessTimer = new QTimer(this);

    _QZmq_context = new QZmq::Context();
    _QZmq_PUB_socket = new QZmq::Socket(QZmq::Socket::Type::Pub, _QZmq_context, this);
    _QZmq_SUB_socket = new QZmq::Socket(QZmq::Socket::Type::Sub, _QZmq_context, this);

    _eventQueue = new QQueue<QJsonDocument>();


    initialiseJsonResponse();

    // ---------------------- Internal class signals ---------------------
    connect(_config, SIGNAL(IpZmqPubAddressChanged(QString)), this, SLOT(addressChanged_PUB(QString)));
    connect(_config, SIGNAL(IpZmqSubAddressChanged(QString)), this, SLOT(addressChanged_SUB(QString)));

    connect(_QZmq_PUB_socket, SIGNAL(messagesWritten(int)), SLOT(sock_messagesWritten(int)));
    connect(_QZmq_SUB_socket, SIGNAL(readyRead()), SLOT(sock_readyRead()));

    connect(_timer, SIGNAL(timeout()), this, SLOT(tryToProcessEvents()));
    //! Don't need to connect the failed attempts to change the address here. That's taken care of elsewhere
    connect(_eventProcessTimer, SIGNAL(timeout()), this, SLOT(tryToSendFeedback()));
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
    connect(this, SIGNAL(setReadoutFrequency(int)), _mpx3gui->getVisualization(), SLOT(setReadoutFrequency(int)));
    connect(this, SIGNAL(loadConfiguration(QString)), _mpx3gui->getVisualization(), SLOT(loadConfiguration(QString)));
    // -------------------------------------------------------------------
}

zmqController::~zmqController()
{
    _timer->stop();
    delete _timer;
    _timer = nullptr;

    _eventProcessTimer->stop();
    delete _eventProcessTimer;
    _eventProcessTimer = nullptr;

    delete _eventQueue;
    delete _QZmq_PUB_socket;
    _QZmq_SUB_socket->unsubscribe(""); //! This may not be necessary
    _QZmq_SUB_socket->setTcpKeepAliveEnabled(false); //! This may not be necessary
    delete _QZmq_SUB_socket;
    delete _QZmq_context;
    _QZmq_context = nullptr;
    _QZmq_PUB_socket = nullptr;
    _QZmq_SUB_socket = nullptr;
    _eventQueue = nullptr;
}

void zmqController::addressChanged_PUB(QString addr)
{
    if ( addr == _PUB_addr ) {
        return;
    } else {
        _PUB_addr = addr;
#ifdef QT_DEBUG
        qDebug() << "[INFO]\tZMQ PUB address set:" << addr;
#endif
    }

    _QZmq_PUB_socket->connectToAddress(_PUB_addr);
    qDebug() << "[INFO]\tZMQ Connected to PUB socket:" << _PUB_addr;

    initialiseJsonResponse();
}

void zmqController::addressChanged_SUB(QString addr)
{
    if ( addr == _SUB_addr ) {
        return;
    } else {
        _SUB_addr = addr;
#ifdef QT_DEBUG
        qDebug() << "[INFO]\tZMQ SUB address set:" << addr;
#endif
    }

    if (_QZmq_SUB_socket != nullptr) {
        _QZmq_SUB_socket->unsubscribe("");
    }

    _QZmq_SUB_socket->connectToAddress(_SUB_addr);
    _QZmq_SUB_socket->subscribe("");
    qDebug() << "[INFO]\tZMQ Connected to SUB socket:" << _SUB_addr;

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
    _JsonDocument = QJsonDocument(root_obj);
}

void zmqController::processEvents()
{
    //!                     Overview
    //!
    //! Received a SEND event --> if reply type == ""
    //! Send REPLY event(s) in response to thats specific SEND event.
    //!    Do not process other events until the first is complete

    _JsonDocument = _eventQueue->dequeue(); //! This is checked to have at least one event otherwise this event will not be called
    QJsonObject root_obj = _JsonDocument.object();

    //! 1. Received a SEND event from server
    if (root_obj["reply type"].toString() == "") {

        _processingEvents = true; //! Used as a controller level lock

        //! 2. REPLY with RCV immediately
        root_obj["reply type"] = "RCV";
        _JsonDocument.setObject( root_obj );
        sendZmqMessage();

        //! If not connected to a SPIDR at this point, you should really connect
        //! However, not all commands need this obviously
        //! Blocking is acceptable because the other commands probably cannot run without being connected
        if (!_mpx3gui->getConfig()->isConnected()) {
            /*if ( _mpx3gui->establish_connection() ) {
                qDebug() << "[INFO]\tZMQ Connected to SPIDR via remote command";
            } else {
                qDebug() << "[ERROR]\tZMQ Failed to connect to a SPIDR via a remote command";
            }*/
            // qDebug() << "[ERROR]\tZMQ Not connected to the SPIDR. Subsequent commands may or may not complete successfully.";
            _isConnectedToSPIDR = false;
        } else {
            _isConnectedToSPIDR = true;
        }

        //! 3. If it's taking a while, REPLY with FDB events at least at 1Hz independently
        //!    Done eleswhere via a QTimer --> eventProcessTimer
        //!    It just checks to see if this is still processing events

        //! 4. Emit relevant signals to trigger whatever we want elsewhere
        //!    This must not be blocking
        if ( JsonContains(root_obj, "command", "take image")) {
            takeImage(root_obj);

        } else if ( JsonContains(root_obj, "command", "take and save image sequence")) {
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
            setCsm(root_obj);

        } else if ( JsonContains(root_obj, "command", "load default equalisation")) {
            loadDefaultEqualisation(root_obj);

        } else if ( JsonContains(root_obj, "command", "load equalisation from folder")) {
            loadEqualisationFromFolder(root_obj);

        } else if ( JsonContains(root_obj, "command", "set readout mode") ) {
            setReadoutMode(root_obj);

        } else if ( JsonContains(root_obj, "command", "set readout frequency") ) {
            setReadoutFrequency(root_obj);

        } else if ( JsonContains(root_obj, "command", "load configuration") ) {
            loadConfiguration(root_obj);

        } else {
            qDebug() << "[ERROR]\tZMQ Failed to parse command or something else... : " << root_obj["UUID"].toString() << root_obj["command"].toString() << "\targ1: " << root_obj["arg1"].toString();

            someCommandHasFailed();
            _processingEvents = false;
        }
    }
    //! 5. when it's processed, REPLY with a ACK event --> see the SLOT dataTakingFinishedAndSaved()
}

void zmqController::takeImage(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ TAKE IMAGE : "  << obj["command"].toString();
#endif
    if (_isConnectedToSPIDR) {
        emit takeImage();
    } else {
        qDebug() << "[ERROR]\tZMQ Is not connected to a SPIDR, could not execute : " << obj["UUID"].toString() << obj["command"].toString() << "\targ1: " << obj["arg1"].toString();
        emit someCommandHasFailed();
    }

}

void zmqController::takeAndSaveImageSequence(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ TAKE AND SAVE IMAGE SEQUENCE : "  << obj["command"].toString();
#endif
    if (_isConnectedToSPIDR) {
        emit takeAndSaveImageSequence();
    } else {
        qDebug() << "[ERROR]\tZMQ Is not connected to a SPIDR, could not execute : " << obj["UUID"].toString() << obj["command"].toString() << "\targ1: " << obj["arg1"].toString();
        emit someCommandHasFailed();
    }

}

void zmqController::saveImage(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SAVE IMAGE : "  << obj["command"].toString() << obj["arg1"].toString();
#endif
    if (_isConnectedToSPIDR) {
        emit saveImageSignal(obj["arg1"].toString());
    } else {
        qDebug() << "[ERROR]\tZMQ Is not connected to a SPIDR, could not execute : " << obj["UUID"].toString() << obj["command"].toString() << "\targ1: " << obj["arg1"].toString();
        emit someCommandHasFailed( QString("DEXTER --> ACQUILA : Is not connected to a SPIDR, could not execute command : " + obj["UUID"].toString() + "  " +  obj["command"].toString()));
    }

}

void zmqController::setExposure(QJsonObject obj)
{
    Q_UNUSED(obj)
#ifdef QT_DEBUG

    qDebug() << "[INFO]\tZMQ SET EXPOSURE : "  << obj["command"].toString() << obj["arg1"].toString().toInt();
#endif

    bool ok;
    int arg1 = obj["arg1"].toString().toInt(&ok);

    //! 1 microsecond is the shortest exposure time that's reasonable
    if (ok && arg1 >= 1) {
        obj["reply"] = QString::number( arg1 );
        _JsonDocument = QJsonDocument(obj);
        emit setExposure(arg1);
    } else {
        someCommandHasFailed("DEXTER --> ACQUILA : Invalid exposure time requested" + QString::number(arg1));
    }
}

void zmqController::setNumberOfFrames(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SET NUMBER OF FRAMES : "  << obj["command"].toString() << obj["arg1"].toString();
#endif

    bool ok = false;
    int arg1 = obj["arg1"].toString().toInt(&ok);
    if (ok && arg1 >= 0) {
        emit setNumberOfFrames(arg1);
    } else {
        someCommandHasFailed( QString("DEXTER --> ACQUILA : Invalid number of frames requested : " + QString::number(arg1)));
    }
}

void zmqController::setThreshold(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SET THRESHOLD : "  << obj["command"].toString() << obj["arg1"].toString() << obj["arg2"].toString();
#endif

    //! Arg1: Which threshold to change
    //! Arg2: Threshold value (DAC units)

    bool ok = false;
    int arg1 = obj["arg1"].toString().toInt(&ok);

    bool ok2 = false;
    int arg2 = obj["arg2"].toString().toInt(&ok2);

    if (!ok || !ok2) {
        qDebug() << "[ERROR]\tZMQ Threshold arguments could not be parsed arg1:" << arg1 << " ok:" << ok << " arg2:" << arg2 << " ok2:" << ok2;
    }

    //! May want to handle the failures better

    if (arg1 >= 0 && arg1 < __max_colors) {
        if (arg2 >= 0 && arg2 < __max_DAC_range) {
            //! SUCCESS, this is actually valid input
            emit setThreshold(arg1, arg2);
        } else {
            //! Threshold value set to a dumb value
            qDebug() << "[ERROR]\tZMQ Threshold value set to an invalid value : " << obj["command"].toString() << obj["arg1"].toString() << obj["arg2"].toString();
            someCommandHasFailed("DEXTER --> ACQUILA : Threshold value set to an invalid value");
        }
    } else {
        //! Specified threshold does not exist, what a fool
        qDebug() << "[ERROR]\tZMQ Specified threshold does not exist : " << obj["command"].toString() << obj["arg1"].toString() << obj["arg2"].toString();
        someCommandHasFailed("DEXTER --> ACQUILA : Specified threshold does not exist");
    }
}

void zmqController::setGainMode(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SET GAIN MODE :"  << obj["command"].toString() << obj["arg1"].toString();
#endif

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
    } else {
        qDebug() << "[ERROR]\tZMQ Could not parse gain mode, should be 'super high', 'high', 'low' or 'super low'";
        emit someCommandHasFailed("DEXTER --> ACQUILA : Could not parse gain mode, should be 'super high', 'high', 'low' or 'super low'");
        return;
    }

    emit setGainMode(val);
}

void zmqController::setCsm(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SET CSM :"  << obj["command"].toString() << obj["arg1"].toString();
#endif

    QString arg1 = obj["arg1"].toString().toLower();
    if (arg1.contains("true") || arg1.contains("on")) {
        emit setCSM(true);
    } else if (arg1.contains("false") || arg1.contains("off")) {
        emit setCSM(false);
    } else {
        emit someCommandHasFailed(QString("DEXTER --> ACQUILA ZMQ : Could not set CSM"));
    }
}

void zmqController::loadDefaultEqualisation(QJsonObject obj)
{
    Q_UNUSED(obj)
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ LOAD DEFAULT EQUALISATION :"  << obj["command"].toString();
#endif
    if (_isConnectedToSPIDR) {
        emit loadDefaultEqualisation();
    } else {
        emit someCommandHasFailed(QString("DEXTER --> ACQUILA ZMQ : Could not load default equalisation, connect to the SPIDR!"));
    }

}

void zmqController::loadEqualisationFromFolder(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ LOAD EQUALISATION FROM FOLDER:"  << obj["command"].toString() << obj["arg1"].toString();
#endif

    if (_isConnectedToSPIDR) {
        QString arg1 = obj["arg1"].toString(); //! You actually need to keep the case here, Linux filesystems are mostly case sensitive of course!

        if ( folderExists(arg1) && fileExists( QDir::cleanPath( QString(arg1 + QDir::separator() + "adj_0"))) && fileExists( QDir::cleanPath( QString(arg1 + QDir::separator() + "mask_0"))) ) {
            //! Probably fine to proceed to loading the equalisation
            emit loadEqualisation(arg1);
        } else {
            qDebug() << "[ERROR]\tZMQ failed to load non-default equalisation from :" << arg1;
            emit someCommandHasFailed(QString("DEXTER --> ACQUILA ZMQ : failed to load non-default equalisation from") + arg1);
        }
    } else {
        emit someCommandHasFailed(QString("DEXTER --> ACQUILA ZMQ : Could not load equalisation from folder, connect to the SPIDR!"));
    }

}

void zmqController::setReadoutMode(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SET READOUT MODE :"  << obj["command"].toString();
#endif
    QString arg1 = obj["arg1"].toString();

    //! Error handling etc done in qcstmglvisualisation
    emit setReadoutMode(arg1);
}

void zmqController::setReadoutFrequency(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ SET READOUT FREQUENCY :"  << obj["command"].toString();
#endif

    bool ok = false;
    int arg1 = obj["arg1"].toString().toInt(&ok);

    if (ok && arg1 >= 0) {
        int readout_mode = _mpx3gui->getConfig()->getPixelDepth();

        switch (readout_mode) {
        case 1:
            if (arg1 < __maximumFPS_1_bit) {
                emit setReadoutFrequency(arg1);
            }
            break;
        case 6:
            if (arg1 < __maximumFPS_6_bit) {
                emit setReadoutFrequency(arg1);
            }
            break;
        case 12:
            if (arg1 < __maximumFPS_12_bit) {
                emit setReadoutFrequency(arg1);
            }
            break;
        case 24:
            someCommandHasFailed( QString("DEXTER --> ACQUILA : Invalid readout frequency requested : " + QString::number(arg1)));
            break;
        default:
            someCommandHasFailed( QString("DEXTER --> ACQUILA : Invalid readout frequency requested : " + QString::number(arg1)));
            break;
        }

    } else {
        someCommandHasFailed( QString("DEXTER --> ACQUILA : Invalid readout frequency requested : " + QString::number(arg1)));
    }
}

void zmqController::loadConfiguration(QJsonObject obj)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tZMQ LOAD CONFIGURATION FILE :"  << obj["command"].toString();
#endif

    QString arg1 = obj["arg1"].toString(); //! You actually need to keep the case here, Linux filesystems are mostly case sensitive of course!

    if ( fileExists( QDir::cleanPath( QString(arg1))) )  {
        //! Probably fine to proceed
        emit loadConfiguration(arg1);
    } else {
        qDebug() << "[ERROR]\tZMQ failed to load configuration from :" << arg1;
        emit someCommandHasFailed(QString("DEXTER --> ACQUILA ZMQ : failed to load configuration from : ") + arg1);
    }
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
    QJsonObject root_obj = _JsonDocument.object();
    root_obj["UUID"] = _currentUUID;
    _JsonDocument = QJsonDocument(root_obj);

#ifdef QT_DEBUG
//    QString doc = QString(JsonDocument.toJson()).toLocal8Bit().replace("\n","").replace("    ","").replace(": ",":");
//    qDebug() << "[INFO]\tZMQ Writing:" <<  doc;

//    if (JsonDocument.object()["component"].toString() == "medipix") {
//        qDebug() << "[INFO]\tZMQ CHECK - JSON successfully encoded: component =" << json_doc.object().value("component").toString();
//    } else {
//        qDebug() << "[INFO]\tZMQ CHECK - JSON encoding FAILED";
//    }
#endif

    QRegExp rx("(\\\"UUID\\\":)\"(\\d+)\"");
    QString response = QString(_JsonDocument.toJson()).replace("\n","").replace("    ","").replace(": ",":").replace(rx,"\\1\\2");

    //! This is just how you construct the QList of QByteArrays, super weird
    const QList<QByteArray> outList = QList<QByteArray>() << response.toLocal8Bit();

    _QZmq_PUB_socket->setWriteQueueEnabled(false); //! This probably isn't necessary
    _QZmq_PUB_socket->write(outList);
}

void zmqController::tryToProcessEvents()
{
    //! Only try to process events if not already processing an event and if there are events to process
    if (!_processingEvents && !_eventQueue->isEmpty()) {
        processEvents();
    }
}

void zmqController::tryToSendFeedback()
{
    //! Doesn't matter if the eventQueue is empty
    if(_processingEvents) {
        QJsonObject root_obj = _JsonDocument.object();
        root_obj["reply type"] = QString("FDB");
        root_obj["UUID"] = _currentUUID;
        _JsonDocument = QJsonDocument(root_obj);
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
    const QList<QByteArray> msg = _QZmq_SUB_socket->read();
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
        _currentUUID = rx.cap(2);
        pos += rx.matchedLength();
    }

    tmp.object()["UUID"] = _currentUUID;
    _eventQueue->push_back( tmp );

    if (!_timer->isActive() && !_eventProcessTimer->isActive()) {
        _timer->start(10); //! milliseconds
        _eventProcessTimer->start(1000); //! milliseconds
    }

}

void zmqController::sock_messagesWritten(int count)
{
    Q_UNUSED(count)
#ifdef QT_DEBUG
//    qDebug() << "[INFO]\tZMQ Messages written:" << count << "\n";
#endif
}

void zmqController::someCommandHasFinished_Successfully()
{
    QJsonObject root_obj = _JsonDocument.object();
    root_obj["reply type"] = QString("ACK");
    root_obj["UUID"] = _currentUUID;
    _JsonDocument = QJsonDocument(root_obj);
    sendZmqMessage();
#ifdef QT_DEBUG
//    qDebug() << "[INFO]\tZMQ Sent ACK";
#endif
    _processingEvents = false;
}

void zmqController::someCommandHasFailed(QString reply)
{
    QJsonObject root_obj = _JsonDocument.object();
    root_obj["reply"] = reply;
    root_obj["reply type"] = QString("ERR");
    root_obj["UUID"] = _currentUUID;
    _JsonDocument = QJsonDocument(root_obj);
    sendZmqMessage();
    qDebug() << "[INFO]\tZMQ Sent ERR. UUID =" << _currentUUID << " Reply =" << reply;
    _processingEvents = false;
}
