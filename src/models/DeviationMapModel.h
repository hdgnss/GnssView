#pragma once

#include <QAbstractListModel>
#include <QVariantMap>

namespace hdgnss {

class DeviationMapModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged)

public:
    enum Roles {
        EastMetersRole = Qt::UserRole + 1,
        NorthMetersRole,
        DistanceMetersRole,
        SequenceRole,
        LatestRole
    };

    explicit DeviationMapModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void clear();

    void addSample(double latitude, double longitude);
    bool updateLastSample(double latitude, double longitude);
    void setFixedCenterEnabled(bool enabled);
    void setFixedCenter(double latitude, double longitude);
    int revision() const;
    QVariantMap stats() const;

#ifdef HDGNSS_REGRESSION_TESTS
    int regressionRawSampleCount() const;
    QVariantMap regressionRawSample(int index) const;
#endif

signals:
    void countChanged();
    void revisionChanged();
    void statsChanged();

private:
    struct RawSample {
        double latitude = 0.0;
        double longitude = 0.0;
    };

    struct DeviationSample {
        double eastMeters = 0.0;
        double northMeters = 0.0;
        double distanceMeters = 0.0;
    };

    void recalculate();
    static QVariantMap buildAxisStats(const QList<double> &values);
    static QVariantMap buildDistanceStats(const QList<double> &values);
    static double percentile(const QList<double> &sortedValues, double ratio);

    QList<RawSample> m_rawSamples;
    QList<DeviationSample> m_samples;
    QVariantMap m_stats;
    bool m_fixedCenterEnabled = false;
    double m_fixedLatitude = 0.0;
    double m_fixedLongitude = 0.0;
    int m_revision = 0;
};

}  // namespace hdgnss
