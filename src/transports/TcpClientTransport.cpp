#include "TcpClientTransport.h"

#include <QMetaObject>

namespace hdgnss {

TcpClientTransport::TcpClientTransport(QObject *parent)
    : ITransport(QStringLiteral("TCP"), parent) {
    connect(&m_socket, &QTcpSocket::readyRead, this, [this]() {
        m_readBuffer.append(m_socket.readAll());
        scheduleBufferedDispatch();
    });
    connect(&m_socket, &QTcpSocket::connected, this, [this]() {
        setStatus(QStringLiteral("Connected %1:%2").arg(m_socket.peerName()).arg(m_socket.peerPort()));
        emitOpenChanged();
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this]() {
        m_readBuffer.clear();
        m_dispatchScheduled = false;
        setStatus(QStringLiteral("Disconnected"));
        emitOpenChanged();
    });
    connect(&m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        const QString message = QStringLiteral("TCP error: %1").arg(m_socket.errorString());
        setStatus(message);
        emit errorOccurred(message);
    });
}

bool TcpClientTransport::openWithSettings(const QVariantMap &settings) {
    close();
    m_host = settings.value(QStringLiteral("host"), QStringLiteral("127.0.0.1")).toString();
    m_port = static_cast<quint16>(settings.value(QStringLiteral("port"), 3001).toUInt());
    setStatus(QStringLiteral("Connecting %1:%2").arg(m_host).arg(m_port));
    m_socket.connectToHost(m_host, m_port);
    return true;
}

void TcpClientTransport::close() {
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.abort();
    }
    m_readBuffer.clear();
    m_dispatchScheduled = false;
    m_host.clear();
    m_port = 0;
    setStatus(QStringLiteral("Disconnected"));
    emitOpenChanged();
}

bool TcpClientTransport::isOpen() const {
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

bool TcpClientTransport::sendData(const QByteArray &payload) {
    if (!isOpen()) {
        return false;
    }
    return m_socket.write(payload) == payload.size();
}

QStringList TcpClientTransport::capabilities() const {
    return {
        QStringLiteral("rx"),
        QStringLiteral("tx"),
        QStringLiteral("tcp-client")
    };
}

QString TcpClientTransport::sessionQualifier() const {
    if (m_host.isEmpty() || m_port == 0) {
        return {};
    }
    return QStringLiteral("%1_%2").arg(m_host).arg(m_port);
}

void TcpClientTransport::scheduleBufferedDispatch() {
    if (m_dispatchScheduled) {
        return;
    }
    m_dispatchScheduled = true;
    QMetaObject::invokeMethod(this, &TcpClientTransport::dispatchBufferedReads, Qt::QueuedConnection);
}

void TcpClientTransport::dispatchBufferedReads() {
    m_dispatchScheduled = false;
    if (m_readBuffer.isEmpty()) {
        return;
    }

    const QByteArray chunk = m_readBuffer.left(kDispatchChunkBytes);
    m_readBuffer.remove(0, chunk.size());
    emit dataReceived(chunk);

    if (!m_readBuffer.isEmpty()) {
        scheduleBufferedDispatch();
    }
}

}  // namespace hdgnss
