#include "websocketserver.h"
#include "QtWebSockets/QWebSocketServer"
#include "QtWebSockets/QWebSocket"
#include <QtCore/QDebug>

QT_USE_NAMESPACE

webSocketServer::webSocketServer(quint16 port, QObject *parent) : QObject(parent),
    m_pWebSocketServer(Q_NULLPTR)
{
    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("WebSocket Server"),
                                              QWebSocketServer::NonSecureMode,
                                              this);
    if (m_pWebSocketServer->listen(QHostAddress::Any, port))
    {
        qDebug() << "WebSocket Server listening on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &webSocketServer::onNewConnection);
    }
}

webSocketServer::~webSocketServer()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void webSocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &webSocketServer::processMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &webSocketServer::socketDisconnected);

    m_clients << pSocket;
    qDebug() << pSocket->peerAddress() << pSocket->peerPort();
}

void webSocketServer::processMessage(QString message)
{
    QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());
    for (QWebSocket *pClient : qAsConst(m_clients)) {
        if (pClient != pSender) //don't echo message back to sender
        {
            pClient->sendTextMessage("[WebSocket]" + message);
            qDebug() << "[WebSocket]" << message;
        } else {
            qDebug() << "[WebSocket] Sender sent" << message;
        }
    }
}

void webSocketServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    qDebug() << "[WebSocket] A client disconnected:" << pClient->peerAddress() << pClient->peerPort();
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}
