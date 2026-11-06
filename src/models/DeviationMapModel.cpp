#include "DeviationMapModel.h"

#include <QSet>

#include <algorithm>
#include <cmath>

namespace hdgnss {

namespace {

constexpr double kEarthRadiusMeters = 6378137.0;
constexpr double kPi = 3.14159265358979323846;

QVariantMap emptyAxisStats() {
    return {
        {QStringLiteral("min"), 0.0},
        {QStringLiteral("max"), 0.0},
        {QStringLiteral("avg"), 0.0},
        {QStringLiteral("stdDev"), 0.0}
    };
}

QVariantMap emptyDistanceStats() {
    QVariantMap stats = emptyAxisStats();
    stats.insert(QStringLiteral("cep50"), 0.0);
    stats.insert(QStringLiteral("cep68"), 0.0);
    stats.insert(QStringLiteral("cep95"), 0.0);
    return stats;
}

QVariantMap defaultStats(bool fixedCenterEnabled, double fixedLatitude, double fixedLongitude) {
    return {
        {QStringLiteral("points"), 0},
        {QStringLiteral("centerMode"), fixedCenterEnabled ? QStringLiteral("Fixed") : QStringLiteral("Average")},
        {QStringLiteral("centerLatitude"), fixedCenterEnabled ? fixedLatitude : 0.0},
        {QStringLiteral("centerLongitude"), fixedCenterEnabled ? fixedLongitude : 0.0},
        {QStringLiteral("maxDistance"), 1.0},
        {QStringLiteral("horizontal"), emptyDistanceStats()},
        {QStringLiteral("latitude"), emptyAxisStats()},
        {QStringLiteral("longitude"), emptyAxisStats()}
    };
}

quint64 packQuantizedCanvasCell(double x, double y) {
    // Quantize to 0.5px buckets so overlapping historical points are deduplicated.
    const qint32 qx = static_cast<qint32>(std::llround(x * 2.0));
    const qint32 qy = static_cast<qint32>(std::llround(y * 2.0));
    return (static_cast<quint64>(static_cast<quint32>(qx)) << 32)
        | static_cast<quint32>(qy);
}

}  // namespace

DeviationMapModel::DeviationMapModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_stats(defaultStats(false, 0.0, 0.0)) {}

int DeviationMapModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rawSamples.size());
}

QVariant DeviationMapModel::data(const QModelIndex &index, int role) const {
    ensureCalculated();
    if (!index.isValid() || index.row() < 0 || index.row() >= m_samples.size()) {
        return {};
    }
    const DeviationSample &sample = m_samples.at(index.row());
    switch (role) {
    case EastMetersRole: return sample.eastMeters;
    case NorthMetersRole: return sample.northMeters;
    case DistanceMetersRole: return sample.distanceMeters;
    case SequenceRole: return index.row() + 1;
    case LatestRole: return index.row() == (m_samples.size() - 1);
    default: return {};
    }
}

QHash<int, QByteArray> DeviationMapModel::roleNames() const {
    return {
        {EastMetersRole, "eastMeters"},
        {NorthMetersRole, "northMeters"},
        {DistanceMetersRole, "distanceMeters"},
        {SequenceRole, "sequence"},
        {LatestRole, "latest"}
    };
}

QVariantMap DeviationMapModel::get(int row) const {
    ensureCalculated();
    if (row < 0 || row >= m_samples.size()) {
        return {};
    }
    const DeviationSample &sample = m_samples.at(row);
    return {
        {QStringLiteral("eastMeters"), sample.eastMeters},
        {QStringLiteral("northMeters"), sample.northMeters},
        {QStringLiteral("distanceMeters"), sample.distanceMeters},
        {QStringLiteral("sequence"), row + 1},
        {QStringLiteral("latest"), row == (m_samples.size() - 1)}
    };
}

