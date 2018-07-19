#include "tcpconnections.h"

TcpConnections::TcpConnections(QObject *parent) : QObject(parent)
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tTCP connections " << this << "created";
#endif
}

TcpConnections::~TcpConnections()
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tTCP connections" << this << "destroyed";
#endif
}


int TcpConnections::count()
{
    QReadWriteLock lock;
    lock.lockForRead();
    int value = m_connections.count();
    lock.unlock();

    return value;
}

void TcpConnections::removeSocket(QTcpSocket *socket)
{
    if(!socket) return;
    if(!m_connections.contains(socket)) return;

    qDebug() << "[INFO]\tTCP connections " << this << "removing socket = " <<  socket;

    if(socket->isOpen()) {
        qDebug() << "[INFO]\tTCP connections " << this << "socket is open, attempting to close it " << socket;
        socket->disconnect();
        socket->close();
    }

    disconnect(m_connections[socket],SIGNAL(dataRecieved(QString)),this,SLOT(on_dataRecieved(QString)));
    disconnect(this,SIGNAL(responseIsReady(QString)),m_connections[socket],SLOT(on_responseIsReady(QString)));

    qDebug() << this << "deleting socket" << socket;
    m_connections.remove(socket);
    socket->deleteLater();

    qDebug() << "[INFO]\tTCP connections " << this << "client count = " << m_connections.count();

}

void TcpConnections::disconnected()
{
    if(!sender()) return;
    qDebug() << "[INFO]\tTCP connections " << this << "disconnecting socket"<< sender();

    QTcpSocket *socket = static_cast<QTcpSocket*>(sender());
    if(!socket) return;

    removeSocket(socket);
}

void TcpConnections::error(QAbstractSocket::SocketError socketError)
{
    if(!sender()) return;
    qDebug() << "[INFO]\tTCP connections " << this << "error in socket" << sender() << " error = " << socketError;

    QTcpSocket *socket = static_cast<QTcpSocket*>(sender());
    if(!socket) return;

    removeSocket(socket);
}

void TcpConnections::start()
{
#ifdef QT_DEBUG
    qDebug() << "[INFO]\tTCP connections " << this << "connections started on" << QThread::currentThread();
#endif
}

void TcpConnections::quit()
{
    if(!sender()) return;
    qDebug() << "[INFO]\tTCP connections " << this << "connections quitting";

    foreach(QTcpSocket *socket, m_connections.keys()) {
        qDebug() << "[INFO]\tTCP connections " << this << "closing socket" << socket;
        removeSocket(socket);
    }

    qDebug() << "[INFO]\tTCP connections " << this << "finishing";
    emit finished();
}

void TcpConnections::accept(qintptr handle, TcpConnecton *connection)
{
    //qDebug() << "*** HEY WATCH THIS";
    QTcpSocket *socket = new QTcpSocket(this);

    if(!socket->setSocketDescriptor(handle)) {
        qWarning() << "[INFO]\tTCP connections " << this << "could not accept connection" << handle;
        connection->deleteLater();
        return;
    }

    // connect(socket,&QTcpSocket::disconnected,this,&TcpConnections::disconnected);
    // connect(socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&TcpConnections::error);

    connect(socket,&QTcpSocket::disconnected, this, &TcpConnections::disconnected);
    connect(socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&TcpConnections::error);

    connection->moveToThread(QThread::currentThread());
    connection->setSocket(socket);
    //connect to data_recived
    connect(connection,SIGNAL(dataRecieved(QString)),this,SLOT(on_dataRecieved(QString)));
    connect(this,SIGNAL(responseIsReady(QString)),connection,SLOT(on_responseIsReady(QString)));
    //always accept one connection
    if(this->count() <= 0)
        m_connections.insert(socket,connection);
    else
    {
        QList<QTcpSocket*> keys = m_connections.keys();
        for (int i = 0; i < keys.length(); ++i) {
            this->removeSocket(keys[i]);
        }
        m_connections.insert(socket,connection);
    }

    m_connections.insert(socket,connection);
    qDebug() << "[INFO]\tTCP connections " << this << "clients = " << m_connections.count();
    emit socket->connected();

}

void TcpConnections::on_dataRecieved(QString data)
{
    qDebug() << "Recieved at tcpconnections : " << data;
    emit dataRecieved(data);
}

void TcpConnections::on_responseIsReady(QString response)
{
    qDebug() << "Response recieved at the tcpconnections : " << response;
    emit responseIsReady(response);
}

