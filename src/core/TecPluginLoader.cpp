#include "TecPluginLoader.h"

#include <memory>

#include <QDir>
#include <QFileInfoList>
#include <QSet>
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
    const QStringList parts = QString::fromLocal8Bit(value).split(QDir::listSeparator(), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        appendIfPresent(paths, part);
    }
}

bool hasExpectedPluginIid(const QPluginLoader &loader, const QString &expectedIid) {
    const QJsonValue iidValue = loader.metaData().value(QStringLiteral("IID"));
    if (!iidValue.isString()) {
        return true;
    }
    return iidValue.toString() == expectedIid;
}

}  // namespace

QStringList TecPluginLoader::defaultSearchPaths(const QString &applicationDirectoryPath,
                                                const QString &userPluginDirectoryRoot) {
    QStringList paths;

    appendIfPresent(paths, QString::fromLocal8Bit(qgetenv("HDGNSS_PLUGIN_DIR")));
    appendDelimitedPaths(paths, qgetenv("HDGNSS_PLUGIN_PATH"));

    if (!userPluginDirectoryRoot.trimmed().isEmpty()) {
        appendIfPresent(paths, userPluginDirectoryRoot);
    } else {
        const QString appDataRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!appDataRoot.isEmpty()) {
            appendIfPresent(paths, QDir(appDataRoot).filePath(QStringLiteral("plugins")));
        }
    }

    if (!applicationDirectoryPath.trimmed().isEmpty()) {
        appendIfPresent(paths, QDir(applicationDirectoryPath).filePath(QStringLiteral("plugins")));
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

void TecPluginLoader::loadFromDirectory(const QString &directoryPath) {
    loadFromDirectories({directoryPath});
}

void TecPluginLoader::loadFromDirectories(const QStringList &directoryPaths) {
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
            continue;
        }

        m_loadedDirectories.append(directoryPath);
        const QFileInfoList entries = directory.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        for (const QFileInfo &entry : entries) {
            const QString canonicalFilePath = entry.canonicalFilePath();
            const QString normalizedFilePath = canonicalFilePath.isEmpty()
                ? QDir::cleanPath(entry.absoluteFilePath())
                : canonicalFilePath;
            if (loadedPluginFiles.contains(normalizedFilePath)) {
                continue;
            }

            auto loader = std::make_unique<QPluginLoader>(entry.absoluteFilePath());
            if (!hasExpectedPluginIid(*loader, QString::fromLatin1(HDGNSS_TEC_DATA_PLUGIN_IID))) {
                continue;
            }
            QObject *instance = loader->instance();
            if (!instance) {
                m_errors.append(QStringLiteral("%1: %2").arg(entry.fileName(), loader->errorString()));
                continue;
            }

            ITecDataPlugin *plugin = qobject_cast<ITecDataPlugin *>(instance);
            if (!plugin) {
                m_errors.append(QStringLiteral("%1: not a HDGNSS TEC plugin").arg(entry.fileName()));
                continue;
            }

            loadedPluginFiles.insert(normalizedFilePath);
            m_plugins.append(plugin);
            m_pluginObjects.append(instance);
            m_loaders.push_back(std::move(loader));
        }
    }
}

QList<ITecDataPlugin *> TecPluginLoader::plugins() const {
    return m_plugins;
}

QList<QObject *> TecPluginLoader::pluginObjects() const {
    return m_pluginObjects;
}

QStringList TecPluginLoader::errors() const {
    return m_errors;
}

QStringList TecPluginLoader::loadedDirectories() const {
    return m_loadedDirectories;
}

}  // namespace hdgnss
