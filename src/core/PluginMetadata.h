#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "hdgnss/IPluginMetadata.h"

namespace hdgnss {

inline QString pluginVersionForObject(QObject *pluginObject) {
    if (!pluginObject) {
        return {};
    }
    if (IPluginMetadata *metadata = qobject_cast<IPluginMetadata *>(pluginObject)) {
        return metadata->pluginVersion().trimmed();
    }
    return {};
}

inline void insertPluginVersion(QVariantMap &descriptor, QObject *pluginObject) {
    descriptor.insert(QStringLiteral("version"), pluginVersionForObject(pluginObject));
}

}  // namespace hdgnss
