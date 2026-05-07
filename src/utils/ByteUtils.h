#pragma once

#include <QByteArray>
#include <QString>

namespace hdgnss::ByteUtils {

QString toHex(const QByteArray &payload, char separator = ' ');
QString toAscii(const QByteArray &payload);
QString toHexPreview(const QByteArray &payload, int maxBytes, char separator = ' ');
QString toAsciiPreview(const QByteArray &payload, int maxBytes);
QByteArray fromHexString(const QString &value);

}
