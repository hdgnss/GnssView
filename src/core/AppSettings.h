#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace hdgnss {

class AppSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool recordRawData READ recordRawData WRITE setRecordRawData NOTIFY recordRawDataChanged)
    Q_PROPERTY(bool recordDecodeLog READ recordDecodeLog WRITE setRecordDecodeLog NOTIFY recordDecodeLogChanged)
    Q_PROPERTY(QString logDirectory READ logDirectory WRITE setLogDirectory NOTIFY logDirectoryChanged)
    Q_PROPERTY(QString defaultLogDirectory READ defaultLogDirectory CONSTANT)
    Q_PROPERTY(bool pluginsEnabled READ pluginsEnabled WRITE setPluginsEnabled NOTIFY pluginsEnabledChanged)
    Q_PROPERTY(QString pluginDirectory READ pluginDirectory WRITE setPluginDirectory NOTIFY pluginDirectoryChanged)
    Q_PROPERTY(QString defaultPluginDirectory READ defaultPluginDirectory CONSTANT)
    Q_PROPERTY(bool rememberWindowGeometry READ rememberWindowGeometry WRITE setRememberWindowGeometry NOTIFY rememberWindowGeometryChanged)
    Q_PROPERTY(bool resetUiOnNewConnection READ resetUiOnNewConnection WRITE setResetUiOnNewConnection NOTIFY resetUiOnNewConnectionChanged)
    Q_PROPERTY(bool checkForUpdatesOnStartup READ checkForUpdatesOnStartup WRITE setCheckForUpdatesOnStartup NOTIFY checkForUpdatesOnStartupChanged)
    Q_PROPERTY(bool useFixedDeviationCenter READ useFixedDeviationCenter WRITE setUseFixedDeviationCenter NOTIFY useFixedDeviationCenterChanged)
    Q_PROPERTY(double fixedDeviationLatitude READ fixedDeviationLatitude WRITE setFixedDeviationLatitude NOTIFY fixedDeviationLatitudeChanged)
    Q_PROPERTY(double fixedDeviationLongitude READ fixedDeviationLongitude WRITE setFixedDeviationLongitude NOTIFY fixedDeviationLongitudeChanged)

public:
    explicit AppSettings(QObject *parent = nullptr);

    bool recordRawData() const;
    bool recordDecodeLog() const;
    QString logDirectory() const;
    QString defaultLogDirectory() const;
    bool pluginsEnabled() const;
    QString pluginDirectory() const;
    QString defaultPluginDirectory() const;
    bool rememberWindowGeometry() const;
    bool resetUiOnNewConnection() const;
    bool checkForUpdatesOnStartup() const;
    bool useFixedDeviationCenter() const;
    double fixedDeviationLatitude() const;
    double fixedDeviationLongitude() const;
    Q_INVOKABLE bool pluginEnabled(const QString &pluginId) const;
    Q_INVOKABLE QVariantMap pluginPrivateSettings(const QString &pluginId) const;
    Q_INVOKABLE QVariant pluginSettingValue(const QString &pluginId,
                                            const QString &key,
                                            const QVariant &defaultValue = QVariant()) const;

public slots:
    void setRecordRawData(bool enabled);
    void setRecordDecodeLog(bool enabled);
    void setLogDirectory(const QString &directory);
    void setPluginsEnabled(bool enabled);
    void setPluginDirectory(const QString &directory);
    void setRememberWindowGeometry(bool enabled);
    void setResetUiOnNewConnection(bool enabled);
    void setCheckForUpdatesOnStartup(bool enabled);
    void setUseFixedDeviationCenter(bool enabled);
    void setFixedDeviationLatitude(double latitude);
    void setFixedDeviationLongitude(double longitude);
    void setPluginEnabled(const QString &pluginId, bool enabled);
    Q_INVOKABLE void setExclusivePluginEnabled(const QVariantList &pluginIds,
                                               const QString &pluginId,
                                               bool enabled);
    void setPluginPrivateSettings(const QString &pluginId, const QVariantMap &settings);
    void setPluginSettingValue(const QString &pluginId, const QString &key, const QVariant &value);

signals:
    void recordRawDataChanged();
    void recordDecodeLogChanged();
    void logDirectoryChanged();
    void pluginsEnabledChanged();
    void pluginDirectoryChanged();
    void rememberWindowGeometryChanged();
    void resetUiOnNewConnectionChanged();
    void checkForUpdatesOnStartupChanged();
    void useFixedDeviationCenterChanged();
    void fixedDeviationLatitudeChanged();
    void fixedDeviationLongitudeChanged();
    void pluginSettingsChanged();
    void pluginAvailabilityChanged();

private:
    void load();
    void storeValue(const QString &key, const QVariant &value);
    void storePluginConfiguration(const QString &pluginId);
    static QString pluginGroupKey(const QString &pluginId);
    static QString cleanedDirectory(const QString &directory);

    bool m_recordRawData = false;
    bool m_recordDecodeLog = false;
    QString m_logDirectory;
    bool m_pluginsEnabled = true;
    QString m_pluginDirectory;
    bool m_rememberWindowGeometry = true;
    bool m_resetUiOnNewConnection = true;
    bool m_checkForUpdatesOnStartup = true;
    bool m_useFixedDeviationCenter = false;
    double m_fixedDeviationLatitude = 0.0;
    double m_fixedDeviationLongitude = 0.0;
    QHash<QString, bool> m_pluginEnabled;
    QHash<QString, QVariantMap> m_pluginPrivateSettings;
};

}  // namespace hdgnss
