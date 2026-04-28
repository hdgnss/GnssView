#include "TecMapOverlayModel.h"

#include <cmath>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QMetaType>
#include <QPainterPath>
#include <QPointF>
#include <QStandardPaths>
#include <limits>

#include "hdgnss/IConfigurablePlugin.h"
#include "src/core/AppSettings.h"
#include "src/core/PluginMetadata.h"
#include "src/core/TecPluginLoader.h"
#include "src/tec/TecMapRenderer.h"

namespace hdgnss {

namespace {

QString overlayDirectoryPath() {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    return QDir(root).filePath(QStringLiteral("hdgnss/tec"));
}

constexpr double kWorldMinY = -91.296;
constexpr double kWorldHeight = 182.592;
constexpr double kDefaultLonStepDegrees = 5.0;
constexpr double kDefaultLatStepDegrees = 2.5;

constexpr double kRobinsonX[] = {
    1.0, 0.9986, 0.9954, 0.99, 0.9822, 0.973, 0.96, 0.9427, 0.9216,
    0.8962, 0.8679, 0.835, 0.7986, 0.7597, 0.7186, 0.6732, 0.6213,
    0.5722, 0.5322
};
constexpr double kRobinsonY[] = {
    0.0, 0.062, 0.124, 0.186, 0.248, 0.31, 0.372, 0.434, 0.4958,
    0.5571, 0.6176, 0.6769, 0.7346, 0.7903, 0.8435, 0.8936, 0.9394,
    0.9761, 1.0
};

double clamp(double value, double minValue, double maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

QPointF robinsonProject(double lonDeg, double latDeg) {
    if (!std::isfinite(lonDeg) || !std::isfinite(latDeg)) {
        return {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};
    }

    const double absLat = clamp(std::abs(latDeg), 0.0, 90.0);
    const int bandIndex = std::min<int>(std::size(kRobinsonX) - 2, static_cast<int>(std::floor(absLat / 5.0)));
    const double bandStart = bandIndex * 5.0;
    const double ratio = (absLat - bandStart) / 5.0;
    const double xFactor = lerp(kRobinsonX[bandIndex], kRobinsonX[bandIndex + 1], ratio);
    const double yFactor = lerp(kRobinsonY[bandIndex], kRobinsonY[bandIndex + 1], ratio);
    const double x = lonDeg * xFactor;
    const double y = (latDeg >= 0.0 ? -1.0 : 1.0) * 90.0 * 1.0144 * yFactor;
    return {x, y};
}

QPointF projectedCanvasPoint(double width, double height, double lonDeg, double latDeg) {
    const QPointF projected = robinsonProject(lonDeg, latDeg);
    if (!std::isfinite(projected.x()) || !std::isfinite(projected.y()) || width <= 0.0 || height <= 0.0) {
        return {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};
    }

    return {
        (projected.x() + 180.0) / 360.0 * width,
        (projected.y() - kWorldMinY) / kWorldHeight * height
    };
}

QPainterPath cellPath(double width,
                      double height,
                      double lonMin,
                      double lonMax,
                      double latMin,
                      double latMax) {
    const double stepDegrees = std::max(0.5, (lonMax - lonMin) / 4.0);
    QPainterPath path;
    bool firstPoint = true;

    auto addPoint = [&](double lon, double lat) {
        const QPointF point = projectedCanvasPoint(width, height, lon, lat);
        if (!std::isfinite(point.x()) || !std::isfinite(point.y())) {
            return;
        }
        if (firstPoint) {
            path.moveTo(point);
            firstPoint = false;
        } else {
            path.lineTo(point);
        }
    };

    for (double lon = lonMin; lon <= lonMax + 0.001; lon += stepDegrees) {
        addPoint(std::min(lon, lonMax), latMin);
    }
    addPoint(lonMax, latMin);
    for (double lon = lonMax; lon >= lonMin - 0.001; lon -= stepDegrees) {
        addPoint(std::max(lon, lonMin), latMax);
    }
    addPoint(lonMin, latMax);

    if (!firstPoint) {
        path.closeSubpath();
    }
    return path;
}

QVariantMap tecPluginDescriptor(QObject *pluginObject, AppSettings *settings) {
    QVariantMap descriptor;
    if (!pluginObject) {
        return descriptor;
    }

    IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(pluginObject);
    const QString pluginId = configurable
        ? configurable->settingsId().trimmed()
        : QStringLiteral("tec.%1").arg(pluginObject->metaObject()->className()).toLower();
    const QString displayName = configurable
        ? configurable->settingsDisplayName().trimmed()
        : QString::fromLatin1(pluginObject->metaObject()->className());
    const QVariantList fieldDefinitions = configurable ? configurable->settingsFields() : QVariantList{};
    QVariantList fields;
    fields.reserve(fieldDefinitions.size());
    for (const QVariant &definitionValue : fieldDefinitions) {
        QVariantMap field = definitionValue.toMap();
        const QString key = field.value(QStringLiteral("key")).toString().trimmed();
        if (key.isEmpty()) {
            continue;
        }
        field.insert(QStringLiteral("value"), settings ? settings->pluginSettingValue(pluginId, key) : QVariant{});
        fields.append(field);
    }

    descriptor.insert(QStringLiteral("pluginId"), pluginId);
    descriptor.insert(QStringLiteral("category"), QStringLiteral("tec"));
    descriptor.insert(QStringLiteral("displayName"), displayName.isEmpty() ? pluginId : displayName);
    insertPluginVersion(descriptor, pluginObject);
    descriptor.insert(QStringLiteral("enabled"), settings ? settings->pluginEnabled(pluginId) : true);
    descriptor.insert(QStringLiteral("fields"), fields);
    descriptor.insert(QStringLiteral("fieldCount"), fields.size());
    return descriptor;
}

}  // namespace

TecMapOverlayModel::TecMapOverlayModel(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_pluginLoader(std::make_unique<TecPluginLoader>()) {
    qRegisterMetaType<hdgnss::TecGridData>("hdgnss::TecGridData");
    m_requestDebounceTimer.setSingleShot(true);
    m_requestDebounceTimer.setInterval(250);
    connect(&m_requestDebounceTimer, &QTimer::timeout, this, &TecMapOverlayModel::requestRefreshForEnabledSources);

    if (m_settings) {
        m_pluginsEnabled = m_settings->pluginsEnabled();
        connect(m_settings, &AppSettings::pluginsEnabledChanged, this, &TecMapOverlayModel::handlePluginsEnabledChanged);
        connect(m_settings, &AppSettings::pluginDirectoryChanged, this, [this]() {
            QMetaObject::invokeMethod(this, &TecMapOverlayModel::reloadPlugin, Qt::QueuedConnection);
        });
        connect(m_settings, &AppSettings::pluginSettingsChanged, this, [this]() {
            QMetaObject::invokeMethod(this, &TecMapOverlayModel::reloadPlugin, Qt::QueuedConnection);
        });
    }

    reloadPlugin();
}

TecMapOverlayModel::~TecMapOverlayModel() {
    disconnectPluginSignals();
    for (int index = 0; index < m_sources.size(); ++index) {
        clearOverlay(index);
    }
}

QUrl TecMapOverlayModel::overlaySource() const {
    return m_overlaySource;
}

bool TecMapOverlayModel::ready() const {
    return m_overlaySource.isValid() && !m_overlaySource.isEmpty();
}

bool TecMapOverlayModel::loading() const {
    return m_loading;
}

QString TecMapOverlayModel::statusText() const {
    return m_statusText;
}

QString TecMapOverlayModel::datasetLabel() const {
    return m_datasetLabel;
}

QVariantList TecMapOverlayModel::availableTecPlugins() const {
    return m_availableTecPlugins;
}

QStringList TecMapOverlayModel::pluginLoadErrors() const {
    return m_pluginLoadErrors;
}

QStringList TecMapOverlayModel::pluginSearchPaths() const {
    return m_pluginSearchPaths;
}

QVariantList TecMapOverlayModel::activeTecSources() const {
    return m_activeTecSources;
}

int TecMapOverlayModel::activeSourceIndex() const {
    for (int index = 0; index < m_activeTecSources.size(); ++index) {
        if (m_activeTecSources.at(index).toMap().value(QStringLiteral("pluginId")).toString() == m_activeSourceId) {
            return index;
        }
    }
    return -1;
}

QString TecMapOverlayModel::activeSourceId() const {
    return m_activeSourceId;
}

QVariantMap TecMapOverlayModel::sampleAtCanvasPoint(qreal x,
                                                    qreal y,
                                                    qreal width,
                                                    qreal height) const {
    QVariantMap result;
    if (m_activeSourceIndex < 0 || m_activeSourceIndex >= m_sources.size()
        || !m_sources.at(m_activeSourceIndex).dataset.isValid()
        || width <= 0.0 || height <= 0.0 || !std::isfinite(x) || !std::isfinite(y)) {
        return result;
    }

    const TecGridData &dataset = m_sources.at(m_activeSourceIndex).dataset;
    const QPointF clickPoint(x, y);
    const double lonHalfStep = (dataset.longitudeStepDegrees > 0.0 ? dataset.longitudeStepDegrees : kDefaultLonStepDegrees) * 0.5;
    const double latHalfStep = (dataset.latitudeStepDegrees > 0.0 ? dataset.latitudeStepDegrees : kDefaultLatStepDegrees) * 0.5;

    int bestIndex = -1;
    double bestDistanceSquared = std::numeric_limits<double>::max();

    for (int i = 0; i < dataset.samples.size(); ++i) {
        const TecSample &sample = dataset.samples.at(i);
        const double lonMin = clamp(sample.longitude - lonHalfStep, -180.0, 180.0);
        const double lonMax = clamp(sample.longitude + lonHalfStep, -180.0, 180.0);
        const double latMin = clamp(sample.latitude - latHalfStep, -90.0, 90.0);
        const double latMax = clamp(sample.latitude + latHalfStep, -90.0, 90.0);
        if (lonMax <= lonMin || latMax <= latMin) {
            continue;
        }

        const QPainterPath path = cellPath(width, height, lonMin, lonMax, latMin, latMax);
        if (!path.isEmpty() && path.contains(clickPoint)) {
            bestIndex = i;
            break;
        }

        const QPointF samplePoint = projectedCanvasPoint(width, height, sample.longitude, sample.latitude);
        if (!std::isfinite(samplePoint.x()) || !std::isfinite(samplePoint.y())) {
            continue;
        }

        const double dx = samplePoint.x() - clickPoint.x();
        const double dy = samplePoint.y() - clickPoint.y();
        const double distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestIndex = i;
        }
    }

    if (bestIndex < 0) {
        return result;
    }

    const QPointF matchedPoint = projectedCanvasPoint(width, height,
                                                      dataset.samples.at(bestIndex).longitude,
                                                      dataset.samples.at(bestIndex).latitude);
    const double dx = matchedPoint.x() - clickPoint.x();
    const double dy = matchedPoint.y() - clickPoint.y();
    if (bestDistanceSquared != std::numeric_limits<double>::max() && bestDistanceSquared > 24.0 * 24.0) {
        return result;
    }
    if (std::isfinite(dx) && std::isfinite(dy) && bestDistanceSquared == std::numeric_limits<double>::max()) {
        bestDistanceSquared = dx * dx + dy * dy;
    }

    const TecSample &sample = dataset.samples.at(bestIndex);
    result.insert(QStringLiteral("valid"), true);
    result.insert(QStringLiteral("longitude"), sample.longitude);
    result.insert(QStringLiteral("latitude"), sample.latitude);
    result.insert(QStringLiteral("tec"), sample.tec);
    result.insert(QStringLiteral("qualityFlag"), sample.qualityFlag);
    result.insert(QStringLiteral("timestampUtc"), dataset.timestampUtc.toUTC());
    result.insert(QStringLiteral("sourceName"), dataset.sourceName);
    return result;
}

void TecMapOverlayModel::setActiveSourceIndex(int index) {
    if (index < 0 || index >= m_activeTecSources.size()) {
        return;
    }

    const QString pluginId = m_activeTecSources.at(index).toMap().value(QStringLiteral("pluginId")).toString();
    for (int sourceIndex = 0; sourceIndex < m_sources.size(); ++sourceIndex) {
        if (m_sources.at(sourceIndex).pluginId == pluginId) {
            if (m_activeSourceIndex == sourceIndex) {
                return;
            }
            m_activeSourceIndex = sourceIndex;
            applyActiveSourceState();
            rebuildDescriptors();
            emit activeSourceChanged();
            return;
        }
    }
}

void TecMapOverlayModel::setTestDataset(const hdgnss::TecGridData &dataset, const QString &pluginId) {
    const QString resolvedPluginId = pluginId.trimmed().isEmpty()
        ? QStringLiteral("tec.test")
        : pluginId.trimmed();

    int sourceIndex = -1;
    for (int index = 0; index < m_sources.size(); ++index) {
        if (m_sources.at(index).pluginId == resolvedPluginId) {
            sourceIndex = index;
            break;
        }
    }

    if (sourceIndex < 0) {
        SourceState source;
        source.pluginId = resolvedPluginId;
        source.displayName = dataset.sourceName.isEmpty() ? resolvedPluginId : dataset.sourceName;
        m_sources.append(source);
        sourceIndex = m_sources.size() - 1;
    }

    m_sources[sourceIndex].dataset = dataset;
    m_sources[sourceIndex].datasetTimestampUtc = dataset.timestampUtc.toUTC();
    m_sources[sourceIndex].statusText = QStringLiteral("TEC ready");
    updateDatasetLabel(sourceIndex);
    m_activeSourceIndex = sourceIndex;
    applyActiveSourceState();
    rebuildDescriptors();
}

bool TecMapOverlayModel::shouldRequestRefresh(const QDateTime &lastRequestObservationTimeUtc,
                                              const QDateTime &observationTimeUtc,
                                              qint64 downloadIntervalSeconds) {
    if (!observationTimeUtc.isValid()) {
        return false;
    }
    if (!lastRequestObservationTimeUtc.isValid()) {
        return true;
    }
    if (downloadIntervalSeconds <= 0) {
        return true;
    }

    return lastRequestObservationTimeUtc.toUTC().secsTo(observationTimeUtc.toUTC()) >= downloadIntervalSeconds;
}

QString TecMapOverlayModel::refreshCountdownText(const QDateTime &lastRequestObservationTimeUtc,
                                                 const QDateTime &observationTimeUtc,
                                                 qint64 downloadIntervalSeconds) {
    if (!lastRequestObservationTimeUtc.isValid() || !observationTimeUtc.isValid() || downloadIntervalSeconds <= 0) {
        return {};
    }

    const qint64 elapsedSeconds = lastRequestObservationTimeUtc.toUTC().secsTo(observationTimeUtc.toUTC());
    const qint64 remainingSeconds = downloadIntervalSeconds - elapsedSeconds;
    if (remainingSeconds <= 0) {
        return QStringLiteral(" | refresh due");
    }

    const int remainingMinutes = static_cast<int>(std::ceil(static_cast<double>(remainingSeconds) / 60.0));
    return remainingMinutes <= 1 ? QStringLiteral(" | next refresh in <1m")
                                 : QStringLiteral(" | next refresh in %1m").arg(remainingMinutes);
}

void TecMapOverlayModel::setObservationTime(const QDateTime &observationTimeUtc) {
    m_pendingObservationTimeUtc = observationTimeUtc.toUTC();
    if (!m_pendingObservationTimeUtc.isValid()) {
        for (int index = 0; index < m_sources.size(); ++index) {
            clearOverlay(index);
            m_sources[index].lastRequestObservationTimeUtc = {};
            m_sources[index].datasetTimestampUtc = {};
            m_sources[index].dataset = {};
            m_sources[index].loading = false;
            updateDatasetLabel(index);
            setSourceStatusText(index, QStringLiteral("TEC waiting for GNSS UTC time"));
        }
        applyActiveSourceState();
        rebuildDescriptors();
        return;
    }

    if (!m_pluginsEnabled || firstEnabledSourceIndex() < 0) {
        applyActiveSourceState();
        rebuildDescriptors();
        return;
    }

    bool hasPendingRefresh = false;
    for (int index = 0; index < m_sources.size(); ++index) {
        const SourceState &source = m_sources.at(index);
        if (!source.plugin || (m_settings && !m_settings->pluginEnabled(source.pluginId))) {
            continue;
        }
        updateDatasetLabel(index);
        if (shouldRequestRefresh(source.lastRequestObservationTimeUtc,
                                 m_pendingObservationTimeUtc,
                                 source.plugin->downloadIntervalSeconds())) {
            setSourceStatusText(index, QStringLiteral("Loading %1").arg(source.displayName));
            hasPendingRefresh = true;
        }
    }

    applyActiveSourceState();
    rebuildDescriptors();
    if (hasPendingRefresh) {
        m_requestDebounceTimer.start();
    }
}

void TecMapOverlayModel::handlePluginsEnabledChanged() {
    m_pluginsEnabled = m_settings && m_settings->pluginsEnabled();
    reloadPlugin();
}

void TecMapOverlayModel::handlePluginLoadingChanged(bool loadingValue) {
    const int index = sourceIndexForSender();
    if (index < 0 || m_sources[index].loading == loadingValue) {
        return;
    }
    m_sources[index].loading = loadingValue;
    applyActiveSourceState();
    rebuildDescriptors();
}

void TecMapOverlayModel::handlePluginDataReady(const hdgnss::TecGridData &dataset) {
    const int index = sourceIndexForSender();
    if (index < 0) {
        return;
    }
    renderDataset(index, dataset);
}

void TecMapOverlayModel::handlePluginErrorOccurred(const QString &message) {
    const int index = sourceIndexForSender();
    if (index < 0) {
        return;
    }
    if (!m_sources.at(index).datasetTimestampUtc.isValid()) {
        clearOverlay(index);
    }
    setSourceStatusText(index, message);
    applyActiveSourceState();
    rebuildDescriptors();
}

void TecMapOverlayModel::reloadPlugin() {
    const QString preferredPluginId = m_activeSourceId;
    disconnectPluginSignals();
    for (int index = 0; index < m_sources.size(); ++index) {
        clearOverlay(index);
    }
    m_sources.clear();
    m_pluginLoader = std::make_unique<TecPluginLoader>();
    m_pluginLoadErrors.clear();
    m_pluginSearchPaths.clear();

    const QString pluginDirectoryRoot = m_settings ? m_settings->pluginDirectory() : QString();
    const QStringList pluginSearchPaths = TecPluginLoader::defaultSearchPaths(QCoreApplication::applicationDirPath(),
                                                                              pluginDirectoryRoot);
    m_pluginSearchPaths = pluginSearchPaths;
    m_pluginLoader->loadFromDirectories(pluginSearchPaths);
    m_pluginLoadErrors = m_pluginLoader->errors();

    const QList<ITecDataPlugin *> plugins = m_pluginLoader->plugins();
    const QList<QObject *> pluginObjects = m_pluginLoader->pluginObjects();
    for (int index = 0; index < plugins.size() && index < pluginObjects.size(); ++index) {
        ITecDataPlugin *plugin = plugins.at(index);
        QObject *pluginObject = pluginObjects.at(index);
        if (!plugin || !pluginObject) {
            continue;
        }

        SourceState source;
        source.plugin = plugin;
        source.pluginObject = pluginObject;

        if (IConfigurablePlugin *configurable = qobject_cast<IConfigurablePlugin *>(pluginObject)) {
            source.pluginId = configurable->settingsId().trimmed();
            source.displayName = configurable->settingsDisplayName().trimmed();
            configurable->applySettings(m_settings ? m_settings->pluginPrivateSettings(source.pluginId) : QVariantMap{});
        }
        if (source.pluginId.isEmpty()) {
            source.pluginId = QStringLiteral("tec.%1").arg(pluginObject->metaObject()->className()).toLower();
        }
        if (source.displayName.isEmpty()) {
            source.displayName = plugin->sourceName().trimmed();
        }
        if (source.displayName.isEmpty()) {
            source.displayName = source.pluginId;
        }

        m_sources.append(source);
        updateDatasetLabel(m_sources.size() - 1);
        connectPluginSignals(pluginObject);
    }

    ensureActiveSourceSelection(preferredPluginId);
    rebuildDescriptors();
    applyActiveSourceState();

    if (m_pendingObservationTimeUtc.isValid() && m_pluginsEnabled && firstEnabledSourceIndex() >= 0) {
        m_requestDebounceTimer.start();
    }
}

void TecMapOverlayModel::connectPluginSignals(QObject *pluginObject) {
    if (!pluginObject) {
        return;
    }

    QObject::connect(pluginObject,
                     SIGNAL(loadingChanged(bool)),
                     this,
                     SLOT(handlePluginLoadingChanged(bool)));
    QObject::connect(pluginObject,
                     SIGNAL(dataReady(hdgnss::TecGridData)),
                     this,
                     SLOT(handlePluginDataReady(hdgnss::TecGridData)));
    QObject::connect(pluginObject,
                     SIGNAL(errorOccurred(QString)),
                     this,
                     SLOT(handlePluginErrorOccurred(QString)));
}

void TecMapOverlayModel::disconnectPluginSignals() {
    for (const SourceState &source : m_sources) {
        if (source.pluginObject) {
            QObject::disconnect(source.pluginObject, nullptr, this, nullptr);
        }
    }
}

void TecMapOverlayModel::rebuildDescriptors() {
    QVariantList availablePlugins;
    QVariantList activeSources;
    availablePlugins.reserve(m_sources.size());
    activeSources.reserve(m_sources.size());

    for (int index = 0; index < m_sources.size(); ++index) {
        const SourceState &source = m_sources.at(index);
        QVariantMap descriptor = tecPluginDescriptor(source.pluginObject, m_settings);
        descriptor.insert(QStringLiteral("pluginId"), source.pluginId);
        descriptor.insert(QStringLiteral("displayName"), source.displayName);
        descriptor.insert(QStringLiteral("loading"), source.loading);
        descriptor.insert(QStringLiteral("ready"), source.overlaySource.isValid() && !source.overlaySource.isEmpty());
        descriptor.insert(QStringLiteral("statusText"), source.statusText);
        descriptor.insert(QStringLiteral("datasetLabel"), source.datasetLabel);
        descriptor.insert(QStringLiteral("active"), index == m_activeSourceIndex);
        availablePlugins.append(descriptor);

        const bool enabled = m_pluginsEnabled && (!m_settings || m_settings->pluginEnabled(source.pluginId));
        if (enabled) {
            activeSources.append(QVariantMap{
                {QStringLiteral("pluginId"), source.pluginId},
                {QStringLiteral("displayName"), source.displayName},
                {QStringLiteral("loading"), source.loading},
                {QStringLiteral("ready"), source.overlaySource.isValid() && !source.overlaySource.isEmpty()},
                {QStringLiteral("statusText"), source.statusText},
                {QStringLiteral("datasetLabel"), source.datasetLabel},
                {QStringLiteral("active"), source.pluginId == m_activeSourceId}
            });
        }
    }

    m_availableTecPlugins = availablePlugins;
    m_activeTecSources = activeSources;
    emit availableTecPluginsChanged();
    emit activeTecSourcesChanged();
}

int TecMapOverlayModel::sourceIndexForSender() const {
    QObject *pluginObject = sender();
    for (int index = 0; index < m_sources.size(); ++index) {
        if (m_sources.at(index).pluginObject == pluginObject) {
            return index;
        }
    }
    return -1;
}

int TecMapOverlayModel::firstEnabledSourceIndex() const {
    if (!m_pluginsEnabled) {
        return -1;
    }
    for (int index = 0; index < m_sources.size(); ++index) {
        if (!m_settings || m_settings->pluginEnabled(m_sources.at(index).pluginId)) {
            return index;
        }
    }
    return -1;
}

void TecMapOverlayModel::ensureActiveSourceSelection(const QString &preferredPluginId) {
    int nextIndex = -1;
    if (!preferredPluginId.isEmpty() && m_pluginsEnabled) {
        for (int index = 0; index < m_sources.size(); ++index) {
            if (m_sources.at(index).pluginId == preferredPluginId
                && (!m_settings || m_settings->pluginEnabled(preferredPluginId))) {
                nextIndex = index;
                break;
            }
        }
    }
    if (nextIndex < 0) {
        nextIndex = firstEnabledSourceIndex();
    }
    m_activeSourceIndex = nextIndex;
}

void TecMapOverlayModel::applyActiveSourceState() {
    const QStringList errors = m_pluginLoader ? m_pluginLoader->errors() : QStringList{};
    QUrl nextOverlaySource;
    QString nextStatusText;
    QString nextDatasetLabel = QStringLiteral("TEC unavailable");
    bool nextLoading = false;
    QString nextActiveSourceId;

    if (m_activeSourceIndex >= 0 && m_activeSourceIndex < m_sources.size()) {
        const SourceState &source = m_sources.at(m_activeSourceIndex);
        nextOverlaySource = source.overlaySource;
        nextStatusText = source.statusText;
        nextDatasetLabel = source.datasetLabel;
        nextLoading = source.loading;
        nextActiveSourceId = source.pluginId;
    } else if (!m_pluginsEnabled) {
        nextStatusText = QStringLiteral("TEC plugins disabled in Settings");
    } else if (m_sources.isEmpty()) {
        nextStatusText = errors.isEmpty()
            ? QStringLiteral("No TEC plugin available")
            : QStringLiteral("TEC plugin load failed: %1").arg(errors.first());
    } else if (firstEnabledSourceIndex() < 0) {
        nextStatusText = QStringLiteral("No enabled TEC plugin available");
    } else if (!m_pendingObservationTimeUtc.isValid()) {
        nextStatusText = QStringLiteral("TEC waiting for GNSS UTC time");
    } else {
        nextStatusText = QStringLiteral("TEC unavailable");
    }

    if (m_overlaySource != nextOverlaySource) {
        m_overlaySource = nextOverlaySource;
        emit overlayChanged();
    }
    if (m_statusText != nextStatusText || m_datasetLabel != nextDatasetLabel) {
        m_statusText = nextStatusText;
        m_datasetLabel = nextDatasetLabel;
        emit statusChanged();
    }
    if (m_loading != nextLoading) {
        m_loading = nextLoading;
        emit loadingChanged();
    }
    m_activeSourceId = nextActiveSourceId;
}

void TecMapOverlayModel::requestRefreshForEnabledSources() {
    if (!m_pendingObservationTimeUtc.isValid() || !m_pluginsEnabled) {
        return;
    }

    for (int index = 0; index < m_sources.size(); ++index) {
        SourceState &source = m_sources[index];
        if (!source.plugin || (m_settings && !m_settings->pluginEnabled(source.pluginId))) {
            continue;
        }
        if (!shouldRequestRefresh(source.lastRequestObservationTimeUtc,
                                  m_pendingObservationTimeUtc,
                                  source.plugin->downloadIntervalSeconds())) {
            continue;
        }

        source.lastRequestObservationTimeUtc = m_pendingObservationTimeUtc;
        updateDatasetLabel(index);
        setSourceStatusText(index, QStringLiteral("Loading %1").arg(source.displayName));
        source.plugin->requestForObservationTime(m_pendingObservationTimeUtc);
    }

    applyActiveSourceState();
    rebuildDescriptors();
}

void TecMapOverlayModel::setSourceStatusText(int index, const QString &text) {
    if (index < 0 || index >= m_sources.size()) {
        return;
    }
    m_sources[index].statusText = text;
}

void TecMapOverlayModel::updateDatasetLabel(int index) {
    if (index < 0 || index >= m_sources.size()) {
        return;
    }

    const SourceState &source = m_sources.at(index);
    QString nextLabel = QStringLiteral("TEC unavailable");
    if (source.datasetTimestampUtc.isValid()) {
        nextLabel = QStringLiteral("%1 | %2")
                        .arg(source.displayName,
                             source.datasetTimestampUtc.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm 'UTC'")));
        if (source.plugin) {
            nextLabel += refreshCountdownText(source.lastRequestObservationTimeUtc,
                                              m_pendingObservationTimeUtc,
                                              source.plugin->downloadIntervalSeconds());
        }
    }
    m_sources[index].datasetLabel = nextLabel;
}

void TecMapOverlayModel::clearOverlay(int index) {
    if (index < 0 || index >= m_sources.size()) {
        return;
    }
    if (!m_sources[index].overlayPath.isEmpty()) {
        QFile::remove(m_sources[index].overlayPath);
        m_sources[index].overlayPath.clear();
    }
    m_sources[index].overlaySource = QUrl();
}

void TecMapOverlayModel::renderDataset(int index, const TecGridData &dataset) {
    if (index < 0 || index >= m_sources.size()) {
        return;
    }

    const QString directoryPath = overlayDirectoryPath();
    QDir directory;
    if (!directory.mkpath(directoryPath)) {
        setSourceStatusText(index, QStringLiteral("TEC overlay cache directory could not be created"));
        applyActiveSourceState();
        rebuildDescriptors();
        return;
    }

    const QImage image = TecMapRenderer::render(dataset);
    const QString filePath = QDir(directoryPath).filePath(
        QStringLiteral("tec-%1-%2-%3.png")
            .arg(dataset.sourceId.isEmpty() ? QStringLiteral("overlay") : dataset.sourceId)
            .arg(dataset.timestampUtc.toUTC().toString(QStringLiteral("yyyyMMddTHHmmss")))
            .arg(++m_revision));
    if (!image.save(filePath)) {
        setSourceStatusText(index, QStringLiteral("TEC overlay image could not be written"));
        applyActiveSourceState();
        rebuildDescriptors();
        return;
    }

    if (!m_sources[index].overlayPath.isEmpty() && m_sources[index].overlayPath != filePath) {
        QFile::remove(m_sources[index].overlayPath);
    }

    m_sources[index].overlayPath = filePath;
    m_sources[index].overlaySource = QUrl::fromLocalFile(filePath);
    m_sources[index].datasetTimestampUtc = dataset.timestampUtc.toUTC();
    m_sources[index].dataset = dataset;
    updateDatasetLabel(index);

    if (dataset.fromCache) {
        setSourceStatusText(index,
                            dataset.stale
                                ? (dataset.availabilityNote.isEmpty()
                                       ? QStringLiteral("Using stale cached %1").arg(m_sources.at(index).displayName)
                                       : dataset.availabilityNote)
                                : QStringLiteral("Using cached %1").arg(m_sources.at(index).displayName));
    } else {
        setSourceStatusText(index, QStringLiteral("%1 ready").arg(m_sources.at(index).displayName));
    }

    applyActiveSourceState();
    rebuildDescriptors();
}

}  // namespace hdgnss
