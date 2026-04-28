import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

GlassPanel {
    id: root
    property int initialTab: 0
    property int selectedRawIndex: -1
    property var pluginInfoTabs: appController ? appController.protocolInfoTabs : []
    property string selectedTabKey: ""
    property var tabEntries: {
        var entries = [
            { kind: "raw", tabTitle: "RawData" },
            { kind: "location", tabTitle: "Location" }
        ]
        for (var i = 0; i < pluginInfoTabs.length; ++i) {
            var panel = pluginInfoTabs[i]
            entries.push({
                kind: "plugin",
                tabTitle: panel.tabTitle || ("Plugin " + (i + 1)),
                panelId: panel.id
            })
        }
        return entries
    }
    title: "Data"
    accent: theme.accentStrong

    Theme {
        id: theme
    }

    Settings {
        id: dataSettings
        category: "DataPanel"
        property int currentTab: 0
    }

    function transportShortName(value) {
        var text = String(value || "").trim().toUpperCase()
        if (text.length === 0) {
            return "-"
        }
        if (text.indexOf("UART") === 0) return "U"
        if (text.indexOf("TCP") === 0) return "T"
        if (text.indexOf("UDP") === 0) return "D"
        return text.charAt(0)
    }

    function rawEntryText(row) {
        if (!rawLogModel || row < 0) {
            return ""
        }
        var entry = rawLogModel.get(row)
        if (!entry || !entry.kind) {
            return ""
        }
        return [
            entry.timestamp || "",
            entry.direction || "",
            transportShortName(entry.transport || ""),
            entry.kind || "",
            entry.message || "-",
            rawEntryContent(row)
        ].join(" ").trim()
    }

    function rawEntryContent(row) {
        if (!rawLogModel || row < 0) {
            return ""
        }
        var entry = rawLogModel.get(row)
        if (!entry || !entry.kind) {
            return ""
        }
        var kind = String(entry.kind || "").toUpperCase()
        return kind === "BIN" ? (entry.hex || "") : (entry.display || "")
    }

    function copySelectedRawContent() {
        if (!appController || selectedRawIndex < 0) {
            return
        }
        var text = rawEntryContent(selectedRawIndex)
        if (text.length > 0) {
            appController.copyText(text)
        }
    }

    function copySelectedRawLine() {
        if (!appController || selectedRawIndex < 0) {
            return
        }
        var text = rawEntryText(selectedRawIndex)
        if (text.length > 0) {
            appController.copyText(text)
        }
    }

    function clampTabIndex(index) {
        return Math.max(0, Math.min(tabEntries.length - 1, index))
    }

    function tabKey(entry) {
        if (!entry) {
            return ""
        }
        if (entry.kind === "plugin" && entry.panelId) {
            return "plugin:" + entry.panelId
        }
        return entry.kind || ""
    }

    function updateSelectedTabKey() {
        var entry = tabEntries[Math.max(0, Math.min(tabs.currentIndex, tabEntries.length - 1))]
        selectedTabKey = tabKey(entry)
    }

    function restoreTabSelection() {
        var preferred = selectedTabKey
        if ((!preferred || preferred.length === 0) && dataSettings.currentTab !== undefined) {
            var savedEntry = tabEntries[clampTabIndex(dataSettings.currentTab)]
            preferred = tabKey(savedEntry)
        }
        if ((!preferred || preferred.length === 0) && initialTab >= 0) {
            preferred = tabKey(tabEntries[clampTabIndex(initialTab)])
        }

        for (var i = 0; i < tabEntries.length; ++i) {
            if (tabKey(tabEntries[i]) === preferred) {
                tabs.currentIndex = i
                updateSelectedTabKey()
                return
            }
        }

        tabs.currentIndex = clampTabIndex(tabs.currentIndex)
        updateSelectedTabKey()
    }

    function scheduleRestoreTabSelection() {
        Qt.callLater(function() {
            restoreTabSelection()
        })
    }

    component StatLabel: Label {
        color: theme.textSecondary
        font.family: theme.bodyFont
        font.pixelSize: theme.labelSize
    }

    component StatValue: Label {
        color: theme.textPrimary
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize + 2
        font.bold: true
        elide: Text.ElideRight
    }

    component ContentFrame: Rectangle {
        radius: theme.controlRadius
        color: theme.surface0
        border.width: 1
        border.color: theme.panelBorderStrong
        clip: true
        antialiasing: true
    }

    component MetricCard: Rectangle {
        property string heading: ""
        property string valueText: "--"
        property string subText: ""
        property color accentColor: theme.accentStrong
        property bool emphasize: false
        property bool compact: false

        radius: theme.cardRadius
        color: theme.surface1
        border.width: 1
        border.color: theme.panelBorder
        implicitHeight: compact ? (emphasize ? 54 : 46) : (emphasize ? 64 : 56)

        Rectangle {
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: 4
            radius: theme.tinyRadius
            color: accentColor
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: compact ? 8 : 10
            spacing: compact ? 0 : 1

            Label {
                text: heading
                color: theme.textSecondary
                font.family: theme.bodyFont
                font.pixelSize: compact ? Math.max(10, theme.labelSize - 1) : theme.labelSize
            }

            Label {
                text: valueText
                color: theme.textPrimary
                font.family: theme.bodyFont
                font.pixelSize: compact
                    ? (emphasize ? theme.bodySize + 2 : theme.bodySize)
                    : (emphasize ? theme.bodySize + 4 : theme.bodySize + 2)
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                visible: subText.length > 0
                text: subText
                color: theme.caption
                font.family: theme.bodyFont
                font.pixelSize: compact ? Math.max(10, theme.labelSize - 1) : theme.labelSize
                elide: Text.ElideRight
            }
        }
    }

    component ProtocolInfoPage: Item {
        property string panelId: ""
        property var panelData: ({})
        property var displayItems: {
            var source = panelData && panelData.items ? panelData.items : []
            return source.slice(0, 25)
        }
        readonly property int cardMinWidth: 118
        readonly property int maxColumns: 5
        readonly property int gridColumns: Math.max(1, Math.min(maxColumns, Math.floor(Math.max(1, width - 20) / cardMinWidth)))

        ContentFrame {
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop
                    columns: gridColumns
                    columnSpacing: 6
                    rowSpacing: 6

                    Repeater {
                        model: displayItems

                        delegate: MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: modelData.label || "--"
                            valueText: modelData.valueText || "--"
                            accentColor: modelData.emphasize ? theme.ok : theme.accentStrong
                            emphasize: !!modelData.emphasize
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        function refreshPanelData() {
            if (!appController || !panelId) {
                panelData = ({})
                return
            }
            panelData = appController.protocolInfoPanel(panelId)
        }

        onPanelIdChanged: refreshPanelData()

        Connections {
            target: appController
            function onProtocolInfoPanelsChanged() {
                refreshPanelData()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.sectionSpacing

        NeonTabBar {
            id: tabs
            Layout.fillWidth: true

            Repeater {
                model: root.tabEntries

                delegate: NeonTabButton {
                    text: modelData.tabTitle || "Tab"
                    minButtonWidth: 82
                    accent: theme.accentStrong
                    onClicked: {
                        root.selectedTabKey = root.tabKey(root.tabEntries[index])
                        dataSettings.currentTab = index
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex
            Repeater {
                model: root.tabEntries

                delegate: Loader {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sourceComponent: {
                        if (modelData.kind === "raw") {
                            return rawPageComponent
                        }
                        if (modelData.kind === "location") {
                            return locationPageComponent
                        }
                        return protocolPageComponent
                    }
                    onLoaded: {
                        if (item && modelData.kind === "plugin") {
                            item.panelId = modelData.panelId || ""
                        }
                    }
                }
            }
        }
    }

    Component {
        id: rawPageComponent

        ColumnLayout {
            spacing: theme.sectionSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "RX/TX Stream"
                    color: theme.textSecondary
                    font.pixelSize: theme.bodySize
                    font.bold: true
                }
                Rectangle {
                    width: 10
                    height: 10
                    radius: theme.microRadius + 1
                    color: theme.rawRx
                }
                Label { text: "RX"; color: theme.textSecondary; font.pixelSize: theme.labelSize }
                Rectangle {
                    width: 10
                    height: 10
                    radius: theme.microRadius + 1
                    color: theme.rawTx
                }
                Label { text: "TX"; color: theme.textSecondary; font.pixelSize: theme.labelSize }
                Item { Layout.fillWidth: true }
                NeonButton {
                    text: "Clear"
                    accent: theme.warning
                    enabled: rawLogModel && rawLogModel.count > 0
                    onClicked: {
                        root.selectedRawIndex = -1
                        if (rawLogModel && typeof rawLogModel.clear === "function") {
                            rawLogModel.clear()
                        }
                    }
                }
                Label {
                    text: rawLogModel && rawLogModel.count !== undefined ? (rawLogModel.count + " frames") : "No stream"
                    color: theme.textSecondary
                    font.pixelSize: theme.labelSize
                }
            }

            ContentFrame {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: rawList
                    property bool autoFollowTail: true
                    property real tailResumeThreshold: 2
                    function maxContentY() {
                        return Math.max(0, contentHeight - height)
                    }
                    function isAtTail() {
                        return count <= 0 || contentY >= maxContentY() - tailResumeThreshold
                    }
                    function scrollToBottom() {
                        if (count <= 0 || !autoFollowTail || rawScrollBar.pressed) {
                            return
                        }
                        positionViewAtIndex(count - 1, ListView.End)
                    }
                    anchors.fill: parent
                    anchors.margins: 6
                    boundsBehavior: Flickable.StopAtBounds
                    boundsMovement: Flickable.StopAtBounds
                    clip: true
                    spacing: 2
                    model: rawLogModel ? rawLogModel : null
                    onCountChanged: {
                        Qt.callLater(function() {
                            rawList.scrollToBottom()
                        })
                    }
                    onContentHeightChanged: {
                        Qt.callLater(function() {
                            rawList.scrollToBottom()
                        })
                    }
                    onHeightChanged: {
                        Qt.callLater(function() {
                            rawList.scrollToBottom()
                        })
                    }
                    onContentYChanged: {
                        if (!rawScrollBar.pressed && rawList.isAtTail()) {
                            rawList.autoFollowTail = true
                        }
                    }

                    delegate: Rectangle {
                        property var panelRoot: root
                        width: rawList.width
                        height: Math.max(30, rawContent.implicitHeight + 10)
                        radius: theme.microRadius + 1
                        readonly property bool selected: rawList.currentIndex === index
                        color: selected
                               ? Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, 0.18)
                               : ((typeof direction !== "undefined" && direction === "TX") ? theme.rawTxRow : theme.rawRxRow)
                        border.width: selected ? 1 : 0
                        border.color: selected ? theme.accentStrong : "transparent"

                        RowLayout {
                            id: rawContent
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            anchors.topMargin: 4
                            anchors.bottomMargin: 4
                            spacing: 3

                            Label {
                                text: (typeof timestamp !== "undefined") ? timestamp : "--:--:--.---"
                                color: theme.textSecondary
                                font.pixelSize: theme.labelSize
                                Layout.preferredWidth: 68
                            }
                            Label {
                                text: (typeof direction !== "undefined" && direction === "TX") ? "T" : "R"
                                color: (typeof direction !== "undefined" && direction === "TX") ? theme.rawTxText : theme.rawRxText
                                font.bold: true
                                font.pixelSize: theme.labelSize
                                horizontalAlignment: Text.AlignHCenter
                                Layout.preferredWidth: 12
                            }
                            Label {
                                text: root.transportShortName((typeof transport !== "undefined") ? transport : "")
                                color: theme.textSecondary
                                font.bold: true
                                font.pixelSize: theme.labelSize
                                horizontalAlignment: Text.AlignHCenter
                                Layout.preferredWidth: 14
                            }
                            Rectangle {
                                readonly property string kindCode: {
                                    var value = String((typeof kind !== "undefined") ? kind : "BIN").toUpperCase()
                                    if (value === "BIN") return "B"
                                    if (value === "ASCII") return "A"
                                    if (value === "NMEA") return "N"
                                    return value.length > 0 ? value.charAt(0) : "?"
                                }
                                Layout.preferredWidth: 22
                                Layout.preferredHeight: 18
                                radius: theme.badgeRadius
                                color: ((typeof kind !== "undefined") ? kind : "BIN") === "NMEA"
                                       ? theme.badgeNmeaBg
                                       : ((((typeof kind !== "undefined") ? kind : "BIN") === "ASCII") ? theme.surface3 : theme.badgeBinBg)
                                border.width: 1
                                border.color: ((typeof kind !== "undefined") ? kind : "BIN") === "NMEA"
                                              ? theme.badgeNmeaBorder
                                              : ((((typeof kind !== "undefined") ? kind : "BIN") === "ASCII") ? theme.textSecondary : theme.badgeBinBorder)

                                Label {
                                    anchors.centerIn: parent
                                    text: parent.kindCode
                                    color: ((typeof kind !== "undefined") ? kind : "BIN") === "NMEA"
                                           ? theme.badgeNmeaText
                                           : ((((typeof kind !== "undefined") ? kind : "BIN") === "ASCII") ? theme.textPrimary : theme.badgeBinText)
                                    font.pixelSize: theme.labelSize
                                    font.bold: true
                                }
                            }
                            Label {
                                text: (typeof message !== "undefined" && message.length > 0) ? message : "-"
                                textFormat: Text.PlainText
                                color: theme.textPrimary
                                font.family: theme.monoFont
                                font.pixelSize: theme.labelSize
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.preferredWidth: 58
                            }
                            Label {
                                text: {
                                    var kindValue = String((typeof kind !== "undefined") ? kind : "BIN").toUpperCase()
                                    if (kindValue === "BIN") {
                                        return (typeof hex !== "undefined") ? hex : ""
                                    }
                                    return (typeof display !== "undefined") ? display : ""
                                }
                                textFormat: Text.PlainText
                                color: String((typeof kind !== "undefined") ? kind : "BIN").toUpperCase() === "BIN"
                                       ? theme.rawTextBin
                                       : theme.textPrimary
                                font.family: theme.monoFont
                                font.pixelSize: theme.monoSize
                                wrapMode: Text.WrapAnywhere
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onClicked: {
                                rawList.currentIndex = index
                                panelRoot.selectedRawIndex = index
                                panelRoot.copySelectedRawContent()
                            }
                            onDoubleClicked: {
                                rawList.currentIndex = index
                                panelRoot.selectedRawIndex = index
                                panelRoot.copySelectedRawLine()
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                        id: rawScrollBar
                        policy: ScrollBar.AsNeeded
                        onPressedChanged: {
                            if (pressed) {
                                rawList.autoFollowTail = false
                            } else if (rawList.isAtTail()) {
                                rawList.autoFollowTail = true
                                Qt.callLater(function() {
                                    rawList.scrollToBottom()
                                })
                            }
                        }
                    }

                    Component.onCompleted: {
                        if (count > 0) {
                            Qt.callLater(function() {
                                rawList.scrollToBottom()
                            })
                        }
                    }
                }

                Label {
                    visible: !rawLogModel || rawLogModel.count === 0
                    anchors.centerIn: parent
                    text: "No RX/TX frames"
                    color: theme.textSecondary
                    font.family: theme.bodyFont
                    font.pixelSize: theme.bodySize
                }

                Shortcut {
                    sequences: [StandardKey.Copy]
                    enabled: tabs.currentIndex === 0 && root.selectedRawIndex >= 0
                    onActivated: root.copySelectedRawLine()
                }
            }
        }
    }

    Component {
        id: locationPageComponent

        Item {
            id: locationPage
            function val(propName, fallbackValue) {
                if (!appController) {
                    return fallbackValue
                }
                var v = appController[propName]
                return v !== undefined ? v : fallbackValue
            }
            function fixed(propName, digits, suffix, fallbackValue) {
                var n = Number(val(propName, Number.NaN))
                if (isNaN(n)) {
                    return fallbackValue || "--"
                }
                return n.toFixed(digits) + (suffix || "")
            }
            function text(propName, fallbackValue) {
                var v = val(propName, "")
                var s = String(v === undefined || v === null ? "" : v)
                return s.length > 0 ? s : (fallbackValue || "--")
            }
            function integer(propName, fallbackValue) {
                var n = Number(val(propName, Number.NaN))
                if (isNaN(n)) {
                    return fallbackValue || "--"
                }
                return String(Math.round(n))
            }

            ContentFrame {
                anchors.fill: parent

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        columns: 5
                        columnSpacing: 6
                        rowSpacing: 6

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Latitude"
                            valueText: locationPage.fixed("latitude", 7, "", "--")
                            accentColor: theme.accent
                            emphasize: true
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Longitude"
                            valueText: locationPage.fixed("longitude", 7, "", "--")
                            accentColor: theme.accentStrong
                            emphasize: true
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Speed"
                            valueText: locationPage.fixed("speed", 2, " m/s", "--")
                            accentColor: theme.warning
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Date"
                            valueText: locationPage.text("utcDate", "---- -- --")
                            accentColor: Qt.lighter(theme.accentStrong, 1.18)
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "UTC"
                            valueText: locationPage.text("utcClock", "--:--:--")
                            accentColor: Qt.lighter(theme.accentStrong, 1.18)
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Altitude"
                            valueText: locationPage.fixed("altitude", 2, " m", "--")
                            accentColor: theme.ok
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Quality"
                            valueText: locationPage.integer("quality", "--")
                            accentColor: theme.ok
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "HDOP"
                            valueText: locationPage.fixed("hdop", 2, "", "--")
                            accentColor: theme.accent
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Satellites"
                            valueText: locationPage.text("satelliteUsage", "0 / 0")
                            accentColor: theme.accentStrong
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Age"
                            valueText: locationPage.fixed("age", 1, " s", "--")
                            accentColor: theme.warning
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Track"
                            valueText: locationPage.fixed("track", 2, "°", "--")
                            accentColor: theme.warning
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Magnetic"
                            valueText: locationPage.fixed("magneticVariation", 2, "°", "--")
                            accentColor: theme.constellationQzsUsed
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Mode"
                            valueText: locationPage.text("mode", "--")
                            accentColor: theme.constellationNavicUsed
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Status"
                            valueText: locationPage.text("navigationStatus", "--")
                            accentColor: locationPage.text("navigationStatus", "--") === "A" ? theme.ok : theme.bad
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Undulation"
                            valueText: locationPage.fixed("undulation", 2, " m", "--")
                            accentColor: theme.accentStrong
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "GPS"
                            valueText: locationPage.text("gpsSignals", "0 / 0")
                            accentColor: theme.constellationGpsUsed
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "GLONASS"
                            valueText: locationPage.text("glonassSignals", "0 / 0")
                            accentColor: theme.constellationGloUsed
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Beidou"
                            valueText: locationPage.text("beidouSignals", "0 / 0")
                            accentColor: theme.constellationBdsUsed
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Galileo"
                            valueText: locationPage.text("galileoSignals", "0 / 0")
                            accentColor: theme.constellationGalUsed
                        }

                        MetricCard {
                            Layout.fillWidth: true
                            compact: true
                            heading: "Other"
                            valueText: locationPage.text("otherSignals", "0 / 0")
                            accentColor: theme.textSecondary
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    Component {
        id: protocolPageComponent
        ProtocolInfoPage {}
    }

    onTabEntriesChanged: {
        scheduleRestoreTabSelection()
    }

    Component.onCompleted: {
        scheduleRestoreTabSelection()
    }
}
