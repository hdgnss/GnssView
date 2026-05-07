#pragma once

#include <memory>
#include <vector>

#include <QList>
#include <QPluginLoader>
#include <QString>
#include <QStringList>

#include "hdgnss/ITransportPlugin.h"

namespace hdgnss {

class TransportPluginLoader {
public:
    static QStringList defaultSearchPaths(const QString &applicationDirectoryPath = QString(),
                                          const QString &userPluginDirectory = QString());

    void loadFromDirectory(const QString &directoryPath);
    void loadFromDirectories(const QStringList &directoryPaths);
    QList<ITransportPlugin *> plugins() const;
    QList<QObject *> pluginObjects() const;
    QStringList errors() const;
    QStringList loadedDirectories() const;

private:
    std::vector<std::unique_ptr<QPluginLoader>> m_loaders;
    QList<ITransportPlugin *> m_plugins;
    QList<QObject *> m_pluginObjects;
    QStringList m_errors;
    QStringList m_loadedDirectories;
};

}  // namespace hdgnss
