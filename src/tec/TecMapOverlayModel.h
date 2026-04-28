#pragma once

#include <memory>

#include <QList>
#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include <QUrl>

#include "hdgnss/ITecDataPlugin.h"
#include "src/tec/TecTypes.h"

namespace hdgnss {

class AppSettings;
class TecPluginLoader;

class TecMapOverlayModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl overlaySource READ overlaySource NOTIFY overlayChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY overlayChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString datasetLabel READ datasetLabel NOTIFY statusChanged)
    Q_PROPERTY(QVariantList availableTecPlugins READ availableTecPlugins NOTIFY availableTecPluginsChanged)
    Q_PROPERTY(QStringList pluginLoadErrors READ pluginLoadErrors NOTIFY availableTecPluginsChanged)
    Q_PROPERTY(QStringList pluginSearchPaths READ pluginSearchPaths NOTIFY availableTecPluginsChanged)
    Q_PROPERTY(QVariantList activeTecSources READ activeTecSources NOTIFY activeTecSourcesChanged)
    Q_PROPERTY(int activeSourceIndex READ activeSourceIndex WRITE setActiveSourceIndex NOTIFY activeSourceChanged)
    Q_PROPERTY(QString activeSourceId READ activeSourceId NOTIFY activeSourceChanged)

public:
    explicit TecMapOverlayModel(AppSettings *settings, QObject *parent = nullptr);
    ~TecMapOverlayModel() override;

    [[nodiscard]] QUrl overlaySource() const;
    [[nodiscard]] bool ready() const;
    [[nodiscard]] bool loading() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString datasetLabel() const;
    [[nodiscard]] QVariantList availableTecPlugins() const;
    [[nodiscard]] QStringList pluginLoadErrors() const;
    [[nodiscard]] QStringList pluginSearchPaths() const;
    [[nodiscard]] QVariantList activeTecSources() const;
    [[nodiscard]] int activeSourceIndex() const;
    [[nodiscard]] QString activeSourceId() const;
    Q_INVOKABLE QVariantMap sampleAtCanvasPoint(qreal x,
                                                qreal y,
                                                qreal width,
                                                qreal height) const;
    Q_INVOKABLE void setActiveSourceIndex(int index);
    void setTestDataset(const hdgnss::TecGridData &dataset, const QString &pluginId = QString());

    void setObservationTime(const QDateTime &observationTimeUtc);
    static bool shouldRequestRefresh(const QDateTime &lastRequestObservationTimeUtc,
                                     const QDateTime &observationTimeUtc,
                                     qint64 downloadIntervalSeconds);
    static QString refreshCountdownText(const QDateTime &lastRequestObservationTimeUtc,
                                        const QDateTime &observationTimeUtc,
                                        qint64 downloadIntervalSeconds);

signals:
    void overlayChanged();
    void loadingChanged();
    void statusChanged();
    void availableTecPluginsChanged();
    void activeTecSourcesChanged();
    void activeSourceChanged();

private slots:
    void handlePluginsEnabledChanged();
    void handlePluginLoadingChanged(bool loadingValue);
    void handlePluginDataReady(const hdgnss::TecGridData &dataset);
    void handlePluginErrorOccurred(const QString &message);

private:
    struct SourceState {
        QString pluginId;
        QString displayName;
        ITecDataPlugin *plugin = nullptr;
        QObject *pluginObject = nullptr;
        QDateTime lastRequestObservationTimeUtc;
        QDateTime datasetTimestampUtc;
        TecGridData dataset;
        QUrl overlaySource;
        QString overlayPath;
        QString statusText = QStringLiteral("TEC waiting for GNSS UTC time");
        QString datasetLabel = QStringLiteral("TEC unavailable");
        bool loading = false;
    };

    void reloadPlugin();
    void connectPluginSignals(QObject *pluginObject);
    void disconnectPluginSignals();
    void rebuildDescriptors();
    int sourceIndexForSender() const;
    int firstEnabledSourceIndex() const;
    void ensureActiveSourceSelection(const QString &preferredPluginId = QString());
    void applyActiveSourceState();
    void requestRefreshForEnabledSources();
    void setSourceStatusText(int index, const QString &text);
    void updateDatasetLabel(int index);
    void clearOverlay(int index);
    void renderDataset(int index, const TecGridData &dataset);

    AppSettings *m_settings = nullptr;
    std::unique_ptr<TecPluginLoader> m_pluginLoader;
    QTimer m_requestDebounceTimer;
    QDateTime m_pendingObservationTimeUtc;
    QList<SourceState> m_sources;
    QUrl m_overlaySource;
    QString m_statusText = QStringLiteral("TEC waiting for GNSS UTC time");
    QString m_datasetLabel = QStringLiteral("TEC unavailable");
    QVariantList m_availableTecPlugins;
    QStringList m_pluginLoadErrors;
    QStringList m_pluginSearchPaths;
    QVariantList m_activeTecSources;
    int m_activeSourceIndex = -1;
    QString m_activeSourceId;
    qint64 m_revision = 0;
    bool m_loading = false;
    bool m_pluginsEnabled = true;
};

}  // namespace hdgnss
