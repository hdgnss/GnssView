#pragma once

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QUrl>

namespace hdgnss {

struct TecSample {
    double longitude = 0.0;
    double latitude = 0.0;
    double tec = 0.0;
    int qualityFlag = 0;
};

struct TecGridData {
    QDateTime timestampUtc;
    QString sourceId;
    QString sourceName;
    QString availabilityNote;
    QList<TecSample> samples;
    double longitudeStepDegrees = 0.0;
    double latitudeStepDegrees = 0.0;
    double minTec = 0.0;
    double maxTec = 0.0;
    bool fromCache = false;
    bool stale = false;

    [[nodiscard]] bool isValid() const {
        return timestampUtc.isValid() && !samples.isEmpty();
    }
};

struct TecSourceEntry {
    QUrl url;
    QDateTime timestampUtc;

    [[nodiscard]] bool isValid() const {
        return url.isValid() && timestampUtc.isValid();
    }
};

}  // namespace hdgnss

Q_DECLARE_METATYPE(hdgnss::TecGridData)
Q_DECLARE_METATYPE(hdgnss::TecSourceEntry)
