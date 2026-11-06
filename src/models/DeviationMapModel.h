#pragma once

#include <deque>
#include <vector>

#include <QAbstractListModel>
#include <QVariantList>
#include <QVariantMap>

namespace hdgnss {

class DeviationMapModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged)

public:
    static constexpr int kMaxSamples = 10000;

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
    Q_INVOKABLE QVariantList canvasPoints(double centerX, double centerY, double maxRadius) const;
    Q_INVOKABLE void clear();

    void addSample(double latitude, double longitude, bool deferNotify = false);
    bool updateLastSample(double latitude, double longitude, bool deferNotify = false);
    void flushDeferredNotifications();
    void setFixedCenterEnabled(bool enabled);
    void setFixedCenter(double latitude, double longitude);
    int revision() const;
    QVariantMap stats() const;

#ifdef HDGNSS_REGRESSION_TESTS
    int regressionRawSampleCount() const;
    QVariantMap regressionRawSample(int index) const;
    void regressionFillRawSamplesForCapTest(int count);
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

    void ensureCalculated() const;
    void markDirty(bool deferNotify);
    void trimRawSamples();
    static double percentileFromPartitioned(std::vector<double> &values,
                                            int startOffset,
                                            double ratio,
                                            int *lowerIndexOut = nullptr);

    std::deque<RawSample> m_rawSamples;
    mutable QList<DeviationSample> m_samples;
    mutable std::vector<double> m_distanceScratch;
    mutable QVariantMap m_stats;
    mutable bool m_calculationDirty = false;
    bool m_notificationsDeferred = false;
    bool m_fixedCenterEnabled = false;
    double m_fixedLatitude = 0.0;
    double m_fixedLongitude = 0.0;
    int m_revision = 0;
};

}  // namespace hdgnss
