#include "AppSettings.h"

#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>

namespace hdgnss {

namespace {

QString chooseDefaultLogDirectory() {
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appData.isEmpty()) {
        return QDir(appData).absoluteFilePath(QStringLiteral("logs"));
    }

    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("runtime-data/logs"));
}

QString chooseDefaultPluginDirectory() {
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appData.isEmpty()) {
        return {};
    }
    const QString pluginDir = QDir(appData).filePath(QStringLiteral("plugins"));
    QDir().mkpath(pluginDir);
    return QDir(pluginDir).absolutePath();
}

}  // namespace

AppSettings::AppSettings(QObject *parent)
    : QObject(parent) {
    load();
}

bool AppSettings::recordRawData() const {
    return m_recordRawData;
}

bool AppSettings::recordDecodeLog() const {
    return m_recordDecodeLog;
}

QString AppSettings::logDirectory() const {
    return m_logDirectory;
}

QString AppSettings::defaultLogDirectory() const {
    return chooseDefaultLogDirectory();
}

bool AppSettings::pluginsEnabled() const {
    return m_pluginsEnabled;
}

QString AppSettings::pluginDirectory() const {
    return m_pluginDirectory;
}

QString AppSettings::defaultPluginDirectory() const {
    return chooseDefaultPluginDirectory();
}

bool AppSettings::rememberWindowGeometry() const {
    return m_rememberWindowGeometry;
}

bool AppSettings::resetUiOnNewConnection() const {
    return m_resetUiOnNewConnection;
}

bool AppSettings::checkForUpdatesOnStartup() const {
    return m_checkForUpdatesOnStartup;
}

bool AppSettings::useFixedDeviationCenter() const {
    return m_useFixedDeviationCenter;
}

double AppSettings::fixedDeviationLatitude() const {
    return m_fixedDeviationLatitude;
}

double AppSettings::fixedDeviationLongitude() const {
    return m_fixedDeviationLongitude;
}

bool AppSettings::pluginEnabled(const QString &pluginId) const {
    const QString cleaned = pluginId.trimmed();
    if (cleaned.isEmpty()) {
        return true;
    }
    return m_pluginEnabled.value(cleaned, true);
}

QVariantMap AppSettings::pluginPrivateSettings(const QString &pluginId) const {
    const QString cleaned = pluginId.trimmed();
    if (cleaned.isEmpty()) {
        return {};
    }
    return m_pluginPrivateSettings.value(cleaned);
}

QVariant AppSettings::pluginSettingValue(const QString &pluginId,
                                         const QString &key,
                                         const QVariant &defaultValue) const {
    if (key.trimmed().isEmpty()) {
        return defaultValue;
    }
    return pluginPrivateSettings(pluginId).value(key, defaultValue);
}

void AppSettings::setRecordRawData(bool enabled) {
    if (enabled && m_logDirectory.trimmed().isEmpty()) {
        return;
    }
    if (m_recordRawData == enabled) {
        return;
    }
    m_recordRawData = enabled;
    storeValue(QStringLiteral("logging/recordRawData"), enabled);
    emit recordRawDataChanged();
}

void AppSettings::setRecordDecodeLog(bool enabled) {
    if (enabled && m_logDirectory.trimmed().isEmpty()) {
        return;
    }
    if (m_recordDecodeLog == enabled) {
        return;
    }
    m_recordDecodeLog = enabled;
    storeValue(QStringLiteral("logging/recordDecodeLog"), enabled);
    emit recordDecodeLogChanged();
}

void AppSettings::setLogDirectory(const QString &directory) {
    const QString cleaned = cleanedDirectory(directory);
    if (m_logDirectory == cleaned) {
        return;
    }
    m_logDirectory = cleaned;
    storeValue(QStringLiteral("logging/logDirectory"), m_logDirectory);
    emit logDirectoryChanged();
    if (m_logDirectory.isEmpty()) {
        if (m_recordRawData) {
            m_recordRawData = false;
            storeValue(QStringLiteral("logging/recordRawData"), false);
            emit recordRawDataChanged();
        }
        if (m_recordDecodeLog) {
            m_recordDecodeLog = false;
            storeValue(QStringLiteral("logging/recordDecodeLog"), false);
            emit recordDecodeLogChanged();
        }
    }
}

void AppSettings::setPluginsEnabled(bool enabled) {
    if (m_pluginsEnabled == enabled) {
        return;
    }
    m_pluginsEnabled = enabled;
    storeValue(QStringLiteral("plugins/enabled"), enabled);
    emit pluginsEnabledChanged();
}

