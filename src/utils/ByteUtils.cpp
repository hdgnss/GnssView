#include "ByteUtils.h"

#include <QRegularExpression>

namespace hdgnss::ByteUtils {

namespace {

QString truncatedSuffix(qsizetype payloadSize, qsizetype previewSize) {
    if (previewSize >= payloadSize) {
        return {};
    }
    return QStringLiteral(" ... (+%1 bytes)").arg(payloadSize - previewSize);
}

}

QString toHex(const QByteArray &payload, char separator) {
    return QString::fromLatin1(payload.toHex(separator).toUpper());
}

QString toAscii(const QByteArray &payload) {
    QString out;
    out.reserve(payload.size());
    for (const char ch : payload) {
        const uchar value = static_cast<uchar>(ch);
        out.append((value >= 32 && value <= 126) ? QChar(value) : QChar('.'));
    }
    return out;
}

QString toHexPreview(const QByteArray &payload, int maxBytes, char separator) {
    const int safeMaxBytes = qMax(0, maxBytes);
    const QByteArray preview = payload.left(safeMaxBytes);
    return toHex(preview, separator) + truncatedSuffix(payload.size(), preview.size());
}

QString toAsciiPreview(const QByteArray &payload, int maxBytes) {
    const int safeMaxBytes = qMax(0, maxBytes);
    const QByteArray preview = payload.left(safeMaxBytes);
    return toAscii(preview) + truncatedSuffix(payload.size(), preview.size());
}

QByteArray fromHexString(const QString &value) {
    QString sanitized = value;
    sanitized.remove(QRegularExpression(QStringLiteral("[^0-9A-Fa-f]")));
    if (sanitized.size() % 2 != 0) {
        sanitized.prepend(u'0');
    }
    return QByteArray::fromHex(sanitized.toLatin1());
}

}  // namespace hdgnss::ByteUtils
