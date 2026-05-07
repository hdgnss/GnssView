#pragma once

#include <QString>
#include <QStringList>
#include <QtPlugin>

namespace hdgnss {

class ITransport;

class ITransportPlugin {
public:
    virtual ~ITransportPlugin() = default;

    virtual QString transportId() const = 0;
    virtual QString transportDisplayName() const = 0;
    virtual QStringList supportedHardwareInterfaces() const {
        return {};
    }
    virtual bool supportsDiscovery() const {
        return false;
    }
    virtual QString probe() {
        return {};
    }
    virtual ITransport *transportInstance() = 0;
};

}  // namespace hdgnss

#define HDGNSS_TRANSPORT_PLUGIN_IID "com.hdgnss.ITransportPlugin/1.0"
Q_DECLARE_INTERFACE(hdgnss::ITransportPlugin, HDGNSS_TRANSPORT_PLUGIN_IID)
