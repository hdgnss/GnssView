#include "RawRecorder.h"

#include <chrono>

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>

#include "src/utils/ByteUtils.h"

namespace hdgnss {

namespace {

constexpr int kFlushIntervalMs = 1000;
constexpr qint64 kFlushByteThreshold = 256 * 1024;
constexpr qint64 kMaxQueuedBytes = 16 * 1024 * 1024;

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

}  // namespace

RawRecorder::RawRecorder(QObject *parent)
    : QObject(parent)
    , m_workerThread(&RawRecorder::workerLoop, this) {}

RawRecorder::~RawRecorder() {
    closeFiles();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopWorker = true;
    }
    m_taskCv.notify_all();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

QString RawRecorder::sessionDirectory() const {
    return m_sessionDirectory;
}

QString RawRecorder::logRootDirectory() const {
    return m_logRootDirectory;
}

QString RawRecorder::binaryFilePath() const {
    return m_binaryFilePath;
}

QString RawRecorder::logFilePath() const {
    return m_logFilePath;
}

QString RawRecorder::jsonlFilePath() const {
    return logFilePath();
}

qint64 RawRecorder::bytesRecorded() const {
    return m_bytesRecorded.load(std::memory_order_relaxed);
}

qint64 RawRecorder::entriesRecorded() const {
    return m_entriesRecorded.load(std::memory_order_relaxed);
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
    m_binaryFilePath.clear();
    m_logFilePath.clear();
    if (!m_logRootDirectory.isEmpty()) {
        QDir().mkpath(m_logRootDirectory);
    }
}

void RawRecorder::setRecordRawEnabled(bool enabled) {
    if (!enabled) {
        flush();
    }
    m_recordRawEnabled = enabled;
}

void RawRecorder::setRecordDecodeEnabled(bool enabled) {
    if (!enabled) {
        flush();
    }
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

quint64 RawRecorder::enqueueTask(WriteTask task) {
    const qint64 taskBytes = task.rawPayload.size() + task.chunk.payload.size();
    quint64 seq = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if ((task.kind == TaskKind::WriteRaw || task.kind == TaskKind::WriteChunk)
            && m_queuedBytes + taskBytes > kMaxQueuedBytes) {
            const QString targetPath = task.kind == TaskKind::WriteRaw ? m_binaryFilePath : m_logFilePath;
            QMetaObject::invokeMethod(this, [this, targetPath]() {
                reportWriteFailure(targetPath, QStringLiteral("Disk write queue overflow"));
            }, Qt::QueuedConnection);
            return 0;
        }
        seq = ++m_nextSequence;
        task.sequence = seq;
        m_queuedBytes += taskBytes;
        m_taskQueue.push_back(std::move(task));
    }
    m_taskCv.notify_one();
    return seq;
}

void RawRecorder::waitForSequence(quint64 sequence) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_doneCv.wait(lock, [this, sequence]() {
        return m_completedSequence >= sequence;
    });
}

void RawRecorder::closeFiles() {
    if (m_sessionDirectory.isEmpty() && m_binaryFilePath.isEmpty() && m_logFilePath.isEmpty()) {
        return;
    }
    WriteTask task;
    task.kind = TaskKind::CloseSession;
    waitForSequence(enqueueTask(std::move(task)));
}

void RawRecorder::startSession(const QString &baseName, const QDateTime &openedAt, const QString &qualifier) {
    closeFiles();
    m_binaryFileOpen.store(false, std::memory_order_relaxed);
    m_bytesRecorded.store(0, std::memory_order_relaxed);
    m_entriesRecorded.store(0, std::memory_order_relaxed);
    m_writeFailureReported = false;
    m_sessionDirectory.clear();
    m_fileStem.clear();
    m_binaryFilePath.clear();
    m_logFilePath.clear();

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

    m_binaryFilePath = QDir(m_sessionDirectory).filePath(QStringLiteral("%1.raw.bin").arg(m_fileStem));
    m_logFilePath = QDir(m_sessionDirectory).filePath(QStringLiteral("%1.log").arg(m_fileStem));

    WriteTask openTask;
    openTask.kind = TaskKind::OpenSession;
    openTask.binaryPath = m_binaryFilePath;
    openTask.logPath = m_logFilePath;
    waitForSequence(enqueueTask(std::move(openTask)));
}

