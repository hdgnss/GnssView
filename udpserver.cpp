#include "udpserver.h"
#include <QDebug>

UdpServer::UdpServer(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
{
    connect(m_socket, &QUdpSocket::readyRead, 
            this, &UdpServer::readPendingDatagrams);
}

bool UdpServer::startServer(quint16 port)
{
    // Stop the server if it's already running
    if (m_socket->state() == QAbstractSocket::BoundState) {
        stopServer();
    }
    
    // Bind to the specified port
    bool success = m_socket->bind(QHostAddress::Any, port);
    
    if (success) {
        qInfo() << "UDP server listening on port" << port;
    } else {
        qWarning() << "Failed to bind UDP socket to port" << port
                   << ":" << m_socket->errorString();
    }
    
    return success;
}

void UdpServer::stopServer()
{
    if (m_socket->state() == QAbstractSocket::BoundState) {
        m_socket->close();
        qInfo() << "UDP server stopped";
    }
}

bool UdpServer::isListening() const
{
    return m_socket->state() == QAbstractSocket::BoundState;
}

void UdpServer::readPendingDatagrams()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress sender = datagram.senderAddress();
        quint16 senderPort = datagram.senderPort();
        emit newDataReceived(data, sender, senderPort);
    }
}