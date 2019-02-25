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

    disconnect(m_connections[socket],SIGNAL(dataReceived(QString)),this,SLOT(on_dataReceived(QString)));
    disconnect(this,SIGNAL(responseIsReady(QString)),m_connections[socket],SLOT(on_responseIsReady(QString)));
    //disconnect(this,SIGNAL(imageIsReady(QByteArray,QByteArray)),m_connections[socket],SLOT(on_imageIsReady(QByteArray,QByteArray)));

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
    connect(connection,SIGNAL(dataReceived(QString)),this,SLOT(on_dataReceived(QString)));
    connect(this,SIGNAL(responseIsReady(QString)),connection,SLOT(on_responseIsReady(QString)));
    //connect(this,SIGNAL(imageIsReady(QByteArray,QByteArray)),connection,SLOT(on_imageIsReady(QByteArray,QByteArray)),Qt::DirectConnection);
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

void TcpConnections::on_dataReceived(QString data)
{
    //qDebug() << "Received at tcpconnections : " << data;
    emit dataReceived(data);
}

void TcpConnections::on_responseIsReady(QString response)
{
    //qDebug() << "Response received at the tcpconnections : " << response;
    emit responseIsReady(response);
}

void TcpConnections::on_imageIsReady(QByteArray header,std::pair<const char*,int> image)
{
     //qDebug() << "Image received at the tcpconnections, size: " << image.size();
     //emit imageIsReady(header,image);
    foreach(TcpConnecton *conn, m_connections.values()) {
        conn->on_imageIsReady(header, image);
    }
}