void RawRecorder::reportWriteFailure(const QString &path, const QString &error) {
    if (m_writeFailureReported) {
        return;
    }
    m_writeFailureReported = true;
    emit writeFailed(QStringLiteral("Failed to write %1: %2").arg(path, error));
}

void RawRecorder::flush() {
    if (m_sessionDirectory.isEmpty()) {
        return;
    }
    WriteTask task;
    task.kind = TaskKind::Flush;
    waitForSequence(enqueueTask(std::move(task)));
}

void RawRecorder::recordRaw(const RawLogEntry &entry) {
    if (!m_recordRawEnabled || entry.direction != DataDirection::Rx
        || m_sessionDirectory.isEmpty() || !m_binaryFileOpen.load(std::memory_order_relaxed)
        || entry.payload.isEmpty()) {
        return;
    }

    const qint64 payloadSize = entry.payload.size();
    WriteTask task;
    task.kind = TaskKind::WriteRaw;
    task.rawPayload = entry.payload;
    if (enqueueTask(std::move(task)) > 0) {
        m_bytesRecorded.fetch_add(payloadSize, std::memory_order_relaxed);
    }
}

void RawRecorder::recordChunk(const QDateTime &timestampUtc,
                              DataDirection direction,
                              const StreamChunk &chunk,
                              const QStringList &decodedLines) {
    if (!m_recordDecodeEnabled || m_sessionDirectory.isEmpty() || chunk.payload.isEmpty()) {
        return;
    }

    WriteTask task;
    task.kind = TaskKind::WriteChunk;
    task.timestampUtc = timestampUtc;
    task.direction = direction;
    task.chunk = chunk;
    task.decodedLines = decodedLines;
    enqueueTask(std::move(task));
}

