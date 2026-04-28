#pragma once

#include <QString>
#include <QtPlugin>

namespace hdgnss {

class IPluginMetadata {
public:
    virtual ~IPluginMetadata() = default;

    virtual QString pluginVersion() const = 0;
};

}  // namespace hdgnss

#define HDGNSS_PLUGIN_METADATA_IID "com.hdgnss.IPluginMetadata/1.0"
Q_DECLARE_INTERFACE(hdgnss::IPluginMetadata, HDGNSS_PLUGIN_METADATA_IID)
