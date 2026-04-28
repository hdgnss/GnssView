#pragma once

#include <QByteArray>
#include <QList>
#include <QtPlugin>

#include "hdgnss/GnssTypes.h"

namespace hdgnss {

class IProtocolPlugin {
public:
    virtual ~IProtocolPlugin() = default;

    virtual QString protocolName() const = 0;
    virtual QList<ProtocolPluginKind> pluginKinds() const = 0;
    virtual int probe(const QByteArray &sample) const = 0;
    virtual QList<ProtocolMessage> feed(const QByteArray &bytes) = 0;
    virtual QByteArray encode(const QVariantMap &command) const = 0;
    virtual QList<QByteArray> packetizeFile(const QByteArray &bytes, QString *errorMessage = nullptr) const {
        if (errorMessage) {
            errorMessage->clear();
        }
        if (bytes.isEmpty()) {
            return {};
        }

        if (!pluginKinds().contains(ProtocolPluginKind::Binary)) {
            constexpr int kDefaultChunkSize = 1024;
            QList<QByteArray> chunks;
            chunks.reserve((bytes.size() + kDefaultChunkSize - 1) / kDefaultChunkSize);
            for (int offset = 0; offset < bytes.size(); offset += kDefaultChunkSize) {
                chunks.append(bytes.mid(offset, kDefaultChunkSize));
            }
            return chunks;
        }

        QList<QByteArray> packets;
        int offset = 0;
        while (offset < bytes.size()) {
            const int frameSize = parseBinaryFrame(bytes.mid(offset));
            if (frameSize <= 0) {
                if (errorMessage) {
                    *errorMessage = frameSize < 0
                        ? QStringLiteral("Incomplete %1 frame at byte %2").arg(protocolName(), QString::number(offset))
                        : QStringLiteral("Invalid %1 frame at byte %2").arg(protocolName(), QString::number(offset));
                }
                return {};
            }
            if (offset + frameSize > bytes.size()) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Truncated %1 frame at byte %2").arg(protocolName(), QString::number(offset));
                }
                return {};
            }

            packets.append(bytes.mid(offset, frameSize));
            offset += frameSize;
        }
        return packets;
    }
    virtual QList<CommandTemplate> commandTemplates() const = 0;
    virtual QList<ProtocolBuildMessage> supportedBuildMessages() const {
        return {};
    }
    virtual QList<ProtocolInfoPanel> infoPanels() const {
        return {};
    }
    virtual bool supportsFullDecode() const = 0;
    virtual int trailingBytesToKeep(const QByteArray &buffer) const {
        Q_UNUSED(buffer);
        return 0;
    }

    // Probe the start of buffer for a complete binary frame.
    // Returns >0 (frame byte count) if a complete frame is present,
    //          0 if the buffer does not start with this protocol's sync,
    //         <0 if a partial frame is in progress and more bytes are needed.
    virtual int parseBinaryFrame(const QByteArray &buffer) const {
        Q_UNUSED(buffer);
        return 0;
    }
};

}  // namespace hdgnss

#define HDGNSS_PROTOCOL_PLUGIN_IID "com.hdgnss.IProtocolPlugin/1.0"
Q_DECLARE_INTERFACE(hdgnss::IProtocolPlugin, HDGNSS_PROTOCOL_PLUGIN_IID)
