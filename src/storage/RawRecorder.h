#pragma once

#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QTimer>

#include "src/core/StreamChunker.h"
#include "src/protocols/GnssTypes.h"

namespace hdgnss {

class RawRecorder : public QObject {
    Q_OBJECT

public:
    explicit RawRecorder(QObject *parent = nullptr);
    ~RawRecorder() override;

    QString sessionDirectory() const;
    QString binaryFilePath() const;
    QString logFilePath() const;
    QString jsonlFilePath() const;
    qint64 bytesRecorded() const;
    qint64 entriesRecorded() const;
    QString logRootDirectory() const;
    void setLogRootDirectory(const QString &directory);
    void setRecordRawEnabled(bool enabled);
    void setRecordDecodeEnabled(bool enabled);
    // Only Rx bytes are written to the raw binary file so it stays a faithful
    // replayable capture of the device output; Tx is kept in the decode log.
    void recordRaw(const RawLogEntry &entry);
    void recordChunk(const QDateTime &timestampUtc,
                     DataDirection direction,
                     const StreamChunk &chunk,
                     const QStringList &decodedLines = {});
    void startSession(const QString &baseName, const QDateTime &openedAt, const QString &qualifier = QString());
    void flush();

signals:
    // Emitted once per session when a write to disk fails (e.g. disk full).
    void writeFailed(const QString &message);

private:
    QString sanitizeFilePart(const QString &value) const;
    void closeFiles();
    void ensureOpen();
    void scheduleFlush();
    void writeLogEntry(const QDateTime &timestampUtc,
                       DataDirection direction,
                       const QString &format,
                       const QString &data);

    QString m_logRootDirectory;
    QString m_sessionDirectory;
    QString m_fileStem;
    QFile m_binaryFile;
    QFile m_logFile;
    QTimer m_flushTimer;
    bool m_recordRawEnabled = false;
    bool m_recordDecodeEnabled = false;
    qint64 m_bytesRecorded = 0;
    qint64 m_entriesRecorded = 0;
    qint64 m_pendingBinaryFlushBytes = 0;
    qint64 m_pendingLogFlushBytes = 0;
    bool m_writeFailureReported = false;
    void reportWriteFailure(const QString &path, const QString &error);
};

}  // namespace hdgnss
