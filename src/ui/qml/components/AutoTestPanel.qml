import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

// Generic automation panel host.
// Dynamically loads the plugin's panel QML (via IAutomationPlugin::panelUi())
// and injects the controller object and style token map.
// The static AutoTestPanel.qml content has been moved to the plugin itself:
//   Plugins/autotest/qml/AutoTestPanelUi.qml

GlassPanel {
    id: root
    title: ""
    accent: theme.accentStrong

    Theme { id: theme }

    // ── Dynamic panel loader ──────────────────────────────────────────────

    property string panelLoadError: ""

    Connections {
        target: appController
        function onAvailableAutomationPluginsChanged() {
            root.reloadPanelUi()
        }
    }

    Connections {
        target: appSettings
        function onUiThemeChanged() {
            Qt.callLater(root.applyPanelStyle)
        }
    }

    Item {
        id: panelHost
        anchors {
            fill: parent
            topMargin: 0
            leftMargin: 0
            rightMargin: 0
            bottomMargin: 0
        }

        property var contentItem: null

        Component.onCompleted: root.reloadPanelUi()
    }

    function reloadPanelUi() {
        root.panelLoadError = ""

        // Destroy any previous instance
        if (panelHost.contentItem) {
            panelHost.contentItem.destroy()
            panelHost.contentItem = null
        }

        var config = appController.automationPanelUiConfig()
        if (!config || !config.type) {
            root.panelLoadError = "AutoTest plugin not loaded"
            return
        }

        var item = null
        if (config.type === "qmlFile") {
            var component = Qt.createComponent(config.qmlFile)
            if (component.status === Component.Ready) {
                item = component.createObject(panelHost)
            } else {
                root.panelLoadError = "AutoTest panel failed to load:\n" + component.errorString()
                console.error("AutoTestPanel: failed to load QML file:", config.qmlFile, component.errorString())
                return
            }
        } else if (config.type === "qmlSource") {
            try {
                item = Qt.createQmlObject(config.qmlSource, panelHost, "AutoTestPanelUi.qml")
            } catch (error) {
                root.panelLoadError = "AutoTest panel failed to load:\n" + error
                console.error("AutoTestPanel: failed to create QML source:", error)
                return
            }
        }

        if (!item) {
            root.panelLoadError = "AutoTest panel failed to load"
            return
        }

        panelHost.contentItem = item
        item.anchors.fill = panelHost

        if ("controller" in item) {
            item.controller = appController.automationController()
        }
        root.applyPanelStyle()
    }

    function applyPanelStyle() {
        if (panelHost.contentItem && "style" in panelHost.contentItem) {
            panelHost.contentItem.style = root.panelStyle()
        }
    }

    function panelStyle() {
        return theme.pluginStyle()
    }

    // ── Fallback overlay (plugin not loaded) ──────────────────────────────

    Rectangle {
        visible: panelHost.contentItem === null
        anchors.fill: parent
        color: theme.surface0
        radius: theme.panelRadius

        Label {
            anchors.centerIn: parent
            width: Math.min(parent.width - 32, 720)
            text: root.panelLoadError.length > 0 ? root.panelLoadError : "AutoTest plugin not loaded"
            color: theme.textSecondary
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }
}
