import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SettingsSection {
    id: section
    title: "Plugins & Extensions"
    accent: theme.accent

    Theme { id: theme }

    function automationPluginIds() {
        var ids = []
        var plugins = appController ? appController.availableAutomationPlugins : []
        if (!plugins) {
            return ids
        }
        for (var i = 0; i < plugins.length; ++i) {
            if (plugins[i] && plugins[i].pluginId) {
                ids.push(plugins[i].pluginId)
            }
        }
        return ids
    }

    function prettifyPluginName(sectionData) {
        if (!sectionData) {
            return "Plugin"
        }
        if (sectionData.displayName && String(sectionData.displayName).trim().length > 0) {
            return String(sectionData.displayName).trim()
        }
        var name = sectionData.pluginId || "Plugin"
        if (name.indexOf("::") >= 0) {
            var parts = name.split("::")
            name = parts[parts.length - 1]
        }
        if (name.indexOf(".") >= 0) {
            var idParts = name.split(".")
            name = idParts[idParts.length - 1]
        }
        if (name.endsWith("Plugin")) {
            name = name.slice(0, -6)
        }
        name = name.replace(/[-_]+/g, " ")
        name = name.replace(/([a-z0-9])([A-Z])/g, "$1 $2")
        return name.length > 0 ? name.charAt(0).toUpperCase() + name.slice(1) : "Plugin"
    }

    function pluginVersionText(sectionData) {
        if (!sectionData || sectionData.version === undefined || sectionData.version === null) {
            return ""
        }
        var version = String(sectionData.version).trim()
        if (version.length === 0) {
            return ""
        }
        return version.toLowerCase().startsWith("v") ? version : "v" + version
    }

    component PluginCardItem: Rectangle {
        id: card
        required property var sectionData
        property bool exclusiveEnable: false
        property var exclusivePluginIds: []
        width: parent ? parent.width : implicitWidth
        implicitHeight: cardLayout.childrenRect.height + 24
        radius: theme.cardRadius
        color: theme.surface2
        border.width: 1
        border.color: theme.panelBorder

        Column {
            id: cardLayout
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            RowLayout {
                width: parent.width
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    text: section.prettifyPluginName(card.sectionData)
                    color: theme.textPrimary
                    font.family: theme.titleFont
                    font.pixelSize: theme.bodySize + 1
                    font.bold: true
                }

                Label {
                    visible: section.pluginVersionText(card.sectionData).length > 0
                    text: section.pluginVersionText(card.sectionData)
                    color: theme.textSecondary
                    font.family: theme.monoFont
                    font.pixelSize: theme.labelSize
                    Layout.alignment: Qt.AlignVCenter
                }

                AppCheckBox {
                    text: "Enabled"
                    checked: !!card.sectionData.enabled
                    onToggled: {
                        if (!appSettings) {
                            return
                        }
                        if (card.exclusiveEnable) {
                            appSettings.setExclusivePluginEnabled(card.exclusivePluginIds, card.sectionData.pluginId, checked)
                        } else {
                            appSettings.setPluginEnabled(card.sectionData.pluginId, checked)
                        }
                    }
                }
            }

            Label {
                width: parent.width
                text: card.sectionData.category === "tec"
                    ? "TEC plugin is discovered from the shared plugin directory. When multiple TEC plugins are enabled, the first loaded plugin is used."
                    : (card.sectionData.category === "automation"
                        ? "Only one AutoTest plugin can be enabled. The enabled plugin owns the AutoTest panel."
                        : (card.sectionData.category === "transport"
                            ? "Transport plugin fields define runtime link parameters. Probe and open actions still happen in the Transport panel."
                            : "Protocol plugin discovery stays available even when the global plugin switch is off, so you can preconfigure it here."))
                color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
                font.family: theme.bodyFont
                font.pixelSize: theme.bodySize
                wrapMode: Text.WordWrap
            }

            PluginSettingsForm {
                width: parent.width
                pluginId: card.sectionData.pluginId || ""
                fields: card.sectionData.fields || []
                category: card.sectionData.category || ""
            }
        }
    }

    Column {
        width: parent.width
        spacing: 14

        // Global plugin switch
        AppCheckBox {
            text: "Enable runtime plugins"
            checked: appSettings ? appSettings.pluginsEnabled : true
            onToggled: if (appSettings) appSettings.pluginsEnabled = checked
        }

        Label {
            width: parent.width
            text: "Global switch controls whether plugins actually run. Each plugin below can still be individually enabled and keep its own private settings."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        Rectangle {
            width: parent.width
            height: 1
            color: theme.divider
        }

        // 1. AutoTest Plugins
        Label {
            text: "AutoTest Plugins"
            color: theme.textPrimary
            font.family: theme.titleFont
            font.pixelSize: theme.bodySize + 1
            font.bold: true
        }

        Repeater {
            model: appController ? appController.availableAutomationPlugins : []
            delegate: PluginCardItem {
                required property var modelData
                sectionData: modelData
                exclusiveEnable: true
                exclusivePluginIds: section.automationPluginIds()
            }
        }

        Label {
            visible: !appController || !appController.availableAutomationPlugins || appController.availableAutomationPlugins.length === 0
            width: parent.width
            text: "No AutoTest plugin was discovered in the current plugin directory."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        Label {
            visible: appController && appController.automationPluginLoadErrors && appController.automationPluginLoadErrors.length > 0
            width: parent.width
            text: "AutoTest plugin load errors:\n" + (appController ? appController.automationPluginLoadErrors.join("\n") : "")
            color: theme.bad
            font.family: theme.monoFont
            font.pixelSize: theme.labelSize
            wrapMode: Text.WordWrap
        }

        Label {
            visible: appController && appController.automationPluginSearchPaths && appController.automationPluginSearchPaths.length > 0
            width: parent.width
            text: "AutoTest plugin search paths:\n" + (appController ? appController.automationPluginSearchPaths.join("\n") : "")
            color: theme.textSecondary
            font.family: theme.monoFont
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }

        Rectangle {
            width: parent.width
            height: 1
            color: theme.divider
        }

        // 2. Protocol Plugins
        Label {
            text: "Protocol Plugins"
            color: theme.textPrimary
            font.family: theme.titleFont
            font.pixelSize: theme.bodySize + 1
            font.bold: true
        }

        Repeater {
            model: appController ? appController.availableProtocolPlugins : []
            delegate: PluginCardItem {
                required property var modelData
                sectionData: modelData
            }
        }

        Label {
            visible: !appController || !appController.availableProtocolPlugins || appController.availableProtocolPlugins.length === 0
            width: parent.width
            text: "No protocol plugin was discovered in the current plugin directory."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        Label {
            visible: appController && appController.protocolPluginLoadErrors && appController.protocolPluginLoadErrors.length > 0
            width: parent.width
            text: "Protocol plugin load errors:\n" + (appController ? appController.protocolPluginLoadErrors.join("\n") : "")
            color: theme.bad
            font.family: theme.monoFont
            font.pixelSize: theme.labelSize
            wrapMode: Text.WordWrap
        }

        Label {
            visible: appController && appController.protocolPluginSearchPaths && appController.protocolPluginSearchPaths.length > 0
            width: parent.width
            text: "Protocol plugin search paths:\n" + (appController ? appController.protocolPluginSearchPaths.join("\n") : "")
            color: theme.textSecondary
            font.family: theme.monoFont
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }

        Rectangle {
            width: parent.width
            height: 1
            color: theme.divider
        }

        // 3. TEC Plugins
        Label {
            text: "TEC Plugins"
            color: theme.textPrimary
            font.family: theme.titleFont
            font.pixelSize: theme.bodySize + 1
            font.bold: true
        }

        Repeater {
            model: tecMapOverlayModel ? tecMapOverlayModel.availableTecPlugins : []
            delegate: PluginCardItem {
                required property var modelData
                sectionData: modelData
            }
        }

        Label {
            visible: !tecMapOverlayModel || !tecMapOverlayModel.availableTecPlugins || tecMapOverlayModel.availableTecPlugins.length === 0
            width: parent.width
            text: "No TEC plugin was discovered in the current plugin directory."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        Label {
            visible: tecMapOverlayModel && tecMapOverlayModel.pluginLoadErrors && tecMapOverlayModel.pluginLoadErrors.length > 0
            width: parent.width
            text: "TEC plugin load errors:\n" + (tecMapOverlayModel ? tecMapOverlayModel.pluginLoadErrors.join("\n") : "")
            color: theme.bad
            font.family: theme.monoFont
            font.pixelSize: theme.labelSize
            wrapMode: Text.WordWrap
        }

        Label {
            visible: tecMapOverlayModel && tecMapOverlayModel.pluginSearchPaths && tecMapOverlayModel.pluginSearchPaths.length > 0
            width: parent.width
            text: "TEC plugin search paths:\n" + (tecMapOverlayModel ? tecMapOverlayModel.pluginSearchPaths.join("\n") : "")
            color: theme.textSecondary
            font.family: theme.monoFont
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }

        Rectangle {
            width: parent.width
            height: 1
            color: theme.divider
        }

        // 4. Transport Plugins
        Label {
            text: "Transport Plugins"
            color: theme.textPrimary
            font.family: theme.titleFont
            font.pixelSize: theme.bodySize + 1
            font.bold: true
        }

        Repeater {
            model: transportViewModel ? transportViewModel.availableTransports.filter(function(item) { return item.category === "plugin" }) : []
            delegate: PluginCardItem {
                required property var modelData
                sectionData: modelData
            }
        }

        Label {
            visible: !transportViewModel || !transportViewModel.availableTransports || transportViewModel.availableTransports.filter(function(item) { return item.category === "plugin" }).length === 0
            width: parent.width
            text: "No transport plugin was discovered in the current plugin directory."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        Label {
            visible: transportViewModel && transportViewModel.transportPluginLoadErrors && transportViewModel.transportPluginLoadErrors.length > 0
            width: parent.width
            text: "Transport plugin load errors:\n" + (transportViewModel ? transportViewModel.transportPluginLoadErrors.join("\n") : "")
            color: theme.bad
            font.family: theme.monoFont
            font.pixelSize: theme.labelSize
            wrapMode: Text.WordWrap
        }

        Label {
            visible: transportViewModel && transportViewModel.transportPluginSearchPaths && transportViewModel.transportPluginSearchPaths.length > 0
            width: parent.width
            text: "Transport plugin search paths:\n" + (transportViewModel ? transportViewModel.transportPluginSearchPaths.join("\n") : "")
            color: theme.textSecondary
            font.family: theme.monoFont
            font.pixelSize: 10
            wrapMode: Text.WordWrap
        }
    }
}
