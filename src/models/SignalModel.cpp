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
    : QAbstractListModel(parent) {}

int SignalModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_satellites.size();
}

QVariant SignalModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_satellites.size()) {
        return {};
    }
    const SatelliteInfo &sat = m_satellites.at(index.row());
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
    for (const SatelliteInfo &sat : m_satellites) {
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

void SignalModel::setSatellites(const QList<SatelliteInfo> &satellites) {
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

int SignalModel::revision() const {
    return m_revision;
}

}  // namespace hdgnss
