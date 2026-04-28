#pragma once

#include <memory>
#include <vector>

#include <QList>
#include <QPluginLoader>
#include <QString>
#include <QStringList>

#include "hdgnss/ITecDataPlugin.h"

namespace hdgnss {

class TecPluginLoader {
public:
    static QStringList defaultSearchPaths(const QString &applicationDirectoryPath = QString(),
                                          const QString &userPluginDirectoryRoot = QString());

    void loadFromDirectory(const QString &directoryPath);
    void loadFromDirectories(const QStringList &directoryPaths);
    QList<ITecDataPlugin *> plugins() const;
    QList<QObject *> pluginObjects() const;
    QStringList errors() const;
    QStringList loadedDirectories() const;

private:
    std::vector<std::unique_ptr<QPluginLoader>> m_loaders;
    QList<ITecDataPlugin *> m_plugins;
    QList<QObject *> m_pluginObjects;
    QStringList m_errors;
    QStringList m_loadedDirectories;
};

}  // namespace hdgnss
