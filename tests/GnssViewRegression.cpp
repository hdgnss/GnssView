#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointF>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QStringList>
#include <QVariantList>

#include <cmath>
#include <cstdlib>
#include <iostream>

#include "src/core/AppController.h"
#include "src/core/AppSettings.h"
#include "src/core/UpdateChecker.h"
#include "src/models/CommandButtonModel.h"
#include "src/tec/TecMapOverlayModel.h"
#include "src/models/DeviationMapModel.h"
#include "src/models/SatelliteModel.h"
#include "src/models/SignalModel.h"
#include "src/protocols/NmeaProtocolPlugin.h"
#include "src/storage/RawRecorder.h"
#include "src/tec/TecMapRenderer.h"

namespace {

using hdgnss::AppController;
using hdgnss::AppSettings;
using hdgnss::CommandButtonModel;
using hdgnss::DeviationMapModel;
using hdgnss::NmeaProtocolPlugin;
using hdgnss::ProtocolMessage;
using hdgnss::RawLogEntry;
using hdgnss::RawRecorder;
using hdgnss::SatelliteInfo;
using hdgnss::SatelliteModel;
using hdgnss::SignalModel;
using hdgnss::TecMapOverlayModel;
using hdgnss::UpdateChecker;

template <typename Predicate>
bool waitUntil(Predicate predicate, int timeoutMs) {
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return predicate();
}

bool expect(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

QByteArray withChecksum(const QByteArray &body) {
    const quint8 checksum = NmeaProtocolPlugin::checksumForBody(body);
    QByteArray sentence = "$" + body + "*";
    const QByteArray hex = QByteArray::number(checksum, 16).rightJustified(2, '0').toUpper();
    sentence += hex;
    sentence += "\r\n";
    return sentence;
}

bool applyNmeaSentence(NmeaProtocolPlugin &plugin, AppController &controller, const QByteArray &body) {
    const QList<ProtocolMessage> messages = plugin.feed(withChecksum(body));
    if (!expect(!messages.isEmpty(), "NMEA sentence should decode to at least one message")) {
        return false;
    }
    for (const ProtocolMessage &message : messages) {
        controller.regressionApplyProtocolMessage(message);
    }
    return true;
}

double clamp(double value, double minValue, double maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

QPointF projectedCanvasPointForTest(double width, double height, double lonDeg, double latDeg) {
    static constexpr double kWorldMinY = -91.296;
    static constexpr double kWorldHeight = 182.592;
    static constexpr double kRobinsonX[] = {
        1.0, 0.9986, 0.9954, 0.99, 0.9822, 0.973, 0.96, 0.9427, 0.9216,
        0.8962, 0.8679, 0.835, 0.7986, 0.7597, 0.7186, 0.6732, 0.6213,
        0.5722, 0.5322
    };
    static constexpr double kRobinsonY[] = {
        0.0, 0.062, 0.124, 0.186, 0.248, 0.31, 0.372, 0.434, 0.4958,
        0.5571, 0.6176, 0.6769, 0.7346, 0.7903, 0.8435, 0.8936, 0.9394,
        0.9761, 1.0
    };

    const double absLat = clamp(std::abs(latDeg), 0.0, 90.0);
    const int bandIndex = std::min<int>(std::size(kRobinsonX) - 2, static_cast<int>(std::floor(absLat / 5.0)));
    const double bandStart = bandIndex * 5.0;
    const double ratio = (absLat - bandStart) / 5.0;
    const double xFactor = lerp(kRobinsonX[bandIndex], kRobinsonX[bandIndex + 1], ratio);
    const double yFactor = lerp(kRobinsonY[bandIndex], kRobinsonY[bandIndex + 1], ratio);
    const double projectedX = lonDeg * xFactor;
    const double projectedY = (latDeg >= 0.0 ? -1.0 : 1.0) * 90.0 * 1.0144 * yFactor;
    return {
        (projectedX + 180.0) / 360.0 * width,
        (projectedY - kWorldMinY) / kWorldHeight * height
    };
}

bool expectMissingSatelliteDropsFromGsv() {
    NmeaProtocolPlugin plugin;

    const QList<ProtocolMessage> first = plugin.feed(withChecksum("GPGSV,1,1,02,01,40,083,42,02,17,273,38"));
    if (!expect(first.size() == 1, "first GSV sentence should decode exactly one message")) {
        return false;
    }
    const QVariantList firstSatellites = first.first().fields.value(QStringLiteral("satellites")).toList();
    if (!expect(firstSatellites.size() == 2, "first GSV cycle should expose two satellites")) {
        return false;
    }

    const QList<ProtocolMessage> second = plugin.feed(withChecksum("GPGSV,1,1,01,01,40,083,41"));
    if (!expect(second.size() == 1, "second GSV sentence should decode exactly one message")) {
        return false;
    }
    const QVariantList secondSatellites = second.first().fields.value(QStringLiteral("satellites")).toList();
    return expect(secondSatellites.size() == 1, "missing satellite should be removed on the next GSV cycle")
        && expect(secondSatellites.first().toMap().value(QStringLiteral("svid")).toInt() == 1, "remaining satellite should be PRN 1");
}

bool expectUpdateCheckerVersionComparison() {
    return expect(UpdateChecker::compareVersions(QStringLiteral("0.1.0"), QStringLiteral("0.1.0")) == 0,
                  "equal semantic versions should compare equal")
        && expect(UpdateChecker::compareVersions(QStringLiteral("v0.1.0"), QStringLiteral("0.1")) == 0,
                  "tag prefix and omitted patch version should not affect comparison")
        && expect(UpdateChecker::compareVersions(QStringLiteral("0.1.9"), QStringLiteral("0.2.0")) < 0,
                  "minor version increase should be detected")
        && expect(UpdateChecker::compareVersions(QStringLiteral("0.10.0"), QStringLiteral("0.2.0")) > 0,
                  "numeric comparison should not compare version parts lexically")
        && expect(UpdateChecker::compareVersions(QStringLiteral("1.2.3-beta.1"), QStringLiteral("1.2.3")) == 0,
                  "pre-release suffix should not make a release tag look older");
}

bool expectRawRecorderRequiresExplicitLogDirectory() {
    RawRecorder recorder;
    recorder.setRecordRawEnabled(true);
    recorder.startSession(QStringLiteral("session"),
                          QDateTime::fromString(QStringLiteral("2026-04-27T00:00:00Z"), Qt::ISODate),
                          QStringLiteral("unit"));

    RawLogEntry entry;
    entry.timestampUtc = QDateTime::fromString(QStringLiteral("2026-04-27T00:00:00Z"), Qt::ISODate);
    entry.payload = QByteArrayLiteral("abc");
    recorder.recordRaw(entry);

    if (!expect(recorder.sessionDirectory().isEmpty(),
                "raw recorder should not create a session without an explicit log directory")
        || !expect(recorder.bytesRecorded() == 0,
                   "raw recorder should not write bytes without an explicit log directory")) {
        return false;
    }

    QTemporaryDir tempDir;
    if (!expect(tempDir.isValid(), "temporary log directory should be valid")) {
        return false;
    }

    recorder.setLogRootDirectory(tempDir.path());
    recorder.startSession(QStringLiteral("session"),
                          QDateTime::fromString(QStringLiteral("2026-04-27T00:00:00Z"), Qt::ISODate),
                          QStringLiteral("unit"));
    recorder.recordRaw(entry);

    return expect(recorder.sessionDirectory().startsWith(tempDir.path()),
                  "raw recorder session should be created under the configured log directory")
        && expect(recorder.bytesRecorded() == 3,
                  "raw recorder should write after a log directory is configured");
}

bool expectBeidouGsaUsesRawPrnWithoutRemap() {
    NmeaProtocolPlugin plugin;

    const QList<ProtocolMessage> gsa = plugin.feed(withChecksum("BDGSA,A,3,208,,,,,,,,,,,,1.6,0.8,1.4"));
    if (!expect(gsa.size() == 1, "BDGSA sentence should decode exactly one message")) {
        return false;
    }

    const QList<ProtocolMessage> gsv = plugin.feed(withChecksum("BDGSV,1,1,02,208,40,083,42,08,17,273,38"));
    if (!expect(gsv.size() == 1, "BDGSV sentence should decode exactly one message")) {
        return false;
    }

    const QVariantList satellites = gsv.first().fields.value(QStringLiteral("satellites")).toList();
    if (!expect(satellites.size() == 2, "BDGSV sentence should expose two BeiDou satellites")) {
        return false;
    }

    QVariantMap sat208, sat8;
    for (const QVariant &v : satellites) {
        const QVariantMap sat = v.toMap();
        const int svid = sat.value(QStringLiteral("svid")).toInt();
        if (svid == 208) sat208 = sat;
        else if (svid == 8) sat8 = sat;
    }
    return expect(!sat208.isEmpty(),
                  "BeiDou GSV PRN should keep the raw received satellite id")
        && expect(!sat8.isEmpty(),
                  "non-matching BeiDou GSV PRN should also keep the raw received satellite id")
        && expect(sat208.value(QStringLiteral("constellation")).toString() == QStringLiteral("BEIDOU"),
                  "BDGSV constellation should stay BeiDou")
        && expect(sat208.value(QStringLiteral("band")).toString() == QStringLiteral("L1"),
                  "BDGSV without signal id should default to the L1/B1/L1C bucket")
        && expect(sat208.value(QStringLiteral("usedInFix")).toBool(),
                  "BeiDou satellite 208 should be marked used from the matching BDGSA sentence")
        && expect(!sat8.value(QStringLiteral("usedInFix")).toBool(),
                  "raw PRN matching should not mark a different received PRN as used");
}

bool expectGpsSbasPrnMapsToSbas() {
    NmeaProtocolPlugin plugin;

    const QList<ProtocolMessage> gsv = plugin.feed(withChecksum("GPGSV,1,1,02,47,41,228,29,19,59,135,34"));
    const QList<ProtocolMessage> gsa = plugin.feed(withChecksum("GPGSA,A,3,47,,,,,,,,,,,,1.6,0.8,1.3"));
    if (!expect(gsv.size() == 1 && gsa.size() == 1, "GPS SBAS test sentences should decode")) {
        return false;
    }

    const QVariantList satellites = gsa.first().fields.value(QStringLiteral("satellites")).toList();
    if (!expect(satellites.size() == 2, "GPS SBAS test should expose both the GPS and SBAS satellites")) {
        return false;
    }

    bool sawGps19 = false;
    bool sawSbas134 = false;
    bool sbasUsed = false;
    for (const QVariant &value : satellites) {
        const QVariantMap sat = value.toMap();
        const QString constellation = sat.value(QStringLiteral("constellation")).toString();
        const int svid = sat.value(QStringLiteral("svid")).toInt();
        if (constellation == QStringLiteral("GPS") && svid == 19) {
            sawGps19 = true;
        }
        if (constellation == QStringLiteral("SBAS") && svid == 134) {
            sawSbas134 = true;
            sbasUsed = sat.value(QStringLiteral("usedInFix")).toBool();
        }
    }

    return expect(sawGps19, "regular GPS PRNs should remain in GPS")
        && expect(sawSbas134, "GPS talker PRN 47 should normalize to SBAS PRN 134")
        && expect(sbasUsed, "normalized SBAS satellite should inherit used-in-fix state from GPGSA");
}

bool expectBandSpecificGsvClearsStaleSatellitesBetweenEpochs() {
    NmeaProtocolPlugin plugin;

    const QList<ProtocolMessage> firstEpochL1 = plugin.feed(withChecksum("BDGSV,6,1,01,208,80,127,39"));
    const QList<ProtocolMessage> firstEpochL5 = plugin.feed(withChecksum("BDGSV,6,5,02,238,70,258,17,240,80,127,17,5"));
    const QList<ProtocolMessage> secondEpochL1 = plugin.feed(withChecksum("BDGSV,6,1,01,208,80,127,39"));
    const QList<ProtocolMessage> secondEpochL5 = plugin.feed(withChecksum("BDGSV,6,5,01,219,65,063,47,5"));
    if (!expect(firstEpochL1.size() == 1 && firstEpochL5.size() == 1
                && secondEpochL1.size() == 1 && secondEpochL5.size() == 1,
                "band-specific GSV epoch test sentences should all decode")) {
        return false;
    }

    const QVariantList satellites = secondEpochL5.first().fields.value(QStringLiteral("satellites")).toList();
    bool sawC219L5 = false;
    bool sawC238L5 = false;
    bool sawC240L5 = false;
    for (const QVariant &value : satellites) {
        const QVariantMap sat = value.toMap();
        if (sat.value(QStringLiteral("constellation")).toString() != QStringLiteral("BEIDOU")
            || sat.value(QStringLiteral("band")).toString() != QStringLiteral("L5")) {
            continue;
        }
        const int svid = sat.value(QStringLiteral("svid")).toInt();
        if (svid == 219) sawC219L5 = true;
        if (svid == 238) sawC238L5 = true;
        if (svid == 240) sawC240L5 = true;
    }

    return expect(sawC219L5, "current-epoch BeiDou L5 satellite should remain visible")
        && expect(!sawC238L5 && !sawC240L5,
                  "stale BeiDou satellites from the previous L5 epoch should be cleared before new signal-id subsets");
}

bool expectSameBandGsaSentencesAccumulateUsedSatellites() {
    NmeaProtocolPlugin plugin;

    const QList<ProtocolMessage> gsv = plugin.feed(withChecksum("GPGSV,1,1,03,04,40,083,42,05,17,273,38,11,52,121,45"));
    if (!expect(gsv.size() == 1, "GPGSV sentence should decode exactly one message")) {
        return false;
    }

    const QList<ProtocolMessage> firstGsa = plugin.feed(withChecksum("GPGSA,A,3,05,,,,,,,,,,,,1.6,0.8,1.4"));
    const QList<ProtocolMessage> secondGsa = plugin.feed(withChecksum("GPGSA,A,3,04,11,,,,,,,,,,,1.6,0.8,1.4"));
    if (!expect(firstGsa.size() == 1 && secondGsa.size() == 1,
                "same-band GSA sentences should both decode")) {
        return false;
    }

    const QVariantList satellites = secondGsa.first().fields.value(QStringLiteral("satellites")).toList();
    if (!expect(satellites.size() == 3, "accumulated same-band GSA test should keep all visible GPS satellites")) {
        return false;
    }

    int usedCount = 0;
    for (const QVariant &value : satellites) {
        if (value.toMap().value(QStringLiteral("usedInFix")).toBool()) {
            ++usedCount;
        }
    }
    return expect(usedCount == 3, "same-band GSA sentences should accumulate used satellites instead of overwriting them");
}

bool expectGsaStateResetsBetweenEpochs() {
    NmeaProtocolPlugin plugin;

    const QList<ProtocolMessage> firstEpochGsv = plugin.feed(withChecksum("QZGSV,4,1,03,03,69,062,40,07,53,170,34,02,40,173,39"));
    const QList<ProtocolMessage> firstEpochGsa = plugin.feed(withChecksum("QZGSA,A,3,02,03,07,,,,,,,,,,1.6,0.8,1.4,9"));
    const QList<ProtocolMessage> secondEpochGsv = plugin.feed(withChecksum("QZGSV,4,1,03,03,69,062,40,07,53,170,34,02,40,173,39"));
    const QList<ProtocolMessage> secondEpochGsa = plugin.feed(withChecksum("QZGSA,A,3,02,03,,,,,,,,,,,1.6,0.8,1.4,9"));
    if (!expect(firstEpochGsv.size() == 1 && firstEpochGsa.size() == 1
                && secondEpochGsv.size() == 1 && secondEpochGsa.size() == 1,
                "QZSS GSA reset test sentences should all decode")) {
        return false;
    }

    const QVariantList satellites = secondEpochGsa.first().fields.value(QStringLiteral("satellites")).toList();
    bool j02Used = false;
    bool j03Used = false;
    bool j07Used = false;
    for (const QVariant &value : satellites) {
        const QVariantMap sat = value.toMap();
        if (sat.value(QStringLiteral("constellation")).toString() != QStringLiteral("QZSS")
            || sat.value(QStringLiteral("band")).toString() != QStringLiteral("L1")) {
            continue;
        }
        const int svid = sat.value(QStringLiteral("svid")).toInt();
        const bool used = sat.value(QStringLiteral("usedInFix")).toBool();
        if (svid == 2) j02Used = used;
        if (svid == 3) j03Used = used;
        if (svid == 7) j07Used = used;
    }

    return expect(j02Used && j03Used, "current-epoch QZSS used satellites should remain highlighted")
        && expect(!j07Used, "QZSS satellites dropped from the new epoch GSA should no longer stay highlighted");
}

bool expectBeidouL1UsageSurvivesB2GsaUpdate() {
    NmeaProtocolPlugin plugin;

    const QList<ProtocolMessage> l1Gsv = plugin.feed(withChecksum("BDGSV,1,1,03,203,40,083,42,206,17,273,38,219,52,121,45"));
    const QList<ProtocolMessage> l1Gsa = plugin.feed(withChecksum("BDGSA,A,3,203,206,219,,,,,,,,,1.6,0.8,1.4"));
    const QList<ProtocolMessage> b2Gsv = plugin.feed(withChecksum("BDGSV,1,1,01,219,52,121,44,5"));
    const QList<ProtocolMessage> b2Gsa = plugin.feed(withChecksum("BDGSA,A,3,219,,,,,,,,,,,,1.6,0.8,1.4,5"));
    if (!expect(l1Gsv.size() == 1 && l1Gsa.size() == 1 && b2Gsv.size() == 1 && b2Gsa.size() == 1,
                "BeiDou L1/B2 test sentences should all decode")) {
        return false;
    }

    const QVariantList satellites = b2Gsa.first().fields.value(QStringLiteral("satellites")).toList();
    bool c203L1Used = false;
    bool c206L1Used = false;
    bool c219L1Used = false;
    bool c219L5Used = false;
    for (const QVariant &value : satellites) {
        const QVariantMap sat = value.toMap();
        if (sat.value(QStringLiteral("constellation")).toString() != QStringLiteral("BEIDOU")) {
            continue;
        }
        const int svid = sat.value(QStringLiteral("svid")).toInt();
        const QString band = sat.value(QStringLiteral("band")).toString();
        const bool used = sat.value(QStringLiteral("usedInFix")).toBool();
        if (svid == 203 && band == QStringLiteral("L1")) c203L1Used = used;
        if (svid == 206 && band == QStringLiteral("L1")) c206L1Used = used;
        if (svid == 219 && band == QStringLiteral("L1")) c219L1Used = used;
        if (svid == 219 && band == QStringLiteral("L5")) c219L5Used = used;
    }

    return expect(c203L1Used && c206L1Used && c219L1Used,
                  "BeiDou L1 used satellites should stay highlighted after a later B2 GSA update")
        && expect(c219L5Used,
                  "BeiDou B2 used satellite should be tracked independently from the L1 bucket");
}

bool expectGnGsaWithoutSignalIdAppliesToAllGsvSignals() {
    NmeaProtocolPlugin plugin;

    // GNGSA with system ID (1 = GPS) but no signal ID — stored under the
    // signal-agnostic bucket "GPS|0".  The fix ensures that GSV sentences
    // carrying an explicit signal ID still inherit the used-in-fix state.
    const QList<ProtocolMessage> gsa = plugin.feed(withChecksum("GNGSA,M,3,04,09,,,,,,,,,,,,1.45,0.84,1.18,1"));
    if (!expect(gsa.size() == 1, "GNGSA should decode to one message")) {
        return false;
    }

    // GPGSV with signal ID 1 (L1 C/A): PRN 04 is listed in the GSA, PRN 22 is not.
    const QList<ProtocolMessage> gsv = plugin.feed(withChecksum("GPGSV,1,1,02,04,22,082,19,22,41,191,36,1"));
    if (!expect(gsv.size() == 1, "GPGSV should decode to one message")) {
        return false;
    }

    const QVariantList satellites = gsv.first().fields.value(QStringLiteral("satellites")).toList();
    if (!expect(satellites.size() == 2, "GPGSV should expose two GPS satellites")) {
        return false;
    }

    bool g04Used = false;
    bool g22Used = false;
    for (const QVariant &value : satellites) {
        const QVariantMap sat = value.toMap();
        const int svid = sat.value(QStringLiteral("svid")).toInt();
        if (svid == 4) g04Used = sat.value(QStringLiteral("usedInFix")).toBool();
        if (svid == 22) g22Used = sat.value(QStringLiteral("usedInFix")).toBool();
    }

    return expect(g04Used,
                  "GNGSA-listed satellite should be marked used even when the GSV carries a signal ID")
        && expect(!g22Used,
                  "satellite absent from the GNGSA used-list should remain unused");
}

bool expectNcTalkerMapsToNavicL5() {
    NmeaProtocolPlugin plugin;

    const QList<ProtocolMessage> gsv = plugin.feed(withChecksum("NCGSV,1,1,02,10,52,165,45,02,27,280,40,1"));
    const QList<ProtocolMessage> gsa = plugin.feed(withChecksum("NCGSA,A,3,02,,,,,,,,,,,,1.8,0.8,1.7,1"));
    if (!expect(gsv.size() == 1 && gsa.size() == 1, "NC talker sentences should decode")) {
        return false;
    }

    const QVariantList satellites = gsa.first().fields.value(QStringLiteral("satellites")).toList();
    if (!expect(satellites.size() == 2, "NCGSV should expose two NavIC satellites")) {
        return false;
    }

    QVariantMap sat10, sat2;
    for (const QVariant &v : satellites) {
        const QVariantMap sat = v.toMap();
        const int svid = sat.value(QStringLiteral("svid")).toInt();
        if (svid == 10) sat10 = sat;
        else if (svid == 2) sat2 = sat;
    }
    return expect(!sat10.isEmpty(), "NCGSV should keep the raw NavIC PRN 10")
        && expect(!sat2.isEmpty(), "NCGSA should match the raw NavIC PRN 2")
        && expect(sat10.value(QStringLiteral("constellation")).toString() == QStringLiteral("NAVIC"),
                  "NC talker should map to NAVIC rather than GNSS/GPS")
        && expect(sat10.value(QStringLiteral("band")).toString() == QStringLiteral("L5"),
                  "NC signal id 1 should render in the L5/E5/B2 bucket")
        && expect(!sat10.value(QStringLiteral("usedInFix")).toBool(),
                  "non-selected NavIC satellite should stay visible-only")
        && expect(sat2.value(QStringLiteral("constellation")).toString() == QStringLiteral("NAVIC"),
                  "all NC satellites should map to NAVIC")
        && expect(sat2.value(QStringLiteral("band")).toString() == QStringLiteral("L5"),
                  "all NC signal id 1 satellites should render in L5")
        && expect(sat2.value(QStringLiteral("usedInFix")).toBool(),
                  "selected NavIC satellite should be highlighted as used");
}

bool expectDeviationMapStats() {
    DeviationMapModel model;
    model.addSample(31.230400, 121.473700);
    model.addSample(31.230410, 121.473715);

    const QVariantMap averageStats = model.stats();
    if (!expect(model.rowCount() == 2, "deviation map should keep two samples")) {
        return false;
    }
    if (!expect(averageStats.value(QStringLiteral("points")).toInt() == 2, "deviation map points stat mismatch")) {
        return false;
    }
    if (!expect(averageStats.value(QStringLiteral("centerMode")).toString() == QStringLiteral("Average"), "deviation map should default to average center")) {
        return false;
    }

    model.setFixedCenterEnabled(true);
    model.setFixedCenter(31.230400, 121.473700);
    const QVariantMap fixedStats = model.stats();
    return expect(fixedStats.value(QStringLiteral("centerMode")).toString() == QStringLiteral("Fixed"), "deviation map should switch to fixed center mode")
        && expect(std::abs(fixedStats.value(QStringLiteral("centerLatitude")).toDouble() - 31.230400) < 1e-9, "fixed center latitude mismatch")
        && expect(std::abs(fixedStats.value(QStringLiteral("centerLongitude")).toDouble() - 121.473700) < 1e-9, "fixed center longitude mismatch")
        && expect(fixedStats.value(QStringLiteral("maxDistance")).toDouble() > 0.0, "deviation map max distance should be positive");
}

bool expectCommandButtonsRoundTripJson() {
    QTemporaryDir tempDir;
    if (!expect(tempDir.isValid(), "temporary directory for command button JSON test should be valid")) {
        return false;
    }

    // Remove any persisted buttons from previous runs so the model starts empty.
    const QString persistPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                                    .filePath(QStringLiteral("command-buttons.json"));
    QFile::remove(persistPath);

    CommandButtonModel sourceModel;
    sourceModel.addButton({
        {QStringLiteral("name"), QStringLiteral("Ping")},
        {QStringLiteral("group"), QStringLiteral("Tools")},
        {QStringLiteral("payload"), QStringLiteral("PING")},
        {QStringLiteral("contentType"), QStringLiteral("ASCII")},
        {QStringLiteral("pluginMessage"), QString()},
        {QStringLiteral("fileDecoder"), QStringLiteral("None")},
        {QStringLiteral("filePacketIntervalMs"), 0}
    });
    sourceModel.addButton({
        {QStringLiteral("name"), QStringLiteral("AST Time")},
        {QStringLiteral("group"), QStringLiteral("Bream")},
        {QStringLiteral("payload"), QString()},
        {QStringLiteral("contentType"), QStringLiteral("Plugin")},
        {QStringLiteral("buildProtocol"), QStringLiteral("Bream")},
        {QStringLiteral("pluginMessage"), QStringLiteral("AST_TIME")},
        {QStringLiteral("fileDecoder"), QStringLiteral("None")},
        {QStringLiteral("filePacketIntervalMs"), 0}
    });
    sourceModel.addButton({
        {QStringLiteral("name"), QStringLiteral("Replay")},
        {QStringLiteral("group"), QStringLiteral("Files")},
        {QStringLiteral("payload"), QStringLiteral("C:/logs/capture.bin")},
        {QStringLiteral("contentType"), QStringLiteral("FILE")},
        {QStringLiteral("pluginMessage"), QStringLiteral("AST_TIME")},
        {QStringLiteral("fileDecoder"), QStringLiteral("Bream")},
        {QStringLiteral("filePacketIntervalMs"), 25}
    });

    const QString jsonPath = QDir(tempDir.path()).filePath(QStringLiteral("command-buttons.json"));
    if (!expect(sourceModel.saveToFile(jsonPath), "command button model should export JSON to the chosen path")) {
        return false;
    }
    if (!expect(QFile::exists(jsonPath), "command button export should create the JSON file")) {
        return false;
    }
    QFile jsonFile(jsonPath);
    if (!expect(jsonFile.open(QIODevice::ReadOnly), "command button export should produce a readable JSON file")) {
        return false;
    }
    const QJsonDocument exportedDoc = QJsonDocument::fromJson(jsonFile.readAll());
    if (!expect(exportedDoc.isArray(), "command button export should produce a JSON array")) {
        return false;
    }
    const QJsonArray exportedArray = exportedDoc.array();
    if (!expect(exportedArray.size() == 3, "command button export should write all saved rows")) {
        return false;
    }
    const QJsonObject exportedAscii = exportedArray.at(0).toObject();
    const QJsonObject exportedBream = exportedArray.at(1).toObject();
    const QJsonObject exportedFile = exportedArray.at(2).toObject();
    if (!expect(!exportedAscii.contains(QStringLiteral("pluginMessage")),
                "ASCII export should omit pluginMessage when it is not used")
        || !expect(!exportedAscii.contains(QStringLiteral("fileDecoder")),
                   "ASCII export should omit fileDecoder when it is not used")
        || !expect(!exportedAscii.contains(QStringLiteral("filePacketIntervalMs")),
                   "ASCII export should omit filePacketIntervalMs when it is not used")
        || !expect(exportedBream.value(QStringLiteral("buildProtocol")).toString() == QStringLiteral("Bream"),
                   "protocol build export should preserve the source protocol")
        || !expect(exportedBream.value(QStringLiteral("pluginMessage")).toString() == QStringLiteral("AST_TIME"),
                   "protocol build export should preserve pluginMessage")
        || !expect(!exportedBream.contains(QStringLiteral("fileDecoder")),
                   "protocol build export should omit file-only decoder settings")
        || !expect(exportedFile.value(QStringLiteral("fileDecoder")).toString() == QStringLiteral("Bream"),
                   "FILE export should preserve fileDecoder")
        || !expect(exportedFile.value(QStringLiteral("filePacketIntervalMs")).toInt() == 25,
                   "FILE export should preserve packet interval")
        || !expect(!exportedFile.contains(QStringLiteral("pluginMessage")),
                   "FILE export should omit pluginMessage when it is not used")) {
        return false;
    }

    CommandButtonModel loadedModel;
    if (!expect(loadedModel.loadFromFile(jsonPath), "command button model should import JSON from the chosen path")) {
        return false;
    }
    if (!expect(loadedModel.rowCount() == 3, "command button import should restore all saved rows")) {
        return false;
    }

    const QVariantMap first = loadedModel.get(0);
    const QVariantMap second = loadedModel.get(1);
    const QVariantMap third = loadedModel.get(2);
    return expect(first.value(QStringLiteral("name")).toString() == QStringLiteral("Ping"),
                  "first imported command button should preserve its name")
        && expect(first.value(QStringLiteral("group")).toString() == QStringLiteral("Tools"),
                  "first imported command button should preserve its group")
        && expect(second.value(QStringLiteral("contentType")).toString() == QStringLiteral("Plugin"),
                  "second imported command button should preserve Plugin content type")
        && expect(second.value(QStringLiteral("buildProtocol")).toString() == QStringLiteral("Bream"),
                  "second imported command button should preserve buildProtocol")
        && expect(second.value(QStringLiteral("pluginMessage")).toString() == QStringLiteral("AST_TIME"),
                  "second imported command button should preserve its plugin message")
        && expect(third.value(QStringLiteral("contentType")).toString() == QStringLiteral("FILE"),
                  "third imported command button should preserve FILE content type")
        && expect(third.value(QStringLiteral("fileDecoder")).toString() == QStringLiteral("Bream"),
                  "third imported command button should preserve fileDecoder")
        && expect(third.value(QStringLiteral("filePacketIntervalMs")).toInt() == 25,
                  "third imported command button should preserve filePacketIntervalMs");
}

bool expectFileCommandDownloadsHttpPayload() {
    const QString persistPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                                    .filePath(QStringLiteral("command-buttons.json"));
    QFile::remove(persistPath);

    const QByteArray downloadBytes("downloaded-binary-payload");

    QTcpServer downloadServer;
    if (!expect(downloadServer.listen(QHostAddress::LocalHost, 0), "HTTP fixture server should listen on localhost")) {
        return false;
    }
    QObject::connect(&downloadServer, &QTcpServer::newConnection, [&downloadServer, downloadBytes]() {
        QTcpSocket *socket = downloadServer.nextPendingConnection();
        QObject::connect(socket, &QTcpSocket::readyRead, [socket, downloadBytes]() {
            socket->readAll();
            QByteArray response;
            response += "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: application/octet-stream\r\n";
            response += "Content-Length: " + QByteArray::number(downloadBytes.size()) + "\r\n";
            response += "Connection: close\r\n\r\n";
            response += downloadBytes;
            socket->write(response);
            socket->disconnectFromHost();
        });
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    });

    QTcpServer sinkServer;
    if (!expect(sinkServer.listen(QHostAddress::LocalHost, 0), "TCP sink server should listen on localhost")) {
        return false;
    }

    QByteArray receivedBytes;
    QTcpSocket *sinkSocket = nullptr;
    QObject::connect(&sinkServer, &QTcpServer::newConnection, [&sinkServer, &sinkSocket, &receivedBytes]() {
        sinkSocket = sinkServer.nextPendingConnection();
        QObject::connect(sinkSocket, &QTcpSocket::readyRead, [sinkSocket, &receivedBytes]() {
            receivedBytes.append(sinkSocket->readAll());
        });
    });

    AppSettings settings;
    AppController controller(&settings);
    if (!expect(controller.transportViewModel()->openTcp(QStringLiteral("127.0.0.1"), sinkServer.serverPort()),
                "controller should open TCP transport to sink")) {
        return false;
    }
    if (!expect(waitUntil([&controller, &sinkSocket]() {
                    return sinkSocket != nullptr && controller.transportViewModel()->transportOpen(QStringLiteral("TCP"));
                }, 1000),
                "TCP sink should accept the controller connection")) {
        return false;
    }

    const QString url = QStringLiteral("http://127.0.0.1:%1/data.bin").arg(downloadServer.serverPort());
    const int row = controller.commandButtonModel()->rowCount();
    controller.commandButtonModel()->addButton({
        {QStringLiteral("name"), QStringLiteral("HTTP Replay")},
        {QStringLiteral("group"), QStringLiteral("Files")},
        {QStringLiteral("payload"), url},
        {QStringLiteral("contentType"), QStringLiteral("FILE")},
        {QStringLiteral("fileDecoder"), QStringLiteral("None")},
        {QStringLiteral("filePacketIntervalMs"), 0}
    });
    controller.triggerButton(row);

    const bool delivered = waitUntil([&receivedBytes, &downloadBytes]() {
        return receivedBytes == downloadBytes;
    }, 3000);
    QFile::remove(persistPath);
    return expect(delivered, "FILE command should download http payload and send the bytes");
}

QVariantMap satellite(const QString &constellation, const QString &band, int svid, int cn0, bool usedInFix = true) {
    return {
        {QStringLiteral("key"), QStringLiteral("%1-%2-%3").arg(constellation, band).arg(svid)},
        {QStringLiteral("constellation"), constellation},
        {QStringLiteral("band"), band},
        {QStringLiteral("svid"), svid},
        {QStringLiteral("azimuth"), 100 + svid},
        {QStringLiteral("elevation"), 20 + svid},
        {QStringLiteral("cn0"), cn0},
        {QStringLiteral("usedInFix"), usedInFix}
    };
}

bool expectAppControllerMergesSatelliteUpdates() {
    AppSettings settings;
    AppController controller(&settings);

    ProtocolMessage first;
    first.fields = {
        {QStringLiteral("satellites"), QVariantList{
            satellite(QStringLiteral("GPS"), QStringLiteral("L1"), 1, 42),
            satellite(QStringLiteral("GPS"), QStringLiteral("L1"), 2, 38)
        }}
    };

    ProtocolMessage second;
    second.fields = {
        {QStringLiteral("satellites"), QVariantList{
            satellite(QStringLiteral("GLONASS"), QStringLiteral("L1"), 65, 35)
        }}
    };

    controller.regressionApplyProtocolMessage(first);
    controller.regressionApplyProtocolMessage(second);
    controller.regressionFlushUiRefresh();

    return expect(controller.regressionSatelliteCacheSize() == 3, "controller should merge satellite updates instead of replacing prior entries")
        && expect(controller.satelliteModel()->rowCount() == 3, "satellite model should retain merged satellites")
        && expect(controller.signalModel()->rowCount() == 3, "signal model should retain merged satellites");
}

bool expectZeroSnrSatellitesStayOutOfInViewDisplay() {
    AppSettings settings;
    AppController controller(&settings);

    ProtocolMessage message;
    message.fields = {
        {QStringLiteral("satellites"), QVariantList{
            satellite(QStringLiteral("GPS"), QStringLiteral("L1"), 1, 42, true),
            satellite(QStringLiteral("GPS"), QStringLiteral("L1"), 2, 0, true),
            satellite(QStringLiteral("GLONASS"), QStringLiteral("L1"), 65, 0, false)
        }}
    };

    controller.regressionApplyProtocolMessage(message);
    controller.regressionFlushUiRefresh();

    return expect(controller.regressionSatelliteCacheSize() == 3,
                  "controller cache should retain raw zero-SNR satellite updates")
        && expect(controller.satellitesInView() == 1,
                  "InView count should exclude zero-SNR satellite signals")
        && expect(controller.satelliteCount() == 1,
                  "satelliteCount should follow the visible InView count")
        && expect(controller.satelliteUsage() == QStringLiteral("1 / 1"),
                  "satellite usage summary should exclude zero-SNR satellites")
        && expect(controller.gpsSignals() == QStringLiteral("1 / 1"),
                  "GPS summary should exclude the zero-SNR GPS satellite")
        && expect(controller.glonassSignals() == QStringLiteral("0 / 0"),
                  "GLONASS summary should exclude zero-SNR satellites")
        && expect(controller.satelliteModel()->rowCount() == 1,
                  "SkyView model should hide zero-SNR satellites")
        && expect(controller.signalModel()->rowCount() == 1,
                  "SignalBars model should hide zero-SNR satellites")
        && expect(controller.signalModel()->itemsForBand(QStringLiteral("L1")).size() == 1,
                  "SignalBars band items should not include zero-SNR bars");
}

SatelliteInfo signalSatellite(const QString &constellation, const QString &band, int signalId, int svid) {
    SatelliteInfo sat;
    sat.key = QStringLiteral("%1-%2-%3-%4").arg(constellation, band).arg(signalId).arg(svid);
    sat.constellation = constellation;
    sat.band = band;
    sat.signalId = signalId;
    sat.svid = svid;
    sat.cn0 = 40;
    sat.usedInFix = true;
    return sat;
}

bool expectSatelliteBandGroupsDriveSkyAndSignalModels() {
    const QList<SatelliteInfo> satellites = {
        signalSatellite(QStringLiteral("GPS"), QStringLiteral("L1"), 1, 1),
        signalSatellite(QStringLiteral("BEIDOU"), QStringLiteral("L1"), 1, 201),
        signalSatellite(QStringLiteral("GALILEO"), QStringLiteral("L1"), 7, 301),
        signalSatellite(QStringLiteral("QZSS"), QStringLiteral("L1"), 2, 193),
        signalSatellite(QStringLiteral("GPS"), QStringLiteral("L5"), 7, 2),
        signalSatellite(QStringLiteral("BEIDOU"), QStringLiteral("L5"), 5, 202),
        signalSatellite(QStringLiteral("GALILEO"), QStringLiteral("L5"), 1, 301),
        signalSatellite(QStringLiteral("GLONASS"), QStringLiteral("L2"), 3, 65),
        signalSatellite(QStringLiteral("BEIDOU"), QStringLiteral("L6"), 8, 203),
        signalSatellite(QStringLiteral("GNSS"), QStringLiteral("S"), 0, 900)
    };

    SatelliteModel skyModel;
    skyModel.setSatellites(satellites);
    QStringList groups;
    for (int i = 0; i < skyModel.rowCount(); ++i) {
        groups.append(skyModel.get(i).value(QStringLiteral("bandGroup")).toString());
    }

    SignalModel signalModel;
    signalModel.setSatellites(satellites);

    return expect(groups == QStringList({
                      QStringLiteral("L1"),
                      QStringLiteral("B1"),
                      QStringLiteral("E1"),
                      QStringLiteral("L1C"),
                      QStringLiteral("L5"),
                      QStringLiteral("B2"),
                      QStringLiteral("E5"),
                      QStringLiteral("L2"),
                      QStringLiteral("B3"),
                      QStringLiteral("UN")
                  }),
                  "sky model should expose the same fine band groups used by Signal Spectrum")
        && expect(signalModel.itemsForBand(QStringLiteral("L1")).size() == 4,
                  "Signal Spectrum L1/B1/E1/L1C tab should use the shared L1+B1+E1+L1C grouping")
        && expect(signalModel.itemsForBand(QStringLiteral("L5")).size() == 3,
                  "Signal Spectrum L5/E5/B2 tab should use the shared L5+B2+E5 grouping")
        && expect(signalModel.itemsForBand(QStringLiteral("L2")).size() == 1,
                  "Signal Spectrum L2 tab should use the shared L2 grouping")
        && expect(signalModel.itemsForBand(QStringLiteral("L6")).size() == 1,
                  "Signal Spectrum L6/E6/B3 tab should use the shared B3 grouping");
}

bool expectNmeaPositionEpochDedupKeepsFiveHzAndPrefersRmc() {
    NmeaProtocolPlugin plugin;
    AppSettings settings;
    AppController controller(&settings);
    const QString todayDateField = QDateTime::currentDateTimeUtc().toString(QStringLiteral("ddMMyy"));

    const QList<ProtocolMessage> gga = plugin.feed(withChecksum("GPGGA,123519.200,3112.4640,N,12135.2000,E,4,12,0.8,10.0,M,0.0,M,,"));
    const QList<ProtocolMessage> gll = plugin.feed(withChecksum("GPGLL,3112.4646,N,12135.2006,E,123519.200,A"));
    const QByteArray rmcBody = QStringLiteral("GPRMC,123519.200,A,3112.4652,N,12135.2012,E,0.0,0.0,%1,0.0,E")
                                   .arg(todayDateField)
                                   .toLatin1();
    const QList<ProtocolMessage> rmc = plugin.feed(withChecksum(rmcBody));
    if (!expect(gga.size() == 1 && gll.size() == 1 && rmc.size() == 1,
                "same-epoch NMEA position sentences should decode")) {
        return false;
    }
    if (!expect(gga.first().fields.value(QStringLiteral("utcTime")).toDateTime().time().msec() == 200,
                "GGA should preserve fractional UTC time for high-rate streams")) {
        return false;
    }

    controller.regressionApplyProtocolMessage(gga.first());
    controller.regressionApplyProtocolMessage(gll.first());
    controller.regressionApplyProtocolMessage(rmc.first());
    if (!expect(controller.deviationMapModel()->rowCount() == 1,
                "same-epoch GGA/GLL/RMC should contribute exactly one deviation point")) {
        return false;
    }

    const QVariantMap firstSample = controller.deviationMapModel()->regressionRawSample(0);
    if (!expect(controller.deviationMapModel()->regressionRawSampleCount() == 1,
                "deviation map raw sample count should stay aligned with the rendered point count")
        || !expect(std::abs(firstSample.value(QStringLiteral("latitude")).toDouble() - 31.207753333333335) < 1e-9,
                "RMC latitude should replace lower-priority position sentences within the same epoch")
        || !expect(std::abs(firstSample.value(QStringLiteral("longitude")).toDouble() - 121.58668666666667) < 1e-9,
                   "RMC longitude should replace lower-priority position sentences within the same epoch")) {
        return false;
    }

    const QList<QByteArray> extraRmcBodies = {
        QStringLiteral("GPRMC,123519.400,A,3112.4652,N,12135.2012,E,0.0,0.0,%1,0.0,E").arg(todayDateField).toLatin1(),
        QStringLiteral("GPRMC,123519.600,A,3112.4652,N,12135.2012,E,0.0,0.0,%1,0.0,E").arg(todayDateField).toLatin1(),
        QStringLiteral("GPRMC,123519.800,A,3112.4652,N,12135.2012,E,0.0,0.0,%1,0.0,E").arg(todayDateField).toLatin1(),
        QStringLiteral("GPRMC,123520.000,A,3112.4652,N,12135.2012,E,0.0,0.0,%1,0.0,E").arg(todayDateField).toLatin1()
    };
    for (const QByteArray &body : extraRmcBodies) {
        const QList<ProtocolMessage> messages = plugin.feed(withChecksum(body));
        if (!expect(messages.size() == 1, "each high-rate RMC sentence should decode to one message")) {
            return false;
        }
        controller.regressionApplyProtocolMessage(messages.first());
    }

    return expect(controller.deviationMapModel()->rowCount() == 5,
                  "5Hz position stream should keep one deviation point per epoch rather than per second");
}

bool expectLocationSummaryMatchesMultisignalNmeaEpoch() {
    NmeaProtocolPlugin plugin;
    AppSettings settings;
    AppController controller(&settings);

    const QList<QByteArray> bodies = {
        "GNRMC,041018.00,A,3112.46434,N,12135.20968,E,0.011,,270426,,,D,V",
        "GNVTG,,T,,M,0.011,N,0.020,K,D",
        "GNGGA,041018.00,3112.46434,N,12135.20968,E,2,12,0.54,50.2,M,10.1,M,,0129",
        "GNGSA,M,3,03,14,17,22,06,19,04,09,11,,,,1.01,0.54,0.86,1",
        "GNGSA,M,3,26,13,15,27,30,21,,,,,,,1.01,0.54,0.86,3",
        "GNGSA,M,3,31,12,08,35,29,26,20,24,22,06,19,,1.01,0.54,0.86,4",
        "GPGSV,4,1,16,01,08,059,31,03,32,043,38,04,16,099,33,06,53,295,41,1",
        "GPGSV,4,2,16,09,17,132,32,11,20,262,33,12,00,325,26,14,43,184,40,1",
        "GPGSV,4,3,16,17,67,049,42,19,57,337,41,21,01,209,28,22,60,201,42,1",
        "GPGSV,4,4,16,35,47,142,37,41,35,237,36,42,41,229,38,50,53,170,38,1",
        "GPGSV,1,1,02,03,32,043,24,04,16,099,18,6",
        "GPGSV,2,1,08,01,08,059,32,03,32,043,40,04,16,099,34,06,53,295,44,8",
        "GPGSV,2,2,08,09,17,132,36,11,20,262,33,14,43,184,41,21,01,209,29,8",
        "GPGSV,1,1,01,40,10,256,,0",
        "GAGSV,3,1,10,03,10,091,30,08,07,139,28,13,57,283,41,14,39,102,41,1",
        "GAGSV,3,2,10,15,50,029,41,21,44,308,40,26,12,249,29,27,67,200,43,1",
        "GAGSV,3,3,10,30,23,158,35,34,05,055,29,1",
        "GAGSV,3,1,10,03,10,091,29,08,07,139,26,13,57,283,40,14,39,102,40,7",
        "GAGSV,3,2,10,15,50,029,39,21,44,308,38,26,12,249,29,27,67,200,40,7",
        "GAGSV,3,3,10,30,23,158,34,34,05,055,28,7",
        "GAGSV,1,1,01,32,09,116,,0",
        "GBGSV,4,1,14,06,50,185,39,07,04,193,25,08,68,016,40,12,25,055,33,3",
        "GBGSV,4,2,14,19,40,082,38,20,12,040,30,21,01,190,27,22,34,153,38,3",
        "GBGSV,4,3,14,24,28,284,35,26,32,229,36,29,14,084,32,31,45,312,37,3",
        "GBGSV,4,4,14,35,57,037,41,39,05,178,26,3",
        "GBGSV,4,1,14,06,50,185,40,07,04,193,26,08,68,016,42,12,25,055,36,5",
        "GBGSV,4,2,14,19,40,082,41,20,12,040,32,21,01,190,29,22,34,153,40,5",
        "GBGSV,4,3,14,24,28,284,37,26,32,229,36,29,14,084,32,31,45,312,40,5",
        "GBGSV,4,4,14,35,57,037,41,39,05,178,28,5",
        "GBGSV,2,1,05,12,25,055,27,19,40,082,18,20,12,040,26,29,14,084,16,8",
        "GBGSV,2,2,05,35,57,037,16,8",
        "GBGSV,2,1,07,01,49,147,,02,31,238,,03,50,200,,04,34,124,,0",
        "GBGSV,2,2,07,09,28,196,,10,10,201,,34,02,283,,0",
        "GNGLL,3112.46434,N,12135.20968,E,041018.00,A,D"
    };

    for (const QByteArray &body : bodies) {
        if (!applyNmeaSentence(plugin, controller, body)) {
            return false;
        }
    }
    controller.regressionFlushUiRefresh();

    return expect(std::abs(controller.latitude() - 31.207739) < 1e-7,
                  "location summary latitude should match the NMEA latitude")
        && expect(std::abs(controller.longitude() - 121.586828) < 1e-7,
                  "location summary longitude should match the NMEA longitude")
        && expect(std::abs(controller.speed() - (0.020 / 3.6)) < 1e-9,
                  "location summary speed should use the VTG km/h value converted to m/s")
        && expect(std::isnan(controller.track()),
                  "empty RMC/VTG course fields should render as unavailable rather than 0 degrees")
        && expect(controller.utcDate() == QStringLiteral("2026-04-27"),
                  "location summary UTC date should come from the RMC date field")
        && expect(controller.utcClock() == QStringLiteral("04:10:18"),
                  "location summary UTC clock should come from the NMEA UTC time")
        && expect(controller.satellitesUsed() == 12,
                  "location summary satellites-used should match GGA")
        && expect(std::abs(controller.hdop() - 0.54) < 1e-9,
                  "location summary HDOP should keep the source precision")
        && expect(controller.satelliteUsage() == QStringLiteral("26 / 40"),
                  "Satellites summary should exclude zero-SNR satellites from viewed counts")
        && expect(controller.gpsSignals() == QStringLiteral("9 / 12"),
                  "GPS summary should count unique satellites instead of signal-frequency duplicates")
        && expect(controller.glonassSignals() == QStringLiteral("0 / 0"),
                  "GLONASS summary should stay empty when no GLONASS GSV is present")
        && expect(controller.beidouSignals() == QStringLiteral("11 / 14"),
                  "BeiDou summary should count unique nonzero-SNR satellites instead of signal-frequency duplicates")
        && expect(controller.galileoSignals() == QStringLiteral("6 / 10"),
                  "Galileo summary should count unique nonzero-SNR satellites instead of signal-frequency duplicates")
        && expect(controller.otherSignals() == QStringLiteral("0 / 4"),
                  "Other summary should exclude zero-SNR SBAS satellites from GPS-talker GSV");
}

bool expectNonLocationQualityDoesNotOverrideGgaQuality() {
    NmeaProtocolPlugin plugin;
    AppSettings settings;
    AppController controller(&settings);

    const QList<ProtocolMessage> gga = plugin.feed(withChecksum(
        "GPGGA,030106.00,3112.463772,N,12135.209361,E,2,57,0.2,44.6,M,10.0,M,0.0,0000"));
    if (!expect(gga.size() == 1, "GGA sentence should decode")) {
        return false;
    }

    controller.regressionApplyProtocolMessage(gga.first());
    if (!expect(controller.quality() == 2, "GGA quality should update the location summary")) {
        return false;
    }

    ProtocolMessage telemetry;
    telemetry.protocol = QStringLiteral("PGLOR");
    telemetry.messageName = QStringLiteral("PGLSTA");
    telemetry.fields = {
        {QStringLiteral("displayMessage"), telemetry.messageName},
        {QStringLiteral("quality"), 1}
    };
    controller.regressionApplyProtocolMessage(telemetry);

    return expect(controller.quality() == 2,
                  "non-location telemetry quality should not overwrite GGA fix quality");
}

bool expectTecTypesStayValid() {
    hdgnss::TecGridData dataset;
    dataset.timestampUtc = QDateTime::fromString(QStringLiteral("2026-04-16T21:35:00Z"), Qt::ISODate);
    dataset.samples.append({121.5, 31.2, 12.0, 0});
    dataset.fromCache = true;
    dataset.stale = true;
    dataset.availabilityNote = QStringLiteral("Network timeout, using cached TEC");

    return expect(dataset.isValid(), "TEC dataset with cache flags should remain valid")
        && expect(dataset.fromCache, "TEC dataset should keep cache flag")
        && expect(dataset.stale, "TEC dataset should keep stale cache flag");
}

bool expectTecRefreshScheduling() {
    const QDateTime firstRequest = QDateTime::fromString(QStringLiteral("2026-04-16T21:35:00Z"), Qt::ISODate);
    const QDateTime beforeRefresh = QDateTime::fromString(QStringLiteral("2026-04-16T22:04:59Z"), Qt::ISODate);
    const QDateTime atRefresh = QDateTime::fromString(QStringLiteral("2026-04-16T22:05:00Z"), Qt::ISODate);
    constexpr qint64 kThirtyMinutesSeconds = 1800;

    return expect(!TecMapOverlayModel::shouldRequestRefresh(firstRequest,
                                                            beforeRefresh,
                                                            kThirtyMinutesSeconds),
                  "TEC refresh should not trigger before the configured interval elapses")
        && expect(TecMapOverlayModel::shouldRequestRefresh(firstRequest,
                                                           atRefresh,
                                                           kThirtyMinutesSeconds),
                  "TEC refresh should trigger once the configured interval elapses");
}

bool expectTecRefreshCountdownText() {
    const QDateTime firstRequest = QDateTime::fromString(QStringLiteral("2026-04-16T21:35:00Z"), Qt::ISODate);
    const QDateTime midWindow = QDateTime::fromString(QStringLiteral("2026-04-16T21:47:10Z"), Qt::ISODate);
    const QDateTime dueTime = QDateTime::fromString(QStringLiteral("2026-04-16T22:05:00Z"), Qt::ISODate);
    constexpr qint64 kThirtyMinutesSeconds = 1800;

    return expect(TecMapOverlayModel::refreshCountdownText(firstRequest,
                                                           midWindow,
                                                           kThirtyMinutesSeconds)
                          == QStringLiteral(" | next refresh in 18m"),
                  "TEC countdown text should show the remaining whole-minute refresh window")
        && expect(TecMapOverlayModel::refreshCountdownText(firstRequest,
                                                           dueTime,
                                                           kThirtyMinutesSeconds)
                          == QStringLiteral(" | refresh due"),
                  "TEC countdown text should switch to refresh due at the interval boundary");
}

bool expectTecMapRendererPaintsGrid() {
    hdgnss::TecGridData dataset;
    dataset.timestampUtc = QDateTime::fromString(QStringLiteral("2026-04-16T21:35:00Z"), Qt::ISODate);
    dataset.sourceId = QStringLiteral("test-tec");
    dataset.sourceName = QStringLiteral("Test TEC");
    dataset.longitudeStepDegrees = 5.0;
    dataset.latitudeStepDegrees = 2.5;
    dataset.samples = {
        {-177.5, -88.75, 8.2, 0},
        {-172.5, -88.75, 12.4, 0},
        {-177.5, -86.25, 18.6, 1},
        {-172.5, -86.25, 24.8, 2}
    };
    dataset.minTec = 8.2;
    dataset.maxTec = 24.8;

    if (!expect(dataset.samples.size() == 4, "TEC renderer test should keep all valid grid samples")) {
        return false;
    }
    if (!expect(std::abs(dataset.longitudeStepDegrees - 5.0) < 1e-9,
                "TEC longitude step should be detected as 5 degrees")) {
        return false;
    }
    if (!expect(std::abs(dataset.latitudeStepDegrees - 2.5) < 1e-9,
                "TEC latitude step should be detected as 2.5 degrees")) {
        return false;
    }

    const QImage image = hdgnss::TecMapRenderer::render(dataset, QSize(256, 128));
    if (!expect(!image.isNull(), "TEC renderer should create an image")) {
        return false;
    }

    int coloredPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                ++coloredPixels;
            }
        }
    }
    return expect(coloredPixels > 0, "TEC renderer should paint at least one visible pixel");
}

