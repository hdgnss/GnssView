#include "FilePacketizer.h"

namespace hdgnss {

namespace {

constexpr int kDefaultChunkSize = 1024;

QList<QByteArray> fixedChunks(const QByteArray &bytes) {
    QList<QByteArray> packets;
    packets.reserve((bytes.size() + kDefaultChunkSize - 1) / kDefaultChunkSize);
    for (int offset = 0; offset < bytes.size(); offset += kDefaultChunkSize) {
        packets.append(bytes.mid(offset, kDefaultChunkSize));
    }
    return packets;
}

}  // namespace

QList<QByteArray> FilePacketizer::packetize(const QByteArray &bytes, QString *errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }
    if (bytes.isEmpty()) {
        return {};
    }
    return fixedChunks(bytes);
}

}  // namespace hdgnss
