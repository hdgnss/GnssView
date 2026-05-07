#pragma once

#include <QList>

#include <functional>

#include "src/core/BinaryProtocolRouter.h"
#include "src/core/StreamChunker.h"
#include "src/protocols/GnssTypes.h"
#include "src/protocols/IProtocolPlugin.h"

namespace hdgnss {

struct ChunkProtocolRegistration {
    QString name;
    StreamChunkKind kind = StreamChunkKind::Text;
    std::function<QList<ProtocolMessage>(const QByteArray &bytes)> feed;
    std::function<QList<CommandTemplate>()> commandTemplates;
};

class ProtocolDispatcher {
public:
    void registerChunkProtocol(ChunkProtocolRegistration registration);
    void registerBinaryProtocol(BinaryProtocolRegistration registration);
    void registerPlugin(IProtocolPlugin &plugin);
    void registerChunkPlugin(StreamChunkKind kind, IProtocolPlugin &plugin);
    void registerBinaryPlugin(IProtocolPlugin &plugin,
                              std::function<int(const QByteArray &buffer)> trailingBytesToKeep);

    QList<ProtocolMessage> routeChunk(const QString &streamKey, const StreamChunk &chunk);
    QList<CommandTemplate> commandTemplates() const;
    void resetStream(const QString &streamKey);
    void resetAllStreams();

private:
    QList<ChunkProtocolRegistration> m_chunkProtocols;
    BinaryProtocolRouter m_binaryRouter;
};

}  // namespace hdgnss
