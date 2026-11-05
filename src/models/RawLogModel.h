#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QTimer>

#include "src/core/StreamChunker.h"
#include "src/protocols/GnssTypes.h"

namespace hdgnss {

class RawLogModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    static constexpr int kMaxEntries = 10000;

    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        DirectionRole,
        TransportRole,
        KindRole,
        MessageRole,
        SizeRole,
        DisplayRole,
        HexRole,
        AsciiRole
    };

    explicit RawLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void clear();
    // Appended entries are queued and published to views in one batch (one
    // rowsInserted / countChanged per flush). Flushes run on a short timer;
    // call flushPending() to publish immediately.
    void flushPending();
    void appendEntry(const RawLogEntry &entry);
    void appendChunk(const QDateTime &timestampUtc,
                     DataDirection direction,
                     const QString &transportName,
                     const QString &kind,
                     const QByteArray &payload);
    void appendProtocolMessage(const QDateTime &timestampUtc,
                               DataDirection direction,
                               const QString &transportName,
                               const QString &kind,
                               const ProtocolMessage &message);

signals:
    void countChanged();

private:
    static constexpr int kTrimBatchSize = 1000;
    static constexpr int kFlushIntervalMs = 50;

    struct DisplayEntry {
        QDateTime timestampUtc;
        DataDirection direction = DataDirection::Rx;
        QString transportName;
        QString kind;
        QString messageName;
        QByteArray payloadPreview;
        int payloadSize = 0;
    };

    void appendDisplayEntry(const DisplayEntry &entry);

    QList<DisplayEntry> m_entries;
    QList<DisplayEntry> m_pending;
    QTimer m_flushTimer;
    QHash<QString, StreamChunker> m_pendingBuffers;
};

}  // namespace hdgnss
