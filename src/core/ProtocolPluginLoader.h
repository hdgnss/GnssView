#pragma once

#include <memory>
#include <vector>

#include <QList>
#include <QPluginLoader>
#include <QStringList>
#include <QString>

#include "hdgnss/IProtocolPlugin.h"

namespace hdgnss {

class ProtocolPluginLoader {
public:
    static QStringList defaultSearchPaths(const QString &applicationDirectoryPath = QString(),
                                          const QString &userPluginDirectory = QString());

    void loadFromDirectory(const QString &directoryPath);
    void loadFromDirectories(const QStringList &directoryPaths);
    QList<IProtocolPlugin *> plugins() const;
    QList<QObject *> pluginObjects() const;
    QStringList errors() const;
    QStringList loadedDirectories() const;

private:
    std::vector<std::unique_ptr<QPluginLoader>> m_loaders;
    QList<IProtocolPlugin *> m_plugins;
    QList<QObject *> m_pluginObjects;
    QStringList m_errors;
    QStringList m_loadedDirectories;
};

}  // namespace hdgnss
