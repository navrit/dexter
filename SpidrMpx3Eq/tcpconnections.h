#ifndef TCPCONNECTIONS_H
#define TCPCONNECTIONS_H

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QTcpSocket>
#include <QMap>
#include <QReadWriteLock>
#include "tcpconnection.h"
#include "canvas.h"


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
    // Pass received data to tcp server
    void dataReceived(QString);
    // Pass the response to tcpconnection
    void responseIsReady(QString);
    // Pass the image to tcpconnection
    void imageIsReady(QByteArray, Canvas);

protected slots:
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);

public slots:

    void start();
    void quit();
    void accept(qintptr handle, TcpConnecton *connection);
    //recieve data from tcp connection
    void on_dataReceived(QString);
    //get response from tcpserver
    void on_responseIsReady(QString);
    //get image from tcpserver
    void on_imageIsReady(QByteArray, Canvas);
};

#endif // TCPCONNECTIONS_H
