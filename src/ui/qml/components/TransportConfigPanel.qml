import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

GlassPanel {
    id: root
    title: "Transport"
    accent: theme.accent

    Theme { id: theme }

    property bool settingsReady: false
    property bool syncingTabs: false
    property int statusTick: 0
    property var transportList: transportViewModel ? transportViewModel.availableTransports : []
    property string lockedTransport: transportViewModel ? transportViewModel.lockedTransport : ""

    Settings {
        id: transportSettings
        category: "TransportConfigPanel"
        property string currentTransport: "UART"
        property string uartPort: ""
        property string uartBaud: "9600"
        property string uartDataBits: "8"
        property string uartStopBits: "1"
        property string uartParity: "None"
        property string uartFlow: "None"
        property string tcpHost: "127.0.0.1"
        property string tcpPort: "2101"
        property string udpAddr: "0.0.0.0"
        property string udpPort: "5000"
        property bool autoReconnect: false
        property string reconnectTransport: ""
    }

    component DenseLabel: Label {
        color: theme.textSecondary
        font.family: theme.bodyFont
        font.pixelSize: theme.labelSize
        verticalAlignment: Text.AlignVCenter
    }

    readonly property int compactLabelWidth: 58

    component SectionTitle: Label {
        color: theme.textPrimary
        font.family: theme.titleFont
        font.pixelSize: theme.bodySize + 1
        font.bold: true
    }

    component HelpText: Label {
        color: theme.textSecondary
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        wrapMode: Text.WordWrap
    }

    component DenseTextField: TextField {
        implicitHeight: theme.controlHeight
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        color: theme.textPrimary
        selectByMouse: true
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
        displayText: {
            if (editable && editText.length > 0) {
                return editText
            }
            return root.comboDisplayValue(combo)
        }
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
                text: {
                    var item = modelData
                    if (item && item.label !== undefined) {
                        return String(item.label)
                    }
                    return String(modelData)
                }
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

    function invoke(methodName, args) {
        if (!transportViewModel) {
            return
        }
        var fn = transportViewModel[methodName]
        if (typeof fn === "function") {
            return fn.apply(transportViewModel, args || [])
        }
        return undefined
    }

    function statusText(value, fallbackText) {
        return value && value.length > 0 ? value : fallbackText
    }

    function transportDescriptorByName(name) {
        if (!transportList) {
            return null
        }
        for (var i = 0; i < transportList.length; ++i) {
            var item = transportList[i]
            if (item && item.transportName === name) {
                return item
            }
        }
        return null
    }

    function currentTransportDescriptor() {
        return transportDescriptorByName(transportSettings.currentTransport)
    }

    function isTransportLockedOut(name) {
        return lockedTransport.length > 0 && lockedTransport !== name
    }

    function isTransportOpenOrLocked(name) {
        if (lockedTransport === name) {
            return true
        }
        return transportViewModel ? transportViewModel.transportOpen(name) : false
    }

    function pluginTransports() {
        var plugins = []
        var seen = ({})
        if (!transportList) {
            return plugins
        }
        for (var i = 0; i < transportList.length; ++i) {
            var transport = transportList[i]
            if (transport.category === "plugin" && !seen[transport.transportName]) {
                seen[transport.transportName] = true
                plugins.push(transport)
            }
        }
        return plugins
    }

    function currentTransportIndex() {
        if (!transportList || transportList.length === 0) {
            return -1
        }
        for (var i = 0; i < transportList.length; ++i) {
            if (transportList[i].transportName === transportSettings.currentTransport) {
                return i
            }
        }
        return 0
    }

    function ensureCurrentTransport() {
        if (!transportList || transportList.length === 0) {
            return
        }
        if (lockedTransport.length > 0 && transportDescriptorByName(lockedTransport)) {
            transportSettings.currentTransport = lockedTransport
            if (transportViewModel) {
                transportViewModel.activeTransport = lockedTransport
            }
            return
        }
        if (!transportDescriptorByName(transportSettings.currentTransport)) {
            transportSettings.currentTransport = transportList[0].transportName
        }
        if (transportViewModel) {
            transportViewModel.activeTransport = transportSettings.currentTransport
        }
    }

    function applyComboValue(combo, value) {
        if (!combo || value === undefined || value === null) {
            return
        }
        var stringValue = String(value)
        if (combo.editable) {
            var editableIndex = combo.find(stringValue)
            combo.currentIndex = editableIndex >= 0 ? editableIndex : -1
            combo.editText = stringValue
            return
        }
        var index = combo.find(stringValue)
        combo.currentIndex = index >= 0 ? index : 0
    }

    function comboDisplayValue(combo) {
        if (!combo || combo.currentIndex < 0 || !combo.model || combo.model.length === undefined || combo.model.length <= combo.currentIndex) {
            return ""
        }
        var item = combo.model[combo.currentIndex]
        if (item && item.label !== undefined) {
            return String(item.label)
        }
        return String(item)
    }

    function comboStoredValue(combo) {
        if (!combo) {
            return ""
        }
        if (combo.editable) {
            return combo.editText
        }
        if (combo.currentIndex < 0 || !combo.model || combo.model.length === undefined || combo.model.length <= combo.currentIndex) {
            return ""
        }
        var item = combo.model[combo.currentIndex]
        if (item && item.value !== undefined) {
            return String(item.value)
        }
        if (item && item.label !== undefined) {
            return String(item.label)
        }
        return String(item)
    }

    function pluginOptionModel(fieldData) {
        var source = fieldData && (fieldData.options || fieldData.choices || fieldData.values)
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

    function pluginFieldValue(fieldData) {
        if (!fieldData) {
            return ""
        }
        if (fieldData.value === undefined || fieldData.value === null) {
            return ""
        }
        return fieldData.value
    }

    function updateCurrentTransport(name) {
        if (isTransportLockedOut(name)) {
            return
        }
        transportSettings.currentTransport = name
        if (transportViewModel) {
            transportViewModel.activeTransport = name
        }
        persistSettings()
    }

    function preserveTransportSelection(name, action) {
        var target = name && name.length > 0 ? name : transportSettings.currentTransport
        root.syncingTabs = true
        if (typeof action === "function") {
            action()
        }
        Qt.callLater(function() {
            if (target && root.transportDescriptorByName(target)) {
                transportSettings.currentTransport = target
                if (transportViewModel) {
                    transportViewModel.activeTransport = target
                }
                var index = root.currentTransportIndex()
                if (index >= 0) {
                    tabs.currentIndex = index
                }
            }
            root.syncingTabs = false
        })
    }

    function restoreSettings() {
        applyComboValue(uartPort, transportSettings.uartPort)
        applyComboValue(uartBaud, transportSettings.uartBaud)
        applyComboValue(dataBits, transportSettings.uartDataBits)
        applyComboValue(stopBits, transportSettings.uartStopBits)
        applyComboValue(parity, transportSettings.uartParity)
        applyComboValue(flowControl, transportSettings.uartFlow)
        tcpHost.text = transportSettings.tcpHost
        tcpPort.text = transportSettings.tcpPort
        udpAddr.text = transportSettings.udpAddr
        udpPort.text = transportSettings.udpPort
        ensureCurrentTransport()
        settingsReady = true
    }

    function persistSettings() {
        if (!settingsReady) {
            return
        }
        transportSettings.uartPort = root.comboStoredValue(uartPort)
        transportSettings.uartBaud = root.comboStoredValue(uartBaud)
        transportSettings.uartDataBits = root.comboStoredValue(dataBits)
        transportSettings.uartStopBits = root.comboStoredValue(stopBits)
        transportSettings.uartParity = root.comboStoredValue(parity)
        transportSettings.uartFlow = root.comboStoredValue(flowControl)
        transportSettings.tcpHost = tcpHost.text
        transportSettings.tcpPort = tcpPort.text
        transportSettings.udpAddr = udpAddr.text
        transportSettings.udpPort = udpPort.text
    }

    function rememberOpenTransport(name) {
        if (!settingsReady) {
            return
        }
        transportSettings.reconnectTransport = name
    }

    function clearReconnectIfMatch(name) {
        if (!settingsReady) {
            return
        }
        if (transportSettings.reconnectTransport === name) {
            transportSettings.reconnectTransport = ""
        }
    }

    function openUartAction() {
        persistSettings()
        if (transportViewModel && transportViewModel.openUart(root.comboStoredValue(uartPort), Number(root.comboStoredValue(uartBaud)), Number(root.comboStoredValue(dataBits)), Number(root.comboStoredValue(stopBits)), root.comboStoredValue(parity), root.comboStoredValue(flowControl))) {
            rememberOpenTransport("UART")
        }
    }

    function closeUartAction() {
        invoke("closeUart")
        clearReconnectIfMatch("UART")
    }

    function openTcpAction() {
        persistSettings()
        if (transportViewModel && transportViewModel.openTcp(tcpHost.text, Number(tcpPort.text))) {
            rememberOpenTransport("TCP")
        }
    }

    function closeTcpAction() {
        invoke("closeTcp")
        clearReconnectIfMatch("TCP")
    }

    function openUdpAction() {
        persistSettings()
        if (transportViewModel && transportViewModel.openUdpServer(udpAddr.text, Number(udpPort.text))) {
            rememberOpenTransport("UDP")
        }
    }

    function closeUdpAction() {
        invoke("closeUdpServer")
        clearReconnectIfMatch("UDP")
    }

    function openPluginTransport(name) {
        if (!transportViewModel) {
            return
        }
        var settingsMap = transportViewModel.transportStoredSettings(name)
        if (transportViewModel.openTransport(name, settingsMap || {})) {
            rememberOpenTransport(name)
        }
    }

    function closePluginTransport(name) {
        invoke("closeTransport", [name])
        clearReconnectIfMatch(name)
    }

    function attemptAutoReconnect() {
        if (!settingsReady || !transportSettings.autoReconnect || !transportSettings.reconnectTransport) {
            return
        }
        var target = transportSettings.reconnectTransport
        if (target === "UART") {
            openUartAction()
            return
        }
        if (target === "TCP") {
            openTcpAction()
            return
        }
        if (target === "UDP") {
            openUdpAction()
            return
        }
        if (transportDescriptorByName(target)) {
            openPluginTransport(target)
        }
    }

    function pluginStatusFallback(descriptor) {
        if (!descriptor) {
            return "Transport idle"
        }
        return descriptor.supportsDiscovery ? "Plugin ready for probe or open" : "Plugin ready"
    }

    Connections {
        target: transportViewModel
        function onSerialPortsChanged() {
            if (root.settingsReady && transportSettings.uartPort.length > 0) {
                root.applyComboValue(uartPort, transportSettings.uartPort)
            }
        }
        function onAvailableTransportsChanged() {
            root.ensureCurrentTransport()
            var index = root.currentTransportIndex()
            if (index >= 0 && tabs.currentIndex !== index) {
                root.syncingTabs = true
                tabs.currentIndex = index
                root.syncingTabs = false
            }
        }
        function onStatusesChanged() {
            root.statusTick += 1
        }
        function onLockedTransportChanged() {
            root.ensureCurrentTransport()
            var index = root.currentTransportIndex()
            if (index >= 0 && tabs.currentIndex !== index) {
                root.syncingTabs = true
                tabs.currentIndex = index
                root.syncingTabs = false
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.sectionSpacing

        NeonTabBar {
            id: tabs
            Layout.fillWidth: true
            onCurrentIndexChanged: {
                if (root.syncingTabs) {
                    return
                }
                if (currentIndex >= 0 && root.transportList && currentIndex < root.transportList.length) {
                    var nextTransport = root.transportList[currentIndex].transportName
                    if (root.isTransportLockedOut(nextTransport)) {
                        root.ensureCurrentTransport()
                        tabs.currentIndex = root.currentTransportIndex()
                        return
                    }
                    root.updateCurrentTransport(nextTransport)
                }
            }

            NeonTabButton {
                text: "UART"
                minButtonWidth: 60
                enabled: !root.isTransportLockedOut("UART")
            }
            NeonTabButton {
                text: "TCP"
                minButtonWidth: 60
                enabled: !root.isTransportLockedOut("TCP")
            }
            NeonTabButton {
                text: "UDP"
                minButtonWidth: 60
                enabled: !root.isTransportLockedOut("UDP")
            }

            Repeater {
                model: root.pluginTransports()
                delegate: NeonTabButton {
                    required property var modelData
                    text: modelData.displayName || modelData.transportName
                    minButtonWidth: 60
                    enabled: !root.isTransportLockedOut(modelData.transportName)
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: theme.sectionSpacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        SectionTitle { text: "UART Link" }
                        Item { Layout.fillWidth: true }
                        NeonButton { text: "Refresh"; onClicked: root.invoke("refreshUartPorts") }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        rowSpacing: 8

                        DenseLabel { text: "Port" }
                        DenseComboBox {
                            id: uartPort
                            Layout.fillWidth: true
                            model: transportViewModel && transportViewModel.uartPorts ? transportViewModel.uartPorts : []
                            editable: true
                            onEditTextChanged: root.persistSettings()
                            onActivated: root.persistSettings()
                            onModelChanged: {
                                if (root.settingsReady && transportSettings.uartPort.length > 0) {
                                    root.applyComboValue(uartPort, transportSettings.uartPort)
                                }
                            }
                        }

                        DenseLabel { text: "Baud" }
                        DenseComboBox {
                            id: uartBaud
                            Layout.fillWidth: true
                            model: ["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"]
                            onActivated: root.persistSettings()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            DenseLabel {
                                text: "Data Bits"
                                Layout.preferredWidth: root.compactLabelWidth
                                horizontalAlignment: Text.AlignRight
                            }
                            DenseComboBox {
                                id: dataBits
                                Layout.fillWidth: true
                                Layout.preferredWidth: 120
                                model: ["8", "7"]
                                onActivated: root.persistSettings()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            DenseLabel {
                                text: "Stop Bits"
                                Layout.preferredWidth: root.compactLabelWidth
                                horizontalAlignment: Text.AlignRight
                            }
                            DenseComboBox {
                                id: stopBits
                                Layout.fillWidth: true
                                Layout.preferredWidth: 120
                                model: ["1", "2"]
                                onActivated: root.persistSettings()
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            DenseLabel {
                                text: "Parity"
                                Layout.preferredWidth: root.compactLabelWidth
                                horizontalAlignment: Text.AlignRight
                            }
                            DenseComboBox {
                                id: parity
                                Layout.fillWidth: true
                                Layout.preferredWidth: 120
                                model: ["None", "Even", "Odd"]
                                onActivated: root.persistSettings()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            DenseLabel {
                                text: "Flow"
                                Layout.preferredWidth: root.compactLabelWidth
                                horizontalAlignment: Text.AlignRight
                            }
                            DenseComboBox {
                                id: flowControl
                                Layout.fillWidth: true
                                Layout.preferredWidth: 120
                                model: ["None", "RTS/CTS", "XON/XOFF"]
                                onActivated: root.persistSettings()
                            }
                        }
                    }

                    HelpText {
                        Layout.fillWidth: true
                        text: root.statusText(transportViewModel ? transportViewModel.uartStatus : "", "UART idle")
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Item { Layout.fillWidth: true }
                        NeonButton {
                            text: "Connect"
                            accent: theme.ok
                            enabled: !root.isTransportOpenOrLocked("UART")
                            onClicked: root.openUartAction()
                        }
                        NeonButton {
                            text: "Disconnect"
                            accent: theme.bad
                            enabled: root.isTransportOpenOrLocked("UART")
                            onClicked: root.closeUartAction()
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: theme.sectionSpacing

                    SectionTitle { text: "TCP Client" }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        rowSpacing: 8

                        DenseLabel { text: "Host" }
                        DenseTextField {
                            id: tcpHost
                            Layout.fillWidth: true
                            text: "127.0.0.1"
                            placeholderText: "Host or IP"
                            onTextEdited: root.persistSettings()
                        }
                        DenseLabel { text: "Port" }
                        DenseTextField {
                            id: tcpPort
                            Layout.fillWidth: true
                            text: "2101"
                            validator: IntValidator { bottom: 1; top: 65535 }
                            onTextEdited: root.persistSettings()
                        }
                    }

                    HelpText {
                        Layout.fillWidth: true
                        text: root.statusText(transportViewModel ? transportViewModel.tcpStatus : "", "TCP disconnected")
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Item { Layout.fillWidth: true }
                        NeonButton {
                            text: "Connect"
                            accent: theme.ok
                            enabled: !root.isTransportOpenOrLocked("TCP")
                            onClicked: root.openTcpAction()
                        }
                        NeonButton {
                            text: "Disconnect"
                            accent: theme.bad
                            enabled: root.isTransportOpenOrLocked("TCP")
                            onClicked: root.closeTcpAction()
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: theme.sectionSpacing

                    SectionTitle { text: "UDP Server" }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        rowSpacing: 8

                        DenseLabel { text: "Bind Address" }
                        DenseTextField {
                            id: udpAddr
                            Layout.fillWidth: true
                            text: "0.0.0.0"
                            placeholderText: "0.0.0.0"
                            onTextEdited: root.persistSettings()
                        }
                        DenseLabel { text: "Port" }
                        DenseTextField {
                            id: udpPort
                            Layout.fillWidth: true
                            text: "5000"
                            validator: IntValidator { bottom: 1; top: 65535 }
                            onTextEdited: root.persistSettings()
                        }
                    }

                    HelpText {
                        Layout.fillWidth: true
                        text: root.statusText(transportViewModel ? transportViewModel.udpStatus : "", "UDP stopped")
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Item { Layout.fillWidth: true }
                        NeonButton {
                            text: "Start"
                            accent: theme.ok
                            enabled: !root.isTransportOpenOrLocked("UDP")
                            onClicked: root.openUdpAction()
                        }
                        NeonButton {
                            text: "Stop"
                            accent: theme.bad
                            enabled: root.isTransportOpenOrLocked("UDP")
                            onClicked: root.closeUdpAction()
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Repeater {
                model: root.pluginTransports()

                delegate: Item {
                    required property var modelData
                    property var descriptorData: modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Connections {
                        target: transportViewModel
                        function onTransportDescriptorChanged(transportName) {
                            if (transportName === modelData.transportName && transportViewModel) {
                                descriptorData = transportViewModel.transportDescriptor(transportName)
                            }
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: theme.sectionSpacing

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            SectionTitle {
                                text: descriptorData.displayName || descriptorData.transportName
                            }
                            Item { Layout.fillWidth: true }
                            NeonButton {
                                visible: !!descriptorData.supportsDiscovery
                                text: "Probe"
                                onClicked: root.preserveTransportSelection(descriptorData.transportName, function() {
                                    root.invoke("probeTransport", [descriptorData.transportName])
                                })
                            }
                        }

                        HelpText {
                            visible: descriptorData.hardwareInterfaces && descriptorData.hardwareInterfaces.length > 0
                            Layout.fillWidth: true
                            text: "Hardware Interfaces: " + descriptorData.hardwareInterfaces.join(", ")
                        }

                        CustomPluginUi {
                            Layout.fillWidth: true
                            visible: root.hasCustomPluginUi(descriptorData)
                            descriptor: descriptorData
                            settingsMap: transportViewModel ? transportViewModel.transportStoredSettings(descriptorData.transportName) : ({})
                            style: root.pluginUiStyle()
                        }

                        Repeater {
                            model: root.hasCustomPluginUi(descriptorData) ? [] : (descriptorData.fields || [])

                            delegate: ColumnLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: 6

                                DenseLabel {
                                    text: parent.modelData.label || parent.modelData.key
                                }

                                Loader {
                                    Layout.fillWidth: true
                                    property var fieldData: parent.modelData
                                    sourceComponent: {
                                        var fieldType = String(fieldData.type || "text").toLowerCase()
                                        if (fieldType === "select" || fieldType === "enum" || fieldType === "combo") {
                                            return comboFieldComponent
                                        }
                                        if (fieldType === "bool" || fieldType === "boolean" || fieldType === "checkbox") {
                                            return boolFieldComponent
                                        }
                                        return textFieldComponent
                                    }
                                }

                                HelpText {
                                    visible: !!(parent.modelData.description && parent.modelData.description.length > 0)
                                    Layout.fillWidth: true
                                    text: parent.modelData.description || ""
                                }
                            }
                        }

                        HelpText {
                            visible: !root.hasCustomPluginUi(descriptorData) && (!descriptorData.fields || descriptorData.fields.length === 0)
                            Layout.fillWidth: true
                            text: "This transport plugin currently has no extra configuration fields."
                        }

                        HelpText {
                            Layout.fillWidth: true
                            text: root.statusTick >= 0 ? root.statusText(
                                transportViewModel ? transportViewModel.transportStatus(descriptorData.transportName) : "",
                                root.pluginStatusFallback(descriptorData)) : ""
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Item { Layout.fillWidth: true }
                            NeonButton {
                                text: "Open"
                                accent: theme.ok
                                enabled: !root.isTransportOpenOrLocked(descriptorData.transportName)
                                onClicked: root.preserveTransportSelection(descriptorData.transportName, function() {
                                    root.openPluginTransport(descriptorData.transportName)
                                })
                            }
                            NeonButton {
                                text: "Close"
                                accent: theme.bad
                                enabled: root.isTransportOpenOrLocked(descriptorData.transportName)
                                onClicked: root.preserveTransportSelection(descriptorData.transportName, function() {
                                    root.closePluginTransport(descriptorData.transportName)
                                })
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }
                }
            }
        }
    }

    function hasCustomPluginUi(descriptor) {
        return !!(descriptor
                  && descriptor.customUi
                  && descriptor.customUi.qmlSource
                  && descriptor.customUi.qmlSource.length > 0)
    }

    function pluginUiStyle() {
        return {
            bodyFont: theme.bodyFont,
            titleFont: theme.titleFont,
            bodySize: theme.bodySize,
            labelSize: theme.labelSize,
            controlHeight: theme.controlHeight,
            controlRadius: theme.controlRadius,
            textPrimary: theme.textPrimary,
            textSecondary: theme.textSecondary,
            inputBg: theme.inputBg,
            inputBorder: theme.inputBorder,
            inputBorderFocus: theme.inputBorderFocus,
            surface3: theme.surface3
        }
    }

    component CustomPluginUi: Item {
        id: customHost
        property var descriptor
        property var settingsMap: ({})
        property var style: ({})
        property var contentItem: null
        property string loadError: ""
        property string loadedSource: ""

        implicitHeight: Math.max(errorText.visible ? errorText.implicitHeight : 0,
                                 contentItem ? Math.max(contentItem.implicitHeight, contentItem.height) : 0)

        function destroyContent() {
            if (contentItem) {
                contentItem.destroy()
                contentItem = null
            }
        }

        function reload() {
            loadError = ""
            if (!root.hasCustomPluginUi(descriptor)) {
                destroyContent()
                loadedSource = ""
                return
            }
            var source = descriptor.customUi.qmlSource
            if (contentItem && loadedSource === source) {
                if ("pluginId" in contentItem) {
                    contentItem.pluginId = descriptor.pluginId || ""
                }
                if ("settings" in contentItem) {
                    contentItem.settings = settingsMap || {}
                }
                if ("fields" in contentItem) {
                    contentItem.fields = descriptor.fields || []
                }
                if ("style" in contentItem) {
                    contentItem.style = customHost.style || {}
                }
                return
            }
            destroyContent()
            try {
                var item = Qt.createQmlObject(source, customHost, descriptor.transportName + "SettingsUi")
                loadedSource = source
                contentItem = item
                item.width = Qt.binding(function() { return customHost.width })
                if ("pluginId" in item) {
                    item.pluginId = descriptor.pluginId || ""
                }
                if ("settings" in item) {
                    item.settings = settingsMap || {}
                }
                if ("fields" in item) {
                    item.fields = descriptor.fields || []
                }
                if ("style" in item) {
                    item.style = customHost.style || {}
                }
                if (item.settingChanged) {
                    item.settingChanged.connect(function(key, value) {
                        if (appSettings && descriptor && descriptor.pluginId) {
                            root.preserveTransportSelection(descriptor.transportName, function() {
                                appSettings.setPluginSettingValue(descriptor.pluginId, key, value)
                            })
                        }
                    })
                }
            } catch (error) {
                loadedSource = ""
                loadError = String(error)
            }
        }

        onDescriptorChanged: reload()
        onSettingsMapChanged: {
            if (contentItem && "settings" in contentItem) {
                contentItem.settings = settingsMap || {}
            }
        }
        onStyleChanged: {
            if (contentItem && "style" in contentItem) {
                contentItem.style = style || {}
            }
        }
        Component.onCompleted: reload()
        Component.onDestruction: destroyContent()

        HelpText {
            id: errorText
            anchors.left: parent.left
            anchors.right: parent.right
            visible: customHost.loadError.length > 0
            text: customHost.loadError
        }
    }

    Component {
        id: textFieldComponent

        Item {
            property var fieldData: parent && parent.fieldData !== undefined ? parent.fieldData : null
            implicitWidth: fieldEditor.implicitWidth
            implicitHeight: fieldEditor.implicitHeight

            DenseTextField {
                id: fieldEditor
                anchors.left: parent.left
                anchors.right: parent.right
                text: root.pluginFieldValue(parent.fieldData)
                placeholderText: parent.fieldData ? (parent.fieldData.placeholder || "") : ""
                echoMode: String(parent.fieldData ? (parent.fieldData.type || "") : "").toLowerCase() === "password"
                    ? TextInput.Password
                    : TextInput.Normal
                validator: {
                    var fieldType = String(parent.fieldData ? (parent.fieldData.type || "") : "").toLowerCase()
                    if (fieldType === "number" || fieldType === "int" || fieldType === "integer") {
                        return intValidatorComponent.createObject(this)
                    }
                    return null
                }
                onEditingFinished: {
                    if (appSettings && parent.fieldData) {
                        appSettings.setPluginSettingValue(currentTransportDescriptor().pluginId, parent.fieldData.key, text)
                    }
                }
            }
        }
    }

    Component {
        id: comboFieldComponent

        Item {
            property var fieldData: parent && parent.fieldData !== undefined ? parent.fieldData : null
            implicitWidth: fieldEditor.implicitWidth
            implicitHeight: fieldEditor.implicitHeight

            DenseComboBox {
                id: fieldEditor
                anchors.left: parent.left
                anchors.right: parent.right
                model: root.pluginOptionModel(parent.fieldData)
                Component.onCompleted: {
                    var value = root.pluginFieldValue(parent.fieldData)
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
                        appSettings.setPluginSettingValue(currentTransportDescriptor().pluginId, parent.fieldData.key, model[currentIndex].value)
                    }
                }
            }
        }
    }

    Component {
        id: boolFieldComponent

        Item {
            property var fieldData: parent && parent.fieldData !== undefined ? parent.fieldData : null
            implicitWidth: fieldEditor.implicitWidth
            implicitHeight: fieldEditor.implicitHeight

            CheckBox {
                id: fieldEditor
                anchors.left: parent.left
                anchors.right: parent.right
                text: parent.fieldData ? (parent.fieldData.checkboxLabel || "Enabled") : "Enabled"
                checked: {
                    var value = root.pluginFieldValue(parent.fieldData)
                    return value === true || String(value).toLowerCase() === "true" || String(value) === "1"
                }
                onToggled: {
                    if (appSettings && parent.fieldData) {
                        appSettings.setPluginSettingValue(currentTransportDescriptor().pluginId, parent.fieldData.key, checked)
                    }
                }
            }
        }
    }

    Component {
        id: intValidatorComponent
        IntValidator {}
    }

    Component.onCompleted: {
        restoreSettings()
        tabs.currentIndex = root.currentTransportIndex()
        Qt.callLater(root.attemptAutoReconnect)
    }
}
