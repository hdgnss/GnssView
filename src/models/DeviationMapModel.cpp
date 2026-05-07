#include "DeviationMapModel.h"

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

}

DeviationMapModel::DeviationMapModel(QObject *parent)
    : QAbstractListModel(parent) {
    m_stats = {
        {QStringLiteral("points"), 0},
        {QStringLiteral("centerMode"), QStringLiteral("Average")},
        {QStringLiteral("centerLatitude"), 0.0},
        {QStringLiteral("centerLongitude"), 0.0},
        {QStringLiteral("maxDistance"), 1.0},
        {QStringLiteral("horizontal"), emptyDistanceStats()},
        {QStringLiteral("latitude"), emptyAxisStats()},
        {QStringLiteral("longitude"), emptyAxisStats()}
    };
}

int DeviationMapModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_samples.size();
}

QVariant DeviationMapModel::data(const QModelIndex &index, int role) const {
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

void DeviationMapModel::clear() {
    if (m_rawSamples.isEmpty() && m_samples.isEmpty()) {
        return;
    }
    beginResetModel();
    m_rawSamples.clear();
    m_samples.clear();
    endResetModel();
    recalculate();
}

void DeviationMapModel::addSample(double latitude, double longitude) {
    if (!std::isfinite(latitude) || !std::isfinite(longitude)) {
        return;
    }
    m_rawSamples.append({latitude, longitude});
    recalculate();
}

bool DeviationMapModel::updateLastSample(double latitude, double longitude) {
    if (!std::isfinite(latitude) || !std::isfinite(longitude) || m_rawSamples.isEmpty()) {
        return false;
    }
    RawSample &lastSample = m_rawSamples.last();
    lastSample.latitude = latitude;
    lastSample.longitude = longitude;
    recalculate();
    return true;
}

void DeviationMapModel::setFixedCenterEnabled(bool enabled) {
    if (m_fixedCenterEnabled == enabled) {
        return;
    }
    m_fixedCenterEnabled = enabled;
    recalculate();
}

void DeviationMapModel::setFixedCenter(double latitude, double longitude) {
    if (qFuzzyCompare(1.0 + m_fixedLatitude, 1.0 + latitude)
        && qFuzzyCompare(1.0 + m_fixedLongitude, 1.0 + longitude)) {
        return;
    }
    m_fixedLatitude = latitude;
    m_fixedLongitude = longitude;
    recalculate();
}

int DeviationMapModel::revision() const {
    return m_revision;
}

QVariantMap DeviationMapModel::stats() const {
    return m_stats;
}

#ifdef HDGNSS_REGRESSION_TESTS
int DeviationMapModel::regressionRawSampleCount() const {
    return m_rawSamples.size();
}

QVariantMap DeviationMapModel::regressionRawSample(int index) const {
    if (index < 0 || index >= m_rawSamples.size()) {
        return {};
    }

    const RawSample &sample = m_rawSamples.at(index);
    return {
        {QStringLiteral("latitude"), sample.latitude},
        {QStringLiteral("longitude"), sample.longitude}
    };
}
#endif

void DeviationMapModel::recalculate() {
    const int oldCount = m_samples.size();

    // Compute center
    double centerLatitude = 0.0;
    double centerLongitude = 0.0;
    if (!m_rawSamples.isEmpty()) {
        if (m_fixedCenterEnabled) {
            centerLatitude = m_fixedLatitude;
            centerLongitude = m_fixedLongitude;
        } else {
            for (const RawSample &sample : std::as_const(m_rawSamples)) {
                centerLatitude += sample.latitude;
                centerLongitude += sample.longitude;
            }
            centerLatitude /= m_rawSamples.size();
            centerLongitude /= m_rawSamples.size();
        }
    }

    // Project all raw samples into metric deviations
    QList<DeviationSample> newSamples;
    QList<double> eastValues;
    QList<double> northValues;
    QList<double> distances;
    newSamples.reserve(m_rawSamples.size());
    eastValues.reserve(m_rawSamples.size());
    northValues.reserve(m_rawSamples.size());
    distances.reserve(m_rawSamples.size());

    const double centerLatRad = centerLatitude * kPi / 180.0;
    for (const RawSample &sample : std::as_const(m_rawSamples)) {
        const double northMeters = (sample.latitude - centerLatitude) * kPi / 180.0 * kEarthRadiusMeters;
        const double eastMeters = (sample.longitude - centerLongitude) * kPi / 180.0 * kEarthRadiusMeters * std::cos(centerLatRad);
        const double distanceMeters = std::hypot(eastMeters, northMeters);
        newSamples.append({eastMeters, northMeters, distanceMeters});
        eastValues.append(eastMeters);
        northValues.append(northMeters);
        distances.append(distanceMeters);
    }

    const int newCount = newSamples.size();

    // Apply model changes with the narrowest possible notification scope
    if (newCount == oldCount + 1) {
        // One sample appended: update existing row data in-place, then insert the new row.
        // The in-place update is needed because adding a point shifts the average center.
        for (int i = 0; i < oldCount; ++i) {
            m_samples[i] = newSamples[i];
        }
        beginInsertRows(QModelIndex(), oldCount, oldCount);
        m_samples.append(newSamples.last());
        endInsertRows();
        // Notify after insert so LatestRole for old last row evaluates correctly (false).
        if (oldCount > 0) {
            emit dataChanged(index(0), index(oldCount - 1),
                {EastMetersRole, NorthMetersRole, DistanceMetersRole, LatestRole});
        }
    } else if (newCount == oldCount && newCount > 0) {
        // Same count but values changed (center move or updateLastSample).
        m_samples = std::move(newSamples);
        emit dataChanged(index(0), index(newCount - 1),
            {EastMetersRole, NorthMetersRole, DistanceMetersRole, LatestRole});
    } else if (newCount == 0 && oldCount == 0) {
        // Nothing to do — called after clear() which already reset the model.
    } else {
        // Fallback for any unexpected size transition.
        beginResetModel();
        m_samples = std::move(newSamples);
        endResetModel();
    }

    // Update aggregate statistics
    m_stats = {
        {QStringLiteral("points"), m_samples.size()},
        {QStringLiteral("centerMode"), m_fixedCenterEnabled ? QStringLiteral("Fixed") : QStringLiteral("Average")},
        {QStringLiteral("centerLatitude"), centerLatitude},
        {QStringLiteral("centerLongitude"), centerLongitude},
        {QStringLiteral("maxDistance"), 1.0},
        {QStringLiteral("horizontal"), buildDistanceStats(distances)},
        {QStringLiteral("latitude"), buildAxisStats(northValues)},
        {QStringLiteral("longitude"), buildAxisStats(eastValues)}
    };

    if (distances.isEmpty()) {
        m_stats.insert(QStringLiteral("maxDistance"), 1.0);
        m_stats.insert(QStringLiteral("horizontal"), emptyDistanceStats());
        m_stats.insert(QStringLiteral("latitude"), emptyAxisStats());
        m_stats.insert(QStringLiteral("longitude"), emptyAxisStats());
    } else {
        const auto maxIt = std::max_element(distances.cbegin(), distances.cend());
        m_stats.insert(QStringLiteral("maxDistance"), qMax(1.0, std::ceil(*maxIt)));
    }

    ++m_revision;
    emit countChanged();
    emit revisionChanged();
    emit statsChanged();
}

QVariantMap DeviationMapModel::buildAxisStats(const QList<double> &values) {
    if (values.isEmpty()) {
        return emptyAxisStats();
    }

    double minValue = values.first();
    double maxValue = values.first();
    double sum = 0.0;
    for (double value : values) {
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
        sum += value;
    }
    const double average = sum / values.size();

    double variance = 0.0;
    for (double value : values) {
        const double delta = value - average;
        variance += delta * delta;
    }
    variance /= values.size();

    return {
        {QStringLiteral("min"), minValue},
        {QStringLiteral("max"), maxValue},
        {QStringLiteral("avg"), average},
        {QStringLiteral("stdDev"), std::sqrt(variance)}
    };
}

QVariantMap DeviationMapModel::buildDistanceStats(const QList<double> &values) {
    if (values.isEmpty()) {
        return emptyDistanceStats();
    }

    QVariantMap stats = buildAxisStats(values);
    QList<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    stats.insert(QStringLiteral("cep50"), percentile(sorted, 0.50));
    stats.insert(QStringLiteral("cep68"), percentile(sorted, 0.68));
    stats.insert(QStringLiteral("cep95"), percentile(sorted, 0.95));
    return stats;
}

double DeviationMapModel::percentile(const QList<double> &sortedValues, double ratio) {
    if (sortedValues.isEmpty()) {
        return 0.0;
    }
    if (sortedValues.size() == 1) {
        return sortedValues.first();
    }

    const double clampedRatio = std::clamp(ratio, 0.0, 1.0);
    const double position = clampedRatio * static_cast<double>(sortedValues.size() - 1);
    const int lowerIndex = static_cast<int>(std::floor(position));
    const int upperIndex = static_cast<int>(std::ceil(position));
    if (lowerIndex == upperIndex) {
        return sortedValues.at(lowerIndex);
    }
    const double weight = position - lowerIndex;
    return sortedValues.at(lowerIndex) * (1.0 - weight) + sortedValues.at(upperIndex) * weight;
}

}  // namespace hdgnss
