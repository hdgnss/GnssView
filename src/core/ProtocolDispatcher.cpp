#include "ProtocolDispatcher.h"

namespace hdgnss {

void ProtocolDispatcher::registerChunkProtocol(ChunkProtocolRegistration registration) {
    m_chunkProtocols.append(std::move(registration));
}

void ProtocolDispatcher::registerBinaryProtocol(BinaryProtocolRegistration registration) {
    m_binaryRouter.registerProtocol(std::move(registration));
}

void ProtocolDispatcher::registerPlugin(IProtocolPlugin &plugin) {
    const QList<ProtocolPluginKind> kinds = plugin.pluginKinds();
    for (ProtocolPluginKind kind : kinds) {
        if (kind == ProtocolPluginKind::Nmea) {
            registerChunkPlugin(StreamChunkKind::Nmea, plugin);
            continue;
        }
        registerBinaryPlugin(plugin, [&plugin](const QByteArray &buffer) {
            return plugin.trailingBytesToKeep(buffer);
        });
    }
}

void ProtocolDispatcher::registerChunkPlugin(StreamChunkKind kind, IProtocolPlugin &plugin) {
    registerChunkProtocol({
        plugin.protocolName(),
        kind,
        [&plugin](const QByteArray &bytes) {
            return plugin.feed(bytes);
        },
        [&plugin]() {
            return plugin.commandTemplates();
        }
    });
}

void ProtocolDispatcher::registerBinaryPlugin(IProtocolPlugin &plugin,
                                              std::function<int(const QByteArray &buffer)> trailingBytesToKeep) {
    registerBinaryProtocol({
        plugin.protocolName(),
        [&plugin](const QByteArray &sample) {
            return plugin.probe(sample);
        },
        [&plugin](const QByteArray &bytes) {
            return plugin.feed(bytes);
        },
        std::move(trailingBytesToKeep),
        [&plugin]() {
            return plugin.commandTemplates();
        }
    });
}

QList<ProtocolMessage> ProtocolDispatcher::routeChunk(const QString &streamKey, const StreamChunk &chunk) {
    if (chunk.payload.isEmpty()) {
        return {};
    }

    if (chunk.kind == StreamChunkKind::Binary) {
        return m_binaryRouter.routeChunk(streamKey, chunk.payload);
    }

    QList<ProtocolMessage> messages;
    for (const ChunkProtocolRegistration &registration : m_chunkProtocols) {
        if (registration.kind != chunk.kind || !registration.feed) {
            continue;
        }
        messages.append(registration.feed(chunk.payload));
    }
    return messages;
}

QList<CommandTemplate> ProtocolDispatcher::commandTemplates() const {
    QList<CommandTemplate> templates;
    for (const ChunkProtocolRegistration &registration : m_chunkProtocols) {
        if (!registration.commandTemplates) {
            continue;
        }
        templates.append(registration.commandTemplates());
    }
    templates.append(m_binaryRouter.commandTemplates());
    return templates;
}

void ProtocolDispatcher::resetStream(const QString &streamKey) {
    m_binaryRouter.resetStream(streamKey);
}

void ProtocolDispatcher::resetAllStreams() {
    m_binaryRouter.resetAllStreams();
}

}  // namespace hdgnss
