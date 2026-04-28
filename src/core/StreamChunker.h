#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <functional>

namespace hdgnss {

enum class StreamChunkKind {
    Nmea,
    Binary,
    Text
};

struct StreamChunk {
    StreamChunkKind kind = StreamChunkKind::Binary;
    QByteArray payload;

    QString kindName() const;
};

class StreamChunker {
public:
    // Returns >0 (frame size) for a complete frame, 0 if not this protocol,
    // <0 if a partial frame is in progress and more bytes are needed.
    using BinaryFramer = std::function<int(const QByteArray &)>;

    explicit StreamChunker(int capacityBytes = 256 * 1024);

    void append(const QByteArray &bytes);
    QList<StreamChunk> takeAvailableChunks(const QList<BinaryFramer> &framers = {});
    int bufferedBytes() const;

private:
    int m_capacityBytes = 0;
    QByteArray m_buffer;
};

}  // namespace hdgnss
