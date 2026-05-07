#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtPlugin>

namespace hdgnss {

// Interface for automation/testing panel plugins.
// Implementations live in the Plugins/ tree as separate shared libraries.
//
// Lifecycle:
//   1. Host calls initialize() after loading.
//   2. Host calls onRawBytesReceived() for every byte frame passing through any transport.
//   3. Plugin emits sendRequested() (via its QObject) to queue bytes for transmission.
//   4. The host calls panelUi() to obtain the plugin's panel QML source and renders it
//      dynamically; controllerObject() is injected into the created QML item.
class IAutomationPlugin {
public:
    virtual ~IAutomationPlugin() = default;

    virtual QString automationId() const = 0;
    virtual QString automationDisplayName() const = 0;

    // Called once after the plugin is loaded and before the QML engine starts.
    virtual void initialize() = 0;

    // Called for every raw byte frame on any transport (both Rx and Tx directions).
    virtual void onRawBytesReceived(const QString &transportName,
                                    const QByteArray &bytes,
                                    bool isRx) = 0;

    // Returns the plugin's primary QObject for:
    //   - Qt signal/slot connections (sendRequested, etc.)
    //   - Injection into the panel QML item as the "controller" property
    // Implementations typically return static_cast<QObject*>(this).
    virtual QObject *controllerObject() = 0;

    // Returns the panel UI descriptor for this plugin.
    // The host uses Qt.createQmlObject() to instantiate the QML source and injects:
    //   - controller: controllerObject()  – the plugin's C++ backend
    //   - style: a QVariantMap of theme tokens (colors, fonts, sizes)
    // The QML root object should declare matching properties:
    //   property var controller
    //   property var style
    // Returns an empty map if the plugin has no custom panel UI (host shows a fallback).
    virtual QVariantMap panelUi() const { return {}; }
};

}  // namespace hdgnss

#define HDGNSS_AUTOMATION_PLUGIN_IID "com.hdgnss.IAutomationPlugin/1.0"
Q_DECLARE_INTERFACE(hdgnss::IAutomationPlugin, HDGNSS_AUTOMATION_PLUGIN_IID)
