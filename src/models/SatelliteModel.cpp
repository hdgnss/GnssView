#include "SatelliteModel.h"

namespace hdgnss {

SatelliteModel::SatelliteModel(QObject *parent)
    : QAbstractListModel(parent) {
    setStore(&m_ownStore);
}

int SatelliteModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_store->size();
}

QVariant SatelliteModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_store->size()) {
        return {};
    }
    const SatelliteInfo &sat = m_store->at(index.row());
    switch (role) {
    case KeyRole: return sat.key;
    case ConstellationRole: return sat.constellation;
    case BandRole: return sat.band;
    case SvidRole: return sat.svid;
    case AzimuthRole: return sat.azimuth;
    case ElevationRole: return sat.elevation;
    case Cn0Role: return sat.cn0;
    case UsedRole: return sat.usedInFix;
    case UsedAliasRole: return sat.usedInFix;
    case BandGroupRole: return satelliteSignalGroup(sat);
    default: return {};
    }
}

QHash<int, QByteArray> SatelliteModel::roleNames() const {
    return {
        {KeyRole, "key"},
        {ConstellationRole, "constellation"},
        {BandRole, "band"},
        {SvidRole, "svid"},
        {AzimuthRole, "azimuth"},
        {ElevationRole, "elevation"},
        {Cn0Role, "cn0"},
        {UsedRole, "usedInFix"},
        {UsedAliasRole, "used"},
        {BandGroupRole, "bandGroup"}
    };
}

QVariantMap SatelliteModel::get(int row) const {
    if (row < 0 || row >= m_store->size()) {
        return {};
    }
    const SatelliteInfo &sat = m_store->at(row);
    return {
        {QStringLiteral("key"), sat.key},
        {QStringLiteral("constellation"), sat.constellation},
        {QStringLiteral("band"), sat.band},
        {QStringLiteral("signalId"), sat.signalId},
        {QStringLiteral("svid"), sat.svid},
        {QStringLiteral("azimuth"), sat.azimuth},
        {QStringLiteral("elevation"), sat.elevation},
        {QStringLiteral("cn0"), sat.cn0},
        {QStringLiteral("usedInFix"), sat.usedInFix},
        {QStringLiteral("bandGroup"), satelliteSignalGroup(sat)}
    };
}

void SatelliteModel::setStore(SatelliteStore *store) {
    SatelliteStore *target = store ? store : &m_ownStore;
    if (target == m_store) {
        return;
    }
    beginResetModel();
    if (m_store) {
        disconnect(m_store, nullptr, this, nullptr);
    }
    m_store = target;
    connect(m_store, &SatelliteStore::aboutToInsert, this, [this](int row) {
        beginInsertRows({}, row, row);
    });
    connect(m_store, &SatelliteStore::inserted, this, [this]() {
        endInsertRows();
    });
    connect(m_store, &SatelliteStore::aboutToRemove, this, [this](int row) {
        beginRemoveRows({}, row, row);
    });
    connect(m_store, &SatelliteStore::removed, this, [this]() {
        endRemoveRows();
    });
    connect(m_store, &SatelliteStore::changed, this, [this](int row) {
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    });
    connect(m_store, &SatelliteStore::countChanged, this, &SatelliteModel::countChanged);
    connect(m_store, &SatelliteStore::revisionChanged, this, [this]() {
        rebuildSummaryStats();
        emit revisionChanged();
    });
    rebuildSummaryStats();
    endResetModel();
    emit countChanged();
    emit revisionChanged();
}

SatelliteStore *SatelliteModel::store() const {
    return m_store;
}

void SatelliteModel::setSatellites(const QList<SatelliteInfo> &satellites) {
    m_store->setSatellites(satellites);
}

int SatelliteModel::revision() const {
    return m_store->revision();
}

QVariantMap SatelliteModel::constellationCounts() const {
    return m_constellationCounts;
}

QVariantMap SatelliteModel::bandStats() const {
    return m_bandStats;
}

void SatelliteModel::rebuildSummaryStats() {
    QHash<QString, int> countsByConstellation;
    struct PairCount {
        int used = 0;
        int view = 0;
    };
    QHash<QString, PairCount> countsByBand;
    PairCount allCount;

    if (m_store) {
        for (const SatelliteInfo &sat : m_store->items()) {
            countsByConstellation[sat.constellation] += 1;
            const QString group = satelliteSignalGroup(sat);
            PairCount &bandEntry = countsByBand[group];
            bandEntry.view += 1;
            allCount.view += 1;
            if (sat.usedInFix) {
                bandEntry.used += 1;
                allCount.used += 1;
            }
        }
    }

    QVariantMap nextConstellations;
    nextConstellations.insert(QStringLiteral("ALL"), allCount.view);
    for (auto it = countsByConstellation.cbegin(); it != countsByConstellation.cend(); ++it) {
        nextConstellations.insert(it.key(), it.value());
    }

    QVariantMap nextBands;
    nextBands.insert(QStringLiteral("ALL"), QVariantMap{
        {QStringLiteral("used"), allCount.used},
        {QStringLiteral("view"), allCount.view}
    });
    for (auto it = countsByBand.cbegin(); it != countsByBand.cend(); ++it) {
        nextBands.insert(it.key(), QVariantMap{
            {QStringLiteral("used"), it.value().used},
            {QStringLiteral("view"), it.value().view}
        });
    }

    m_constellationCounts = std::move(nextConstellations);
    m_bandStats = std::move(nextBands);
}

}  // namespace hdgnss
