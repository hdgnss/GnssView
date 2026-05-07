#pragma once

#include <QByteArray>
#include <QTcpSocket>

#include "ITransport.h"

namespace hdgnss {

class TcpClientTransport : public ITransport {
    Q_OBJECT

public:
    explicit TcpClientTransport(QObject *parent = nullptr);

    bool openWithSettings(const QVariantMap &settings) override;
    void close() override;
    bool isOpen() const override;
    bool sendData(const QByteArray &payload) override;
    QStringList capabilities() const override;
    QString sessionQualifier() const override;

private:
    void scheduleBufferedDispatch();
    void dispatchBufferedReads();

    static constexpr int kDispatchChunkBytes = 16 * 1024;

    QTcpSocket m_socket;
    QByteArray m_readBuffer;
    bool m_dispatchScheduled = false;
    QString m_host;
    quint16 m_port = 0;
};

}  // namespace hdgnss
