#pragma once

#include <memory>
#include <vector>

#include <QList>
#include <QPluginLoader>
#include <QString>
#include <QStringList>

#include "hdgnss/IAutomationPlugin.h"

namespace hdgnss {

class AutomationPluginLoader {
public:
    // Uses the same search-path convention as ProtocolPluginLoader.
    static QStringList defaultSearchPaths(const QString &applicationDirectoryPath = {},
                                          const QString &userPluginDirectory = {});

    void loadFromDirectory(const QString &directoryPath);
    void loadFromDirectories(const QStringList &directoryPaths);

    QList<IAutomationPlugin *> plugins() const;
    QList<QObject *> pluginObjects() const;
    QStringList errors() const;
    QStringList loadedDirectories() const;

private:
    std::vector<std::unique_ptr<QPluginLoader>> m_loaders;
    QList<IAutomationPlugin *> m_plugins;
    QList<QObject *> m_pluginObjects;
    QStringList m_errors;
    QStringList m_loadedDirectories;
};

}  // namespace hdgnss
