#pragma once

#include <QAbstractListModel>

#include "src/protocols/GnssTypes.h"

namespace hdgnss {

class SignalModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    enum Roles {
        LabelRole = Qt::UserRole + 1,
        BandRole,
        ConstellationRole,
        SignalIdRole,
        SvidRole,
        Cn0Role,
        StrengthRole,
        UsedRole,
        BandGroupRole
    };

    explicit SignalModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantList itemsForBand(const QString &band) const;
    void setSatellites(const QList<SatelliteInfo> &satellites);
    int revision() const;

signals:
    void countChanged();
    void revisionChanged();

private:
    QList<SatelliteInfo> m_satellites;
    int m_revision = 0;
};

}  // namespace hdgnss
