#include "AutomationPluginLoader.h"

#include <memory>

#include <QDir>
#include <QDebug>
#include <QFileInfoList>
#include <QSet>
#include <QPluginLoader>
#include <QStandardPaths>

namespace hdgnss {

namespace {

QString normalizedDirectoryPath(const QString &path) {
    if (path.trimmed().isEmpty()) {
        return {};
    }
    const QFileInfo info(path);
    if (info.exists()) {
        const QString canonical = info.canonicalFilePath();
        if (!canonical.isEmpty()) {
            return canonical;
        }
    }
    return QDir::cleanPath(QDir(path).absolutePath());
}

void appendIfPresent(QStringList &paths, const QString &path) {
    const QString normalized = normalizedDirectoryPath(path);
    if (!normalized.isEmpty()) {
        paths.append(normalized);
    }
}

void appendDelimitedPaths(QStringList &paths, const QByteArray &value) {
    const QStringList parts = QString::fromLocal8Bit(value).split(
        QDir::listSeparator(), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        appendIfPresent(paths, part);
    }
}

bool pluginDebugLoggingEnabled() {
    static const bool enabled = qEnvironmentVariableIntValue("HDGNSS_PLUGIN_DEBUG") > 0;
    return enabled;
}

bool hasExpectedPluginIid(const QPluginLoader &loader, const QString &expectedIid) {
    const QJsonValue iidValue = loader.metaData().value(QStringLiteral("IID"));
    if (!iidValue.isString()) {
        if (pluginDebugLoggingEnabled()) {
            qDebug() << "[Automation] Plugin" << loader.fileName() << "missing IID in metadata";
        }
        return false;
    }
    if (iidValue.toString() != expectedIid) {
        return false;
    }
    return true;
}

}  // namespace

QStringList AutomationPluginLoader::defaultSearchPaths(const QString &applicationDirectoryPath,
                                                       const QString &userPluginDirectory) {
    QStringList paths;

    appendIfPresent(paths, QString::fromLocal8Bit(qgetenv("HDGNSS_PLUGIN_DIR")));
    appendDelimitedPaths(paths, qgetenv("HDGNSS_PLUGIN_PATH"));

    if (!userPluginDirectory.trimmed().isEmpty()) {
        appendIfPresent(paths, userPluginDirectory);
    } else {
        const QString appDataRoot =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!appDataRoot.isEmpty()) {
            appendIfPresent(paths, QDir(appDataRoot).filePath(QStringLiteral("plugins")));
        }
    }

    if (!applicationDirectoryPath.trimmed().isEmpty()) {
        appendIfPresent(paths,
                        QDir(applicationDirectoryPath).filePath(QStringLiteral("plugins")));
    }

    QStringList deduplicated;
    QSet<QString> seen;
    for (const QString &path : paths) {
        if (seen.contains(path)) {
            continue;
        }
        seen.insert(path);
        deduplicated.append(path);
    }
    return deduplicated;
}

void AutomationPluginLoader::loadFromDirectory(const QString &directoryPath) {
    loadFromDirectories({directoryPath});
}

void AutomationPluginLoader::loadFromDirectories(const QStringList &directoryPaths) {
    QSet<QString> loadedPluginFiles;
    for (const auto &loader : m_loaders) {
        const QString fileName = loader ? loader->fileName() : QString();
        if (!fileName.isEmpty()) {
            loadedPluginFiles.insert(QFileInfo(fileName).canonicalFilePath());
            loadedPluginFiles.insert(QDir::cleanPath(QFileInfo(fileName).absoluteFilePath()));
        }
    }

    QSet<QString> seenDirectories;
    for (const QString &rawPath : directoryPaths) {
        const QString directoryPath = normalizedDirectoryPath(rawPath);
        if (directoryPath.isEmpty() || seenDirectories.contains(directoryPath)) {
            continue;
        }
        seenDirectories.insert(directoryPath);

        QDir directory(directoryPath);
        if (!directory.exists()) {
            if (pluginDebugLoggingEnabled()) {
                qDebug() << "[Automation] Search directory does not exist:" << directoryPath;
            }
            continue;
        }

        if (pluginDebugLoggingEnabled()) {
            qDebug() << "[Automation] Searching for plugins in:" << directoryPath;
        }
        m_loadedDirectories.append(directoryPath);
        const QFileInfoList entries =
            directory.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        for (const QFileInfo &entry : entries) {
            const QString canonicalFilePath = entry.canonicalFilePath();
            const QString normalizedFilePath = canonicalFilePath.isEmpty()
                ? QDir::cleanPath(entry.absoluteFilePath())
                : canonicalFilePath;
            if (loadedPluginFiles.contains(normalizedFilePath)) {
                continue;
            }

            auto loader = std::make_unique<QPluginLoader>(entry.absoluteFilePath());
            if (!hasExpectedPluginIid(*loader,
                                      QString::fromLatin1(HDGNSS_AUTOMATION_PLUGIN_IID))) {
                continue;
            }
            QObject *instance = loader->instance();
            if (!instance) {
                m_errors.append(
                    QStringLiteral("%1: %2").arg(entry.fileName(), loader->errorString()));
                continue;
            }

            IAutomationPlugin *plugin = qobject_cast<IAutomationPlugin *>(instance);
            if (!plugin) {
                qWarning() << "[Automation] Instance from" << entry.fileName() << "is NOT an IAutomationPlugin";
                m_errors.append(
                    QStringLiteral("%1: not an HDGNSS automation plugin").arg(entry.fileName()));
                continue;
            }

            if (pluginDebugLoggingEnabled()) {
                qDebug() << "[Automation] Loaded plugin:" << entry.fileName();
            }
            loadedPluginFiles.insert(normalizedFilePath);
            m_plugins.append(plugin);
            m_pluginObjects.append(instance);
            m_loaders.push_back(std::move(loader));
        }
    }
}

QList<IAutomationPlugin *> AutomationPluginLoader::plugins() const {
    return m_plugins;
}

QList<QObject *> AutomationPluginLoader::pluginObjects() const {
    return m_pluginObjects;
}

QStringList AutomationPluginLoader::errors() const {
    return m_errors;
}

QStringList AutomationPluginLoader::loadedDirectories() const {
    return m_loadedDirectories;
}

}  // namespace hdgnss
