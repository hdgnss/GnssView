#pragma once

#include <QUdpSocket>

#include "ITransport.h"

namespace hdgnss {

class UdpServerTransport : public ITransport {
    Q_OBJECT

public:
    explicit UdpServerTransport(QObject *parent = nullptr);

    bool openWithSettings(const QVariantMap &settings) override;
    void close() override;
    bool isOpen() const override;
    bool sendData(const QByteArray &payload) override;
    QStringList capabilities() const override;
    QString sessionQualifier() const override;

    QString peerDescription() const;

private:
    QUdpSocket m_socket;
    QHostAddress m_lastSenderAddress;
    quint16 m_lastSenderPort = 0;
    QHostAddress m_boundAddress;
    quint16 m_boundPort = 0;
};

}  // namespace hdgnss
