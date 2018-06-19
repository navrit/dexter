#ifndef TCPCONNECTON_H
#define TCPCONNECTON_H

#include <QObject>
#include <QDebug>
#include <QTcpSocket>
#include "commandhandler.h"

class TcpConnecton : public QObject
{
    Q_OBJECT
public:
    explicit TcpConnecton(QObject *parent = 0);
    ~TcpConnecton();

    virtual void setSocket(QTcpSocket *socket);

protected:
    QTcpSocket *m_socket;
    QTcpSocket *getSocket();
    //handle the incomming command
    CommandHandler *cmdHandler;

signals:
    void cmdRecieved(char*);
public slots:
    virtual void connected();
    virtual void disconnected();
    virtual void readyRead();
    virtual void bytesWritten(qint64 bytes);
    virtual void stateChanged(QAbstractSocket::SocketState socketState);
    virtual void error(QAbstractSocket::SocketError socketError);
    //added slots
    void on_dataIsDecoded(QString, QByteArray, bool);

private:
    void sendData(QString);     //to send regular data
    void sendData(QByteArray);  //to send image

};

#endif // TCPCONNECTON_H
