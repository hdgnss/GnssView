import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root
    property string defaultBand: "L1"
    readonly property var bandTabs: [
        { key: "L1", label: "L1/B1/E1/L1C" },
        { key: "L2", label: "L2" },
        { key: "L5", label: "L5/E5/B2" },
        { key: "L6", label: "L6/E6/B3" }
    ]
    title: "Signal Spectrum"
    accent: theme.accentStrong

    Theme { id: theme }

    function satColor(constellationName, used, signalId) {
        return theme.signalColor(constellationName, used, signalId)
    }

    function constellationOrder(constellationName) {
        if (constellationName === "GPS") return 0
        if (constellationName === "GLONASS") return 1
        if (constellationName === "GALILEO") return 2
        if (constellationName === "BEIDOU") return 3
        if (constellationName === "QZSS") return 4
        if (constellationName === "SBAS") return 5
        if (constellationName === "NAVIC" || constellationName === "IRNSS") return 6
        return 9
    }

    function constellationShortLabel(constellationName) {
        if (constellationName === "GPS") return "G"
        if (constellationName === "GLONASS") return "R"
        if (constellationName === "GALILEO") return "E"
        if (constellationName === "BEIDOU") return "C"
        if (constellationName === "QZSS") return "J"
        if (constellationName === "SBAS") return "S"
        if (constellationName === "NAVIC" || constellationName === "IRNSS") return "I"
        return "U"
    }

    function groupedBandItems(items) {
        var sorted = []
        for (var i = 0; i < items.length; ++i) {
            var item = items[i]
            sorted.push(item)
        }
        sorted.sort(function(a, b) {
            var orderDiff = root.constellationOrder(a.constellation || "GNSS") - root.constellationOrder(b.constellation || "GNSS")
            if (orderDiff !== 0) {
                return orderDiff
            }
            return String(a.label || "").localeCompare(String(b.label || ""))
        })

        for (var j = 0; j < sorted.length; ++j) {
            var item = sorted[j]
            var constellation = item.constellation || "GNSS"
            sorted[j] = {
                kind: "satellite",
                label: item.label,
                constellation: constellation,
                bandGroup: item.bandGroup,
                signalId: item.signalId,
                svid: item.svid,
                strength: item.strength,
                usedInFix: item.usedInFix
            }
        }
        return sorted
    }

    function prnText(modelData) {
        if (modelData && modelData.label) {
            return String(modelData.label)
        }
        if (modelData && modelData.svid !== undefined) {
            return String(modelData.svid)
        }
        var raw = String(modelData && modelData.label ? modelData.label : "")
        var digits = raw.replace(/^\D+/, "")
        return digits.length > 0 ? digits : raw
    }

    function clampStrength(value) {
        return Math.max(0, Math.min(50, Number(value || 0)))
    }

    function bandIndexForKey(bandKey) {
        for (var i = 0; i < bandTabs.length; ++i) {
            if (bandTabs[i].key === bandKey) {
                return i
            }
        }
        return 0
    }

    function bandKeyForIndex(index) {
        if (index >= 0 && index < bandTabs.length) {
            return bandTabs[index].key
        }
        return "L1"
    }

    function bandLabelForIndex(index) {
        if (index >= 0 && index < bandTabs.length) {
            return bandTabs[index].label
        }
        return "L1/B1/E1/L1C"
    }

    Component.onCompleted: {
        tabs.currentIndex = root.bandIndexForKey(defaultBand)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.sectionSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            NeonTabBar {
                id: tabs
                Layout.fillWidth: true
                NeonTabButton { text: root.bandTabs[0].label; minButtonWidth: 132; accent: theme.accentStrong }
                NeonTabButton { text: root.bandTabs[1].label; minButtonWidth: 112; accent: theme.accentStrong }
                NeonTabButton { text: root.bandTabs[2].label; minButtonWidth: 112; accent: theme.accentStrong }
                NeonTabButton { text: root.bandTabs[3].label; minButtonWidth: 112; accent: theme.accentStrong }
            }

            Rectangle {
                Layout.preferredWidth: 320
                Layout.preferredHeight: theme.controlHeight
                radius: theme.pillRadius
                color: theme.surface2
                border.width: 1
                border.color: theme.panelBorderStrong
                clip: true
                antialiasing: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Repeater {
                        model: [
                            { name: "GPS", color: root.satColor("GPS", true, 0) },
                            { name: "GLONASS", color: root.satColor("GLONASS", true, 0) },
                            { name: "GALILEO", color: root.satColor("GALILEO", true, 0) },
                            { name: "BEIDOU", color: root.satColor("BEIDOU", true, 0) },
                            { name: "QZSS", color: root.satColor("QZSS", true, 0) },
                            { name: "SBAS", color: root.satColor("SBAS", true, 0) },
                            { name: "NAVIC", color: root.satColor("NAVIC", true, 0) }
                        ]

                        RowLayout {
                            required property var modelData
                            spacing: 4

                            Rectangle { width: 10; height: 10; radius: theme.microRadius + 1; color: modelData.color }
                            Label { text: root.constellationShortLabel(modelData.name); color: theme.textSecondary; font.pixelSize: theme.labelSize }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle { width: 10; height: 10; radius: theme.microRadius + 1; color: theme.visibleHint }
                    Label { text: "Visible tint"; color: theme.textSecondary; font.pixelSize: theme.labelSize }
                }
            }
        }

        Rectangle {
            id: instrumentPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: theme.cardRadius
            color: theme.surface0
            border.width: 1
            border.color: theme.panelBorderStrong
            clip: true
            antialiasing: true

            readonly property string currentBand: root.bandKeyForIndex(tabs.currentIndex)
            readonly property string currentBandLabel: root.bandLabelForIndex(tabs.currentIndex)
            readonly property int modelCount: signalModel ? signalModel.count : 0
            readonly property int modelRevision: signalModel ? signalModel.revision : 0
            readonly property var bandItems: {
                void modelRevision
                return signalModel && signalModel.itemsForBand ? signalModel.itemsForBand(currentBand) : []
            }
            readonly property var groupedItems: root.groupedBandItems(bandItems)
            readonly property int satelliteCount: bandItems.length
            readonly property real slotSpacing: 1
            readonly property real plotTopPadding: 28
            readonly property real plotBottomPadding: 32
            readonly property real displayStrengthMax: 50
            readonly property int gridLineCount: 5
            readonly property real plotHeight: Math.max(0, instrumentChart.height - plotTopPadding - plotBottomPadding)
            readonly property real plotBaseline: plotTopPadding + plotHeight
            readonly property real plotLabelStep: plotHeight / gridLineCount
            readonly property real chartHorizontalPadding: 6
            readonly property int labelStride: {
                if (barWidth <= 10) return 3
                if (barWidth <= 12) return 2
                return 1
            }
            readonly property real barWidth: {
                if (satelliteCount <= 0) {
                    return 18
                }
                var reservedSpacing = Math.max(0, satelliteCount - 1) * slotSpacing
                var available = Math.max(0, instrumentChart.width - chartHorizontalPadding * 2 - reservedSpacing)
                return Math.max(9, Math.min(22, available / satelliteCount))
            }
            readonly property real contentWidth: {
                if (satelliteCount <= 0) {
                    return instrumentChart.width
                }
                return Math.max(
                    instrumentChart.width,
                    chartHorizontalPadding * 2
                        + satelliteCount * barWidth
                        + Math.max(0, satelliteCount - 1) * slotSpacing)
            }
            property real hotStrength: -1
            property string hotLabel: ""
            property string hotConstellation: ""
            property bool hotUsed: false
            readonly property real peakStrength: {
                var peak = 0
                for (var i = 0; i < bandItems.length; ++i) {
                    var value = Number(bandItems[i].strength !== undefined ? bandItems[i].strength : 0)
                    if (value > peak) {
                        peak = value
                    }
                }
                return peak
            }
            readonly property string peakLabel: {
                var peak = -1
                var best = "--"
                for (var i = 0; i < bandItems.length; ++i) {
                    var value = Number(bandItems[i].strength !== undefined ? bandItems[i].strength : 0)
                    if (value > peak) {
                        peak = value
                        best = bandItems[i].label || "SV"
                    }
                }
                return best
            }
            readonly property int usedCount: bandItems.filter(function(item) { return item.usedInFix === true }).length

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: instrumentPanel.currentBandLabel + " front-end"
                        color: theme.textPrimary
                        font.family: theme.titleFont
                        font.pixelSize: theme.bodySize + 1
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: instrumentPanel.bandItems.length + " channels"
                        color: theme.textSecondary
                        font.pixelSize: theme.labelSize
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: theme.controlRadius
                    color: theme.surface0
                    border.width: 1
                    border.color: theme.panelBorder
                    clip: true
                    antialiasing: true

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Item {
                            Layout.preferredWidth: 34
                            Layout.fillHeight: true

                            Repeater {
                                model: ["50", "40", "30", "20", "10", "0"]
                                Label {
                                    required property int index
                                    required property string modelData
                                    x: 0
                                    y: instrumentPanel.plotTopPadding + index * instrumentPanel.plotLabelStep - height * 0.5
                                    width: parent.width
                                    horizontalAlignment: Text.AlignLeft
                                    text: modelData
                                    color: theme.chartLabelMuted
                                    font.pixelSize: theme.labelSize
                                }
                            }
                        }

                        Item {
                            id: instrumentChart
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Canvas {
                                id: gridCanvas
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.reset()
                                    ctx.lineWidth = 1
                                    var plotTop = instrumentPanel.plotTopPadding
                                    var plotHeight = instrumentPanel.plotHeight
                                    var plotBottom = instrumentPanel.plotBaseline
                                    for (var i = 0; i <= instrumentPanel.gridLineCount; ++i) {
                                        var y = plotBottom - i * (plotHeight / instrumentPanel.gridLineCount)
                                        ctx.strokeStyle = i === 0 ? theme.chartGridMajor : theme.chartGridMinor
                                        ctx.beginPath()
                                        ctx.moveTo(0, y)
                                        ctx.lineTo(width, y)
                                        ctx.stroke()
                                    }

                                    if (instrumentPanel.bandItems.length > 0) {
                                        var peakDisplayStrength = root.clampStrength(instrumentPanel.peakStrength)
                                        var peakY = plotBottom - peakDisplayStrength / instrumentPanel.displayStrengthMax * plotHeight
                                        ctx.strokeStyle = theme.chartPeak
                                        ctx.lineWidth = 1
                                        ctx.setLineDash([6, 4])
                                        ctx.beginPath()
                                        ctx.moveTo(0, peakY)
                                        ctx.lineTo(width, peakY)
                                        ctx.stroke()
                                        ctx.setLineDash([])

                                        ctx.fillStyle = theme.chartPeakText
                                        ctx.font = theme.labelSize + "px " + theme.bodyFont
                                        ctx.textAlign = "right"
                                        ctx.fillText("PEAK " + Math.round(peakDisplayStrength), width - 8, Math.max(plotTop - 6, peakY - 4))
                                        ctx.textAlign = "left"
                                    }
                                }
                            }

                            onWidthChanged: gridCanvas.requestPaint()
                            onHeightChanged: gridCanvas.requestPaint()
                            Connections {
                                target: instrumentPanel
                                function onBandItemsChanged() { gridCanvas.requestPaint() }
                                function onPeakStrengthChanged() { gridCanvas.requestPaint() }
                                function onCurrentBandChanged() { gridCanvas.requestPaint() }
                            }

                            Flickable {
                                id: barsFlick
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                    bottom: parent.bottom
                                    top: parent.top
                                }
                                clip: true
                                contentWidth: instrumentPanel.contentWidth
                                contentHeight: height
                                boundsBehavior: Flickable.StopAtBounds
                                interactive: barsFlick.contentWidth > barsFlick.width

                                Row {
                                    id: barsRow
                                    x: instrumentPanel.chartHorizontalPadding
                                    y: 0
                                    height: parent.height
                                    spacing: instrumentPanel.slotSpacing

                                    Repeater {
                                        model: instrumentPanel.groupedItems

                                        Item {
                                            required property var modelData
                                            readonly property real strength: Number(modelData.strength !== undefined ? modelData.strength : 0)
                                            readonly property real displayStrength: root.clampStrength(strength)
                                            readonly property bool hasSignal: displayStrength > 0
                                            readonly property real barHeight: hasSignal
                                                ? Math.max(4, Math.min(instrumentPanel.plotHeight,
                                                                       displayStrength / instrumentPanel.displayStrengthMax * instrumentPanel.plotHeight))
                                                : 4
                                            readonly property bool used: modelData.usedInFix === true
                                            readonly property string label: root.prnText(modelData)
                                            readonly property string constellation: modelData.constellation || "GNSS"
                                            readonly property int signalId: Number(modelData.signalId !== undefined ? modelData.signalId : 0)
                                            readonly property color fillColor: root.satColor(constellation, used, signalId)
                                            readonly property bool alternateSignal: signalId > 0
                                            readonly property bool isHot: hover.hovered

                                            width: instrumentPanel.barWidth
                                            height: parent.height

                                            Rectangle {
                                                visible: true
                                                anchors {
                                                    left: parent.left
                                                    right: parent.right
                                                    bottom: parent.bottom
                                                    bottomMargin: instrumentPanel.plotBottomPadding
                                                }
                                                radius: theme.microRadius - 1
                                                height: barHeight
                                                color: isHot ? Qt.lighter(fillColor, 1.18) : fillColor
                                                opacity: hasSignal ? 1.0 : 0.45
                                                border.width: 1
                                                border.color: isHot ? theme.textPrimary : (used ? theme.tooltipText : theme.visibleHint)

                                                Behavior on color {
                                                    ColorAnimation { duration: 100 }
                                                }

                                                Rectangle {
                                                    visible: alternateSignal && hasSignal
                                                    anchors {
                                                        left: parent.left
                                                        right: parent.right
                                                        top: parent.top
                                                        margins: 1
                                                    }
                                                    height: Math.min(6, Math.max(3, parent.height * 0.18))
                                                    radius: theme.tinyRadius + 1
                                                    color: theme.signalCapColor(signalId)
                                                    opacity: isHot ? 0.95 : 0.8
                                                }
                                            }

                                            Rectangle {
                                                visible: true
                                                anchors {
                                                    left: parent.left
                                                    right: parent.right
                                                    bottom: parent.bottom
                                                    bottomMargin: instrumentPanel.plotBottomPadding
                                                }
                                                radius: theme.microRadius
                                                height: Math.max(6, Math.min(instrumentPanel.plotHeight + 4, barHeight + 4))
                                                color: Qt.rgba(fillColor.r, fillColor.g, fillColor.b, isHot ? 0.12 : 0.05)
                                                border.width: isHot ? 1 : 0
                                                border.color: Qt.rgba(0.92, 0.97, 1.0, 0.35)
                                                z: -1
                                            }

                                            Rectangle {
                                                visible: true
                                                anchors {
                                                    left: parent.left
                                                    right: parent.right
                                                    bottom: parent.bottom
                                                    bottomMargin: instrumentPanel.plotBottomPadding
                                                }
                                                y: parent.height - instrumentPanel.plotBottomPadding - barHeight
                                                height: 2
                                                radius: theme.tinyRadius
                                                color: isHot ? theme.textPrimary : theme.visibleHint
                                            }

                                            Rectangle {
                                                visible: true
                                                anchors {
                                                    left: parent.left
                                                    right: parent.right
                                                    bottom: parent.bottom
                                                    bottomMargin: instrumentPanel.plotBottomPadding
                                                }
                                                y: parent.height - instrumentPanel.plotBottomPadding - 2
                                                height: 1
                                                color: theme.divider
                                            }

                                            Label {
                                                visible: strength > 0
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                y: Math.max(2, parent.height - instrumentPanel.plotBottomPadding - barHeight - height - 4)
                                                text: String(Math.round(strength))
                                                color: isHot ? theme.textPrimary : Qt.lighter(theme.textSecondary, 1.25)
                                                font.family: theme.monoFont
                                                font.pixelSize: Math.max(9, Math.min(theme.labelSize, instrumentPanel.barWidth - 1))
                                                font.bold: true
                                                horizontalAlignment: Text.AlignHCenter
                                                opacity: isHot ? 1.0 : 0.96
                                            }

                                            HoverHandler {
                                                id: hover
                                                enabled: true
                                            }

                                            onIsHotChanged: {
                                                if (isHot) {
                                                    instrumentPanel.hotLabel = label
                                                    instrumentPanel.hotConstellation = constellation
                                                    instrumentPanel.hotStrength = strength
                                                    instrumentPanel.hotUsed = used
                                                } else if (instrumentPanel.hotLabel === label && instrumentPanel.hotConstellation === constellation) {
                                                    instrumentPanel.hotLabel = ""
                                                    instrumentPanel.hotConstellation = ""
                                                    instrumentPanel.hotStrength = -1
                                                    instrumentPanel.hotUsed = false
                                                }
                                            }

                                            InstrumentToolTip {
                                                active: parent.isHot
                                                titleText: constellation + " " + label + "  " + instrumentPanel.currentBandLabel
                                                detailText: "C/N0 " + Math.round(displayStrength) + " dB-Hz" + (used ? "  Used in fix" : "  Visible only")
                                                accent: parent.fillColor
                                                x: parent.x + parent.width * 0.5 - width * 0.5
                                                y: parent.y - height - 10
                                            }
                                        }
                                    }
                                }

                                Row {
                                    x: instrumentPanel.chartHorizontalPadding
                                    y: parent.height - instrumentPanel.plotBottomPadding
                                    height: instrumentPanel.plotBottomPadding
                                    spacing: instrumentPanel.slotSpacing

                                    Repeater {
                                        model: instrumentPanel.groupedItems

                                        Item {
                                            required property var modelData
                                            readonly property string label: root.prnText(modelData)
                                            readonly property bool hot: instrumentPanel.hotLabel === label
                                                && instrumentPanel.hotConstellation === (modelData.constellation || "GNSS")

                                            width: instrumentPanel.barWidth
                                            height: parent.height

                                            Text {
                                                visible: hot || instrumentPanel.labelStride === 1 || (index % instrumentPanel.labelStride === 0)
                                                anchors.centerIn: parent
                                                text: label
                                                color: hot ? theme.textPrimary : Qt.lighter(theme.textSecondary, 1.2)
                                                font.family: theme.monoFont
                                                font.pixelSize: Math.max(9, theme.labelSize - 2)
                                                font.bold: true
                                                opacity: hot ? 1.0 : 0.92
                                                rotation: -90
                                                transformOrigin: Item.Center
                                                elide: Text.ElideNone
                                            }
                                        }
                                    }
                                }

                                ScrollBar.horizontal: ScrollBar {
                                    policy: barsFlick.contentWidth > barsFlick.width ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: instrumentPanel.bandItems.length === 0
                                text: "No signal data on " + instrumentPanel.currentBandLabel
                                color: theme.textSecondary
                                font.pixelSize: theme.bodySize
                            }
                        }

                        Rectangle {
                            visible: false
                            Layout.preferredWidth: 0
                            Layout.maximumWidth: 0
                            Layout.minimumWidth: 0
                        }
                    }
                }
            }
        }
    }
}
