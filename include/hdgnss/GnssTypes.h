#pragma once

#include <limits>

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>

namespace hdgnss {

enum class DataDirection {
    Rx,
    Tx
};

enum class ProtocolPluginKind {
    Nmea,
    Binary
};

struct RawLogEntry {
    QDateTime timestampUtc;
    DataDirection direction = DataDirection::Rx;
    QString transportName;
    QByteArray payload;
};

struct ProtocolMessage {
    QString protocol;
    QString messageName;
    QByteArray rawFrame;
    QString logDecodeText;
    QVariantMap fields;
};

struct CommandTemplate {
    QString name;
    QString group;
    QString protocol;
    QString description;
    QString payload;
    QString contentType;
};

struct ProtocolBuildMessage {
    QString name;
    QString protocol;
    QString description;
    QString defaultPayload;
};

struct ProtocolInfoField {
    QString id;
    QString label;
    QString valueKey;
    QString group;
    QString fallbackText = QStringLiteral("--");
    QString unit;
    int precision = -1;
    bool emphasize = false;
};

struct ProtocolInfoPanel {
    QString id;
    QString protocol;
    QString tabTitle;
    QString title;
    QList<ProtocolInfoField> fields;
    bool replaceOnMessage = true;
};

struct GnssLocation {
    bool validFix = false;
    QDateTime utcTime;
    double latitude = std::numeric_limits<double>::quiet_NaN();
    double longitude = std::numeric_limits<double>::quiet_NaN();
    double altitudeMeters = std::numeric_limits<double>::quiet_NaN();
    double undulationMeters = std::numeric_limits<double>::quiet_NaN();
    double speedMps = std::numeric_limits<double>::quiet_NaN();
    double courseDegrees = std::numeric_limits<double>::quiet_NaN();
    double magneticVariationDegrees = std::numeric_limits<double>::quiet_NaN();
    double differentialAgeSeconds = std::numeric_limits<double>::quiet_NaN();
    double hdop = std::numeric_limits<double>::quiet_NaN();
    double vdop = std::numeric_limits<double>::quiet_NaN();
    double pdop = std::numeric_limits<double>::quiet_NaN();
    double gstRms = std::numeric_limits<double>::quiet_NaN();
    double latitudeSigma = std::numeric_limits<double>::quiet_NaN();
    double longitudeSigma = std::numeric_limits<double>::quiet_NaN();
    double altitudeSigma = std::numeric_limits<double>::quiet_NaN();
    QString fixType;
    QString mode;
    QString status;
    int quality = 0;
    int satellitesUsed = 0;
    int satellitesInView = 0;
};

struct SatelliteInfo {
    QString key;
    QString constellation;
    QString band = QStringLiteral("L1");
    int signalId = 0;
    int svid = 0;
    int azimuth = 0;
    int elevation = 0;
    int cn0 = 0;
    bool usedInFix = false;
};

inline bool satelliteHasVisibleSignal(const SatelliteInfo &sat) {
    return sat.cn0 > 0;
}

inline QString satelliteSignalGroup(const SatelliteInfo &sat) {
    const QString bandName = sat.band.trimmed().toUpper();
    const QString constellationName = sat.constellation.trimmed().toUpper();

    if (bandName.startsWith(QStringLiteral("B2"))
        || (bandName == QStringLiteral("L5") && constellationName == QStringLiteral("BEIDOU"))) {
        return QStringLiteral("B2");
    }
    if (bandName.startsWith(QStringLiteral("E5"))
        || (bandName == QStringLiteral("L5") && constellationName == QStringLiteral("GALILEO"))) {
        return QStringLiteral("E5");
    }
    if (bandName == QStringLiteral("L5") || bandName.startsWith(QStringLiteral("L5"))) {
        return QStringLiteral("L5");
    }
    if (bandName == QStringLiteral("B1") || bandName.startsWith(QStringLiteral("B1"))
        || (bandName == QStringLiteral("L1") && constellationName == QStringLiteral("BEIDOU"))) {
        return QStringLiteral("B1");
    }
    if (bandName == QStringLiteral("E1") || bandName.startsWith(QStringLiteral("E1"))
        || (bandName == QStringLiteral("L1") && constellationName == QStringLiteral("GALILEO"))) {
        return QStringLiteral("E1");
    }
    if (bandName == QStringLiteral("L1C") || bandName.startsWith(QStringLiteral("L1C"))
        || (bandName == QStringLiteral("L1")
            && constellationName == QStringLiteral("QZSS")
            && (sat.signalId == 2 || sat.signalId == 3))) {
        return QStringLiteral("L1C");
    }
    if (bandName == QStringLiteral("L1") || bandName.startsWith(QStringLiteral("L1"))) {
        return QStringLiteral("L1");
    }
    if (bandName == QStringLiteral("L2") || bandName.startsWith(QStringLiteral("L2"))
        || bandName == QStringLiteral("G2") || bandName.startsWith(QStringLiteral("G2"))) {
        return QStringLiteral("L2");
    }
    if (bandName == QStringLiteral("L6") || bandName.startsWith(QStringLiteral("L6"))
        || bandName.startsWith(QStringLiteral("E6"))
        || bandName.startsWith(QStringLiteral("B3"))
        || bandName.startsWith(QStringLiteral("G3"))) {
        return QStringLiteral("B3");
    }
    return QStringLiteral("UN");
}

inline bool satelliteMatchesSignalTab(const SatelliteInfo &sat, const QString &tabBand) {
    const QString signalGroup = satelliteSignalGroup(sat);
    const QString tab = tabBand.trimmed().toUpper();
    if (tab == QStringLiteral("L1")) {
        return signalGroup == QStringLiteral("L1")
            || signalGroup == QStringLiteral("B1")
            || signalGroup == QStringLiteral("E1")
            || signalGroup == QStringLiteral("L1C");
    }
    if (tab == QStringLiteral("L5")) {
        return signalGroup == QStringLiteral("L5")
            || signalGroup == QStringLiteral("B2")
            || signalGroup == QStringLiteral("E5");
    }
    if (tab == QStringLiteral("L6")) {
        return signalGroup == QStringLiteral("B3");
    }
    return signalGroup == tab;
}

}  // namespace hdgnss
