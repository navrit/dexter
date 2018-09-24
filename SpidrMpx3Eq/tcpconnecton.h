#ifndef TCPCONNECTON_H
#define TCPCONNECTON_H

#include <QObject>
#include <QDebug>
#include <QTcpSocket>
#include <QRegularExpression>
#include <QMutex>

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
    QMutex mutex;

signals:
    void dataRecieved(QString);

public slots:
    virtual void connected();
    virtual void disconnected();
    virtual void readyRead();
    virtual void bytesWritten(qint64 bytes);
    virtual void stateChanged(QAbstractSocket::SocketState socketState);
    virtual void error(QAbstractSocket::SocketError socketError);
    //for sending the response to the client(get response from tcpconnections)
    void on_responseIsReady(QString);
    //for sending the data(image) to the client(get data from tcpconnections)
    void on_imageIsReady(QByteArray, std::pair<const char*,int>);
};

#endif // TCPCONNECTON_H
