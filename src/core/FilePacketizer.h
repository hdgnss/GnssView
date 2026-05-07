#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace hdgnss {

class FilePacketizer {
public:
    static QList<QByteArray> packetize(const QByteArray &bytes, QString *errorMessage = nullptr);
};

}  // namespace hdgnss