void AppSettings::setPluginDirectory(const QString &directory) {
    const QString cleaned = cleanedDirectory(directory);
    if (m_pluginDirectory == cleaned) {
        return;
    }
    m_pluginDirectory = cleaned;
    storeValue(QStringLiteral("plugins/directory"), m_pluginDirectory);
    emit pluginDirectoryChanged();
}

void AppSettings::setRememberWindowGeometry(bool enabled) {
    if (m_rememberWindowGeometry == enabled) {
        return;
    }
    m_rememberWindowGeometry = enabled;
    storeValue(QStringLiteral("app/rememberWindowGeometry"), enabled);
    emit rememberWindowGeometryChanged();
}

void AppSettings::setResetUiOnNewConnection(bool enabled) {
    if (m_resetUiOnNewConnection == enabled) {
        return;
    }
    m_resetUiOnNewConnection = enabled;
    storeValue(QStringLiteral("session/resetUiOnNewConnection"), enabled);
    emit resetUiOnNewConnectionChanged();
}

void AppSettings::setCheckForUpdatesOnStartup(bool enabled) {
    if (m_checkForUpdatesOnStartup == enabled) {
        return;
    }
    m_checkForUpdatesOnStartup = enabled;
    storeValue(QStringLiteral("updates/checkOnStartup"), enabled);
    emit checkForUpdatesOnStartupChanged();
}

void AppSettings::setUseFixedDeviationCenter(bool enabled) {
    if (m_useFixedDeviationCenter == enabled) {
        return;
    }
    m_useFixedDeviationCenter = enabled;
    storeValue(QStringLiteral("deviation/useFixedCenter"), enabled);
    emit useFixedDeviationCenterChanged();
}

void AppSettings::setFixedDeviationLatitude(double latitude) {
    if (qFuzzyCompare(1.0 + m_fixedDeviationLatitude, 1.0 + latitude)) {
        return;
    }
    m_fixedDeviationLatitude = latitude;
    storeValue(QStringLiteral("deviation/fixedLatitude"), latitude);
    emit fixedDeviationLatitudeChanged();
}

void AppSettings::setFixedDeviationLongitude(double longitude) {
    if (qFuzzyCompare(1.0 + m_fixedDeviationLongitude, 1.0 + longitude)) {
        return;
    }
    m_fixedDeviationLongitude = longitude;
    storeValue(QStringLiteral("deviation/fixedLongitude"), longitude);
    emit fixedDeviationLongitudeChanged();
}

void AppSettings::setPluginEnabled(const QString &pluginId, bool enabled) {
    const QString cleaned = pluginId.trimmed();
    if (cleaned.isEmpty() || pluginEnabled(cleaned) == enabled) {
        return;
    }
    m_pluginEnabled.insert(cleaned, enabled);
    storePluginConfiguration(cleaned);
    emit pluginSettingsChanged();
    emit pluginAvailabilityChanged();
}

void AppSettings::setExclusivePluginEnabled(const QVariantList &pluginIds,
                                            const QString &pluginId,
                                            bool enabled) {
    QStringList cleanedIds;
    cleanedIds.reserve(pluginIds.size() + 1);
    for (const QVariant &value : pluginIds) {
        const QString cleaned = value.toString().trimmed();
        if (!cleaned.isEmpty() && !cleanedIds.contains(cleaned)) {
            cleanedIds.append(cleaned);
        }
    }

    const QString cleanedTarget = pluginId.trimmed();
    if (cleanedTarget.isEmpty()) {
        return;
    }
    if (!cleanedIds.contains(cleanedTarget)) {
        cleanedIds.append(cleanedTarget);
    }

    bool changed = false;
    for (const QString &id : std::as_const(cleanedIds)) {
        const bool desiredEnabled = enabled && id == cleanedTarget;
        if (pluginEnabled(id) == desiredEnabled) {
            continue;
        }
        m_pluginEnabled.insert(id, desiredEnabled);
        storePluginConfiguration(id);
        changed = true;
    }

    if (!changed) {
        return;
    }
    emit pluginSettingsChanged();
    emit pluginAvailabilityChanged();
}

void AppSettings::setPluginPrivateSettings(const QString &pluginId, const QVariantMap &settings) {
    const QString cleaned = pluginId.trimmed();
    if (cleaned.isEmpty() || m_pluginPrivateSettings.value(cleaned) == settings) {
        return;
    }
    if (settings.isEmpty()) {
        m_pluginPrivateSettings.remove(cleaned);
    } else {
        m_pluginPrivateSettings.insert(cleaned, settings);
    }
    storePluginConfiguration(cleaned);
    emit pluginSettingsChanged();
}

