#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class webSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit webSocketServer(quint16 port, QObject *parent = Q_NULLPTR);
    virtual ~webSocketServer();

private:
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    QString name = "WebSocket Server";

signals:

public slots:

private Q_SLOTS:
    void onNewConnection();
    void processMessage(QString message);
    void socketDisconnected();
};

#endif // WEBSOCKETSERVER_H
