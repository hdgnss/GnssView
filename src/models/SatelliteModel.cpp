#include "SatelliteModel.h"

namespace hdgnss {

SatelliteModel::SatelliteModel(QObject *parent)
    : QAbstractListModel(parent) {}

int SatelliteModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_satellites.size();
}

QVariant SatelliteModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_satellites.size()) {
        return {};
    }
    const SatelliteInfo &sat = m_satellites.at(index.row());
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
    if (row < 0 || row >= m_satellites.size()) {
        return {};
    }
    const SatelliteInfo &sat = m_satellites.at(row);
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

void SatelliteModel::setSatellites(const QList<SatelliteInfo> &satellites) {
    beginResetModel();
    m_satellites.clear();
    m_satellites.reserve(satellites.size());
    for (const SatelliteInfo &sat : satellites) {
        if (satelliteHasVisibleSignal(sat)) {
            m_satellites.append(sat);
        }
    }
    endResetModel();
    ++m_revision;
    emit countChanged();
    emit revisionChanged();
}

int SatelliteModel::revision() const {
    return m_revision;
}

}  // namespace hdgnss
