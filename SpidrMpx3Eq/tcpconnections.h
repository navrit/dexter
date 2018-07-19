#ifndef TCPCONNECTIONS_H
#define TCPCONNECTIONS_H

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QTcpSocket>
#include <QMap>
#include <QReadWriteLock>
#include "tcpconnecton.h"


class TcpConnections : public QObject
{
    Q_OBJECT
public:
    explicit TcpConnections(QObject *parent = nullptr);
    ~TcpConnections();

    virtual int count();

protected:
    QMap<QTcpSocket*, TcpConnecton*> m_connections;
    void removeSocket(QTcpSocket *socket);

signals:
    void quitting();
    void finished();
    //pass recieved data to tcp server
    void dataRecieved(QString);
    //pass the response to tcpconnection
    void responseIsReady(QString);

protected slots:
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);

public slots:

    void start();
    void quit();
    void accept(qintptr handle, TcpConnecton *connection);
    //recieve data from tcp connection
    void on_dataRecieved(QString);
    //get response from tcpserver
    void on_responseIsReady(QString);
};

#endif // TCPCONNECTIONS_H