QVariantList DeviationMapModel::canvasPoints(double centerX, double centerY, double maxRadius) const {
    ensureCalculated();
    if (m_samples.isEmpty() || !std::isfinite(centerX) || !std::isfinite(centerY)
        || !std::isfinite(maxRadius) || maxRadius <= 0.0) {
        return {};
    }

    const double scaleDistance = std::max(1.0, m_stats.value(QStringLiteral("maxDistance")).toDouble());
    const double metersToPixels = maxRadius / scaleDistance;
    const int totalCount = m_samples.size();

    QVariantList points;
    points.reserve(std::min(totalCount, 2048));
    QSet<quint64> seenCells;
    seenCells.reserve(std::min(totalCount, 2048));

    for (int i = 0; i < totalCount - 1; ++i) {
        const DeviationSample &sample = m_samples.at(i);
        const double px = centerX + sample.eastMeters * metersToPixels;
        const double py = centerY - sample.northMeters * metersToPixels;
        const quint64 cellKey = packQuantizedCanvasCell(px, py);
        if (seenCells.contains(cellKey)) {
            continue;
        }
        seenCells.insert(cellKey);
        points.append(QVariantMap{
            {QStringLiteral("x"), px},
            {QStringLiteral("y"), py},
            {QStringLiteral("latest"), false}
        });
    }

    const DeviationSample &latestSample = m_samples.constLast();
    points.append(QVariantMap{
        {QStringLiteral("x"), centerX + latestSample.eastMeters * metersToPixels},
        {QStringLiteral("y"), centerY - latestSample.northMeters * metersToPixels},
        {QStringLiteral("latest"), true}
    });
    return points;
}

void DeviationMapModel::clear() {
    if (m_rawSamples.empty() && m_samples.isEmpty() && !m_calculationDirty && !m_notificationsDeferred) {
        return;
    }
    beginResetModel();
    m_rawSamples.clear();
    m_samples.clear();
    m_distanceScratch.clear();
    m_stats = defaultStats(m_fixedCenterEnabled, m_fixedLatitude, m_fixedLongitude);
    m_calculationDirty = false;
    m_notificationsDeferred = false;
    endResetModel();

    ++m_revision;
    emit countChanged();
    emit revisionChanged();
    emit statsChanged();
}

void DeviationMapModel::addSample(double latitude, double longitude, bool deferNotify) {
    if (!std::isfinite(latitude) || !std::isfinite(longitude)) {
        return;
    }
    m_rawSamples.push_back({latitude, longitude});
    trimRawSamples();
    markDirty(deferNotify);
}

bool DeviationMapModel::updateLastSample(double latitude, double longitude, bool deferNotify) {
    if (!std::isfinite(latitude) || !std::isfinite(longitude) || m_rawSamples.empty()) {
        return false;
    }
    RawSample &lastSample = m_rawSamples.back();
    lastSample.latitude = latitude;
    lastSample.longitude = longitude;
    markDirty(deferNotify);
    return true;
}

void DeviationMapModel::flushDeferredNotifications() {
    if (!m_notificationsDeferred) {
        return;
    }
    m_notificationsDeferred = false;
    ++m_revision;
    emit countChanged();
    emit revisionChanged();
    emit statsChanged();
}

void DeviationMapModel::setFixedCenterEnabled(bool enabled) {
    if (m_fixedCenterEnabled == enabled) {
        return;
    }
    m_fixedCenterEnabled = enabled;
    markDirty(false);
}

void DeviationMapModel::setFixedCenter(double latitude, double longitude) {
    if (qFuzzyCompare(1.0 + m_fixedLatitude, 1.0 + latitude)
        && qFuzzyCompare(1.0 + m_fixedLongitude, 1.0 + longitude)) {
        return;
    }
    m_fixedLatitude = latitude;
    m_fixedLongitude = longitude;
    markDirty(false);
}

int DeviationMapModel::revision() const {
    return m_revision;
}

QVariantMap DeviationMapModel::stats() const {
    ensureCalculated();
    return m_stats;
}