void AppSettings::setPluginSettingValue(const QString &pluginId, const QString &key, const QVariant &value) {
    const QString cleanedPluginId = pluginId.trimmed();
    const QString cleanedKey = key.trimmed();
    if (cleanedPluginId.isEmpty() || cleanedKey.isEmpty()) {
        return;
    }

    QVariantMap values = m_pluginPrivateSettings.value(cleanedPluginId);
    QVariant normalizedValue = value;
    if (normalizedValue.metaType().id() == QMetaType::QString) {
        const QString text = normalizedValue.toString().trimmed();
        if (text.isEmpty()) {
            normalizedValue = QVariant();
        } else {
            normalizedValue = text;
        }
    }

    if (!normalizedValue.isValid() || normalizedValue.isNull()) {
        if (!values.contains(cleanedKey)) {
            return;
        }
        values.remove(cleanedKey);
    } else {
        if (values.value(cleanedKey) == normalizedValue) {
            return;
        }
        values.insert(cleanedKey, normalizedValue);
    }

    setPluginPrivateSettings(cleanedPluginId, values);
}

void AppSettings::load() {
    QSettings settings;
    m_recordRawData = settings.value(QStringLiteral("logging/recordRawData"), false).toBool();
    m_recordDecodeLog = settings.value(QStringLiteral("logging/recordDecodeLog"), false).toBool();
    m_logDirectory = cleanedDirectory(settings.value(QStringLiteral("logging/logDirectory")).toString());
    if (m_logDirectory.isEmpty()) {
        m_recordRawData = false;
        m_recordDecodeLog = false;
    }

    m_pluginsEnabled = settings.value(QStringLiteral("plugins/enabled"), true).toBool();
    m_pluginDirectory = cleanedDirectory(settings.value(QStringLiteral("plugins/directory"), defaultPluginDirectory()).toString());
    if (m_pluginDirectory.isEmpty()) {
        m_pluginDirectory = defaultPluginDirectory();
    }

    m_rememberWindowGeometry = settings.value(QStringLiteral("app/rememberWindowGeometry"), true).toBool();
    m_resetUiOnNewConnection = settings.value(QStringLiteral("session/resetUiOnNewConnection"), true).toBool();
    m_checkForUpdatesOnStartup = settings.value(QStringLiteral("updates/checkOnStartup"), true).toBool();
    m_useFixedDeviationCenter = settings.value(QStringLiteral("deviation/useFixedCenter"), false).toBool();
    m_fixedDeviationLatitude = settings.value(QStringLiteral("deviation/fixedLatitude"), 0.0).toDouble();
    m_fixedDeviationLongitude = settings.value(QStringLiteral("deviation/fixedLongitude"), 0.0).toDouble();

    m_pluginEnabled.clear();
    m_pluginPrivateSettings.clear();
    settings.beginGroup(QStringLiteral("pluginConfigs"));
    const QStringList pluginGroups = settings.childGroups();
    for (const QString &group : pluginGroups) {
        settings.beginGroup(group);
        const QString pluginId = settings.value(QStringLiteral("id")).toString().trimmed();
        if (!pluginId.isEmpty()) {
            m_pluginEnabled.insert(pluginId, settings.value(QStringLiteral("enabled"), true).toBool());
            const QVariantMap values = settings.value(QStringLiteral("values")).toMap();
            if (!values.isEmpty()) {
                m_pluginPrivateSettings.insert(pluginId, values);
            }
        }
        settings.endGroup();
    }
    settings.endGroup();
}

void AppSettings::storeValue(const QString &key, const QVariant &value) {
    QSettings settings;
    settings.setValue(key, value);
}

void AppSettings::storePluginConfiguration(const QString &pluginId) {
    const QString cleaned = pluginId.trimmed();
    if (cleaned.isEmpty()) {
        return;
    }

    const bool enabled = m_pluginEnabled.value(cleaned, true);
    const QVariantMap values = m_pluginPrivateSettings.value(cleaned);
    QSettings settings;
    settings.beginGroup(QStringLiteral("pluginConfigs"));
    settings.remove(pluginGroupKey(cleaned));
    if (!enabled || !values.isEmpty()) {
        settings.beginGroup(pluginGroupKey(cleaned));
        settings.setValue(QStringLiteral("id"), cleaned);
        settings.setValue(QStringLiteral("enabled"), enabled);
        settings.setValue(QStringLiteral("values"), values);
        settings.endGroup();
    }
    settings.endGroup();
}

QString AppSettings::pluginGroupKey(const QString &pluginId) {
    return QString::fromLatin1(QUrl::toPercentEncoding(pluginId.trimmed()));
}

QString AppSettings::cleanedDirectory(const QString &directory) {
    if (directory.trimmed().isEmpty()) {
        return {};
    }
    return QDir(directory).absolutePath();
}

}  // namespace hdgnss
