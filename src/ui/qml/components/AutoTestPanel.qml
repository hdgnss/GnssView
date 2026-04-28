import QtQuick
import QtQuick.Controls

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

    Connections {
        target: appController
        function onAvailableAutomationPluginsChanged() {
            root.reloadPanelUi()
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
        // Destroy any previous instance
        if (panelHost.contentItem) {
            panelHost.contentItem.destroy()
            panelHost.contentItem = null
        }

        var config = appController.automationPanelUiConfig()
        if (!config || !config.type) {
            return
        }

        var item = null
        if (config.type === "qmlFile") {
            var component = Qt.createComponent(config.qmlFile)
            if (component.status === Component.Ready) {
                item = component.createObject(panelHost)
            } else {
                console.error("AutoTestPanel: failed to load QML file:", config.qmlFile, component.errorString())
                return
            }
        } else if (config.type === "qmlSource") {
            item = Qt.createQmlObject(config.qmlSource, panelHost, "AutoTestPanelUi.qml")
        }

        if (!item) {
            return
        }

        panelHost.contentItem = item
        item.anchors.fill = panelHost

        if ("controller" in item) {
            item.controller = appController.automationController()
        }
        if ("style" in item) {
            item.style = root.panelStyle()
        }
    }

    function panelStyle() {
        return {
            bodyFont:          theme.bodyFont,
            monoFont:          theme.monoFont,
            titleFont:         theme.titleFont,
            bodySize:          theme.bodySize,
            labelSize:         theme.labelSize,
            controlHeight:     theme.controlHeight,
            controlRadius:     theme.controlRadius,
            pillRadius:        theme.pillRadius,
            panelRadius:       theme.panelRadius,
            sectionSpacing:    theme.sectionSpacing,
            textPrimary:       theme.textPrimary,
            textSecondary:     theme.textSecondary,
            hoverText:         theme.hoverText,
            caption:           theme.caption,
            accent:            theme.accent,
            bad:               theme.bad,
            ok:                theme.ok,
            warning:           theme.warning,
            inputBg:           theme.inputBg,
            inputBorder:       theme.inputBorder,
            inputBorderFocus:  theme.inputBorderFocus,
            surface0:          theme.surface0,
            surface1:          theme.surface1,
            surface2:          theme.surface2,
            surface3:          theme.surface3,
            surface4:          theme.surface4,
            panelBase:         theme.panelBase,
            chartBg:           theme.chartBg,
            tooltipBg:         theme.tooltipBg,
            tooltipText:       theme.tooltipText,
            tooltipDetail:     theme.tooltipDetail,
            panelBorder:       theme.panelBorder,
            panelBorderStrong: theme.panelBorderStrong,
            border:            theme.border,
            borderStrong:      theme.borderStrong,
            tabBarBg:          theme.tabBarBg,
            tabBarBorder:      theme.tabBarBorder,
            tabButtonBg:       theme.tabButtonBg,
            tabButtonHover:    theme.tabButtonHover,
            tabButtonActive:   theme.tabButtonActive,
            tabSelectedAccent: theme.tabSelectedAccent
        }
    }

    // ── Fallback overlay (plugin not loaded) ──────────────────────────────

    Rectangle {
        visible: panelHost.contentItem === null
        anchors.fill: parent
        color: theme.surface0
        radius: theme.panelRadius

        Label {
            anchors.centerIn: parent
            text: "AutoTest plugin not loaded"
            color: theme.textSecondary
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
        }
    }
}
