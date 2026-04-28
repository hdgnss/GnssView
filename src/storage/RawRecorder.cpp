#include "RawRecorder.h"

#include <QDir>
#include <QFileInfo>

#include "src/utils/ByteUtils.h"

namespace hdgnss {

namespace {

QString directionName(DataDirection direction) {
    return direction == DataDirection::Rx ? QStringLiteral("RX") : QStringLiteral("TX");
}

QString stripTrailingLineEndings(const QByteArray &payload) {
    QByteArray trimmed = payload;
    while (!trimmed.isEmpty() && (trimmed.endsWith('\r') || trimmed.endsWith('\n'))) {
        trimmed.chop(1);
    }
    return QString::fromLatin1(trimmed);
}

}

RawRecorder::RawRecorder(QObject *parent)
    : QObject(parent) {
}

RawRecorder::~RawRecorder() {
    closeFiles();
}

QString RawRecorder::sessionDirectory() const {
    return m_sessionDirectory;
}

QString RawRecorder::logRootDirectory() const {
    return m_logRootDirectory;
}

QString RawRecorder::binaryFilePath() const {
    return m_binaryFile.fileName();
}

QString RawRecorder::logFilePath() const {
    return m_logFile.fileName();
}

QString RawRecorder::jsonlFilePath() const {
    return logFilePath();
}

qint64 RawRecorder::bytesRecorded() const {
    return m_bytesRecorded;
}

qint64 RawRecorder::entriesRecorded() const {
    return m_entriesRecorded;
}

void RawRecorder::setLogRootDirectory(const QString &directory) {
    const QString cleaned = directory.trimmed().isEmpty() ? QString() : QDir(directory).absolutePath();
    if (m_logRootDirectory == cleaned) {
        return;
    }
    closeFiles();
    m_logRootDirectory = cleaned;
    m_sessionDirectory.clear();
    m_fileStem.clear();
    if (!m_logRootDirectory.isEmpty()) {
        QDir().mkpath(m_logRootDirectory);
    }
}

void RawRecorder::setRecordRawEnabled(bool enabled) {
    m_recordRawEnabled = enabled;
}

void RawRecorder::setRecordDecodeEnabled(bool enabled) {
    m_recordDecodeEnabled = enabled;
}

QString RawRecorder::sanitizeFilePart(const QString &value) const {
    QString out = value.trimmed();
    if (out.isEmpty()) {
        return {};
    }
    for (QChar &ch : out) {
        if (!(ch.isLetterOrNumber() || ch == QLatin1Char('-') || ch == QLatin1Char('_'))) {
            ch = QLatin1Char('_');
        }
    }
    return out;
}

void RawRecorder::closeFiles() {
    if (m_binaryFile.isOpen()) {
        m_binaryFile.close();
    }
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void RawRecorder::startSession(const QString &baseName, const QDateTime &openedAt, const QString &qualifier) {
    closeFiles();
    m_bytesRecorded = 0;
    m_entriesRecorded = 0;
    m_sessionDirectory.clear();
    m_fileStem.clear();

    if (m_logRootDirectory.isEmpty()) {
        return;
    }

    const QString baseTimeSuffix = openedAt.toUTC().toString(QStringLiteral("yyyyMMdd-hhmmss-zzz"));
    QString timeSuffix = baseTimeSuffix;
    m_sessionDirectory = QDir(m_logRootDirectory).filePath(timeSuffix);
    for (int attempt = 1; QFileInfo::exists(m_sessionDirectory); ++attempt) {
        timeSuffix = QStringLiteral("%1-%2").arg(baseTimeSuffix).arg(attempt);
        m_sessionDirectory = QDir(m_logRootDirectory).filePath(timeSuffix);
    }
    if (!QDir().mkpath(m_sessionDirectory)) {
        m_sessionDirectory.clear();
        return;
    }

    const QString cleanBase = sanitizeFilePart(baseName.isEmpty() ? QStringLiteral("session") : baseName);
    const QString cleanQualifier = sanitizeFilePart(qualifier);
    m_fileStem = cleanQualifier.isEmpty()
        ? QStringLiteral("%1_%2").arg(cleanBase, timeSuffix)
        : QStringLiteral("%1_%2_%3").arg(cleanBase, cleanQualifier, timeSuffix);

    ensureOpen();
}

void RawRecorder::ensureOpen() {
    if (m_sessionDirectory.isEmpty()) {
        return;
    }
    if (!m_binaryFile.isOpen()) {
        m_binaryFile.setFileName(QDir(m_sessionDirectory).filePath(QStringLiteral("%1.raw.bin").arg(m_fileStem)));
        m_binaryFile.open(QIODevice::WriteOnly | QIODevice::Append);
    }
    if (!m_logFile.isOpen()) {
        m_logFile.setFileName(QDir(m_sessionDirectory).filePath(QStringLiteral("%1.log").arg(m_fileStem)));
        m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
}

void RawRecorder::writeLogEntry(const QDateTime &timestampUtc,
                                DataDirection direction,
                                const QString &format,
                                const QString &data) {
    const QString line = QStringLiteral("%1,%2,%3:  %4\n")
                             .arg(timestampUtc.toString(Qt::ISODateWithMs),
                                  directionName(direction),
                                  format,
                                  data);
    m_logFile.write(line.toUtf8());
    ++m_entriesRecorded;
}

void RawRecorder::recordRaw(const RawLogEntry &entry) {
    if (!m_recordRawEnabled) {
        return;
    }
    ensureOpen();
    if (!m_binaryFile.isOpen()) {
        return;
    }

    const qint64 written = m_binaryFile.write(entry.payload);
    if (written > 0) {
        m_bytesRecorded += written;
    }
    m_binaryFile.flush();
}

void RawRecorder::recordChunk(const QDateTime &timestampUtc,
                              DataDirection direction,
                              const StreamChunk &chunk,
                              const QStringList &decodedLines) {
    if (!m_recordDecodeEnabled) {
        return;
    }
    ensureOpen();
    if (!m_logFile.isOpen()) {
        return;
    }

    if (chunk.payload.isEmpty()) {
        return;
    }

    if (chunk.kind == StreamChunkKind::Binary) {
        writeLogEntry(timestampUtc, direction, QStringLiteral("HEX"), ByteUtils::toHex(chunk.payload));
        for (const QString &decodedLine : decodedLines) {
            const QStringList parts = decodedLine.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            for (const QString &part : parts) {
                const QString trimmed = part.trimmed();
                if (!trimmed.isEmpty()) {
                    writeLogEntry(timestampUtc, direction, QStringLiteral("DEC"), trimmed);
                }
            }
        }
    } else {
        writeLogEntry(timestampUtc, direction, QStringLiteral("ASC"), stripTrailingLineEndings(chunk.payload));
    }

    m_logFile.flush();
}

}  // namespace hdgnss