#ifdef HDGNSS_REGRESSION_TESTS
int DeviationMapModel::regressionRawSampleCount() const {
    return static_cast<int>(m_rawSamples.size());
}

QVariantMap DeviationMapModel::regressionRawSample(int index) const {
    if (index < 0 || index >= static_cast<int>(m_rawSamples.size())) {
        return {};
    }

    const RawSample &sample = m_rawSamples[static_cast<size_t>(index)];
    return {
        {QStringLiteral("latitude"), sample.latitude},
        {QStringLiteral("longitude"), sample.longitude}
    };
}

void DeviationMapModel::regressionFillRawSamplesForCapTest(int count) {
    m_rawSamples.clear();
    for (int i = 0; i < count; ++i) {
        m_rawSamples.push_back({31.0 + i * 0.000001, 121.0 + i * 0.000001});
    }
    trimRawSamples();
    markDirty(false);
}
#endif

void DeviationMapModel::trimRawSamples() {
    while (static_cast<int>(m_rawSamples.size()) > kMaxSamples) {
        m_rawSamples.pop_front();
    }
}

void DeviationMapModel::markDirty(bool deferNotify) {
    m_calculationDirty = true;
    if (deferNotify) {
        m_notificationsDeferred = true;
        return;
    }
    m_notificationsDeferred = false;
    ++m_revision;
    emit countChanged();
    emit revisionChanged();
    emit statsChanged();
}

void DeviationMapModel::ensureCalculated() const {
    if (!m_calculationDirty) {
        return;
    }
    m_calculationDirty = false;

    const int count = static_cast<int>(m_rawSamples.size());
    if (count == 0) {
        m_samples.clear();
        m_distanceScratch.clear();
        m_stats = defaultStats(m_fixedCenterEnabled, m_fixedLatitude, m_fixedLongitude);
        return;
    }

    double centerLatitude = 0.0;
    double centerLongitude = 0.0;
    if (m_fixedCenterEnabled) {
        centerLatitude = m_fixedLatitude;
        centerLongitude = m_fixedLongitude;
    } else {
        for (const RawSample &sample : m_rawSamples) {
            centerLatitude += sample.latitude;
            centerLongitude += sample.longitude;
        }
        centerLatitude /= count;
        centerLongitude /= count;
    }

    m_samples.resize(count);
    m_distanceScratch.resize(static_cast<size_t>(count));

    const double degToMeters = kPi / 180.0 * kEarthRadiusMeters;
    const double eastDegToMeters = degToMeters * std::cos(centerLatitude * kPi / 180.0);

    double eastMin = 0.0;
    double eastMax = 0.0;
    double eastSum = 0.0;
    double northMin = 0.0;
    double northMax = 0.0;
    double northSum = 0.0;
    double distMin = 0.0;
    double distMax = 0.0;
    double distSum = 0.0;

    for (int i = 0; i < count; ++i) {
        const RawSample &raw = m_rawSamples[static_cast<size_t>(i)];
        const double northMeters = (raw.latitude - centerLatitude) * degToMeters;
        const double eastMeters = (raw.longitude - centerLongitude) * eastDegToMeters;
        const double distanceMeters = std::hypot(eastMeters, northMeters);

        m_samples[i] = {eastMeters, northMeters, distanceMeters};
        m_distanceScratch[static_cast<size_t>(i)] = distanceMeters;

        if (i == 0) {
            eastMin = eastMax = eastSum = eastMeters;
            northMin = northMax = northSum = northMeters;
            distMin = distMax = distSum = distanceMeters;
        } else {
            eastMin = std::min(eastMin, eastMeters);
            eastMax = std::max(eastMax, eastMeters);
            eastSum += eastMeters;
            northMin = std::min(northMin, northMeters);
            northMax = std::max(northMax, northMeters);
            northSum += northMeters;
            distMin = std::min(distMin, distanceMeters);
            distMax = std::max(distMax, distanceMeters);
            distSum += distanceMeters;
        }
    }

    const double eastAvg = eastSum / count;
    const double northAvg = northSum / count;
    const double distAvg = distSum / count;

    double eastVariance = 0.0;
    double northVariance = 0.0;
    double distVariance = 0.0;
    for (int i = 0; i < count; ++i) {
        const DeviationSample &sample = m_samples.at(i);
        const double de = sample.eastMeters - eastAvg;
        const double dn = sample.northMeters - northAvg;
        const double dd = sample.distanceMeters - distAvg;
        eastVariance += de * de;
        northVariance += dn * dn;
        distVariance += dd * dd;
    }
    eastVariance /= count;
    northVariance /= count;
    distVariance /= count;

    int low50 = 0;
    int low68 = 0;
    const double cep50 = percentileFromPartitioned(m_distanceScratch, 0, 0.50, &low50);
    const double cep68 = percentileFromPartitioned(m_distanceScratch, low50, 0.68, &low68);
    const double cep95 = percentileFromPartitioned(m_distanceScratch, low68, 0.95);

    const QVariantMap horizontalStats = {
        {QStringLiteral("min"), distMin},
        {QStringLiteral("max"), distMax},
        {QStringLiteral("avg"), distAvg},
        {QStringLiteral("stdDev"), std::sqrt(distVariance)},
        {QStringLiteral("cep50"), cep50},
        {QStringLiteral("cep68"), cep68},
        {QStringLiteral("cep95"), cep95}
    };
    const QVariantMap latitudeStats = {
        {QStringLiteral("min"), northMin},
        {QStringLiteral("max"), northMax},
        {QStringLiteral("avg"), northAvg},
        {QStringLiteral("stdDev"), std::sqrt(northVariance)}
    };
    const QVariantMap longitudeStats = {
        {QStringLiteral("min"), eastMin},
        {QStringLiteral("max"), eastMax},
        {QStringLiteral("avg"), eastAvg},
        {QStringLiteral("stdDev"), std::sqrt(eastVariance)}
    };

    m_stats = {
        {QStringLiteral("points"), count},
        {QStringLiteral("centerMode"), m_fixedCenterEnabled ? QStringLiteral("Fixed") : QStringLiteral("Average")},
        {QStringLiteral("centerLatitude"), centerLatitude},
        {QStringLiteral("centerLongitude"), centerLongitude},
        {QStringLiteral("maxDistance"), qMax(1.0, std::ceil(distMax))},
        {QStringLiteral("horizontal"), horizontalStats},
        {QStringLiteral("latitude"), latitudeStats},
        {QStringLiteral("longitude"), longitudeStats}
    };
}

