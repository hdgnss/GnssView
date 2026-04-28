#include "RawLogModel.h"

#include <QDebug>

#include "src/utils/ByteUtils.h"

namespace hdgnss {

namespace {

constexpr int kPreviewBytes = 256;
constexpr int kMaxHexDisplayChars = 86;

bool rawDataScrollDebugEnabled() {
  static const bool enabled =
      qEnvironmentVariableIntValue("HDGNSS_RAWDATA_SCROLL_DEBUG") > 0;
  return enabled;
}

QString previewSuffix(int payloadSize, int previewSize) {
  if (previewSize >= payloadSize) {
    return {};
  }
  return QStringLiteral(" ... (+%1 B)").arg(payloadSize - previewSize);
}

QString displayTextForEntry(const QString &kind,
                            const QByteArray &payloadPreview, int payloadSize) {
  if (kind == QStringLiteral("NMEA") || kind == QStringLiteral("ASCII")) {
    QString text = QString::fromLatin1(payloadPreview);
    text.replace(QStringLiteral("\r\n"), QStringLiteral(" "));
    text.replace(QLatin1Char('\n'), QLatin1Char(' '));
    text.replace(QLatin1Char('\r'), QLatin1Char(' '));
    return text.trimmed() + previewSuffix(payloadSize, payloadPreview.size());
  }

  const QString hexText = ByteUtils::toHex(payloadPreview);
  if (hexText.size() <= kMaxHexDisplayChars) {
    return hexText + previewSuffix(payloadSize, payloadPreview.size());
  }

  return hexText.left(kMaxHexDisplayChars).trimmed() +
         QStringLiteral(" (%1 B)").arg(payloadSize);
}

} // namespace

RawLogModel::RawLogModel(QObject *parent) : QAbstractListModel(parent) {}

int RawLogModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : m_entries.size();
}

QVariant RawLogModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
    return {};
  }

  const DisplayEntry &entry = m_entries.at(index.row());
  switch (role) {
  case TimestampRole:
    return entry.timestampUtc.toString(QStringLiteral("HH:mm:ss.zzz"));
  case DirectionRole:
    return entry.direction == DataDirection::Rx ? QStringLiteral("RX")
                                                : QStringLiteral("TX");
  case TransportRole:
    return entry.transportName;
  case KindRole:
    return entry.kind;
  case MessageRole:
    return entry.messageName;
  case SizeRole:
    return entry.payloadSize;
  case DisplayRole:
    return displayTextForEntry(entry.kind, entry.payloadPreview,
                               entry.payloadSize);
  case HexRole:
    return displayTextForEntry(QStringLiteral("BIN"), entry.payloadPreview,
                               entry.payloadSize);
  case AsciiRole:
    return ByteUtils::toAscii(entry.payloadPreview) +
           previewSuffix(entry.payloadSize, entry.payloadPreview.size());
  default:
    return {};
  }
}

QHash<int, QByteArray> RawLogModel::roleNames() const {
  return {{TimestampRole, "timestamp"}, {DirectionRole, "direction"},
          {TransportRole, "transport"}, {KindRole, "kind"},
          {MessageRole, "message"},     {SizeRole, "size"},
          {DisplayRole, "display"},     {HexRole, "hex"},
          {AsciiRole, "ascii"}};
}

QVariantMap RawLogModel::get(int row) const {
  if (row < 0 || row >= m_entries.size()) {
    return {};
  }
  const DisplayEntry &entry = m_entries.at(row);
  return {{QStringLiteral("timestamp"),
           entry.timestampUtc.toString(Qt::ISODateWithMs)},
          {QStringLiteral("direction"), entry.direction == DataDirection::Rx
                                            ? QStringLiteral("RX")
                                            : QStringLiteral("TX")},
          {QStringLiteral("transport"), entry.transportName},
          {QStringLiteral("kind"), entry.kind},
          {QStringLiteral("message"), entry.messageName},
          {QStringLiteral("display"),
           displayTextForEntry(entry.kind, entry.payloadPreview,
                               entry.payloadSize)},
          {QStringLiteral("hex"),
           displayTextForEntry(QStringLiteral("BIN"), entry.payloadPreview,
                               entry.payloadSize)},
          {QStringLiteral("ascii"),
           ByteUtils::toAscii(entry.payloadPreview) +
               previewSuffix(entry.payloadSize, entry.payloadPreview.size())},
          {QStringLiteral("size"), entry.payloadSize}};
}

void RawLogModel::clear() {
  if (m_entries.isEmpty() && m_pendingBuffers.isEmpty()) {
    return;
  }
  if (rawDataScrollDebugEnabled()) {
    qInfo().noquote() << "[RawDataModel] clear entries=" << m_entries.size()
                      << "pendingStreams=" << m_pendingBuffers.size();
  }
  beginResetModel();
  m_entries.clear();
  m_pendingBuffers.clear();
  endResetModel();
  emit countChanged();
}

void RawLogModel::appendEntry(const RawLogEntry &entry) {
  const QString streamKey =
      QStringLiteral("%1:%2")
          .arg(entry.transportName)
          .arg(entry.direction == DataDirection::Rx ? QStringLiteral("RX")
                                                    : QStringLiteral("TX"));

  StreamChunker &buffer = m_pendingBuffers[streamKey];
  buffer.append(entry.payload);
  const QList<StreamChunk> chunks = buffer.takeAvailableChunks();

  for (const StreamChunk &chunk : chunks) {
    appendDisplayEntry(DisplayEntry{
        entry.timestampUtc, entry.direction, entry.transportName,
        chunk.kindName(), QString{}, chunk.payload.left(kPreviewBytes),
        static_cast<int>(chunk.payload.size())});
  }
  emit countChanged();
}

void RawLogModel::appendChunk(const QDateTime &timestampUtc,
                              DataDirection direction,
                              const QString &transportName, const QString &kind,
                              const QByteArray &payload) {
  appendDisplayEntry(DisplayEntry{timestampUtc, direction, transportName, kind,
                                  QString{}, payload.left(kPreviewBytes),
                                  static_cast<int>(payload.size())});
  emit countChanged();
}

void RawLogModel::appendProtocolMessage(const QDateTime &timestampUtc,
                                        DataDirection direction,
                                        const QString &transportName,
                                        const QString &kind,
                                        const ProtocolMessage &message) {
  appendDisplayEntry(DisplayEntry{timestampUtc, direction, transportName, kind,
                                  message.messageName,
                                  message.rawFrame.left(kPreviewBytes),
                                  static_cast<int>(message.rawFrame.size())});
  emit countChanged();
}

void RawLogModel::appendDisplayEntry(const DisplayEntry &entry) {
  const int row = m_entries.size();
  beginInsertRows({}, row, row);
  m_entries.append(entry);
  endInsertRows();
}

} // namespace hdgnss
