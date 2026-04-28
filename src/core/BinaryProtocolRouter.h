#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>

#include <functional>

#include "src/protocols/GnssTypes.h"

namespace hdgnss {

struct BinaryProtocolRegistration {
    QString name;
    std::function<int(const QByteArray &sample)> probe;
    std::function<QList<ProtocolMessage>(const QByteArray &bytes)> feed;
    std::function<int(const QByteArray &buffer)> trailingBytesToKeep;
    std::function<QList<CommandTemplate>()> commandTemplates;
};

class BinaryProtocolRouter {
public:
    void registerProtocol(BinaryProtocolRegistration registration);
    QList<ProtocolMessage> routeChunk(const QString &streamKey, const QByteArray &payload);
    QList<CommandTemplate> commandTemplates() const;
    void resetStream(const QString &streamKey);
    void resetAllStreams();

private:
    struct StreamState {
        QByteArray pending;
        int activeProtocolIndex = -1;
    };

    QList<BinaryProtocolRegistration> m_protocols;
    QHash<QString, StreamState> m_streamStates;
};

}  // namespace hdgnss
