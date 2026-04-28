import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

GlassPanel {
    id: root
    property string groupHint: "Commands"
    title: groupHint
    accent: theme.accent

    Theme { id: theme }

    property int selectedIndex: -1
    property string selectedName: ""
    property string selectedGroup: ""
    property string selectedPayload: ""
    property string selectedType: "ASCII"
    property var protocolBuildMessages: appController && appController.protocolBuildMessages ? appController.protocolBuildMessages : []
    property string selectedBuildProtocol: "Bream"
    property var selectedProtocolBuildMessageModel: appController && typeof appController.protocolBuildMessagesForProtocol === "function"
        ? appController.protocolBuildMessagesForProtocol(selectedBuildProtocol)
        : []
    property string defaultProtocolBuildMessage: selectedProtocolBuildMessageModel.length > 0 ? String(selectedProtocolBuildMessageModel[0].name || "") : ""
    property string selectedPluginMessage: defaultProtocolBuildMessage
    property string selectedFileDecoder: "None"
    property int selectedFilePacketIntervalMs: 0
    property int buttonsPerRow: 3
    property string configSearchText: ""
    property string configFilterGroup: "All Groups"
    property var groupsModel: commandButtonModel && commandButtonModel.groups ? commandButtonModel.groups : ["General"]
    property var fileDecoderModel: appController && appController.fileSendDecoders ? appController.fileSendDecoders : ["None"]
    property bool hasMultipleGroups: groupsModel && groupsModel.length > 1
    property string activeGroup: groupsModel.length > 0 ? String(groupsModel[Math.min(groupTabs.currentIndex, groupsModel.length - 1)]) : "General"
    property var currentButtons: []

    Settings {
        id: uiSettings
        category: "CommandButtonsPanel"
        property int buttonsPerRow: 3
        property int configWidth: 1120
        property int configHeight: 760
        property int configX: 120
        property int configY: 100
        property string activeGroup: ""
    }

    component DenseLabel: Label {
        color: theme.textSecondary
        font.family: theme.bodyFont
        font.pixelSize: theme.labelSize
    }

    component DenseTextField: TextField {
        implicitHeight: theme.controlHeight
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        color: theme.textPrimary
        background: Rectangle {
            radius: theme.controlRadius
            color: theme.inputBg
            border.width: 1
            border.color: parent.activeFocus ? theme.inputBorderFocus : theme.inputBorder
        }
    }

    component DenseComboBox: ComboBox {
        id: combo
        implicitHeight: theme.controlHeight
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        displayText: currentIndex >= 0 ? currentText : editText
        leftPadding: 0
        rightPadding: 0
        background: Rectangle {
            radius: theme.controlRadius
            color: theme.inputBg
            border.width: 1
            border.color: parent.activeFocus ? theme.inputBorderFocus : theme.inputBorder
            antialiasing: true
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
        indicator: Canvas {
            x: parent.width - width - 8
            y: (parent.height - height) / 2
            width: 10
            height: 6
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.fillStyle = theme.textSecondary
                ctx.beginPath()
                ctx.moveTo(0, 0)
                ctx.lineTo(width, 0)
                ctx.lineTo(width / 2, height)
                ctx.closePath()
                ctx.fill()
            }
        }
        delegate: ItemDelegate {
            width: ListView.view ? ListView.view.width : parent.width
            height: theme.controlHeight
            highlighted: combo.highlightedIndex === index
            padding: 0
            contentItem: Text {
                text: modelData
                leftPadding: 10
                rightPadding: 10
                color: highlighted ? theme.textPrimary : theme.textSecondary
                font.family: theme.bodyFont
                font.pixelSize: theme.bodySize
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            background: Rectangle {
                radius: theme.controlRadius - 2
                color: highlighted ? theme.tabButtonActive : (combo.currentIndex === index ? theme.surface3 : "transparent")
                border.width: highlighted ? 1 : 0
                border.color: highlighted ? theme.inputBorderFocus : "transparent"
            }
        }
        popup: Popup {
            y: combo.height + 6
            width: combo.width
            padding: 6
            implicitHeight: Math.min(contentItem.implicitHeight + 12, 220)
            background: Rectangle {
                radius: theme.panelRadius
                color: theme.tooltipBg
                border.width: 1
                border.color: theme.panelBorderStrong
                antialiasing: true
            }
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
                spacing: 4
                ScrollBar.vertical: ScrollBar {
                    policy: ListView.view && ListView.view.contentHeight > ListView.view.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }
            }
        }
    }

    function invoke(modelObject, methodName, args) {
        if (!modelObject) {
            return
        }
        var fn = modelObject[methodName]
        if (typeof fn === "function") {
            fn.apply(modelObject, args || [])
        }
    }

    function currentDefinition() {
        return {
            name: root.selectedName,
            group: root.selectedGroup.length > 0 ? root.selectedGroup : root.activeGroup,
            payload: root.selectedPayload,
            contentType: root.selectedType,
            buildProtocol: root.selectedBuildProtocol,
            pluginMessage: root.selectedPluginMessage,
            fileDecoder: root.selectedFileDecoder,
            filePacketIntervalMs: root.selectedFilePacketIntervalMs
        }
    }

    function protocolBuildProtocols() {
        var names = []
        for (var i = 0; i < protocolBuildMessages.length; ++i) {
            var item = protocolBuildMessages[i]
            var protocolName = item && item.protocol ? String(item.protocol) : ""
            if (protocolName.length > 0 && names.indexOf(protocolName) < 0) {
                names.push(protocolName)
            }
        }
        return names
    }

    function protocolBuildMessageDefinition(protocolName, messageName) {
        if (!appController || typeof appController.protocolBuildMessageDefinition !== "function") {
            return {}
        }
        return appController.protocolBuildMessageDefinition(protocolName || "", messageName || "")
    }

    function defaultProtocolBuildMessageFor(protocolName) {
        if (!appController || typeof appController.protocolBuildMessagesForProtocol !== "function") {
            return ""
        }
        var messages = appController.protocolBuildMessagesForProtocol(protocolName || "")
        return messages.length > 0 ? String(messages[0].name || "") : ""
    }

    function applyPluginSelection(protocolName, messageName) {
        var resolvedProtocol = protocolName || root.selectedBuildProtocol
        var resolvedMessage = messageName || defaultProtocolBuildMessageFor(resolvedProtocol)
        var definition = root.protocolBuildMessageDefinition(resolvedProtocol, resolvedMessage)
        root.selectedBuildProtocol = resolvedProtocol
        root.selectedPluginMessage = resolvedMessage
        root.selectedPayload = definition.defaultPayload || ""
    }

    function loadSelection(item) {
        if (!item) {
            return
        }
        selectedIndex = item.index !== undefined ? item.index : -1
        selectedName = item.name || ""
        selectedGroup = item.group || activeGroup
        selectedPayload = item.payload || ""
        selectedType = item.contentType || "ASCII"
        selectedBuildProtocol = item.buildProtocol || "Bream"
        selectedPluginMessage = item.pluginMessage || defaultProtocolBuildMessage
        selectedFileDecoder = item.fileDecoder || "None"
        selectedFilePacketIntervalMs = item.filePacketIntervalMs !== undefined ? Number(item.filePacketIntervalMs) : 0
        syncPayloadEditor(true)
    }

    function clearSelection() {
        selectedIndex = -1
        selectedName = ""
        selectedGroup = activeGroup
        selectedPayload = ""
        selectedType = "ASCII"
        selectedBuildProtocol = "Bream"
        selectedPluginMessage = defaultProtocolBuildMessage
        selectedFileDecoder = "None"
        selectedFilePacketIntervalMs = 0
        syncPayloadEditor(true)
    }

    function ensureSelection() {
        if (selectedIndex >= 0 || !commandButtonModel || commandButtonModel.rowCount() <= 0) {
            return
        }
        root.loadSelection(commandButtonModel.get(0))
        selectedIndex = 0
    }

    function syncPayloadEditor(force) {
        if (payloadEditor && (force || !payloadEditor.activeFocus) && payloadEditor.text !== root.selectedPayload) {
            payloadEditor.text = root.selectedPayload
        }
    }

    function clampConfigSize() {
        if (!root.Window) {
            return
        }
        configPopup.width = Math.max(860, Math.min(root.Window.width - 40, configPopup.width))
        configPopup.height = Math.max(560, Math.min(root.Window.height - 40, configPopup.height))
    }

    function clampConfigPosition() {
        if (!root.Window) {
            return
        }
        configPopup.x = Math.max(20, Math.min(root.Window.width - configPopup.width - 20, configPopup.x))
        configPopup.y = Math.max(20, Math.min(root.Window.height - configPopup.height - 20, configPopup.y))
    }

    function matchesConfigFilter(itemName, itemGroup, itemType, itemPluginMessage, itemFileDecoder) {
        var groupOk = configFilterGroup === "All Groups" || itemGroup === configFilterGroup
        if (!groupOk) {
            return false
        }
        var query = configSearchText.trim().toLowerCase()
        if (query.length === 0) {
            return true
        }
        var haystack = (itemName + " " + itemGroup + " " + itemType + " " + (itemPluginMessage || "") + " " + (itemFileDecoder || "")).toLowerCase()
        return haystack.indexOf(query) !== -1
    }

    function restoreActiveGroup() {
        if (!groupsModel || groupsModel.length === 0) {
            return
        }
        var remembered = uiSettings.activeGroup
        var index = groupsModel.indexOf(remembered)
        if (index < 0) {
            index = 0
        }
        if (hasMultipleGroups) {
            groupTabs.currentIndex = index
        }
        uiSettings.activeGroup = String(groupsModel[index])
    }

    function refreshCurrentButtons() {
        currentButtons = commandButtonModel ? commandButtonModel.buttonsForGroup(activeGroup) : []
    }

    Connections {
        target: commandButtonModel
        function onRowsInserted() { root.refreshCurrentButtons() }
        function onRowsRemoved() { root.refreshCurrentButtons() }
        function onDataChanged() { root.refreshCurrentButtons() }
        function onModelReset() { root.refreshCurrentButtons() }
        function onLayoutChanged() { root.refreshCurrentButtons() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.sectionSpacing

        RowLayout {
            Layout.fillWidth: true
            visible: root.hasMultipleGroups
            spacing: 8

            NeonTabBar {
                id: groupTabs
                Layout.fillWidth: true
                onCurrentIndexChanged: {
                    if (root.hasMultipleGroups && root.groupsModel.length > 0) {
                        uiSettings.activeGroup = String(root.groupsModel[Math.min(currentIndex, root.groupsModel.length - 1)])
                    }
                }

                Repeater {
                    model: root.groupsModel
                    NeonTabButton {
                        required property var modelData
                        text: String(modelData)
                        minButtonWidth: 70
                        accent: theme.accent
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: theme.panelRadius
            color: theme.surface1
            border.width: 1
            border.color: theme.panelBorder
            clip: true
            antialiasing: true

            Flickable {
                anchors.fill: parent
                anchors.margins: 10
                clip: true
                contentWidth: width
                contentHeight: commandGrid.implicitHeight
                visible: root.currentButtons.length > 0

                Grid {
                    id: commandGrid
                    width: parent.width
                    columns: Math.max(1, root.buttonsPerRow)
                    spacing: 8

                    Repeater {
                        model: root.currentButtons

                        NeonButton {
                            required property var modelData
                            width: Math.max(1, Math.floor((commandGrid.width - Math.max(0, commandGrid.columns - 1) * commandGrid.spacing) / Math.max(1, commandGrid.columns)))
                            height: 42
                            text: modelData.name || "Command"
                            accent: theme.accent
                            font.pixelSize: theme.bodySize
                            onClicked: {
                                if (appController && typeof appController.triggerButton === "function" && modelData.index !== undefined) {
                                    appController.triggerButton(modelData.index)
                                } else if (appController && typeof appController.sendCommand === "function") {
                                    appController.sendCommand(modelData.payload || "", modelData.contentType || "ASCII", "Default")
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 28, 420)
                visible: root.currentButtons.length === 0
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 12
                    height: 12
                    radius: theme.microRadius + 2
                    color: theme.accent
                }

                Label {
                    Layout.fillWidth: true
                    text: "No commands in this group"
                    color: theme.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    font.family: theme.titleFont
                    font.pixelSize: theme.bodySize + 3
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: "Open Config to add or import templates."
                    color: theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font.family: theme.bodyFont
                    font.pixelSize: theme.bodySize
                }
            }

            NeonButton {
                text: "Config"
                accent: theme.accentStrong
                anchors {
                    right: parent.right
                    bottom: parent.bottom
                    margins: 12
                }
                onClicked: {
                    root.ensureSelection()
                    configPopup.open()
                }
            }
        }
    }

    Popup {
        id: configPopup
        parent: Overlay.overlay
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(Math.max(860, uiSettings.configWidth), root.Window.width - 40)
        height: Math.min(Math.max(560, uiSettings.configHeight), root.Window.height - 40)
        x: Math.max(20, Math.min(root.Window.width - width - 20, uiSettings.configX))
        y: Math.max(20, Math.min(root.Window.height - height - 20, uiSettings.configY))
        padding: 0
        onOpened: {
            root.clampConfigSize()
            root.clampConfigPosition()
        }
        onClosed: root.refreshCurrentButtons()
        onWidthChanged: if (visible) uiSettings.configWidth = width
        onHeightChanged: if (visible) uiSettings.configHeight = height
        onXChanged: if (visible) uiSettings.configX = x
        onYChanged: if (visible) uiSettings.configY = y

        background: Rectangle {
            radius: theme.radius
            color: theme.surface0
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
                Layout.preferredHeight: 56
                radius: theme.panelRadius
                color: theme.surface2
                border.width: 1
                border.color: theme.panelBorder

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 1

                            Label {
                                text: "Command Button Config"
                                color: theme.textPrimary
                                font.family: theme.titleFont
                                font.pixelSize: theme.titleSize + 2
                                font.bold: true
                            }

                            Label {
                                text: "Add, edit, group, import, export, and tune all command templates here."
                                color: theme.textSecondary
                                font.pixelSize: theme.labelSize
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            cursorShape: Qt.SizeAllCursor
                            property real startMouseX: 0
                            property real startMouseY: 0
                            property real startPopupX: 0
                            property real startPopupY: 0

                            onPressed: function(mouse) {
                                startMouseX = mouse.x
                                startMouseY = mouse.y
                                startPopupX = configPopup.x
                                startPopupY = configPopup.y
                            }

                            onPositionChanged: function(mouse) {
                                if (!pressed) {
                                    return
                                }
                                configPopup.x = startPopupX + mouse.x - startMouseX
                                configPopup.y = startPopupY + mouse.y - startMouseY
                                root.clampConfigPosition()
                            }
                        }
                    }

                    NeonButton {
                        text: "Close"
                        accent: theme.bad
                        onClicked: configPopup.close()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 360
                    Layout.fillHeight: true
                    radius: theme.panelRadius
                    color: theme.surface1
                    border.width: 1
                    border.color: theme.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            width: parent.width

                            NeonButton {
                                text: "Add"
                                onClicked: {
                                    root.clearSelection()
                                    root.selectedGroup = root.activeGroup
                                }
                            }
                            NeonButton {
                                text: "Delete"
                                accent: theme.bad
                                enabled: root.selectedIndex >= 0
                                onClicked: {
                                    root.invoke(commandButtonModel, "removeButton", [String(root.selectedIndex)])
                                    root.clearSelection()
                                }
                            }
                            DenseLabel { text: "Per Row" }
                            Rectangle {
                                width: 212
                                height: theme.controlHeight
                                radius: theme.controlRadius
                                color: theme.inputBg
                                border.width: 1
                                border.color: theme.inputBorder

                                Row {
                                    anchors.fill: parent
                                    anchors.margins: 3
                                    spacing: 4

                                    Repeater {
                                        model: [1, 2, 3, 4, 5, 6]
                                        NeonTabButton {
                                            required property int modelData
                                            width: 30
                                            height: parent.height
                                            minButtonWidth: 30
                                            text: String(modelData)
                                            accent: theme.accent
                                            checked: root.buttonsPerRow === modelData
                                            onClicked: root.buttonsPerRow = modelData
                                        }
                                    }
                                }
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            width: parent.width

                            NeonButton { text: "Import"; onClicked: root.invoke(commandButtonModel, "importJson") }
                            NeonButton { text: "Export"; onClicked: root.invoke(commandButtonModel, "exportJson") }

                            NeonButton {
                                text: "From Plugin"
                                onClicked: root.invoke(appController, "importPluginTemplates")
                            }

                            Label {
                                width: Math.max(120, parent.width - x)
                                text: root.selectedIndex >= 0 ? "Selected #" + root.selectedIndex : "New command draft"
                                color: theme.textSecondary
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: theme.labelSize
                                elide: Text.ElideRight
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            DenseTextField {
                                Layout.fillWidth: true
                                text: root.configSearchText
                                placeholderText: "Search command name, group, type, or message"
                                onTextEdited: root.configSearchText = text
                            }

                            DenseComboBox {
                                Layout.preferredWidth: 150
                                model: ["All Groups"].concat(root.groupsModel)
                                currentIndex: Math.max(0, model.indexOf(root.configFilterGroup))
                                onActivated: root.configFilterGroup = currentText
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            radius: theme.controlRadius
                            color: theme.surface2
                            border.width: 1
                            border.color: theme.panelBorder

                            Label {
                                anchors.centerIn: parent
                                text: "All Commands"
                                color: theme.textPrimary
                                font.pixelSize: theme.bodySize
                                font.bold: true
                            }
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 6
                            model: commandButtonModel ? commandButtonModel : null

                            delegate: Rectangle {
                                required property int index
                                required property string name
                                required property string group
                                required property string contentType
                                required property string buildProtocol
                                required property string payload
                                required property string pluginMessage
                                required property string fileDecoder
                                required property int filePacketIntervalMs
                                readonly property bool isSelected: root.selectedIndex === index
                                readonly property bool matchesFilter: root.matchesConfigFilter(name, group, contentType, pluginMessage, fileDecoder)

                                width: ListView.view.width
                                height: matchesFilter ? 60 : 0
                            radius: theme.cardRadius
                                visible: matchesFilter
                                color: isSelected ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.20) : theme.surface2
                                border.width: 1
                                border.color: isSelected ? theme.accent : theme.panelBorderStrong

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 1

                                    Label {
                                        text: name
                                        color: theme.textPrimary
                                        font.pixelSize: theme.bodySize
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: group + "  |  "
                                              + (contentType === "Plugin"
                                                 ? "Plugin / " + ((buildProtocol || "Bream") + " / " + (pluginMessage || "-"))
                                                 : contentType + (contentType === "FILE" ? " / " + (fileDecoder || "None") + " / " + filePacketIntervalMs + "ms" : ""))
                                        color: theme.textSecondary
                                        font.pixelSize: theme.labelSize
                                        elide: Text.ElideRight
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.loadSelection({
                                        index: index,
                                        name: name,
                                        group: group,
                                        payload: payload,
                                        contentType: contentType,
                                        buildProtocol: buildProtocol,
                                        pluginMessage: pluginMessage,
                                        fileDecoder: fileDecoder,
                                        filePacketIntervalMs: filePacketIntervalMs
                                    })
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: theme.panelRadius
                    color: theme.surface1
                    border.width: 1
                    border.color: theme.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            radius: theme.controlRadius
                            color: theme.surface2
                            border.width: 1
                            border.color: theme.panelBorder

                            Label {
                                anchors.centerIn: parent
                                text: root.selectedIndex >= 0 ? "Edit Command" : "Create Command"
                                color: theme.textPrimary
                                font.pixelSize: theme.bodySize
                                font.bold: true
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 10
                            rowSpacing: 8

                            DenseLabel { text: "Name" }
                            DenseTextField {
                                Layout.fillWidth: true
                                text: root.selectedName
                                onTextEdited: root.selectedName = text
                            }

                            DenseLabel { text: "Group" }
                            DenseTextField {
                                Layout.fillWidth: true
                                text: root.selectedGroup
                                placeholderText: root.activeGroup
                                onTextEdited: root.selectedGroup = text
                            }

                            DenseLabel { text: "Type" }
                            DenseComboBox {
                                Layout.fillWidth: true
                                model: ["ASCII", "HEX", "Plugin", "FILE"]
                                currentIndex: Math.max(0, model.indexOf(root.selectedType))
                                onActivated: {
                                    var previousType = root.selectedType
                                    root.selectedType = currentText
                                    if (root.selectedType === "Plugin" && previousType !== root.selectedType) {
                                        root.applyPluginSelection(root.selectedBuildProtocol, root.defaultProtocolBuildMessageFor(root.selectedBuildProtocol))
                                    }
                                }
                            }

                            DenseLabel {
                                text: "Protocol"
                                visible: root.selectedType === "Plugin"
                            }
                            DenseComboBox {
                                Layout.fillWidth: true
                                visible: root.selectedType === "Plugin"
                                model: root.protocolBuildProtocols()
                                currentIndex: Math.max(0, model.indexOf(root.selectedBuildProtocol))
                                onActivated: {
                                    root.applyPluginSelection(currentText, root.defaultProtocolBuildMessageFor(currentText))
                                }
                            }

                            DenseLabel {
                                text: "Message"
                                visible: root.selectedType === "Plugin"
                            }
                            DenseComboBox {
                                Layout.fillWidth: true
                                visible: root.selectedType === "Plugin"
                                model: root.selectedProtocolBuildMessageModel.map(function(item) { return item.name })
                                currentIndex: Math.max(0, model.indexOf(root.selectedPluginMessage))
                                onActivated: {
                                    root.applyPluginSelection(root.selectedBuildProtocol, currentText)
                                }
                            }

                            DenseLabel {
                                text: "File Decoder"
                                visible: root.selectedType === "FILE"
                            }
                            DenseComboBox {
                                Layout.fillWidth: true
                                visible: root.selectedType === "FILE"
                                model: root.fileDecoderModel
                                currentIndex: Math.max(0, model.indexOf(root.selectedFileDecoder))
                                onActivated: root.selectedFileDecoder = currentText
                            }

                            DenseLabel {
                                text: "Packet Gap (ms)"
                                visible: root.selectedType === "FILE"
                            }
                            DenseTextField {
                                Layout.fillWidth: true
                                visible: root.selectedType === "FILE"
                                text: String(root.selectedFilePacketIntervalMs)
                                validator: IntValidator { bottom: 0 }
                                placeholderText: "0"
                                onTextEdited: root.selectedFilePacketIntervalMs = text.length > 0 ? Number(text) : 0
                            }

                        }

                        DenseLabel {
                            text: root.selectedType === "FILE"
                                ? "File Path / URL"
                                : (root.selectedType === "Plugin" ? "Parameters" : "Payload")
                        }
                        TextArea {
                            id: payloadEditor
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            wrapMode: TextEdit.WrapAnywhere
                            placeholderText: root.selectedType === "FILE"
                                ? "/path/to/data.bin or https://example.com/data.bin"
                                : (root.selectedType === "Plugin"
                                    ? (root.protocolBuildMessageDefinition(root.selectedBuildProtocol, root.selectedPluginMessage).defaultPayload || "")
                                    : "")
                            color: theme.textPrimary
                            font.family: theme.monoFont
                            font.pixelSize: theme.monoSize
                            selectByMouse: true
                            onTextChanged: {
                                if (activeFocus) {
                                    root.selectedPayload = text
                                }
                            }
                            background: Rectangle {
                                radius: theme.controlRadius
                                color: theme.inputBg
                                border.width: 1
                                border.color: parent.activeFocus ? theme.inputBorderFocus : theme.inputBorder
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.selectedType === "FILE"
                            text: "When a file decoder is selected, the file is sent as decoder-specific protocol frames. Select None to send 1024-byte packets. Packet Gap waits this many milliseconds after each packet is sent; 0 resumes on the next event-loop tick without blocking the UI or receive path."
                            color: theme.textSecondary
                            wrapMode: Text.WordWrap
                            font.pixelSize: theme.labelSize
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.selectedType === "Plugin"
                            text: root.protocolBuildMessageDefinition(root.selectedBuildProtocol, root.selectedPluginMessage).description || "No plugin-provided description."
                            color: theme.textSecondary
                            wrapMode: Text.WordWrap
                            font.pixelSize: theme.labelSize
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            NeonButton {
                                text: root.selectedIndex >= 0 ? "Update" : "Create"
                                accent: theme.ok
                                onClicked: {
                                    if (root.selectedIndex >= 0) {
                                        root.invoke(commandButtonModel, "updateButton", [root.selectedIndex, root.currentDefinition()])
                                    } else {
                                        root.invoke(commandButtonModel, "addButton", [root.currentDefinition()])
                                    }
                                }
                            }

                            NeonButton {
                                text: "Reset"
                                onClicked: root.clearSelection()
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: "Config changes apply immediately to the live command list."
                                color: theme.textSecondary
                                font.pixelSize: theme.labelSize
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            width: 22
            height: 22
            radius: theme.pillRadius
            color: theme.surface3
            border.width: 1
            border.color: theme.panelBorderStrong
            anchors {
                right: parent.right
                bottom: parent.bottom
                margins: 10
            }

            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.strokeStyle = theme.textSecondary
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.moveTo(width * 0.35, height * 0.72)
                    ctx.lineTo(width * 0.72, height * 0.35)
                    ctx.moveTo(width * 0.50, height * 0.82)
                    ctx.lineTo(width * 0.82, height * 0.50)
                    ctx.stroke()
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeFDiagCursor
                property real startMouseX: 0
                property real startMouseY: 0
                property real startWidth: 0
                property real startHeight: 0

                onPressed: function(mouse) {
                    startMouseX = mouse.x
                    startMouseY = mouse.y
                    startWidth = configPopup.width
                    startHeight = configPopup.height
                }

                onPositionChanged: function(mouse) {
                    if (!pressed) {
                        return
                    }
                    configPopup.width = Math.max(860, Math.min(root.Window.width - 40, startWidth + mouse.x - startMouseX))
                    configPopup.height = Math.max(560, Math.min(root.Window.height - 40, startHeight + mouse.y - startMouseY))
                }
            }
        }
    }

    onSelectedPayloadChanged: syncPayloadEditor()
    onActiveGroupChanged: refreshCurrentButtons()
    onButtonsPerRowChanged: uiSettings.buttonsPerRow = buttonsPerRow
    onDefaultProtocolBuildMessageChanged: {
        if (selectedType === "Plugin"
            && (!selectedPluginMessage || selectedProtocolBuildMessageModel.map(function(item) { return item.name }).indexOf(selectedPluginMessage) < 0)) {
            applyPluginSelection(selectedBuildProtocol, defaultProtocolBuildMessage)
        }
    }
    onGroupsModelChanged: {
        restoreActiveGroup()
        refreshCurrentButtons()
    }
    Component.onCompleted: {
        buttonsPerRow = Math.max(1, Math.min(6, uiSettings.buttonsPerRow))
        restoreActiveGroup()
        refreshCurrentButtons()
        syncPayloadEditor()
    }
}
