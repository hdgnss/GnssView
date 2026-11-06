#include "StreamChunker.h"

#include <QByteArrayView>
#include <algorithm>

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

bool looksLikeDigitTaggedTextRun(QByteArrayView buffer, int start) {
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

bool isTextLike(QByteArrayView payload) {
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

// When returning false because a non-printable byte was encountered at `end` or
// because `buffer` ended without any '\r'/'\n', `*skipToOut` is set to that
// index so callers scanning for the next text boundary can skip all intermediate
// start positions in O(1) instead of re-scanning the same range in O(N^2).
bool looksLikeTextRun(QByteArrayView buffer,
                      int start,
                      bool allowShortPlainText,
                      int *skipToOut = nullptr) {
    if (skipToOut) {
        *skipToOut = start;
    }
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
            const QByteArrayView lineSlice = buffer.first(end + 1);
            if (dollarPrefixed) {
                return textLength >= 4 && letterCount >= 2;
            }
            if (digitPrefixed) {
                return looksLikeDigitTaggedTextRun(lineSlice, start);
            }
            if (start + 1 < buffer.size()
                && isAsciiDigit(buffer.at(start + 1))
                && looksLikeDigitTaggedTextRun(lineSlice, start + 1)) {
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
            if (skipToOut) {
                *skipToOut = end;
            }
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
    if (skipToOut) {
        *skipToOut = static_cast<int>(buffer.size());
    }
    return false;
}

int findLineEnd(QByteArrayView buffer) {
    for (int i = 0; i < buffer.size(); ++i) {
        const char ch = buffer.at(i);
        if (ch == '\r' || ch == '\n') {
            return i;
        }
    }
    return -1;
}

int consumeLineEnding(QByteArrayView buffer, int index) {
    int end = index;
    while (end < buffer.size() && (buffer.at(end) == '\r' || buffer.at(end) == '\n')) {
        ++end;
    }
    return end;
}

StreamChunkKind classifyChunk(const QByteArray &payload) {
    QByteArrayView trimmed(payload);
    while (!trimmed.isEmpty() && (trimmed.endsWith('\r') || trimmed.endsWith('\n'))) {
        trimmed.chop(1);
    }
    if (trimmed.startsWith('$') && NmeaProtocolPlugin::validateChecksum(trimmed.toByteArray())) {
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
    // Do not drop unparsed head bytes under normal bursts before
    // takeAvailableChunks() can extract complete frames; enforce a 4x safety
    // ceiling only if append() is called repeatedly without draining.
    const int safetyCapBytes = m_capacityBytes > 0
        ? std::max(m_capacityBytes * 4, 1024 * 1024)
        : 0;
    if (safetyCapBytes > 0 && m_buffer.size() > safetyCapBytes) {
        m_buffer.remove(0, m_buffer.size() - safetyCapBytes);
    }
}

QList<StreamChunk> StreamChunker::takeAvailableChunks(const QList<BinaryFramer> &framers) {
    QList<StreamChunk> chunks;

    // Walk the buffer with a read offset and a zero-copy view; erasing the
    // consumed prefix once at the end avoids O(n^2) memmoves per call.
    int offset = 0;
    QByteArray view;
    QByteArrayView sliceView;
    auto refreshView = [&]() {
        const char *ptr = m_buffer.constData() + offset;
        const int remaining = m_buffer.size() - offset;
        view = QByteArray::fromRawData(ptr, remaining);
        sliceView = QByteArrayView(ptr, remaining);
    };
    // fromRawData views must never escape; always deep-copy emitted payloads.
    auto owned = [&](int length) {
        return QByteArray(sliceView.constData(), length);
    };
    auto finish = [&]() {
        view = QByteArray();
        sliceView = QByteArrayView();
        if (offset > 0) {
            m_buffer.remove(0, offset);
        }
        if (m_capacityBytes > 0 && m_buffer.size() > m_capacityBytes) {
            m_buffer.remove(0, m_buffer.size() - m_capacityBytes);
        }
    };

    while (offset < m_buffer.size()) {
        refreshView();
        int binaryFrameSize = 0;
        bool pendingBinaryFrame = false;
        for (const BinaryFramer &framer : framers) {
            const int result = framer(view);
            if (result > 0) {
                binaryFrameSize = result;
                break;
            }
            if (result < 0) {
                pendingBinaryFrame = true;
            }
        }
        if (binaryFrameSize > 0) {
            chunks.append({StreamChunkKind::Binary, owned(binaryFrameSize)});
            offset += binaryFrameSize;
            continue;
        }
        if (pendingBinaryFrame) {
            break;
        }

        int initialSkipTo = 0;
        if (looksLikeTextRun(sliceView, 0, true, &initialSkipTo)) {
            const int lineEnd = findLineEnd(sliceView);
            if (lineEnd < 0) {
                if (sliceView.size() > kMaxBufferedTextBytes) {
                    const QByteArray payload = owned(sliceView.size());
                    chunks.append({classifyChunk(payload), payload});
                    offset = m_buffer.size();
                }
                break;
            }

            const int consumeEnd = consumeLineEnding(sliceView, lineEnd);
            const QByteArray line = owned(consumeEnd);
            chunks.append({classifyChunk(line), line});
            offset += consumeEnd;
            continue;
        }

        int nextBoundary = -1;
        for (int i = std::max(1, initialSkipTo + 1); i < sliceView.size(); ++i) {
            int skipTo = i;
            if (looksLikeTextRun(sliceView, i, false, &skipTo)) {
                nextBoundary = i;
                break;
            }
            if (skipTo > i) {
                i = skipTo;
            }
        }

        if (nextBoundary > 0) {
            chunks.append({StreamChunkKind::Binary, owned(nextBoundary)});
            offset += nextBoundary;
            continue;
        }

        if (initialSkipTo == sliceView.size() || isTextLike(sliceView)) {
            if (sliceView.size() > kMaxBufferedTextBytes) {
                chunks.append({StreamChunkKind::Text, owned(sliceView.size())});
                offset = m_buffer.size();
            }
            break;
        }

        chunks.append({StreamChunkKind::Binary, owned(sliceView.size())});
        offset = m_buffer.size();
    }

    finish();
    return mergeBinaryChunks(chunks);
}

int StreamChunker::bufferedBytes() const {
    return m_buffer.size();
}

}  // namespace hdgnss
