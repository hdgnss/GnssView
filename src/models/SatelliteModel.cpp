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
        emit countChanged();
    });
    connect(m_store, &SatelliteStore::aboutToRemove, this, [this](int row) {
        beginRemoveRows({}, row, row);
    });
    connect(m_store, &SatelliteStore::removed, this, [this]() {
        endRemoveRows();
        emit countChanged();
    });
    connect(m_store, &SatelliteStore::changed, this, [this](int row) {
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    });
    connect(m_store, &SatelliteStore::revisionChanged, this, &SatelliteModel::revisionChanged);
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

}  // namespace hdgnss
