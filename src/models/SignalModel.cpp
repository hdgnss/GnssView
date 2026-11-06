#include "SignalModel.h"

#include <algorithm>

namespace hdgnss {

namespace {

int constellationSortOrder(const QString &constellation) {
    if (constellation == QStringLiteral("GPS")) return 0;
    if (constellation == QStringLiteral("GLONASS")) return 1;
    if (constellation == QStringLiteral("GALILEO")) return 2;
    if (constellation == QStringLiteral("BEIDOU")) return 3;
    if (constellation == QStringLiteral("QZSS")) return 4;
    if (constellation == QStringLiteral("SBAS")) return 5;
    if (constellation == QStringLiteral("NAVIC") || constellation == QStringLiteral("IRNSS")) return 6;
    return 9;
}

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
    const int currentRev = revision();
    if (m_cachedSignalRevision != currentRev) {
        m_cachedBandItems.clear();
        m_cachedSignalRevision = currentRev;
    }

    const QString cacheKey = band.trimmed().toUpper();
    if (const auto it = m_cachedBandItems.constFind(cacheKey); it != m_cachedBandItems.cend()) {
        return it.value();
    }

    struct SortEntry {
        const SatelliteInfo *sat = nullptr;
        QString label;
        int order = 9;
    };
    QList<SortEntry> matching;
    matching.reserve(m_store->size());
    for (const SatelliteInfo &sat : m_store->items()) {
        if (!satelliteHasVisibleSignal(sat) || !satelliteMatchesSignalTab(sat, cacheKey)) {
            continue;
        }
        matching.append({
            &sat,
            satelliteLabel(sat),
            constellationSortOrder(sat.constellation)
        });
    }

    std::sort(matching.begin(), matching.end(), [](const SortEntry &a, const SortEntry &b) {
        if (a.order != b.order) {
            return a.order < b.order;
        }
        return a.label < b.label;
    });

    QVariantList items;
    items.reserve(matching.size());
    for (const SortEntry &entry : matching) {
        const SatelliteInfo &sat = *entry.sat;
        items.append(QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("satellite")},
            {QStringLiteral("label"), entry.label},
            {QStringLiteral("constellation"), sat.constellation.isEmpty() ? QStringLiteral("GNSS") : sat.constellation},
            {QStringLiteral("band"), sat.band},
            {QStringLiteral("bandGroup"), satelliteSignalGroup(sat)},
            {QStringLiteral("signalId"), sat.signalId},
            {QStringLiteral("svid"), sat.svid},
            {QStringLiteral("strength"), sat.cn0},
            {QStringLiteral("usedInFix"), sat.usedInFix}
        });
    }

    m_cachedBandItems.insert(cacheKey, items);
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
    m_cachedBandItems.clear();
    m_cachedSignalRevision = -1;
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
    connect(m_store, &SatelliteStore::signalChanged, this, [this](int row) {
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
    });
    connect(m_store, &SatelliteStore::countChanged, this, &SignalModel::countChanged);
    connect(m_store, &SatelliteStore::signalRevisionChanged, this, [this]() {
        m_cachedBandItems.clear();
        m_cachedSignalRevision = -1;
        emit revisionChanged();
    });
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
    return m_store->signalRevision();
}

}  // namespace hdgnss
