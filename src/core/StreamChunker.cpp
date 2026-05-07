#include "StreamChunker.h"

#include "src/protocols/NmeaProtocolPlugin.h"

namespace hdgnss {

namespace {

constexpr int kMaxBufferedTextBytes = 8192;

bool isPrintableAscii(char ch) {
    const unsigned char value = static_cast<unsigned char>(ch);
    return value == '\r' || value == '\n' || value == '\t' || (value >= 0x20 && value <= 0x7E);
}

bool isAsciiLetter(char ch) {
    const unsigned char value = static_cast<unsigned char>(ch);
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}

bool isAsciiDigit(char ch) {
    const unsigned char value = static_cast<unsigned char>(ch);
    return value >= '0' && value <= '9';
}

bool looksLikeDigitTaggedTextRun(const QByteArray &buffer, int start) {
    if (start < 0 || start >= buffer.size() || !isAsciiDigit(buffer.at(start))) {
        return false;
    }

    int pos = start;
    int digitCount = 0;
    while (pos < buffer.size() && isAsciiDigit(buffer.at(pos))) {
        ++pos;
        ++digitCount;
    }
    if (digitCount < 6 || pos >= buffer.size() || !isAsciiLetter(buffer.at(pos))) {
        return false;
    }
    ++pos;
    if (pos >= buffer.size() || buffer.at(pos) != ' ') {
        return false;
    }
    ++pos;

    bool hasPayload = false;
    for (; pos < buffer.size(); ++pos) {
        const char current = buffer.at(pos);
        if (current == '\r' || current == '\n') {
            return hasPayload;
        }
        if (!isPrintableAscii(current)) {
            return false;
        }
        hasPayload = true;
    }
    return false;
}

bool isTextLike(const QByteArray &payload) {
    if (payload.isEmpty()) {
        return false;
    }
    for (const char ch : payload) {
        if (!isPrintableAscii(ch)) {
            return false;
        }
    }
    return true;
}

bool looksLikeTextRun(const QByteArray &buffer, int start, bool allowShortPlainText) {
    if (start < 0 || start >= buffer.size() || !isPrintableAscii(buffer.at(start))) {
        return false;
    }

    const char first = buffer.at(start);
    const bool dollarPrefixed = first == '$';
    const bool digitPrefixed = isAsciiDigit(first);
    const bool alnumPrefixed = isAsciiLetter(first) || digitPrefixed;
    if (!dollarPrefixed && !alnumPrefixed) {
        return false;
    }

    int letterCount = dollarPrefixed ? 0 : (isAsciiLetter(first) ? 1 : 0);
    bool shortRunIsAlphabetic = !dollarPrefixed && isAsciiLetter(first);
    bool sawWhitespace = false;

    for (int end = start + 1; end < buffer.size(); ++end) {
        const char current = buffer.at(end);
        if (current == '\r' || current == '\n') {
            const int textLength = end - start;
            if (dollarPrefixed) {
                return textLength >= 4 && letterCount >= 2;
            }
            if (digitPrefixed) {
                return looksLikeDigitTaggedTextRun(buffer.left(end + 1), start);
            }
            if (start + 1 < buffer.size()
                && isAsciiDigit(buffer.at(start + 1))
                && looksLikeDigitTaggedTextRun(buffer.left(end + 1), start + 1)) {
                return false;
            }
            if (letterCount < 2) {
                return false;
            }
            if (allowShortPlainText && textLength <= 3) {
                return shortRunIsAlphabetic;
            }
            return textLength >= 8 || sawWhitespace;
        }
        if (!isPrintableAscii(current)) {
            return false;
        }
        if (current == ' ' || current == '\t') {
            sawWhitespace = true;
        }
        if (isAsciiLetter(current)) {
            ++letterCount;
        } else if (end - start <= 3) {
            shortRunIsAlphabetic = false;
        }
    }
    return false;
}

int findLineEnd(const QByteArray &buffer) {
    for (int i = 0; i < buffer.size(); ++i) {
        const char ch = buffer.at(i);
        if (ch == '\r' || ch == '\n') {
            return i;
        }
    }
    return -1;
}

int consumeLineEnding(const QByteArray &buffer, int index) {
    int end = index;
    while (end < buffer.size() && (buffer.at(end) == '\r' || buffer.at(end) == '\n')) {
        ++end;
    }
    return end;
}

StreamChunkKind classifyChunk(const QByteArray &payload) {
    QByteArray trimmed = payload;
    while (!trimmed.isEmpty() && (trimmed.endsWith('\r') || trimmed.endsWith('\n'))) {
        trimmed.chop(1);
    }
    if (trimmed.startsWith('$') && NmeaProtocolPlugin::validateChecksum(trimmed)) {
        return StreamChunkKind::Nmea;
    }
    return isTextLike(payload) ? StreamChunkKind::Text : StreamChunkKind::Binary;
}

QList<StreamChunk> mergeBinaryChunks(const QList<StreamChunk> &chunks) {
    QList<StreamChunk> merged;
    for (const StreamChunk &chunk : chunks) {
        if (chunk.payload.isEmpty()) {
            continue;
        }
        if (!merged.isEmpty()
            && merged.last().kind == StreamChunkKind::Binary
            && chunk.kind == StreamChunkKind::Binary) {
            merged.last().payload.append(chunk.payload);
            continue;
        }
        merged.append(chunk);
    }
    return merged;
}

}  // namespace

QString StreamChunk::kindName() const {
    switch (kind) {
    case StreamChunkKind::Nmea:
        return QStringLiteral("NMEA");
    case StreamChunkKind::Binary:
        return QStringLiteral("BIN");
    case StreamChunkKind::Text:
        return QStringLiteral("ASCII");
    }
    return QStringLiteral("BIN");
}

StreamChunker::StreamChunker(int capacityBytes)
    : m_capacityBytes(capacityBytes) {}

void StreamChunker::append(const QByteArray &bytes) {
    if (bytes.isEmpty()) {
        return;
    }
    m_buffer.append(bytes);
    if (m_capacityBytes > 0 && m_buffer.size() > m_capacityBytes) {
        m_buffer.remove(0, m_buffer.size() - m_capacityBytes);
    }
}

QList<StreamChunk> StreamChunker::takeAvailableChunks(const QList<BinaryFramer> &framers) {
    QList<StreamChunk> chunks;

    while (!m_buffer.isEmpty()) {
        int binaryFrameSize = 0;
        bool pendingBinaryFrame = false;
        for (const BinaryFramer &framer : framers) {
            const int result = framer(m_buffer);
            if (result > 0) {
                binaryFrameSize = result;
                break;
            }
            if (result < 0) {
                pendingBinaryFrame = true;
            }
        }
        if (binaryFrameSize > 0) {
            chunks.append({StreamChunkKind::Binary, m_buffer.left(binaryFrameSize)});
            m_buffer.remove(0, binaryFrameSize);
            continue;
        }
        if (pendingBinaryFrame) {
            break;
        }

        if (looksLikeTextRun(m_buffer, 0, true)) {
            const int lineEnd = findLineEnd(m_buffer);
            if (lineEnd < 0) {
                if (m_buffer.size() > kMaxBufferedTextBytes) {
                    chunks.append({classifyChunk(m_buffer), m_buffer});
                    m_buffer.clear();
                }
                break;
            }

            const int consumeEnd = consumeLineEnding(m_buffer, lineEnd);
            const QByteArray line = m_buffer.left(consumeEnd);
            chunks.append({classifyChunk(line), line});
            m_buffer.remove(0, consumeEnd);
            continue;
        }

        int nextBoundary = -1;
        for (int i = 1; i < m_buffer.size(); ++i) {
            if (looksLikeTextRun(m_buffer, i, false)) {
                nextBoundary = i;
                break;
            }
        }

        if (nextBoundary > 0) {
            chunks.append({StreamChunkKind::Binary, m_buffer.left(nextBoundary)});
            m_buffer.remove(0, nextBoundary);
            continue;
        }

        if (isTextLike(m_buffer)) {
            if (m_buffer.size() > kMaxBufferedTextBytes) {
                chunks.append({StreamChunkKind::Text, m_buffer});
                m_buffer.clear();
            }
            break;
        }

        chunks.append({StreamChunkKind::Binary, m_buffer});
        m_buffer.clear();
    }

    return mergeBinaryChunks(chunks);
}

int StreamChunker::bufferedBytes() const {
    return m_buffer.size();
}

}  // namespace hdgnss
