#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <algorithm>
#include <utility>

#include "src/protocols/GnssTypes.h"

namespace hdgnss {

// Shared retention policy for the satellite caches kept by the app controller
// and by protocol parsers.
constexpr qint64 kSatelliteRetentionMs = 30 * 1000;

// Drops satellites not seen within `retentionMs`, then evicts the least
// recently seen entries until at most `maxCount` remain. O(n + k log k)
// instead of rescanning for the oldest entry once per eviction.
inline void pruneSatelliteCache(QHash<QString, SatelliteInfo> &satellites,
                                QHash<QString, qint64> &lastSeenMs,
                                qint64 nowMs,
                                int maxCount,
                                qint64 retentionMs = kSatelliteRetentionMs) {
    for (auto it = lastSeenMs.begin(); it != lastSeenMs.end();) {
        if ((nowMs - it.value()) > retentionMs) {
            satellites.remove(it.key());
            it = lastSeenMs.erase(it);
            continue;
        }
        ++it;
    }

    const qsizetype excess = lastSeenMs.size() - maxCount;
    if (excess <= 0) {
        return;
    }
    QList<std::pair<qint64, QString>> byAge;
    byAge.reserve(lastSeenMs.size());
    for (auto it = lastSeenMs.cbegin(); it != lastSeenMs.cend(); ++it) {
        byAge.append({it.value(), it.key()});
    }
    std::partial_sort(byAge.begin(), byAge.begin() + excess, byAge.end(),
                      [](const auto &a, const auto &b) { return a.first < b.first; });
    for (qsizetype i = 0; i < excess; ++i) {
        lastSeenMs.remove(byAge.at(i).second);
        satellites.remove(byAge.at(i).second);
    }
}

}  // namespace hdgnss
