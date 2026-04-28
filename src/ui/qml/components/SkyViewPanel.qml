import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root
    title: "SkyView"
    accent: theme.accentStrong
    readonly property int modelRevision: satelliteModel ? satelliteModel.revision : 0
    property string focusConstellation: ""
    property string focusId: ""
    property real focusAzimuth: 0
    property real focusElevation: 0
    property real focusCn0: 0
    property bool focusUsed: false
    property bool focusActive: false

    Theme {
        id: theme
    }

    function satColor(constellationName, used) {
        return theme.constellationColor(constellationName, used)
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

    function satCount(constellationName) {
        void root.modelRevision
        if (!satelliteModel || satelliteModel.count === undefined) {
            return 0
        }
        var count = 0
        for (var i = 0; i < satelliteModel.count; ++i) {
            var item = satelliteModel.get(i)
            if (item && item.constellation === constellationName) {
                count += 1
            }
        }
        return count
    }

    function satelliteBandGroup(item) {
        var groupName = String(item && item.bandGroup !== undefined ? item.bandGroup : "").toUpperCase()
        return groupName.length > 0 ? groupName : "UN"
    }

    function bandStats(groupName) {
        void root.modelRevision
        var stats = { used: 0, view: 0 }
        if (!satelliteModel || satelliteModel.count === undefined) {
            return stats
        }
        for (var i = 0; i < satelliteModel.count; ++i) {
            var item = satelliteModel.get(i)
            if (groupName !== "ALL" && root.satelliteBandGroup(item) !== groupName) {
                continue
            }
            stats.view += 1
            if (item && item.usedInFix === true) {
                stats.used += 1
            }
        }
        return stats
    }

    function statLabelText(label) {
        if (label.length === 2) {
            return label + "  "
        }
        if (label.length === 3) {
            return label + " "
        }
        return label
    }

    function statLine(label, groupName) {
        var stats = root.bandStats(groupName)
        return root.statLabelText(label) + " U: " + stats.used + " V: " + stats.view
    }

    function setFocus(constellationName, idText, azimuthValue, elevationValue, cn0Value, usedValue) {
        focusConstellation = constellationName || "GNSS"
        focusId = idText || "?"
        focusAzimuth = Number(azimuthValue || 0)
        focusElevation = Number(elevationValue || 0)
        focusCn0 = Number(cn0Value || 0)
        focusUsed = usedValue === true
        focusActive = true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: theme.sectionSpacing

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            radius: theme.pillRadius
            color: theme.tabBarBg
            border.width: 1
            border.color: theme.tabBarBorder

            Row {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4

                Repeater {
                    model: [
                        { name: "GPS", color: theme.constellationGpsUsed },
                        { name: "GLONASS", color: theme.constellationGloUsed },
                        { name: "GALILEO", color: theme.constellationGalUsed },
                        { name: "BEIDOU", color: theme.constellationBdsUsed },
                        { name: "QZSS", color: theme.constellationQzsUsed },
                        { name: "SBAS", color: theme.constellationSbsUsed },
                        { name: "NAVIC", color: theme.constellationNavicUsed }
                    ]

                    Rectangle {
                        required property var modelData
                        width: Math.floor((parent.width - parent.spacing * 6) / 7)
                        height: 26
                        radius: theme.pillRadius
                        color: theme.surface3
                        border.width: 1
                        border.color: theme.panelBorderStrong

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 4

                            Rectangle {
                                width: 8
                                height: 8
                                radius: theme.microRadius
                                color: modelData.color
                                border.width: 1
                                border.color: theme.textPrimary
                            }

                            Label {
                                text: root.constellationShortLabel(modelData.name)
                                color: theme.textPrimary
                                font.pixelSize: theme.labelSize
                                font.bold: true
                            }


                            Label {
                                text: String(root.satCount(modelData.name))
                                color: theme.textSecondary
                                font.pixelSize: theme.labelSize - 2
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                anchors.fill: parent
                radius: theme.panelRadius
                color: theme.chartBg
                border.width: 1
                border.color: theme.panelBorder
                clip: true

                Canvas {
                    id: chart
                    anchors.fill: parent
                    anchors.margins: 10
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        var cx = width * 0.5
                        var cy = height * 0.52
                        var r = Math.min(width, height) * 0.40

                        var ringColors = [theme.chartAxisMuted, theme.chartGridMinor, theme.chartGridMajor]
                        for (var i = 3; i >= 1; --i) {
                            ctx.strokeStyle = ringColors[3 - i]
                            ctx.lineWidth = i === 3 ? 1.2 : 1
                            ctx.beginPath()
                            ctx.arc(cx, cy, r * i / 3.0, 0, Math.PI * 2)
                            ctx.stroke()
                        }

                        ctx.strokeStyle = theme.chartAxis
                        ctx.lineWidth = 1
                        ctx.beginPath()
                        ctx.moveTo(cx - r, cy)
                        ctx.lineTo(cx + r, cy)
                        ctx.moveTo(cx, cy - r)
                        ctx.lineTo(cx, cy + r)
                        ctx.stroke()

                        ctx.setLineDash([3, 5])
                        ctx.strokeStyle = theme.chartAxisMuted
                        ctx.beginPath()
                        ctx.moveTo(cx - r * 0.72, cy - r * 0.72)
                        ctx.lineTo(cx + r * 0.72, cy + r * 0.72)
                        ctx.moveTo(cx + r * 0.72, cy - r * 0.72)
                        ctx.lineTo(cx - r * 0.72, cy + r * 0.72)
                        ctx.stroke()
                        ctx.setLineDash([])

                        ctx.fillStyle = theme.chartLabel
                        ctx.font = theme.labelSize + "px " + theme.bodyFont
                        ctx.fillText("N", cx - 4, cy - r - 8)
                        ctx.fillText("S", cx - 4, cy + r + 14)
                        ctx.fillText("W", cx - r - 14, cy + 4)
                        ctx.fillText("E", cx + r + 8, cy + 4)

                        ctx.fillStyle = theme.chartLabelMuted
                        ctx.fillText("60", cx + 6, cy - r / 3 - 4)
                        ctx.fillText("30", cx + 6, cy - 2 * r / 3 - 4)
                        ctx.fillText("0", cx + 6, cy - r - 4)
                    }
                }

                Rectangle {
                    id: focusPanel
                    visible: root.focusActive
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: parent.top
                        topMargin: 14
                    }
                    width: 94
                    height: 54
                    radius: theme.cardRadius
                    color: theme.surface1
                    border.width: 1
                    border.color: theme.panelBorderStrong
                    z: 20

                    Rectangle {
                        anchors {
                            left: parent.left
                            top: parent.top
                            bottom: parent.bottom
                            margins: 0
                        }
                        width: 5
                        radius: theme.tinyRadius
                        color: root.satColor(root.focusConstellation, root.focusUsed)
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        anchors.topMargin: 7
                        anchors.bottomMargin: 7
                        spacing: 1

                        Label {
                            text: root.focusConstellation + " " + root.focusId
                            color: theme.textPrimary
                            font.pixelSize: theme.bodySize
                            font.bold: true
                        }
                        Label {
                            text: "Az " + Math.round(root.focusAzimuth)
                                  + "  El " + Math.round(root.focusElevation)
                            color: theme.tooltipDetail
                            font.pixelSize: theme.labelSize + 1
                        }
                        Label {
                            text: (root.focusUsed ? "Used" : "View") + "  C/N0 " + Math.round(root.focusCn0)
                            color: root.satColor(root.focusConstellation, root.focusUsed)
                            font.pixelSize: theme.labelSize + 1
                            font.bold: true
                        }
                    }
                }

                Column {
                    anchors {
                        left: parent.left
                        top: parent.top
                        leftMargin: 16
                        topMargin: 16
                    }
                    spacing: 2
                    z: 20

                    Label {
                        text: root.statLine("L1", "L1")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                    Label {
                        text: root.statLine("B1", "B1")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                    Label {
                        text: root.statLine("E1", "E1")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                }

                Column {
                    anchors {
                        right: parent.right
                        top: parent.top
                        rightMargin: 16
                        topMargin: 16
                    }
                    spacing: 2
                    z: 20

                    Label {
                        text: root.statLine("ALL", "ALL")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                    Label {
                        text: root.statLine("L1C", "L1C")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                }

                Column {
                    anchors {
                        left: parent.left
                        bottom: parent.bottom
                        leftMargin: 16
                        bottomMargin: 16
                    }
                    spacing: 2
                    z: 20

                    Label {
                        text: root.statLine("L5", "L5")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                    Label {
                        text: root.statLine("B2", "B2")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                    Label {
                        text: root.statLine("E5", "E5")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                }

                Column {
                    anchors {
                        right: parent.right
                        bottom: parent.bottom
                        rightMargin: 16
                        bottomMargin: 16
                    }
                    spacing: 2
                    z: 20

                    Label {
                        text: root.statLine("L2", "L2")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                    Label {
                        text: root.statLine("B3", "B3")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                    Label {
                        text: root.statLine("UN", "UN")
                        color: theme.textSecondary
                        font.family: theme.monoFont
                        font.pixelSize: theme.monoSize
                        font.bold: true
                    }
                }

                Repeater {
                    model: satelliteModel ? satelliteModel : null

                    Rectangle {
                        required property int azimuth
                        required property int elevation
                        required property string constellation
                        required property int svid
                        required property int cn0
                        required property bool usedInFix

                        readonly property real az: azimuth
                        readonly property real el: elevation
                        readonly property string satConstellation: constellation
                        readonly property bool satUsed: usedInFix
                        readonly property string satId: String(svid)
                        readonly property real satSnr: cn0
                        readonly property bool isHot: hover.hovered

                        width: isHot ? 18 : (satUsed ? 14 : 12)
                        height: width
                        radius: width * 0.5
                        border.width: satUsed ? 1.4 : 1
                        border.color: isHot ? theme.textPrimary : (satUsed ? theme.tooltipText : theme.tooltipDetail)
                        color: root.satColor(satConstellation, satUsed)
                        z: isHot ? 6 : 3

                        Behavior on width {
                            NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
                        }

                        readonly property real cx: chart.x + chart.width * 0.5
                        readonly property real cy: chart.y + chart.height * 0.52
                        readonly property real maxR: Math.min(chart.width, chart.height) * 0.40
                        readonly property real rr: maxR * (90 - Math.max(0, Math.min(90, el))) / 90
                        readonly property real theta: az * Math.PI / 180.0

                        x: cx + Math.sin(theta) * rr - width * 0.5
                        y: cy - Math.cos(theta) * rr - height * 0.5

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width + (isHot ? 18 : 10)
                            height: width
                            radius: width * 0.5
                            color: isHot ? Qt.rgba(0.75, 0.97, 1.0, 0.08) : "transparent"
                            border.width: isHot ? 1 : (satUsed ? 1 : 0)
                            border.color: isHot ? Qt.rgba(0.75, 0.97, 1.0, 0.55)
                                                  : (satUsed ? Qt.rgba(0.75, 0.97, 1.0, 0.22) : "transparent")
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width + (isHot ? 30 : 0)
                            height: width
                            radius: width * 0.5
                            visible: isHot
                            color: Qt.rgba(0.30, 0.73, 1.0, 0.08)
                            border.width: 0
                        }

                        HoverHandler {
                            id: hover
                            onHoveredChanged: {
                                if (hovered) {
                                    root.setFocus(satConstellation, satId, az, el, satSnr, satUsed)
                                }
                            }
                        }

                        TapHandler {
                            onTapped: {
                                root.setFocus(satConstellation, satId, az, el, satSnr, satUsed)
                            }
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: {
                        root.focusActive = false
                    }
                }
            }
        }
    }
}
