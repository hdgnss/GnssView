#include "BinaryProtocolRouter.h"

#include <algorithm>

namespace hdgnss {

void BinaryProtocolRouter::registerProtocol(BinaryProtocolRegistration registration) {
    m_protocols.append(std::move(registration));
}

QList<ProtocolMessage> BinaryProtocolRouter::routeChunk(const QString &streamKey, const QByteArray &payload) {
    if (payload.isEmpty()) {
        return {};
    }

    StreamState &state = m_streamStates[streamKey];
    if (state.activeProtocolIndex >= 0 && state.activeProtocolIndex < m_protocols.size()) {
        return m_protocols.at(state.activeProtocolIndex).feed(payload);
    }

    state.pending.append(payload);

    int bestIndex = -1;
    int bestScore = 0;
    for (int i = 0; i < m_protocols.size(); ++i) {
        const BinaryProtocolRegistration &registration = m_protocols.at(i);
        if (!registration.probe) {
            continue;
        }
        const int score = registration.probe(state.pending);
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    if (bestIndex >= 0) {
        state.activeProtocolIndex = bestIndex;
        const QByteArray bytes = state.pending;
        state.pending.clear();
        return m_protocols.at(bestIndex).feed(bytes);
    }

    int keep = 0;
    for (const BinaryProtocolRegistration &registration : m_protocols) {
        if (!registration.trailingBytesToKeep) {
            continue;
        }
        keep = std::max(keep, registration.trailingBytesToKeep(state.pending));
    }
    if (keep > 0 && keep < state.pending.size()) {
        state.pending = state.pending.right(keep);
    } else if (keep == 0) {
        state.pending.clear();
    }

    return {};
}

QList<CommandTemplate> BinaryProtocolRouter::commandTemplates() const {
    QList<CommandTemplate> templates;
    for (const BinaryProtocolRegistration &registration : m_protocols) {
        if (!registration.commandTemplates) {
            continue;
        }
        templates.append(registration.commandTemplates());
    }
    return templates;
}

void BinaryProtocolRouter::resetStream(const QString &streamKey) {
    m_streamStates.remove(streamKey);
}

void BinaryProtocolRouter::resetAllStreams() {
    m_streamStates.clear();
}

}  // namespace hdgnss
