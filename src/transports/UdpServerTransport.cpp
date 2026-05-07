#include "UdpServerTransport.h"

namespace hdgnss {

UdpServerTransport::UdpServerTransport(QObject *parent)
    : ITransport(QStringLiteral("UDP"), parent) {
    connect(&m_socket, &QUdpSocket::readyRead, this, [this]() {
        while (m_socket.hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(int(m_socket.pendingDatagramSize()));
            m_socket.readDatagram(datagram.data(), datagram.size(), &m_lastSenderAddress, &m_lastSenderPort);
            setStatus(QStringLiteral("Listening, last peer %1:%2").arg(m_lastSenderAddress.toString()).arg(m_lastSenderPort));
            emit dataReceived(datagram);
        }
    });
    connect(&m_socket, &QUdpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        const QString message = QStringLiteral("UDP error: %1").arg(m_socket.errorString());
        setStatus(message);
        emit errorOccurred(message);
    });
}

bool UdpServerTransport::openWithSettings(const QVariantMap &settings) {
    close();
    const QHostAddress address(settings.value(QStringLiteral("address"), QStringLiteral("0.0.0.0")).toString());
    const quint16 port = static_cast<quint16>(settings.value(QStringLiteral("port"), 5000).toUInt());
    if (!m_socket.bind(address, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        const QString message = QStringLiteral("UDP bind failed: %1").arg(m_socket.errorString());
        setStatus(message);
        emit errorOccurred(message);
        emitOpenChanged();
        return false;
    }
    m_boundAddress = m_socket.localAddress();
    m_boundPort = m_socket.localPort();
    setStatus(QStringLiteral("Listening on %1:%2").arg(address.toString()).arg(port));
    emitOpenChanged();
    return true;
}

void UdpServerTransport::close() {
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close();
    }
    m_lastSenderAddress = QHostAddress();
    m_lastSenderPort = 0;
    m_boundAddress = QHostAddress();
    m_boundPort = 0;
    setStatus(QStringLiteral("Stopped"));
    emitOpenChanged();
}

bool UdpServerTransport::isOpen() const {
    return m_socket.state() == QAbstractSocket::BoundState;
}

bool UdpServerTransport::sendData(const QByteArray &payload) {
    if (!isOpen()) {
        return false;
    }
    if (m_lastSenderPort == 0) {
        setStatus(QStringLiteral("UDP ready, waiting for peer"));
        return false;
    }
    return m_socket.writeDatagram(payload, m_lastSenderAddress, m_lastSenderPort) == payload.size();
}

QStringList UdpServerTransport::capabilities() const {
    return {
        QStringLiteral("rx"),
        QStringLiteral("tx"),
        QStringLiteral("udp-server")
    };
}

QString UdpServerTransport::sessionQualifier() const {
    if (m_boundPort == 0) {
        return {};
    }
    return QStringLiteral("%1_%2").arg(m_boundAddress.toString()).arg(m_boundPort);
}

QString UdpServerTransport::peerDescription() const {
    if (m_lastSenderPort == 0) {
        return QStringLiteral("No peer");
    }
    return QStringLiteral("%1:%2").arg(m_lastSenderAddress.toString()).arg(m_lastSenderPort);
}

}  // namespace hdgnss
