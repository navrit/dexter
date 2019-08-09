#include "tcpconnection.h"
#include <iostream>
#include <QMutex>

TcpConnecton::TcpConnecton(QObject *parent) : QObject(parent)
{
    qDebug() << this << "Created";
}

TcpConnecton::~TcpConnecton()
{
    qDebug() << this << "Destroyed";
}

void TcpConnecton::setSocket(QTcpSocket *socket)
{
    m_socket = socket;
    connect(m_socket,&QTcpSocket::connected, this, &TcpConnecton::connected);
    connect(m_socket,&QTcpSocket::disconnected, this, &TcpConnecton::disconnected);
    connect(m_socket,&QTcpSocket::readyRead, this, &TcpConnecton::readyRead);
    //connect(m_socket,&QTcpSocket::bytesWritten, this, &TcpConnecton::bytesWritten);
    connect(m_socket,&QTcpSocket::stateChanged, this, &TcpConnecton::stateChanged);
    connect(m_socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&TcpConnecton::error);

}

QTcpSocket *TcpConnecton::getSocket()
{
    if(!sender()) return 0;
    return static_cast<QTcpSocket*>(sender());
}

void TcpConnecton::connected()
{
    if(!sender()) return;
    qDebug() << this << " connected " << sender();
}

void TcpConnecton::disconnected()
{
    if(!sender()) return;
    qDebug() << this << " disconnected " << getSocket();
}

void TcpConnecton::readyRead()
{
    if(!sender()) return;
    //qDebug() << this << " readyRead " << getSocket();
    QTcpSocket *socket = getSocket();
    char rcv_data[4096];
    memset(rcv_data,0,sizeof(rcv_data));
    socket->read(rcv_data,sizeof(rcv_data));
    QString receivedData;
    receivedData.sprintf("%s",rcv_data);
    //for debg purpose
    //qDebug() << "received data from client : " << receivedData;
    emit dataReceived(receivedData.split("\n").at(0).split("\r").at(0));
    if(!socket) return;
    //socket->close();
}

void TcpConnecton::bytesWritten(qint64 bytes)
{
    if(!sender()) return;
    qDebug() << this << " bytesWritten " << getSocket() << " number of bytes = " << bytes;
}

void TcpConnecton::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(!sender()) return;
    qDebug() << this << " stateChanged " << getSocket() << " state = " << socketState;
}

void TcpConnecton::error(QAbstractSocket::SocketError socketError)
{
    if(!sender()) return;
    qDebug() << this << " error " << getSocket() << " error = " << socketError;
}

void TcpConnecton::on_responseIsReady(QString response)
{
    //qDebug() << "Response received at the tcpconnection : " << response;
    QByteArray ba = response.toUtf8();
    //qDebug()<<"size:"<<ba.size();
    mutex.lock();
    int sndSize = m_socket->write(ba);
    m_socket->flush();
    m_socket->waitForBytesWritten();
    mutex.unlock();
    qDebug()<<"size:"<<sndSize;
}

void TcpConnecton::on_imageIsReady(QByteArray header, Canvas image)
{

    //qDebug() << "Data received at the tcpconnection.";
    //qDebug()<<"header size:"<<header.size();
   // qDebug()<<"Image size:"<<image.size();
    mutex.lock();
    int sndSize = m_socket->write(header);
    sndSize = m_socket->write((const char*) image.image.get(), image.size);
    m_socket->flush();
    //m_socket->waitForBytesWritten();
    mutex.unlock();

    Q_UNUSED(sndSize);
}
