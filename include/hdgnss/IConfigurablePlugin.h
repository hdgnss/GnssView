#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtPlugin>

namespace hdgnss {

class IConfigurablePlugin {
public:
    virtual ~IConfigurablePlugin() = default;

    virtual QString settingsCategory() const = 0;
    virtual QString settingsId() const = 0;
    virtual QString settingsDisplayName() const = 0;
    virtual QVariantList settingsFields() const {
        return {};
    }
    virtual void applySettings(const QVariantMap &settings) {
        Q_UNUSED(settings);
    }
};

}  // namespace hdgnss

#define HDGNSS_CONFIGURABLE_PLUGIN_IID "com.hdgnss.IConfigurablePlugin/1.0"
Q_DECLARE_INTERFACE(hdgnss::IConfigurablePlugin, HDGNSS_CONFIGURABLE_PLUGIN_IID)
