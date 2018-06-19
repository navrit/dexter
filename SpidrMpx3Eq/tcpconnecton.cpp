#include "tcpconnecton.h"
#include <iostream>
#include <QThread>

TcpConnecton::TcpConnecton(QObject *parent) : QObject(parent)
{
    qDebug() << this << "Created";
    cmdHandler = new CommandHandler;
    connect(this,SIGNAL(cmdRecieved(char*)),cmdHandler,SLOT(on_cmdRecieved(char*)));
    //just for now; it could be changed
    connect(cmdHandler,SIGNAL(commandIsDecoded(QString,QByteArray,bool)),this,SLOT(on_dataIsDecoded(QString,QByteArray,bool)));
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
    connect(m_socket,&QTcpSocket::bytesWritten, this, &TcpConnecton::bytesWritten);
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
    qDebug() << this << " readyRead " << getSocket();
    //recieve the commands
    char rcv_data[4096];
    memset(rcv_data,0,sizeof(rcv_data));
    int len = getSocket()->read(rcv_data,sizeof(rcv_data));
    qDebug() << " Data Length is : " <<len;
    qDebug() << " Data is : " << rcv_data;
    cmdHandler->setCmd(rcv_data);
    cmdHandler->fetchCmd();
   // qDebug()<<"decoded data is: " << cmdHandler->getData();

//    QTcpSocket *socket = getSocket();
//    if(!socket) return;
//    socket->close();
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

void TcpConnecton::on_dataIsDecoded(QString data,QByteArray im,bool isImage)
{
    sendData(QString(data+"\n"));
    if(isImage)
        sendData(im);
}

void TcpConnecton::sendData(QString data)
{
//        QByteArray ba = data.toLatin1();
//        int sizeToWrite = data.length();
//        int byteIndex = 0;
//        const int sizeToSend = 1024;
//        while(sizeToWrite > 0){
//            QString sendStr ="";
//            for(int i = 0; i<sizeToSend;i++){
//                //if(byteIndex + i >= sizeToWrite)
//                   // break;
//                QString castStr(ba.at(byteIndex + i));
//                sendStr += castStr;
//            }
//            sizeToWrite -= sizeToSend;
//            byteIndex += sizeToSend;
//            QByteArray ba2 = sendStr.toLatin1();
//            char *snd_data = ba2.data();
//            qDebug()<<"size:"<<ba2.size();
//            m_socket->write(snd_data,sizeToSend);
//            m_socket->flush();
//            m_socket->waitForBytesWritten();

//        }







    QByteArray ba = data.toLatin1();
    qDebug()<<"size:"<<ba.size();
    int sndSize = m_socket->write(ba);
    m_socket->flush();
    m_socket->waitForBytesWritten();
    qDebug()<<"size:"<<sndSize;
}

void TcpConnecton::sendData(QByteArray image)
{
    const int chunk = 8 * 1024;
    const int imageSize = image.length();
    int remainSize = imageSize;
    int idx = 0;
    int sum = 0;
    while(remainSize >= 0)
    {
        QByteArray imageToSend = image.mid(idx,chunk);
        int sndSize = m_socket->write(imageToSend);
        m_socket->flush();
        m_socket->waitForBytesWritten();
       // QThread::msleep(500);
        qDebug()<<"size:"<<sndSize;
        idx += chunk;
        sum += sndSize;
        remainSize -= chunk;
    }
    qDebug()<<"Total Size = "<< imageSize;
    qDebug()<<"Total Sent Size = "<< sum;
   // QByteArray ba = image.
//    int sndSize = m_socket->write(image);
//    m_socket->flush();
//    m_socket->waitForBytesWritten();
//    qDebug()<<"size:"<<sndSize;
}

