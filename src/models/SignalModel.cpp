#include "SignalModel.h"

namespace hdgnss {

namespace {

QString constellationPrefix(const QString &constellation) {
    if (constellation == QStringLiteral("GPS")) return QStringLiteral("G");
    if (constellation == QStringLiteral("GLONASS")) return QStringLiteral("R");
    if (constellation == QStringLiteral("GALILEO")) return QStringLiteral("E");
    if (constellation == QStringLiteral("BEIDOU")) return QStringLiteral("C");
    if (constellation == QStringLiteral("QZSS")) return QStringLiteral("J");
    if (constellation == QStringLiteral("SBAS")) return QStringLiteral("S");
    if (constellation == QStringLiteral("NAVIC") || constellation == QStringLiteral("IRNSS")) return QStringLiteral("I");
    return QStringLiteral("U");
}

QString satelliteLabel(const SatelliteInfo &sat) {
    return QStringLiteral("%1%2")
        .arg(constellationPrefix(sat.constellation))
        .arg(sat.svid, 2, 10, QLatin1Char('0'));
}

}  // namespace

SignalModel::SignalModel(QObject *parent)
    : QAbstractListModel(parent) {
    setStore(&m_ownStore);
}

int SignalModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_store->size();
}

QVariant SignalModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_store->size()) {
        return {};
    }
    const SatelliteInfo &sat = m_store->at(index.row());
    switch (role) {
    case LabelRole: return satelliteLabel(sat);
    case BandRole: return sat.band;
    case ConstellationRole: return sat.constellation;
    case SignalIdRole: return sat.signalId;
    case SvidRole: return sat.svid;
    case Cn0Role: return sat.cn0;
    case StrengthRole: return sat.cn0;
    case UsedRole: return sat.usedInFix;
    case BandGroupRole: return satelliteSignalGroup(sat);
    default: return {};
    }
}

QHash<int, QByteArray> SignalModel::roleNames() const {
    return {
        {LabelRole, "label"},
        {BandRole, "band"},
        {ConstellationRole, "constellation"},
        {SignalIdRole, "signalId"},
        {SvidRole, "svid"},
        {Cn0Role, "cn0"},
        {StrengthRole, "strength"},
        {UsedRole, "usedInFix"},
        {BandGroupRole, "bandGroup"}
    };
}

QVariantList SignalModel::itemsForBand(const QString &band) const {
    QVariantList items;
    for (const SatelliteInfo &sat : m_store->items()) {
        if (!satelliteHasVisibleSignal(sat) || !satelliteMatchesSignalTab(sat, band)) {
            continue;
        }
        items.append(QVariantMap{
            {QStringLiteral("label"), satelliteLabel(sat)},
            {QStringLiteral("constellation"), sat.constellation},
            {QStringLiteral("band"), sat.band},
            {QStringLiteral("bandGroup"), satelliteSignalGroup(sat)},
            {QStringLiteral("signalId"), sat.signalId},
            {QStringLiteral("svid"), sat.svid},
            {QStringLiteral("strength"), sat.cn0},
            {QStringLiteral("usedInFix"), sat.usedInFix}
        });
    }
    return items;
}

void SignalModel::setStore(SatelliteStore *store) {
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
    connect(m_store, &SatelliteStore::revisionChanged, this, &SignalModel::revisionChanged);
    endResetModel();
    emit countChanged();
    emit revisionChanged();
}

SatelliteStore *SignalModel::store() const {
    return m_store;
}

void SignalModel::setSatellites(const QList<SatelliteInfo> &satellites) {
    m_store->setSatellites(satellites);
}

int SignalModel::revision() const {
    return m_store->revision();
}

}  // namespace hdgnss
