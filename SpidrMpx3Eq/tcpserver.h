#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QDebug>
#include <QTcpServer>
#include <QThread>
#include "tcpconnections.h"
#include "tcpconnection.h"

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);
    ~TcpServer();

    virtual bool listen(const QHostAddress &address, quint16 port);
    virtual void close();
    virtual qint64 port();

protected:
    QThread *m_thread;
    TcpConnections *m_connections;
    virtual void incomingConnection(qintptr descriptor); //qint64, qHandle, qintptr, uint
    virtual void accept(qintptr descriptor, TcpConnecton *connection);

signals:
    void accepting(qintptr handle, TcpConnecton *connection);
    void finished();
    //inform outside world about the received data at socket
    void dataReceived(QString);
    //pass the response to tcpconnections
    void responseIsReady(QString);
    //pass the image to tcpconnections
    void imageIsReady(QByteArray,std::pair<const char*,int>);

public slots:
    void complete();
    //receive data from tcp connections
    void on_dataReceived(QString);
    //get response from outside world
    void on_responseIsReady(QString);
    //get image from outside world
    void on_imageIsReady(QByteArray,std::pair<const char*,int>);
};

#endif // TCPSERVER_H
