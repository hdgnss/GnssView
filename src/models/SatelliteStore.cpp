#include "SatelliteStore.h"

namespace hdgnss {

namespace {

bool sameSignalState(const SatelliteInfo &a, const SatelliteInfo &b) {
    return a.constellation == b.constellation
        && a.band == b.band
        && a.signalId == b.signalId
        && a.svid == b.svid
        && a.cn0 == b.cn0
        && a.usedInFix == b.usedInFix;
}

bool sameSatellite(const SatelliteInfo &a, const SatelliteInfo &b) {
    return sameSignalState(a, b)
        && a.azimuth == b.azimuth
        && a.elevation == b.elevation;
}

}  // namespace

SatelliteStore::SatelliteStore(QObject *parent)
    : QObject(parent) {}

int SatelliteStore::size() const {
    return m_items.size();
}

const SatelliteInfo &SatelliteStore::at(int row) const {
    return m_items.at(row);
}

const QList<SatelliteInfo> &SatelliteStore::items() const {
    return m_items;
}

int SatelliteStore::revision() const {
    return m_revision;
}

int SatelliteStore::signalRevision() const {
    return m_signalRevision;
}

void SatelliteStore::setSatellites(const QList<SatelliteInfo> &satellites) {
    const int oldSize = m_items.size();
    QHash<QString, const SatelliteInfo *> incoming;
    incoming.reserve(satellites.size());
    for (const SatelliteInfo &sat : satellites) {
        if (satelliteHasVisibleSignal(sat)) {
            incoming.insert(sat.key, &sat);  // last duplicate wins
        }
    }

    bool modified = false;
    bool signalModified = false;

    // 1. Drop rows whose key disappeared (back to front keeps indices valid).
    for (int row = m_items.size() - 1; row >= 0; --row) {
        if (incoming.contains(m_items.at(row).key)) {
            continue;
        }
        emit aboutToRemove(row);
        m_items.removeAt(row);
        emit removed();
        modified = true;
        signalModified = true;
    }

    // 2. Update surviving rows in place.
    QSet<QString> present;
    present.reserve(m_items.size());
    for (int row = 0; row < m_items.size(); ++row) {
        const SatelliteInfo &next = *incoming.value(m_items.at(row).key);
        present.insert(next.key);
        if (!sameSatellite(m_items.at(row), next)) {
            const bool sigChanged = !sameSignalState(m_items.at(row), next);
            m_items[row] = next;
            emit changed(row);
            modified = true;
            if (sigChanged) {
                emit signalChanged(row);
                signalModified = true;
            }
        }
    }

    // 3. Append new keys in input order.
    for (const SatelliteInfo &sat : satellites) {
        if (!satelliteHasVisibleSignal(sat) || present.contains(sat.key)) {
            continue;
        }
        present.insert(sat.key);
        const int row = m_items.size();
        emit aboutToInsert(row);
        m_items.append(*incoming.value(sat.key));
        emit inserted();
        modified = true;
        signalModified = true;
    }

    if (m_items.size() != oldSize) {
        emit countChanged();
    }
    if (signalModified) {
        ++m_signalRevision;
        emit signalRevisionChanged();
    }
    if (modified) {
        ++m_revision;
        emit revisionChanged();
    }
}

}  // namespace hdgnss
