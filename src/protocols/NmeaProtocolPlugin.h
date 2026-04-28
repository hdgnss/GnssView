#pragma once

#include <QHash>
#include <QSet>

#include "IProtocolPlugin.h"

namespace hdgnss {

class NmeaProtocolPlugin : public IProtocolPlugin {
public:
    NmeaProtocolPlugin() = default;

    QString protocolName() const override;
    QList<ProtocolPluginKind> pluginKinds() const override;
    int probe(const QByteArray &sample) const override;
    QList<ProtocolMessage> feed(const QByteArray &bytes) override;
    QByteArray encode(const QVariantMap &command) const override;
    QList<CommandTemplate> commandTemplates() const override;
    bool supportsFullDecode() const override;
    void resetState();

    static bool validateChecksum(const QByteArray &sentence);
    static quint8 checksumForBody(const QByteArray &body);

private:
    QList<ProtocolMessage> parseSentence(const QByteArray &sentence);
    static bool isHexByte(char high, char low);
    static QString sentenceType(const QList<QByteArray> &fields);
    static QString constellationFromTalker(const QString &talker);
    static QString constellationFromSystemId(int systemId);
    static QString bandFromGsvSignalId(const QString &constellation, int signalId);
    static QString bandFromGsaSignalId(const QString &constellation, int signalId);
    static double parseLatLon(const QString &value, const QString &hemisphere);
    static QTime parseUtcTime(const QString &timeField);
    static QDateTime mergeUtcDateTime(const QString &dateField, const QString &timeField);
    static QString fixTypeName(int quality);
    void updateUsedSatellites(const QString &constellation, const QString &band, int signalId, const QSet<int> &svids);
    bool isSatelliteUsed(const QString &constellation, const QString &band, int signalId, int svid) const;
    QVariantList satelliteVariantList() const;
    static QString signalKey(int signalId);
    static QString usedKey(const QString &constellation, int signalId);

    QByteArray m_buffer;
    QHash<QString, SatelliteInfo> m_satellites;
    QHash<QString, QSet<int>> m_usedSatelliteIdsByConstellation;
    QHash<QString, QSet<QString>> m_seenGsvSignalsByConstellation;
    QHash<QString, QSet<QString>> m_updatedGsaSignalsByConstellation;
};

}  // namespace hdgnss
