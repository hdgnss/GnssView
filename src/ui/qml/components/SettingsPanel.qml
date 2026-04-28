import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    parent: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    readonly property real hostWidth: parent ? parent.width : 960
    readonly property real hostHeight: parent ? parent.height : 760
    width: Math.max(360, Math.min(920, hostWidth - 40))
    height: Math.max(320, Math.min(760, hostHeight - 40))
    x: Math.max(20, (hostWidth - width) * 0.5)
    y: Math.max(20, (hostHeight - height) * 0.5)
    padding: 0

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

    component DenseLabel: Label {
        color: theme.textSecondary
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        wrapMode: Text.WordWrap
    }

    component FieldLabel: Label {
        color: theme.textPrimary
        font.family: theme.bodyFont
        font.pixelSize: theme.labelSize
        font.bold: true
        wrapMode: Text.WordWrap
    }

    component HelpLabel: Label {
        color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        wrapMode: Text.WordWrap
    }

    component DenseField: TextField {
        implicitHeight: theme.controlHeight
        color: theme.textPrimary
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        selectByMouse: true
        padding: 10
        background: Rectangle {
            radius: theme.controlRadius
            color: theme.inputBg
            border.width: 1
            border.color: parent.activeFocus ? theme.inputBorderFocus : theme.inputBorder
        }
    }

    component SectionTitle: Label {
        color: theme.textPrimary
        font.family: theme.titleFont
        font.pixelSize: theme.bodySize + 1
        font.bold: true
    }

    component SettingsCheckBox: CheckBox {
        id: control
        spacing: 10
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        leftPadding: indicator.width + spacing
        implicitHeight: theme.controlHeight

        indicator: Rectangle {
            implicitWidth: 22
            implicitHeight: 22
            x: 0
            y: parent.topPadding + (parent.availableHeight - height) * 0.5
            radius: 6
            color: control.checked
                ? Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, 0.18)
                : Qt.rgba(theme.surface2.r, theme.surface2.g, theme.surface2.b, 0.94)
            border.width: 1
            border.color: control.checked ? theme.accentStrong : theme.panelBorderStrong

            Text {
                anchors.centerIn: parent
                text: control.checked ? "✓" : ""
                color: theme.textPrimary
                font.family: theme.titleFont
                font.pixelSize: 15
                font.bold: true
            }
        }

        contentItem: Text {
            text: control.text
            font: control.font
            color: control.enabled ? theme.textPrimary : theme.textSecondary
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }

    component SettingsSection: Rectangle {
        required property string title
        property color accent: theme.accent
        default property alias content: sectionContent.data
        width: parent ? parent.width : implicitWidth
        implicitHeight: sectionHeader.height + sectionContent.childrenRect.height + 28
        radius: theme.radius
        color: theme.panelBase
        border.width: 1
        border.color: theme.panelBorderStrong

        Item {
            id: sectionHeader
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: 34

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: accent
                }

                Label {
                    Layout.fillWidth: true
                    text: title
                    color: theme.textPrimary
                    font.family: theme.titleFont
                    font.pixelSize: theme.bodySize + 1
                    font.bold: true
                }
            }
        }

        Item {
            id: sectionContent
            anchors {
                left: parent.left
                right: parent.right
                top: sectionHeader.bottom
                leftMargin: 12
                rightMargin: 12
                topMargin: 8
            }
        }
    }

    component PluginCard: Rectangle {
        id: pluginCard
        required property var sectionData
        property bool exclusiveEnable: false
        property var exclusivePluginIds: []
        width: parent ? parent.width : implicitWidth
        implicitHeight: pluginCardContent.childrenRect.height + 20
        radius: theme.cardRadius
        color: theme.surface2
        border.width: 1
        border.color: theme.panelBorder
        clip: true

        Rectangle {
            anchors {
                left: parent.left
            right: parent.right
            top: parent.top
            }
            height: 1
            color: theme.panelBorder
        }

        Column {
            id: pluginCardContent
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            RowLayout {
                width: parent.width
                spacing: 10

                SectionTitle {
                    Layout.fillWidth: true
                    text: prettifyPluginName(sectionData)
                }

                DenseLabel {
                    visible: root.pluginVersionText(pluginCard.sectionData).length > 0
                    text: root.pluginVersionText(pluginCard.sectionData)
                    Layout.alignment: Qt.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: theme.labelSize
                    elide: Text.ElideRight
                }

                SettingsCheckBox {
                    id: enabledCheck
                    text: "Enabled"
                    checked: !!sectionData.enabled
                    onToggled: {
                        if (!appSettings) {
                            return
                        }
                        if (exclusiveEnable) {
                            appSettings.setExclusivePluginEnabled(exclusivePluginIds, sectionData.pluginId, checked)
                        } else {
                            appSettings.setPluginEnabled(sectionData.pluginId, checked)
                        }
                    }
                }
            }

            HelpLabel {
                text: sectionData.category === "tec"
                    ? "TEC plugin is discovered from the shared plugin directory. When multiple TEC plugins are enabled, the first loaded plugin is used."
                    : (sectionData.category === "automation"
                        ? "Only one AutoTest plugin can be enabled. The enabled plugin owns the AutoTest panel."
                        : (sectionData.category === "transport"
                            ? "Transport plugin fields define runtime link parameters. Probe and open actions still happen in the Transport panel."
                            : "Protocol plugin discovery stays available even when the global plugin switch is off, so you can preconfigure it here."))
            }

            Repeater {
                model: sectionData.fields || []

                delegate: Column {
                    required property var modelData
                    width: parent.width
                    spacing: 6

                    FieldLabel {
                        text: modelData.label || modelData.key
                    }

                    Loader {
                        width: parent.width
                        property var fieldData: modelData
                        sourceComponent: {
                            var fieldType = String(modelData.type || "text").toLowerCase()
                            if (fieldType === "select" || fieldType === "enum" || fieldType === "combo") {
                                return pluginComboField
                            }
                            if (fieldType === "bool" || fieldType === "boolean" || fieldType === "checkbox") {
                                return pluginBoolField
                            }
                            return pluginTextField
                        }
                    }

                    HelpLabel {
                        visible: !!(modelData.description && modelData.description.length > 0)
                        text: modelData.description || ""
                    }
                }
            }

            HelpLabel {
                visible: sectionData.category !== "automation" && (!sectionData.fields || sectionData.fields.length === 0)
                text: "This plugin currently has no extra private settings."
            }
        }
    }

    component PluginComboBox: ComboBox {
        id: combo
        implicitHeight: theme.controlHeight
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        textRole: "label"
        valueRole: "value"
        displayText: {
            if (currentIndex >= 0 && model && model.length !== undefined && model.length > currentIndex) {
                var item = model[currentIndex]
                if (item && item.label !== undefined) {
                    return String(item.label)
                }
            }
            return currentText
        }
        background: Rectangle {
            radius: theme.controlRadius
            color: theme.inputBg
            border.width: 1
            border.color: parent.activeFocus ? theme.inputBorderFocus : theme.inputBorder
        }
        contentItem: Text {
            leftPadding: 10
            rightPadding: 26
            text: parent.displayText
            color: theme.textPrimary
            font: parent.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component PluginFieldCheckBox: CheckBox {
        id: control
        spacing: 10
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        leftPadding: indicator.width + spacing
        implicitHeight: theme.controlHeight

        indicator: Rectangle {
            implicitWidth: 22
            implicitHeight: 22
            x: 0
            y: parent.topPadding + (parent.availableHeight - height) * 0.5
            radius: 6
            color: control.checked
                ? Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, 0.18)
                : Qt.rgba(theme.surface2.r, theme.surface2.g, theme.surface2.b, 0.94)
            border.width: 1
            border.color: control.checked ? theme.accentStrong : theme.panelBorderStrong

            Text {
                anchors.centerIn: parent
                text: control.checked ? "✓" : ""
                color: theme.textPrimary
                font.family: theme.titleFont
                font.pixelSize: 15
                font.bold: true
            }
        }

        contentItem: Text {
            text: control.text
            font: control.font
            color: control.enabled ? theme.textPrimary : theme.textSecondary
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }

    Component {
        id: pluginTextField

        Item {
            property var fieldData: parent && parent.fieldData !== undefined ? parent.fieldData : null
            implicitWidth: fieldEditor.implicitWidth
            implicitHeight: fieldEditor.implicitHeight
            width: parent ? parent.width : implicitWidth

            DenseField {
                id: fieldEditor
                width: parent.width
                text: parent.fieldData && parent.fieldData.value !== undefined && parent.fieldData.value !== null
                    ? String(parent.fieldData.value)
                    : ""
                placeholderText: parent.fieldData ? (parent.fieldData.placeholder || "") : ""
                echoMode: String(parent.fieldData ? (parent.fieldData.type || "") : "").toLowerCase() === "password"
                    ? TextInput.Password
                    : TextInput.Normal
                onEditingFinished: {
                    if (appSettings && parent.fieldData) {
                        appSettings.setPluginSettingValue(sectionData.pluginId, parent.fieldData.key, text)
                    }
                }
            }
        }
    }

    Component {
        id: pluginComboField

        Item {
            property var fieldData: parent && parent.fieldData !== undefined ? parent.fieldData : null
            implicitWidth: fieldEditor.implicitWidth
            implicitHeight: fieldEditor.implicitHeight
            width: parent ? parent.width : implicitWidth

            PluginComboBox {
                id: fieldEditor
                width: parent.width
                model: {
                    var source = parent.fieldData && (parent.fieldData.options || parent.fieldData.choices || parent.fieldData.values)
                    if (!source || source.length === undefined) {
                        return []
                    }
                    var result = []
                    for (var i = 0; i < source.length; ++i) {
                        var item = source[i]
                        if (typeof item === "object") {
                            result.push({
                                label: item.label !== undefined ? String(item.label) : String(item.value),
                                value: item.value !== undefined ? item.value : item.label
                            })
                        } else {
                            result.push({ label: String(item), value: item })
                        }
                    }
                    return result
                }
                Component.onCompleted: {
                    var value = parent.fieldData ? parent.fieldData.value : undefined
                    for (var i = 0; i < model.length; ++i) {
                        if (String(model[i].value) === String(value)) {
                            currentIndex = i
                            return
                        }
                    }
                    if (model.length > 0) {
                        currentIndex = 0
                    }
                }
                onActivated: {
                    if (appSettings && parent.fieldData && currentIndex >= 0 && currentIndex < model.length) {
                        appSettings.setPluginSettingValue(sectionData.pluginId, parent.fieldData.key, model[currentIndex].value)
                    }
                }
            }
        }
    }

    Component {
        id: pluginBoolField

        Item {
            property var fieldData: parent && parent.fieldData !== undefined ? parent.fieldData : null
            implicitWidth: fieldEditor.implicitWidth
            implicitHeight: fieldEditor.implicitHeight
            width: parent ? parent.width : implicitWidth

            PluginFieldCheckBox {
                id: fieldEditor
                width: parent.width
                text: parent.fieldData ? (parent.fieldData.checkboxLabel || "Enabled") : "Enabled"
                checked: {
                    var value = parent.fieldData ? parent.fieldData.value : undefined
                    return value === true || String(value).toLowerCase() === "true" || String(value) === "1"
                }
                onToggled: {
                    if (appSettings && parent.fieldData) {
                        appSettings.setPluginSettingValue(sectionData.pluginId, parent.fieldData.key, checked)
                    }
                }
            }
        }
    }

    background: Rectangle {
        radius: theme.radius
        color: theme.panelBase
        border.width: 1
        border.color: theme.panelBorderStrong
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0.01, 0.03, 0.08, 0.68)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: theme.radius
            color: theme.panelBase
            border.width: 1
            border.color: theme.panelBorderStrong

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: "Application Settings"
                        color: theme.textPrimary
                        font.family: theme.titleFont
                        font.pixelSize: theme.titleSize
                        font.bold: true
                    }

                    DenseLabel {
                        text: "Configure logs, plugin discovery, private plugin parameters, and app window behavior."
                    }
                }

                NeonButton {
                    text: "Close"
                    accent: theme.bad
                    onClicked: root.close()
                }
            }
        }

        Flickable {
            id: settingsFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: settingsContent.childrenRect.height
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            ScrollBar.vertical: ScrollBar {}

            Column {
                id: settingsContent
                width: settingsFlick.width
                spacing: 12

                SettingsSection {
                    id: loggingSection
                    width: parent.width
                    title: "Logging"
                    accent: theme.accentStrong

                    Column {
                        width: parent.width
                        spacing: 10

                        RowLayout {
                            width: parent.width
                            spacing: 12

                            SettingsCheckBox {
                                text: "Record raw data"
                                enabled: appSettings ? appSettings.logDirectory.length > 0 : false
                                checked: appSettings ? appSettings.recordRawData : false
                                onToggled: if (appSettings) appSettings.recordRawData = checked
                            }

                            SettingsCheckBox {
                                text: "Record decode log"
                                enabled: appSettings ? appSettings.logDirectory.length > 0 : false
                                checked: appSettings ? appSettings.recordDecodeLog : false
                                onToggled: if (appSettings) appSettings.recordDecodeLog = checked
                            }
                        }

                        FieldLabel {
                            text: "Log root directory"
                        }

                        RowLayout {
                            width: parent.width
                            spacing: 8

                            DenseField {
                                width: Math.max(120, parent.width - 90)
                                text: appSettings ? appSettings.logDirectory : ""
                                onEditingFinished: if (appSettings) appSettings.logDirectory = text
                            }

                            NeonButton {
                                text: "Default"
                                onClicked: if (appSettings) appSettings.logDirectory = appSettings.defaultLogDirectory
                            }
                        }

                        HelpLabel {
                            text: appSettings && appSettings.logDirectory.length > 0
                                  ? "Raw data writes the binary stream capture. Decode log writes the parsed text log into the next recording session."
                                  : "Set a log root directory before enabling raw or decode logging."
                        }
                    }
                }

                SettingsSection {
                    id: pluginsSection
                    width: parent.width
                    title: "Plugins"
                    accent: theme.accent

                    Column {
                        width: parent.width
                        spacing: 12

                        SettingsCheckBox {
                            text: "Enable runtime plugins"
                            checked: appSettings ? appSettings.pluginsEnabled : true
                            onToggled: if (appSettings) appSettings.pluginsEnabled = checked
                        }

                        FieldLabel {
                            text: "Plugin load directory"
                        }

                        RowLayout {
                            width: parent.width
                            spacing: 8

                            DenseField {
                                width: Math.max(120, parent.width - 90)
                                text: appSettings ? appSettings.pluginDirectory : ""
                                onEditingFinished: if (appSettings) appSettings.pluginDirectory = text
                            }

                            NeonButton {
                                text: "Default"
                                onClicked: if (appSettings) appSettings.pluginDirectory = appSettings.defaultPluginDirectory
                            }
                        }

                        HelpLabel {
                            text: "Global switch controls whether plugins actually run. Each plugin below can still be individually enabled and keep its own private settings."
                        }

                        SectionTitle {
                            text: "AutoTest Plugins"
                        }

                        Repeater {
                            model: appController ? appController.availableAutomationPlugins : []
                            delegate: PluginCard {
                                required property var modelData
                                sectionData: modelData
                                exclusiveEnable: true
                                exclusivePluginIds: root.automationPluginIds()
                            }
                        }

                        HelpLabel {
                            visible: !appController || !appController.availableAutomationPlugins || appController.availableAutomationPlugins.length === 0
                            text: "No AutoTest plugin was discovered in the current plugin directory."
                        }

                        HelpLabel {
                            visible: appController
                                     && appController.automationPluginLoadErrors
                                     && appController.automationPluginLoadErrors.length > 0
                            text: "AutoTest plugin load errors:\n" + appController.automationPluginLoadErrors.join("\n")
                        }

                        HelpLabel {
                            visible: appController
                                     && appController.automationPluginSearchPaths
                                     && appController.automationPluginSearchPaths.length > 0
                            text: "AutoTest plugin search paths:\n" + appController.automationPluginSearchPaths.join("\n")
                        }

                        SectionTitle {
                            text: "Protocol Plugins"
                        }

                        Repeater {
                            model: appController ? appController.availableProtocolPlugins : []
                            delegate: PluginCard {
                                required property var modelData
                                sectionData: modelData
                            }
                        }

                        HelpLabel {
                            visible: !appController || !appController.availableProtocolPlugins || appController.availableProtocolPlugins.length === 0
                            text: "No protocol plugin was discovered in the current plugin directory."
                        }

                        HelpLabel {
                            visible: appController
                                     && appController.protocolPluginLoadErrors
                                     && appController.protocolPluginLoadErrors.length > 0
                            text: "Protocol plugin load errors:\n" + appController.protocolPluginLoadErrors.join("\n")
                        }

                        HelpLabel {
                            visible: appController
                                     && appController.protocolPluginSearchPaths
                                     && appController.protocolPluginSearchPaths.length > 0
                            text: "Protocol plugin search paths:\n" + appController.protocolPluginSearchPaths.join("\n")
                        }

                        SectionTitle {
                            text: "TEC Plugins"
                        }

                        Repeater {
                            model: tecMapOverlayModel ? tecMapOverlayModel.availableTecPlugins : []
                            delegate: PluginCard {
                                required property var modelData
                                sectionData: modelData
                            }
                        }

                        HelpLabel {
                            visible: !tecMapOverlayModel || !tecMapOverlayModel.availableTecPlugins || tecMapOverlayModel.availableTecPlugins.length === 0
                            text: "No TEC plugin was discovered in the current plugin directory."
                        }

                        HelpLabel {
                            visible: tecMapOverlayModel
                                     && tecMapOverlayModel.pluginLoadErrors
                                     && tecMapOverlayModel.pluginLoadErrors.length > 0
                            text: "TEC plugin load errors:\n" + tecMapOverlayModel.pluginLoadErrors.join("\n")
                        }

                        HelpLabel {
                            visible: tecMapOverlayModel
                                     && tecMapOverlayModel.pluginSearchPaths
                                     && tecMapOverlayModel.pluginSearchPaths.length > 0
                            text: "TEC plugin search paths:\n" + tecMapOverlayModel.pluginSearchPaths.join("\n")
                        }

                        SectionTitle {
                            text: "Transport Plugins"
                        }

                        Repeater {
                            model: transportViewModel ? transportViewModel.availableTransports.filter(function(item) { return item.category === "plugin" }) : []
                            delegate: PluginCard {
                                required property var modelData
                                sectionData: modelData
                            }
                        }

                        HelpLabel {
                            visible: !transportViewModel
                                     || !transportViewModel.availableTransports
                                     || transportViewModel.availableTransports.filter(function(item) { return item.category === "plugin" }).length === 0
                            text: "No transport plugin was discovered in the current plugin directory."
                        }

                        HelpLabel {
                            visible: transportViewModel
                                     && transportViewModel.transportPluginLoadErrors
                                     && transportViewModel.transportPluginLoadErrors.length > 0
                            text: "Transport plugin load errors:\n" + transportViewModel.transportPluginLoadErrors.join("\n")
                        }

                        HelpLabel {
                            visible: transportViewModel
                                     && transportViewModel.transportPluginSearchPaths
                                     && transportViewModel.transportPluginSearchPaths.length > 0
                            text: "Transport plugin search paths:\n" + transportViewModel.transportPluginSearchPaths.join("\n")
                        }
                    }
                }

                SettingsSection {
                    id: appSection
                    width: parent.width
                    title: "Application"
                    accent: theme.ok

                    Column {
                        width: parent.width
                        spacing: 10

                        FieldLabel {
                            text: "Version"
                        }

                        RowLayout {
                            width: parent.width
                            spacing: 8

                            DenseLabel {
                                Layout.fillWidth: true
                                text: updateChecker
                                      ? ("Current " + updateChecker.currentVersion
                                         + (updateChecker.latestVersion && updateChecker.latestVersion.length > 0
                                            ? " | Latest " + updateChecker.latestVersion
                                            : ""))
                                      : "Current version unavailable"
                            }

                            NeonButton {
                                text: updateChecker && updateChecker.checking ? "Checking" : "Check"
                                enabled: updateChecker ? !updateChecker.checking : false
                                onClicked: if (updateChecker) updateChecker.checkForUpdates(false)
                            }

                            NeonButton {
                                text: "Release"
                                visible: updateChecker && updateChecker.releaseUrl && updateChecker.releaseUrl.length > 0
                                onClicked: if (updateChecker) updateChecker.openReleasePage()
                            }

                            NeonButton {
                                text: "Download"
                                visible: updateChecker && updateChecker.hasUpdate
                                accent: theme.ok
                                onClicked: if (updateChecker) updateChecker.openDownloadPage()
                            }
                        }

                        SettingsCheckBox {
                            text: "Check for updates on startup"
                            checked: appSettings ? appSettings.checkForUpdatesOnStartup : true
                            onToggled: if (appSettings) appSettings.checkForUpdatesOnStartup = checked
                        }

                        HelpLabel {
                            text: updateChecker ? updateChecker.statusText : "Update checker is not available."
                        }

                        SettingsCheckBox {
                            text: "Remember main window position and size"
                            checked: appSettings ? appSettings.rememberWindowGeometry : true
                            onToggled: if (appSettings) appSettings.rememberWindowGeometry = checked
                        }

                        SettingsCheckBox {
                            text: "Clear RawData, SkyView, Signal Spectrum, statistics, and Deviation Map on a new transport connection"
                            checked: appSettings ? appSettings.resetUiOnNewConnection : true
                            onToggled: if (appSettings) appSettings.resetUiOnNewConnection = checked
                        }
                    }
                }

                SettingsSection {
                    id: deviationSection
                    width: parent.width
                    title: "Deviation Map"
                    accent: theme.accentStrong

                    Column {
                        width: parent.width
                        spacing: 10

                        SettingsCheckBox {
                            text: "Use fixed center latitude/longitude"
                            checked: appSettings ? appSettings.useFixedDeviationCenter : false
                            onToggled: if (appSettings) appSettings.useFixedDeviationCenter = checked
                        }

                        GridLayout {
                            width: parent.width
                            columns: 2
                            columnSpacing: 8
                            rowSpacing: 8

                            FieldLabel { text: "Latitude" }
                            DenseField {
                                Layout.fillWidth: true
                                text: appSettings ? String(appSettings.fixedDeviationLatitude) : "0"
                                enabled: appSettings ? appSettings.useFixedDeviationCenter : false
                                onEditingFinished: if (appSettings) appSettings.fixedDeviationLatitude = Number(text)
                            }

                            FieldLabel { text: "Longitude" }
                            DenseField {
                                Layout.fillWidth: true
                                text: appSettings ? String(appSettings.fixedDeviationLongitude) : "0"
                                enabled: appSettings ? appSettings.useFixedDeviationCenter : false
                                onEditingFinished: if (appSettings) appSettings.fixedDeviationLongitude = Number(text)
                            }
                        }

                        HelpLabel {
                            text: "If disabled, the map uses the average of all recorded positions in the current session as the center."
                        }
                    }
                }
            }
        }
    }
}
