#include "NmeaProtocolPlugin.h"

#include <cmath>
#include <limits>
#include <QDate>
#include <QRegularExpression>
#include <QTimeZone>

namespace hdgnss {

namespace {

double parseOptionalDouble(const QByteArray &field) {
    if (field.isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return field.toDouble();
}

int parseNmeaIdField(const QString &field) {
    if (field.isEmpty()) {
        return 0;
    }

    bool ok = false;
    const int hexValue = field.toInt(&ok, 16);
    if (ok) {
        return hexValue;
    }

    const int decimalValue = field.toInt(&ok, 10);
    return ok ? decimalValue : 0;
}

QString exactSignalChannelName(const QString &constellation, int signalId) {
    if (signalId <= 0) {
        return {};
    }

    if (constellation == QStringLiteral("GPS")) {
        switch (signalId) {
        case 1: return QStringLiteral("L1 C/A");
        case 2: return QStringLiteral("L1 P(Y)");
        case 3: return QStringLiteral("L1 M");
        case 4: return QStringLiteral("L2 P(Y)");
        case 5: return QStringLiteral("L2C-M");
        case 6: return QStringLiteral("L2C-L");
        case 7: return QStringLiteral("L5-I");
        case 8: return QStringLiteral("L5-Q");
        default: return {};
        }
    }

    if (constellation == QStringLiteral("GLONASS")) {
        switch (signalId) {
        case 1: return QStringLiteral("L1 C/A");
        case 2: return QStringLiteral("L1 P");
        case 3: return QStringLiteral("L2 C/A");
        case 4: return QStringLiteral("L2 P");
        default: return {};
        }
    }

    if (constellation == QStringLiteral("GALILEO")) {
        switch (signalId) {
        case 1: return QStringLiteral("E5a");
        case 2: return QStringLiteral("E5b");
        case 3: return QStringLiteral("E5a+b");
        case 4: return QStringLiteral("E6-A");
        case 5: return QStringLiteral("E6-BC");
        case 6: return QStringLiteral("L1-A");
        case 7: return QStringLiteral("L1-BC");
        default: return {};
        }
    }

    if (constellation == QStringLiteral("BEIDOU")) {
        switch (signalId) {
        case 1: return QStringLiteral("B1I");
        case 2: return QStringLiteral("B1Q");
        case 3: return QStringLiteral("B1C");
        case 4: return QStringLiteral("B1A");
        case 5: return QStringLiteral("B2-a");
        case 6: return QStringLiteral("B2-b");
        case 7: return QStringLiteral("B2 a+b");
        case 8: return QStringLiteral("B3I");
        case 9: return QStringLiteral("B3Q");
        case 10: return QStringLiteral("B3A");
        case 11: return QStringLiteral("B2I");
        case 12: return QStringLiteral("B2Q");
        default: return {};
        }
    }

    if (constellation == QStringLiteral("QZSS")) {
        switch (signalId) {
        case 1: return QStringLiteral("L1 C/A");
        case 2: return QStringLiteral("L1C (D)");
        case 3: return QStringLiteral("L1C (P)");
        case 4: return QStringLiteral("LIS");
        case 5: return QStringLiteral("L2C-M");
        case 6: return QStringLiteral("L2C-L");
        case 7: return QStringLiteral("L5-I");
        case 8: return QStringLiteral("L5-Q");
        case 9: return QStringLiteral("L6D");
        case 10: return QStringLiteral("L6E");
        default: return {};
        }
    }

    if (constellation == QStringLiteral("NAVIC")) {
        switch (signalId) {
        case 1: return QStringLiteral("L5-SPS");
        case 2: return QStringLiteral("S-SPS");
        case 3: return QStringLiteral("L5-RS");
        case 4: return QStringLiteral("S-RS");
        case 5: return QStringLiteral("L1-SPS");
        default: return {};
        }
    }

    return {};
}

QString coarseBandFromSignalChannel(const QString &constellation, const QString &channelName) {
    if (channelName.isEmpty()) {
        return QStringLiteral("L1");
    }

    if (channelName.startsWith(QStringLiteral("L1"))
        || channelName == QStringLiteral("LIS")
        || channelName.startsWith(QStringLiteral("B1"))) {
        return QStringLiteral("L1");
    }

    if (channelName.startsWith(QStringLiteral("L2"))) {
        return QStringLiteral("L2");
    }

    if (channelName.startsWith(QStringLiteral("L5"))
        || channelName.startsWith(QStringLiteral("E5"))
        || channelName.startsWith(QStringLiteral("B2"))) {
        return QStringLiteral("L5");
    }

    if (channelName.startsWith(QStringLiteral("L6"))
        || channelName.startsWith(QStringLiteral("E6"))
        || channelName.startsWith(QStringLiteral("B3"))) {
        return QStringLiteral("L6");
    }

    if (constellation == QStringLiteral("NAVIC") && channelName.startsWith(QStringLiteral("S-"))) {
        // The UI only exposes L-band buckets. Keep NavIC S-band in the auxiliary high-band tab.
        return QStringLiteral("L6");
    }

    return QStringLiteral("L1");
}

QString inferConstellationFromSvid(int svid) {
    if (svid >= 1 && svid <= 32) return QStringLiteral("GPS");
    if (svid >= 120 && svid <= 158) return QStringLiteral("SBAS");
    if (svid >= 65 && svid <= 96) return QStringLiteral("GLONASS");
    if (svid >= 193 && svid <= 200) return QStringLiteral("QZSS");
    if (svid >= 201 && svid <= 237) return QStringLiteral("BEIDOU");
    if (svid >= 301 && svid <= 336) return QStringLiteral("GALILEO");
    if (svid >= 401 && svid <= 414) return QStringLiteral("NAVIC");
    return QStringLiteral("GNSS");
}

struct NormalizedNmeaSatelliteId {
    QString constellation;
    int svid = 0;
};

NormalizedNmeaSatelliteId normalizeNmeaSatellite(const QString &talkerConstellation, int rawSvid) {
    if ((talkerConstellation == QStringLiteral("GPS") || talkerConstellation == QStringLiteral("GNSS"))
        && rawSvid >= 33 && rawSvid <= 64) {
        return {QStringLiteral("SBAS"), rawSvid + 87};
    }

    const QString normalizedConstellation = talkerConstellation == QStringLiteral("GNSS")
        ? inferConstellationFromSvid(rawSvid)
        : talkerConstellation;
    return {normalizedConstellation, rawSvid};
}

}  // namespace

QString NmeaProtocolPlugin::protocolName() const {
    return QStringLiteral("NMEA");
}

QList<ProtocolPluginKind> NmeaProtocolPlugin::pluginKinds() const {
    return {ProtocolPluginKind::Nmea};
}

int NmeaProtocolPlugin::probe(const QByteArray &sample) const {
    return sample.contains('$') ? 80 : 0;
}

QList<ProtocolMessage> NmeaProtocolPlugin::feed(const QByteArray &bytes) {
    m_buffer.append(bytes);
    QList<ProtocolMessage> messages;

    while (true) {
        int start = m_buffer.indexOf('$');
        if (start < 0) {
            m_buffer.clear();
            break;
        }
        if (start > 0) {
            m_buffer.remove(0, start);
        }

        if (m_buffer.size() < 7) {
            break;
        }

        const int star = m_buffer.indexOf('*', 1);
        if (star < 0) {
            const int nextStart = m_buffer.indexOf('$', 1);
            if (nextStart > 0) {
                m_buffer.remove(0, nextStart);
                continue;
            }
            break;
        }

        if (star + 2 >= m_buffer.size()) {
            break;
        }

        if (!isHexByte(m_buffer.at(star + 1), m_buffer.at(star + 2))) {
            m_buffer.remove(0, 1);
            continue;
        }

        int sentenceEnd = star + 3;
        while (sentenceEnd < m_buffer.size()
               && (m_buffer.at(sentenceEnd) == '\r' || m_buffer.at(sentenceEnd) == '\n')) {
            ++sentenceEnd;
        }

        const QByteArray sentence = m_buffer.left(star + 3);
        m_buffer.remove(0, sentenceEnd);
        if (!sentence.isEmpty()) {
            const QList<ProtocolMessage> decoded = parseSentence(sentence);
            messages.append(decoded);
        }
    }

    return messages;
}

QByteArray NmeaProtocolPlugin::encode(const QVariantMap &command) const {
    QByteArray text = command.value(QStringLiteral("text")).toByteArray();
    if (!text.startsWith('$')) {
        text.prepend('$');
    }
    if (!text.endsWith("\r\n")) {
        text.append("\r\n");
    }
    return text;
}

QList<CommandTemplate> NmeaProtocolPlugin::commandTemplates() const {
    return {
        {QStringLiteral("Poll GGA"), QStringLiteral("NMEA"), QStringLiteral("NMEA"), QStringLiteral("Request GGA sentence"), QStringLiteral("$EIGNQ,GGA*56\r\n"), QStringLiteral("ASCII")}
    };
}

bool NmeaProtocolPlugin::supportsFullDecode() const {
    return true;
}

bool NmeaProtocolPlugin::validateChecksum(const QByteArray &sentence) {
    if (!sentence.startsWith('$')) {
        return false;
    }
    const int star = sentence.indexOf('*');
    if (star < 0 || star + 2 >= sentence.size()) {
        return false;
    }
    bool ok = false;
    const quint8 expected = sentence.mid(star + 1, 2).toUInt(&ok, 16);
    if (!ok) {
        return false;
    }
    return checksumForBody(sentence.mid(1, star - 1)) == expected;
}

void NmeaProtocolPlugin::resetState() {
    m_buffer.clear();
    m_satellites.clear();
    m_usedSatelliteIdsByConstellation.clear();
    m_seenGsvSignalsByConstellation.clear();
    m_updatedGsaSignalsByConstellation.clear();
}

quint8 NmeaProtocolPlugin::checksumForBody(const QByteArray &body) {
    quint8 checksum = 0;
    for (const char ch : body) {
        checksum ^= static_cast<quint8>(ch);
    }
    return checksum;
}

bool NmeaProtocolPlugin::isHexByte(char high, char low) {
    auto isHexDigit = [](char ch) {
        return (ch >= '0' && ch <= '9')
            || (ch >= 'a' && ch <= 'f')
            || (ch >= 'A' && ch <= 'F');
    };
    return isHexDigit(high) && isHexDigit(low);
}

QList<ProtocolMessage> NmeaProtocolPlugin::parseSentence(const QByteArray &sentence) {
    QList<ProtocolMessage> out;
    if (!validateChecksum(sentence)) {
        return out;
    }
    const int star = sentence.indexOf('*');
    const QList<QByteArray> rawFields = sentence.mid(1, star - 1).split(',');
    if (rawFields.isEmpty()) {
        return out;
    }

    const QString talkerType = QString::fromLatin1(rawFields.first());
    if (talkerType.size() < 5) {
        return out;
    }
    const QString talker = talkerType.left(2);
    const bool proprietary = talkerType.startsWith(QLatin1Char('P'));
    const QString type = proprietary && rawFields.size() > 1
        ? QString::fromLatin1(rawFields.value(1))
        : sentenceType(rawFields);

    ProtocolMessage message;
    message.protocol = protocolName();
    message.messageName = type;
    message.rawFrame = sentence;

    if (type == QStringLiteral("GGA")) {
        const QString lat = QString::fromLatin1(rawFields.value(2));
        const QString latHem = QString::fromLatin1(rawFields.value(3));
        const QString lon = QString::fromLatin1(rawFields.value(4));
        const QString lonHem = QString::fromLatin1(rawFields.value(5));
        const int quality = rawFields.value(6).toInt();
        message.fields = {
            {QStringLiteral("utcTime"), mergeUtcDateTime(QString(), QString::fromLatin1(rawFields.value(1)))},
            {QStringLiteral("latitude"), parseLatLon(lat, latHem)},
            {QStringLiteral("longitude"), parseLatLon(lon, lonHem)},
            {QStringLiteral("quality"), quality},
            {QStringLiteral("altitudeMeters"), parseOptionalDouble(rawFields.value(9))},
            {QStringLiteral("undulationMeters"), parseOptionalDouble(rawFields.value(11))},
            {QStringLiteral("satellitesUsed"), rawFields.value(7).toInt()},
            {QStringLiteral("hdop"), parseOptionalDouble(rawFields.value(8))},
            {QStringLiteral("differentialAgeSeconds"), parseOptionalDouble(rawFields.value(13))},
            {QStringLiteral("fixType"), fixTypeName(quality)},
            {QStringLiteral("validFix"), quality > 0}
        };
        out.append(message);
    } else if (type == QStringLiteral("RMC")) {
        const QString dateField = QString::fromLatin1(rawFields.value(9));
        const QString timeField = QString::fromLatin1(rawFields.value(1));
        const double magneticVariation = parseOptionalDouble(rawFields.value(10));
        double signedMagneticVariation = magneticVariation;
        const QString variationDirection = QString::fromLatin1(rawFields.value(11)).trimmed().toUpper();
        if (!std::isnan(signedMagneticVariation) && variationDirection == QStringLiteral("W")) {
            signedMagneticVariation *= -1.0;
        }
        const double speedKnots = parseOptionalDouble(rawFields.value(7));
        message.fields = {
            {QStringLiteral("latitude"), parseLatLon(QString::fromLatin1(rawFields.value(3)), QString::fromLatin1(rawFields.value(4)))},
            {QStringLiteral("longitude"), parseLatLon(QString::fromLatin1(rawFields.value(5)), QString::fromLatin1(rawFields.value(6)))},
            {QStringLiteral("speedMps"), std::isnan(speedKnots) ? speedKnots : speedKnots * 0.514444},
            {QStringLiteral("courseDegrees"), parseOptionalDouble(rawFields.value(8))},
            {QStringLiteral("utcTime"), mergeUtcDateTime(dateField, timeField)},
            {QStringLiteral("status"), QString::fromLatin1(rawFields.value(2))},
            {QStringLiteral("magneticVariationDegrees"), signedMagneticVariation},
            {QStringLiteral("mode"), QString::fromLatin1(rawFields.value(12))},
            {QStringLiteral("validFix"), rawFields.value(2) == "A"}
        };
        out.append(message);
    } else if (type == QStringLiteral("VTG")) {
        const double speedKph = parseOptionalDouble(rawFields.value(7));
        message.fields = {
            {QStringLiteral("courseDegrees"), parseOptionalDouble(rawFields.value(1))},
            {QStringLiteral("speedMps"), std::isnan(speedKph) ? speedKph : speedKph / 3.6}
        };
        out.append(message);
    } else if (type == QStringLiteral("GLL")) {
        message.fields = {
            {QStringLiteral("latitude"), parseLatLon(QString::fromLatin1(rawFields.value(1)), QString::fromLatin1(rawFields.value(2)))},
            {QStringLiteral("longitude"), parseLatLon(QString::fromLatin1(rawFields.value(3)), QString::fromLatin1(rawFields.value(4)))},
            {QStringLiteral("utcTime"), mergeUtcDateTime(QString(), QString::fromLatin1(rawFields.value(5)))},
            {QStringLiteral("validFix"), rawFields.value(6) == "A"}
        };
        out.append(message);
    } else if (type == QStringLiteral("ZDA")) {
        const QDate date(rawFields.value(4).toInt(), rawFields.value(3).toInt(), rawFields.value(2).toInt());
        const QTime time = parseUtcTime(QString::fromLatin1(rawFields.value(1)));
        message.fields = {
            {QStringLiteral("utcTime"), QDateTime(date, time, QTimeZone::UTC)}
        };
        out.append(message);
    } else if (type == QStringLiteral("GST")) {
        message.fields = {
            {QStringLiteral("gstRms"), rawFields.value(2).toDouble()},
            {QStringLiteral("latitudeSigma"), rawFields.value(6).toDouble()},
            {QStringLiteral("longitudeSigma"), rawFields.value(7).toDouble()},
            {QStringLiteral("altitudeSigma"), rawFields.value(8).toDouble()}
        };
        out.append(message);
    } else if (type == QStringLiteral("GSA")) {
        QString gsaConstellation = constellationFromTalker(talker);
        QString gsaBand = QStringLiteral("L1");
        int svidStart = 3;
        int pdopIndex = 15;
        int hdopIndex = 16;
        int vdopIndex = 17;
        int systemIdIndex = -1;
        int signalIdIndex = -1;

        if (proprietary && talkerType == QStringLiteral("PSSGR")) {
            svidStart = 4;
            pdopIndex = 16;
            hdopIndex = 17;
            vdopIndex = 18;
            systemIdIndex = 19;
            signalIdIndex = 20;
        } else if (talker == QStringLiteral("GN") && rawFields.size() > 18) {
            systemIdIndex = 18;
        } else if (rawFields.size() > 18) {
            signalIdIndex = 18;
        }

        if (systemIdIndex >= 0 && systemIdIndex < rawFields.size()) {
            const QString fromSystem = constellationFromSystemId(parseNmeaIdField(rawFields.value(systemIdIndex)));
            if (!fromSystem.isEmpty()) {
                gsaConstellation = fromSystem;
            }
        }
        if (signalIdIndex >= 0 && signalIdIndex < rawFields.size()) {
            const QString fromSignal = bandFromGsaSignalId(gsaConstellation, parseNmeaIdField(rawFields.value(signalIdIndex)));
            if (!fromSignal.isEmpty()) {
                gsaBand = fromSignal;
            }
        }

        QHash<QString, QSet<int>> usedSvidsByConstellation;
        for (int i = svidStart; i < svidStart + 12 && i < rawFields.size(); ++i) {
            const int svid = rawFields.value(i).toInt();
            if (svid > 0) {
                const NormalizedNmeaSatelliteId normalized = normalizeNmeaSatellite(gsaConstellation, svid);
                usedSvidsByConstellation[normalized.constellation].insert(normalized.svid);
            }
        }
        const int gsaSignalId = signalIdIndex >= 0 && signalIdIndex < rawFields.size()
            ? parseNmeaIdField(rawFields.value(signalIdIndex))
            : 0;
        if (usedSvidsByConstellation.isEmpty()) {
            updateUsedSatellites(gsaConstellation, gsaBand, gsaSignalId, {});
        } else {
            for (auto it = usedSvidsByConstellation.cbegin(); it != usedSvidsByConstellation.cend(); ++it) {
                updateUsedSatellites(it.key(), gsaBand, gsaSignalId, it.value());
            }
        }
        message.fields = {
            {QStringLiteral("fixType"), rawFields.value(2).toInt()},
            {QStringLiteral("pdop"), parseOptionalDouble(rawFields.value(pdopIndex))},
            {QStringLiteral("hdop"), parseOptionalDouble(rawFields.value(hdopIndex))},
            {QStringLiteral("vdop"), parseOptionalDouble(rawFields.value(vdopIndex))},
            {QStringLiteral("satellites"), satelliteVariantList()}
        };
        out.append(message);
    } else if (type == QStringLiteral("GSV")) {
        const QString talkerConstellation = constellationFromTalker(talker);
        const int sentenceNumber = rawFields.value(2).toInt();
        const int payloadFieldCount = rawFields.size() - 4;
        const int signalId = (payloadFieldCount > 0 && payloadFieldCount % 4 == 1)
            ? parseNmeaIdField(rawFields.value(rawFields.size() - 1))
            : 0;
        const QString signalBand = bandFromGsvSignalId(talkerConstellation, signalId);
        if (sentenceNumber <= 1) {
            m_seenGsvSignalsByConstellation.remove(talkerConstellation);
            m_updatedGsaSignalsByConstellation.remove(talkerConstellation);
        }
        const QString currentSignalKey = signalKey(signalId);
        QSet<QString> &seenSignals = m_seenGsvSignalsByConstellation[talkerConstellation];
        if (!seenSignals.contains(currentSignalKey)) {
            seenSignals.insert(currentSignalKey);
            for (auto it = m_satellites.begin(); it != m_satellites.end();) {
                const bool sameSignal = it->signalId == signalId;
                const bool sameConstellation = talkerConstellation == QStringLiteral("GNSS")
                    || it->constellation == talkerConstellation;
                if (sameSignal && sameConstellation) {
                    it = m_satellites.erase(it);
                    continue;
                }
                ++it;
            }
        }
        for (int base = 4; base + 3 < rawFields.size(); base += 4) {
            const int rawSvid = rawFields.value(base).toInt();
            if (rawSvid <= 0) {
                continue;
            }
            const NormalizedNmeaSatelliteId normalized = normalizeNmeaSatellite(talkerConstellation, rawSvid);
            SatelliteInfo sat;
            sat.constellation = normalized.constellation;
            sat.band = bandFromGsvSignalId(sat.constellation, signalId);
            sat.signalId = signalId;
            sat.svid = normalized.svid;
            sat.elevation = rawFields.value(base + 1).toInt();
            sat.azimuth = rawFields.value(base + 2).toInt();
            sat.cn0 = rawFields.value(base + 3).toInt();
            sat.usedInFix = isSatelliteUsed(sat.constellation, sat.band, sat.signalId, sat.svid);
            sat.key = QStringLiteral("%1-%2-%3")
                .arg(sat.constellation)
                .arg(sat.signalId)
                .arg(sat.svid);
            m_satellites.insert(sat.key, sat);
        }
        message.fields = {
            {QStringLiteral("satellites"), satelliteVariantList()},
            {QStringLiteral("satellitesInView"), rawFields.value(3).toInt()}
        };
        out.append(message);
    }

    return out;
}

QString NmeaProtocolPlugin::sentenceType(const QList<QByteArray> &fields) {
    return QString::fromLatin1(fields.first().right(3));
}

QString NmeaProtocolPlugin::constellationFromTalker(const QString &talker) {
    if (talker == "GP") return QStringLiteral("GPS");
    if (talker == "GL") return QStringLiteral("GLONASS");
    if (talker == "GA") return QStringLiteral("GALILEO");
    if (talker == "BD" || talker == "GB") return QStringLiteral("BEIDOU");
    if (talker == "QZ" || talker == "GQ") return QStringLiteral("QZSS");
    if (talker == "GI" || talker == "NC") return QStringLiteral("NAVIC");
    return QStringLiteral("GNSS");
}

QString NmeaProtocolPlugin::constellationFromSystemId(int systemId) {
    switch (systemId) {
    case 1: return QStringLiteral("GPS");
    case 2: return QStringLiteral("GLONASS");
    case 3: return QStringLiteral("GALILEO");
    case 4: return QStringLiteral("BEIDOU");
    case 5: return QStringLiteral("QZSS");
    case 6: return QStringLiteral("NAVIC");
    default: return QString();
    }
}

QString NmeaProtocolPlugin::bandFromGsvSignalId(const QString &constellation, int signalId) {
    if (signalId <= 0) {
        return QStringLiteral("L1");
    }
    return coarseBandFromSignalChannel(constellation, exactSignalChannelName(constellation, signalId));
}

QString NmeaProtocolPlugin::bandFromGsaSignalId(const QString &constellation, int signalId) {
    if (signalId <= 0) {
        return QStringLiteral("L1");
    }
    return bandFromGsvSignalId(constellation, signalId);
}

double NmeaProtocolPlugin::parseLatLon(const QString &value, const QString &hemisphere) {
    if (value.isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double numeric = value.toDouble();
    const double degrees = std::floor(numeric / 100.0);
    const double minutes = numeric - degrees * 100.0;
    double decimal = degrees + minutes / 60.0;
    if (hemisphere == "S" || hemisphere == "W") {
        decimal *= -1.0;
    }
    return decimal;
}

QTime NmeaProtocolPlugin::parseUtcTime(const QString &timeField) {
    const QString trimmed = timeField.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QStringList parts = trimmed.split(QLatin1Char('.'));
    const QString hhmmss = parts.value(0).left(6);
    QTime time = QTime::fromString(hhmmss, QStringLiteral("hhmmss"));
    if (!time.isValid()) {
        return {};
    }

    const QString fraction = parts.value(1);
    if (fraction.isEmpty()) {
        return time;
    }

    bool ok = false;
    const int milliseconds = fraction.left(3).leftJustified(3, QLatin1Char('0')).toInt(&ok);
    return ok ? time.addMSecs(milliseconds) : QTime{};
}

QDateTime NmeaProtocolPlugin::mergeUtcDateTime(const QString &dateField, const QString &timeField) {
    const QTime time = parseUtcTime(timeField);
    QDate date = QDateTime::currentDateTimeUtc().date();
    if (dateField.size() == 6) {
        date = QDate(2000 + dateField.mid(4, 2).toInt(), dateField.mid(2, 2).toInt(), dateField.mid(0, 2).toInt());
    }
    if (!time.isValid()) {
        return {};
    }
    return QDateTime(date, time, QTimeZone::UTC);
}

QString NmeaProtocolPlugin::fixTypeName(int quality) {
    switch (quality) {
    case 1: return QStringLiteral("Single Point");
    case 2: return QStringLiteral("DGPS");
    case 4: return QStringLiteral("RTK Fixed");
    case 5: return QStringLiteral("RTK Float");
    case 6: return QStringLiteral("Dead Reckoning");
    case 7: return QStringLiteral("Manual Input");
    case 8: return QStringLiteral("Simulator");
    case 9: return QStringLiteral("SBAS");
    default: return QStringLiteral("No Fix");
    }
}

void NmeaProtocolPlugin::updateUsedSatellites(const QString &constellation, const QString &band, int signalId, const QSet<int> &svids) {
    if (constellation == QStringLiteral("GNSS") || constellation.isEmpty()) {
        QHash<QString, QSet<int>> regrouped;
        for (int svid : svids) {
            regrouped[inferConstellationFromSvid(svid)].insert(svid);
        }
        for (auto it = regrouped.cbegin(); it != regrouped.cend(); ++it) {
            updateUsedSatellites(it.key(), band, signalId, it.value());
        }
        return;
    }

    const QString normalizedBand = band.isEmpty() ? QStringLiteral("L1") : band;
    const QString currentSignalKey = signalKey(signalId);
    QSet<QString> &updatedSignals = m_updatedGsaSignalsByConstellation[constellation];
    const QString key = usedKey(constellation, signalId);
    QSet<int> mergedSvids = svids;
    if (updatedSignals.contains(currentSignalKey) && !mergedSvids.isEmpty()) {
        mergedSvids.unite(m_usedSatelliteIdsByConstellation.value(key));
    }
    updatedSignals.insert(currentSignalKey);
    m_usedSatelliteIdsByConstellation.insert(key, mergedSvids);
    for (auto it = m_satellites.begin(); it != m_satellites.end(); ++it) {
        if (it->constellation == constellation && it->signalId == signalId) {
            it->usedInFix = mergedSvids.contains(it->svid);
        }
    }
    // When a signal-specific GSA arrives but the corresponding GSV reported satellites
    // without any signal ID (signalId == 0), update those signal-agnostic entries too —
    // but only when there is no dedicated signal-agnostic GSA for this constellation,
    // so that an existing "all-signals" GSA bucket is not accidentally overwritten.
    if (signalId > 0 && !m_usedSatelliteIdsByConstellation.contains(usedKey(constellation, 0))) {
        for (auto it = m_satellites.begin(); it != m_satellites.end(); ++it) {
            if (it->constellation == constellation && it->signalId == 0) {
                it->usedInFix = mergedSvids.contains(it->svid);
            }
        }
    }
}

bool NmeaProtocolPlugin::isSatelliteUsed(const QString &constellation, const QString &band, int signalId, int svid) const {
    const QString exactKey = usedKey(constellation, signalId);
    if (m_usedSatelliteIdsByConstellation.contains(exactKey)) {
        return m_usedSatelliteIdsByConstellation.value(exactKey).contains(svid);
    }
    if (signalId != 0) {
        return m_usedSatelliteIdsByConstellation.value(usedKey(constellation, 0)).contains(svid);
    }
    return false;
}

QVariantList NmeaProtocolPlugin::satelliteVariantList() const {
    QVariantList satellites;
    for (const SatelliteInfo &sat : std::as_const(m_satellites)) {
        satellites.append(QVariantMap{
            {QStringLiteral("key"), sat.key},
            {QStringLiteral("constellation"), sat.constellation},
            {QStringLiteral("band"), sat.band},
            {QStringLiteral("signalId"), sat.signalId},
            {QStringLiteral("svid"), sat.svid},
            {QStringLiteral("azimuth"), sat.azimuth},
            {QStringLiteral("elevation"), sat.elevation},
            {QStringLiteral("cn0"), sat.cn0},
            {QStringLiteral("usedInFix"), sat.usedInFix}
        });
    }
    return satellites;
}

QString NmeaProtocolPlugin::signalKey(int signalId) {
    return QString::number(signalId);
}

QString NmeaProtocolPlugin::usedKey(const QString &constellation, int signalId) {
    return QStringLiteral("%1|%2").arg(constellation, signalKey(signalId));
}

}  // namespace hdgnss
