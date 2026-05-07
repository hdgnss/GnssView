#pragma once

#include <QVariantMap>
#include <QtPlugin>

namespace hdgnss {

class IPluginSettingsUi {
public:
    virtual ~IPluginSettingsUi() = default;

    virtual QVariantMap settingsUi() const = 0;
};

}  // namespace hdgnss

#define HDGNSS_PLUGIN_SETTINGS_UI_IID "com.hdgnss.IPluginSettingsUi/1.0"
Q_DECLARE_INTERFACE(hdgnss::IPluginSettingsUi, HDGNSS_PLUGIN_SETTINGS_UI_IID)