void RawRecorder::workerLoop() {
    QFile binaryFile;
    QFile logFile;
    qint64 pendingBinaryFlushBytes = 0;
    qint64 pendingLogFlushBytes = 0;

    auto flushOpenFiles = [&]() {
        if (pendingBinaryFlushBytes > 0 && binaryFile.isOpen()) {
            binaryFile.flush();
        }
        if (pendingLogFlushBytes > 0 && logFile.isOpen()) {
            logFile.flush();
        }
        pendingBinaryFlushBytes = 0;
        pendingLogFlushBytes = 0;
    };

    auto postWriteFailure = [this](const QString &path, const QString &error) {
        QMetaObject::invokeMethod(this, [this, path, error]() {
            reportWriteFailure(path, error);
        }, Qt::QueuedConnection);
    };

    auto writeLogLine = [&](const QDateTime &timestampUtc,
                            DataDirection direction,
                            const QString &format,
                            const QString &data) {
        if (!logFile.isOpen()) {
            return;
        }
        const QString line = QStringLiteral("%1,%2,%3:  %4\n")
                                 .arg(timestampUtc.toString(Qt::ISODateWithMs),
                                      directionName(direction),
                                      format,
                                      data);
        const QByteArray lineBytes = line.toUtf8();
        const qint64 written = logFile.write(lineBytes);
        if (written != lineBytes.size()) {
            postWriteFailure(logFile.fileName(), logFile.errorString());
        }
        if (written > 0) {
            pendingLogFlushBytes += written;
        }
        m_entriesRecorded.fetch_add(1, std::memory_order_relaxed);
    };

    while (true) {
        std::deque<WriteTask> batch;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_taskQueue.empty() && !m_stopWorker) {
                if (pendingBinaryFlushBytes > 0 || pendingLogFlushBytes > 0) {
                    m_taskCv.wait_for(lock, std::chrono::milliseconds(kFlushIntervalMs));
                } else {
                    m_taskCv.wait(lock);
                }
            }
            if (m_taskQueue.empty()) {
                if (m_stopWorker) {
                    break;
                }
                lock.unlock();
                flushOpenFiles();
                continue;
            }
            batch.swap(m_taskQueue);
            m_queuedBytes = 0;
        }

        quint64 lastProcessedSeq = 0;
        for (WriteTask &task : batch) {
            switch (task.kind) {
            case TaskKind::OpenSession:
                flushOpenFiles();
                if (binaryFile.isOpen()) {
                    binaryFile.close();
                }
                if (logFile.isOpen()) {
                    logFile.close();
                }
                if (!task.binaryPath.isEmpty()) {
                    binaryFile.setFileName(task.binaryPath);
                    if (!binaryFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
                        binaryFile.close();
                    }
                }
                m_binaryFileOpen.store(binaryFile.isOpen(), std::memory_order_relaxed);
                if (!task.logPath.isEmpty()) {
                    logFile.setFileName(task.logPath);
                    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                        logFile.close();
                    }
                }
                break;

            case TaskKind::CloseSession:
                flushOpenFiles();
                if (binaryFile.isOpen()) {
                    binaryFile.close();
                }
                m_binaryFileOpen.store(false, std::memory_order_relaxed);
                if (logFile.isOpen()) {
                    logFile.close();
                }
                break;

            case TaskKind::WriteRaw:
                if (binaryFile.isOpen() && !task.rawPayload.isEmpty()) {
                    const qint64 written = binaryFile.write(task.rawPayload);
                    if (written != task.rawPayload.size()) {
                        const qint64 unwritten = task.rawPayload.size() - std::max<qint64>(0, written);
                        m_bytesRecorded.fetch_sub(unwritten, std::memory_order_relaxed);
                        postWriteFailure(binaryFile.fileName(), binaryFile.errorString());
                    }
                    if (written > 0) {
                        pendingBinaryFlushBytes += written;
                        if (pendingBinaryFlushBytes >= kFlushByteThreshold) {
                            flushOpenFiles();
                        }
                    }
                } else if (!task.rawPayload.isEmpty()) {
                    m_bytesRecorded.fetch_sub(task.rawPayload.size(), std::memory_order_relaxed);
                }
                break;

            case TaskKind::WriteChunk:
                if (logFile.isOpen() && !task.chunk.payload.isEmpty()) {
                    if (task.chunk.kind == StreamChunkKind::Binary) {
                        writeLogLine(task.timestampUtc,
                                     task.direction,
                                     QStringLiteral("HEX"),
                                     ByteUtils::toHex(task.chunk.payload));
                        for (const QString &decodedLine : std::as_const(task.decodedLines)) {
                            const QStringList parts = decodedLine.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                            for (const QString &part : parts) {
                                const QString trimmed = part.trimmed();
                                if (!trimmed.isEmpty()) {
                                    writeLogLine(task.timestampUtc, task.direction, QStringLiteral("DEC"), trimmed);
                                }
                            }
                        }
                    } else {
                        writeLogLine(task.timestampUtc,
                                     task.direction,
                                     QStringLiteral("ASC"),
                                     stripTrailingLineEndings(task.chunk.payload));
                    }
                    if (pendingLogFlushBytes >= kFlushByteThreshold) {
                        flushOpenFiles();
                    }
                }
                break;

            case TaskKind::Flush:
                flushOpenFiles();
                break;
            }
            lastProcessedSeq = task.sequence;
        }

        if (lastProcessedSeq > 0) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_completedSequence = lastProcessedSeq;
            }
            m_doneCv.notify_all();
        }
    }

    flushOpenFiles();
    if (binaryFile.isOpen()) {
        binaryFile.close();
    }
    if (logFile.isOpen()) {
        logFile.close();
    }
}

}  // namespace hdgnss
