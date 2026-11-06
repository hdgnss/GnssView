#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QStringList>

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
    enum class TaskKind {
        OpenSession,
        CloseSession,
        WriteRaw,
        WriteChunk,
        Flush
    };

    struct WriteTask {
        TaskKind kind = TaskKind::Flush;
        quint64 sequence = 0;
        QString binaryPath;
        QString logPath;
        QByteArray rawPayload;
        QDateTime timestampUtc;
        DataDirection direction = DataDirection::Rx;
        StreamChunk chunk;
        QStringList decodedLines;
    };

    QString sanitizeFilePart(const QString &value) const;
    void closeFiles();
    quint64 enqueueTask(WriteTask task);
    void waitForSequence(quint64 sequence);
    void workerLoop();
    void reportWriteFailure(const QString &path, const QString &error);

    QString m_logRootDirectory;
    QString m_sessionDirectory;
    QString m_fileStem;
    QString m_binaryFilePath;
    QString m_logFilePath;
    bool m_recordRawEnabled = false;
    bool m_recordDecodeEnabled = false;
    std::atomic<bool> m_binaryFileOpen{false};
    std::atomic<qint64> m_bytesRecorded{0};
    std::atomic<qint64> m_entriesRecorded{0};
    bool m_writeFailureReported = false;

    std::mutex m_mutex;
    std::condition_variable m_taskCv;
    std::condition_variable m_doneCv;
    std::deque<WriteTask> m_taskQueue;
    qint64 m_queuedBytes = 0;
    quint64 m_nextSequence = 0;
    quint64 m_completedSequence = 0;
    bool m_stopWorker = false;
    std::thread m_workerThread;
};

}  // namespace hdgnss