double DeviationMapModel::percentileFromPartitioned(std::vector<double> &values,
                                                    int startOffset,
                                                    double ratio,
                                                    int *lowerIndexOut) {
    if (values.empty()) {
        if (lowerIndexOut) {
            *lowerIndexOut = 0;
        }
        return 0.0;
    }
    if (values.size() == 1) {
        if (lowerIndexOut) {
            *lowerIndexOut = 0;
        }
        return values.front();
    }

    const double clampedRatio = std::clamp(ratio, 0.0, 1.0);
    const double position = clampedRatio * static_cast<double>(values.size() - 1);
    const int lowerIndex = static_cast<int>(std::floor(position));
    const int upperIndex = static_cast<int>(std::ceil(position));
    const int safeStart = std::clamp(startOffset, 0, lowerIndex);

    std::nth_element(values.begin() + safeStart,
                     values.begin() + lowerIndex,
                     values.end());
    if (lowerIndexOut) {
        *lowerIndexOut = lowerIndex;
    }

    const double lowerValue = values[static_cast<size_t>(lowerIndex)];
    if (lowerIndex == upperIndex) {
        return lowerValue;
    }

    const double upperValue = *std::min_element(values.begin() + lowerIndex + 1, values.end());
    const double weight = position - lowerIndex;
    return lowerValue * (1.0 - weight) + upperValue * weight;
}

}  // namespace hdgnss