bool expectTecSampleLookupByCanvasPoint() {
    hdgnss::TecGridData dataset;
    dataset.timestampUtc = QDateTime::fromString(QStringLiteral("2026-04-17T04:45:00Z"), Qt::ISODate);
    dataset.sourceId = QStringLiteral("noaa-glotec-geojson");
    dataset.sourceName = QStringLiteral("GloTEC");
    dataset.longitudeStepDegrees = 5.0;
    dataset.latitudeStepDegrees = 2.5;
    dataset.samples = {
        {122.5, 31.25, 40.38, 0},
        {142.5, 21.25, 74.27, 1}
    };

    AppSettings settings;
    TecMapOverlayModel model(&settings);
    model.setTestDataset(dataset);
    const QPointF clickPoint = projectedCanvasPointForTest(964.0, 488.0, 122.5, 31.25);
    const QVariantMap sample = model.sampleAtCanvasPoint(clickPoint.x(), clickPoint.y(), 964.0, 488.0);
    return expect(sample.value(QStringLiteral("valid")).toBool(),
                  "TEC sample lookup should find a grid cell from the clicked map point")
        && expect(std::abs(sample.value(QStringLiteral("longitude")).toDouble() - 122.5) < 1e-6,
                  "TEC sample lookup should return the matched longitude")
        && expect(std::abs(sample.value(QStringLiteral("latitude")).toDouble() - 31.25) < 1e-6,
                  "TEC sample lookup should return the matched latitude")
        && expect(std::abs(sample.value(QStringLiteral("tec")).toDouble() - 40.38) < 1e-6,
                  "TEC sample lookup should return the raw TEC value");
}

}  // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GnssViewRegression"));
    QCoreApplication::setOrganizationName(QStringLiteral("hdgnss"));

    if (!expectMissingSatelliteDropsFromGsv()) {
        return EXIT_FAILURE;
    }
    if (!expectBeidouGsaUsesRawPrnWithoutRemap()) {
        return EXIT_FAILURE;
    }
    if (!expectGpsSbasPrnMapsToSbas()) {
        return EXIT_FAILURE;
    }
    if (!expectBandSpecificGsvClearsStaleSatellitesBetweenEpochs()) {
        return EXIT_FAILURE;
    }
    if (!expectSameBandGsaSentencesAccumulateUsedSatellites()) {
        return EXIT_FAILURE;
    }
    if (!expectGsaStateResetsBetweenEpochs()) {
        return EXIT_FAILURE;
    }
    if (!expectBeidouL1UsageSurvivesB2GsaUpdate()) {
        return EXIT_FAILURE;
    }
    if (!expectGnGsaWithoutSignalIdAppliesToAllGsvSignals()) {
        return EXIT_FAILURE;
    }
    if (!expectNcTalkerMapsToNavicL5()) {
        return EXIT_FAILURE;
    }
    if (!expectUpdateCheckerVersionComparison()) {
        return EXIT_FAILURE;
    }
    if (!expectRawRecorderRequiresExplicitLogDirectory()) {
        return EXIT_FAILURE;
    }
    if (!expectDeviationMapStats()) {
        return EXIT_FAILURE;
    }
    if (!expectCommandButtonsRoundTripJson()) {
        return EXIT_FAILURE;
    }
    if (!expectFileCommandDownloadsHttpPayload()) {
        return EXIT_FAILURE;
    }
    if (!expectAppControllerMergesSatelliteUpdates()) {
        return EXIT_FAILURE;
    }
    if (!expectZeroSnrSatellitesStayOutOfInViewDisplay()) {
        return EXIT_FAILURE;
    }
    if (!expectSatelliteBandGroupsDriveSkyAndSignalModels()) {
        return EXIT_FAILURE;
    }
    if (!expectNmeaPositionEpochDedupKeepsFiveHzAndPrefersRmc()) {
        return EXIT_FAILURE;
    }
    if (!expectLocationSummaryMatchesMultisignalNmeaEpoch()) {
        return EXIT_FAILURE;
    }
    if (!expectNonLocationQualityDoesNotOverrideGgaQuality()) {
        return EXIT_FAILURE;
    }
    if (!expectTecTypesStayValid()) {
        return EXIT_FAILURE;
    }
    if (!expectTecRefreshScheduling()) {
        return EXIT_FAILURE;
    }
    if (!expectTecRefreshCountdownText()) {
        return EXIT_FAILURE;
    }
    if (!expectTecMapRendererPaintsGrid()) {
        return EXIT_FAILURE;
    }
    if (!expectTecSampleLookupByCanvasPoint()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
