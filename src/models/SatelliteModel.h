#pragma once

#include <QAbstractListModel>

#include "src/models/SatelliteStore.h"
#include "src/protocols/GnssTypes.h"

namespace hdgnss {

class SatelliteModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        ConstellationRole,
        BandRole,
        SvidRole,
        AzimuthRole,
        ElevationRole,
        Cn0Role,
        UsedRole,
        UsedAliasRole,
        BandGroupRole
    };

    explicit SatelliteModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    // Binds the model to a shared store (not owned). Without one, the model
    // uses a private store so setSatellites() still works standalone.
    void setStore(SatelliteStore *store);
    SatelliteStore *store() const;
    void setSatellites(const QList<SatelliteInfo> &satellites);
    int revision() const;

signals:
    void countChanged();
    void revisionChanged();

private:
    SatelliteStore *m_store = nullptr;
    SatelliteStore m_ownStore;
};

}  // namespace hdgnss
