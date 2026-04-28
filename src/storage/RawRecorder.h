#pragma once

#include <QDateTime>
#include <QFile>
#include <QObject>

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
    void recordRaw(const RawLogEntry &entry);
    void recordChunk(const QDateTime &timestampUtc,
                     DataDirection direction,
                     const StreamChunk &chunk,
                     const QStringList &decodedLines = {});
    void startSession(const QString &baseName, const QDateTime &openedAt, const QString &qualifier = QString());

private:
    QString sanitizeFilePart(const QString &value) const;
    void closeFiles();
    void ensureOpen();
    void writeLogEntry(const QDateTime &timestampUtc,
                       DataDirection direction,
                       const QString &format,
                       const QString &data);

    QString m_logRootDirectory;
    QString m_sessionDirectory;
    QString m_fileStem;
    QFile m_binaryFile;
    QFile m_logFile;
    bool m_recordRawEnabled = false;
    bool m_recordDecodeEnabled = false;
    qint64 m_bytesRecorded = 0;
    qint64 m_entriesRecorded = 0;
};

}  // namespace hdgnss
