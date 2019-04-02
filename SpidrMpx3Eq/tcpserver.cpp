#include "tcpserver.h"

TcpServer::TcpServer(QObject *parent) : QTcpServer(parent)
{
    //qDebug() << "[INFO]\t" << this << "created"; // on" << QThread::currentThread();
}

TcpServer::~TcpServer()
{
    qDebug() << "[INFO]\t" << this << "destroyed";
}

bool TcpServer::listen(const QHostAddress &address, quint16 port)
{
    if(!QTcpServer::listen(address,port)) return false;
    //if(!QTcpServer::listen(address,port)) return false;

    m_thread = new QThread(this);
    m_connections = new TcpConnections();

    connect(m_connections,SIGNAL(dataReceived(QString)),this,SLOT(on_dataReceived(QString)));
    connect(this,SIGNAL(responseIsReady(QString)),m_connections,SLOT(on_responseIsReady(QString)));
    //connect(this,SIGNAL(imageIsReady(QByteArray,QByteArray)),m_connections,SLOT(on_imageIsReady(QByteArray,QByteArray)));

    connect(m_thread,&QThread::started,m_connections,&TcpConnections::start, Qt::QueuedConnection);
    connect(this, &TcpServer::accepting,m_connections,&TcpConnections::accept, Qt::QueuedConnection);
    connect(this,&TcpServer::finished,m_connections,&TcpConnections::quit, Qt::QueuedConnection);
    connect(m_connections,&TcpConnections::finished,this,&TcpServer::complete, Qt::QueuedConnection);

    m_connections->moveToThread(m_thread);
    m_thread->start();

    return true;
}

void TcpServer::close()
{
    qDebug() << "[INFO]\t" << this << "CLOSE";
    emit finished();
    QTcpServer::close();
}

qint64 TcpServer::port()
{
    if (isListening()) {
        return this->serverPort();
    } else {
        return 1000;
    }
}

void TcpServer::incomingConnection(qintptr descriptor)
{
    qDebug() << "[INFO]\t" << this << "attempting to accept connection" << descriptor;
    TcpConnecton *connection = new TcpConnecton();
    accept(descriptor, connection);

}

void TcpServer::accept(qintptr descriptor, TcpConnecton *connection)
{
    qDebug() << "[INFO]\t" << this << "accepting the connection" << descriptor;
    connection->moveToThread(m_thread);
    emit accepting(descriptor, connection);
}

void TcpServer::complete()
{
    if(!m_thread)
    {
        qWarning() << "[INFO]\t" << this << "exiting complete there was no thread!";
        return;
    }

    qDebug() << "[INFO]\t" << this << "Complete called, destroying thread";
    delete m_connections;

    qDebug() << "[INFO]\t" << this << "Quitting thread";
    m_thread->quit();
    m_thread->wait();

    delete m_thread;

    qDebug() << "[INFO]\t" << this << "complete";

}

void TcpServer::on_dataReceived(QString data)
{
    //qDebug() << "Received at tcpserver : " << data;
    emit dataReceived(data);

}

void TcpServer::on_responseIsReady(QString response)
{
    qDebug() << "Response: " << response;
    emit responseIsReady(response);
}

void TcpServer::on_imageIsReady(QByteArray header, Canvas image)
{
    //qDebug() << "Image received at the tcpserver, size: " << image.size();
    //emit imageIsReady(header,image);
    m_connections->on_imageIsReady(header, image);
}
