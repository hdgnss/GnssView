import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root
    property string reservedTitle: "Deviation Map"
    title: reservedTitle
    accent: theme.accentStrong
    readonly property int modelRevision: deviationMapModel ? deviationMapModel.revision : 0
    readonly property var statistics: deviationMapModel ? deviationMapModel.stats : ({})

    Theme { id: theme }

    function numberText(value, digits) {
        var numeric = Number(value)
        if (!isFinite(numeric)) {
            return "--"
        }
        return numeric.toFixed(digits === undefined ? 2 : digits)
    }

    function statsValue(group, key) {
        var block = statistics && statistics[group] ? statistics[group] : null
        return block && block[key] !== undefined ? block[key] : 0
    }

    function maxDistanceValue() {
        var distance = statistics && statistics.maxDistance !== undefined ? Number(statistics.maxDistance) : 1
        return Math.max(1, distance)
    }

    function ringLabel(index) {
        return numberText(maxDistanceValue() * index / 4.0, 2) + "m"
    }

    function axisAverageText(group) {
        var avg = root.numberText(root.statsValue(group, "avg"), 2) + " m"
        return statistics && statistics.centerMode === "Average"
            ? "Avg: " + avg + " (centered)"
            : "Avg: " + avg
    }

    onModelRevisionChanged: chart.requestPaint()

    component StatLabel: Label {
        color: theme.textSecondary
        font.pixelSize: theme.labelSize
        textFormat: Text.PlainText
    }

    component StatHeader: Label {
        color: theme.textPrimary
        font.bold: true
        font.pixelSize: theme.bodySize
        textFormat: Text.PlainText
    }

    Item {
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            radius: theme.panelRadius
            color: theme.chartBg
            border.width: 1
            border.color: theme.panelBorderStrong
            clip: true

            Canvas {
                id: chart
                anchors.fill: parent
                anchors.margins: 10
                onPaint: {
                    void root.modelRevision
                    var ctx = getContext("2d")
                    ctx.reset()
                    var cx = width * 0.5
                    var cy = height * 0.52
                    var r = Math.min(width, height) * 0.40

                    for (var i = 4; i >= 1; --i) {
                        ctx.setLineDash(i === 4 ? [] : [3, 5])
                        ctx.strokeStyle = i === 4 ? theme.chartAxis : theme.chartGridMajor
                        ctx.lineWidth = i === 4 ? 1.2 : 1
                        ctx.beginPath()
                        ctx.arc(cx, cy, r * i / 4.0, 0, Math.PI * 2)
                        ctx.stroke()
                    }
                    ctx.setLineDash([])

                    ctx.strokeStyle = theme.chartAxis
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.moveTo(cx - r, cy)
                    ctx.lineTo(cx + r, cy)
                    ctx.moveTo(cx, cy - r)
                    ctx.lineTo(cx, cy + r)
                    ctx.stroke()

                    ctx.fillStyle = theme.chartLabel
                    ctx.font = theme.labelSize + "px " + theme.bodyFont
                    ctx.fillText("N", cx - 4, cy - r - 8)
                    ctx.fillText("S", cx - 4, cy + r + 14)
                    ctx.fillText("W", cx - r - 16, cy + 4)
                    ctx.fillText("E", cx + r + 8, cy + 4)

                    ctx.fillStyle = theme.chartLabelMuted
                    ctx.fillText(root.ringLabel(1), cx + 8, cy - r / 4 - 4)
                    ctx.fillText(root.ringLabel(2), cx + 8, cy - r / 2 - 4)
                    ctx.fillText(root.ringLabel(3), cx + 8, cy - 3 * r / 4 - 4)
                    ctx.fillText(root.ringLabel(4), cx + 8, cy - r - 4)
                }
            }

            Repeater {
                model: deviationMapModel ? deviationMapModel : null

                Rectangle {
                    required property double eastMeters
                    required property double northMeters
                    required property double distanceMeters
                    required property bool latest

                    readonly property real cx: chart.x + chart.width * 0.5
                    readonly property real cy: chart.y + chart.height * 0.52
                    readonly property real maxR: Math.min(chart.width, chart.height) * 0.40
                    readonly property real scaleDistance: root.maxDistanceValue()

                    width: latest ? 8 : 5
                    height: width
                    radius: width * 0.5
                    color: latest ? theme.accentStrong : Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, 0.56)
                    border.width: latest ? 1 : 0
                    border.color: latest ? theme.textPrimary : "transparent"

                    x: cx + maxR * (eastMeters / scaleDistance) - width * 0.5
                    y: cy - maxR * (northMeters / scaleDistance) - height * 0.5
                }
            }

            Item {
                anchors {
                    left: parent.left
                    top: parent.top
                    leftMargin: 18
                    topMargin: 14
                }
                width: 184
                height: statsColumn.implicitHeight

                Column {
                    id: statsColumn
                    anchors.fill: parent
                    spacing: 3

                    StatHeader { width: parent.width; text: "Longitude (E-W)"; horizontalAlignment: Text.AlignLeft }
                    StatLabel { width: parent.width; text: "Min: " + root.numberText(root.statsValue("longitude", "min"), 2) + " m"; horizontalAlignment: Text.AlignLeft }
                    StatLabel { width: parent.width; text: "Max: " + root.numberText(root.statsValue("longitude", "max"), 2) + " m"; horizontalAlignment: Text.AlignLeft }
                    StatLabel { width: parent.width; text: root.axisAverageText("longitude"); horizontalAlignment: Text.AlignLeft }
                    StatLabel { width: parent.width; text: "StdDev: " + root.numberText(root.statsValue("longitude", "stdDev"), 2) + " m"; horizontalAlignment: Text.AlignLeft }
                }
            }

            Item {
                anchors {
                    right: parent.right
                    top: parent.top
                    rightMargin: 18
                    topMargin: 14
                }
                width: 204
                height: hpeColumn.implicitHeight

                Column {
                    id: hpeColumn
                    anchors.fill: parent
                    spacing: 3

                    StatHeader { width: parent.width; text: "Horizontal (HPE)"; horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "Min: " + root.numberText(root.statsValue("horizontal", "min"), 2) + " m"; horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "Max: " + root.numberText(root.statsValue("horizontal", "max"), 2) + " m"; horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "Avg: " + root.numberText(root.statsValue("horizontal", "avg"), 2) + " m"; horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "CEP50: " + root.numberText(root.statsValue("horizontal", "cep50"), 2) + " m"; horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "CEP68: " + root.numberText(root.statsValue("horizontal", "cep68"), 2) + " m"; horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "CEP95: " + root.numberText(root.statsValue("horizontal", "cep95"), 2) + " m"; horizontalAlignment: Text.AlignRight }
                }
            }

            Item {
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                    leftMargin: 18
                    bottomMargin: 18
                }
                width: 208
                height: latitudeColumn.implicitHeight

                Column {
                    id: latitudeColumn
                    anchors.fill: parent
                    spacing: 3

                    StatHeader { width: parent.width; text: "Latitude (N-S)"; horizontalAlignment: Text.AlignLeft }
                    StatLabel { width: parent.width; text: "Min: " + root.numberText(root.statsValue("latitude", "min"), 2) + " m"; horizontalAlignment: Text.AlignLeft }
                    StatLabel { width: parent.width; text: "Max: " + root.numberText(root.statsValue("latitude", "max"), 2) + " m"; horizontalAlignment: Text.AlignLeft }
                    StatLabel { width: parent.width; text: root.axisAverageText("latitude"); horizontalAlignment: Text.AlignLeft }
                    StatLabel { width: parent.width; text: "StdDev: " + root.numberText(root.statsValue("latitude", "stdDev"), 2) + " m"; horizontalAlignment: Text.AlignLeft }
                }
            }

            Item {
                anchors {
                    right: parent.right
                    bottom: parent.bottom
                    rightMargin: 18
                    bottomMargin: 18
                }
                width: 208
                height: longitudeColumn.implicitHeight

                Column {
                    id: longitudeColumn
                    anchors.fill: parent
                    spacing: 3

                    StatHeader { width: parent.width; text: "Statistics"; horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "Points: " + Number(statistics.points || 0); horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "Center: " + (statistics.centerMode || "Average"); horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "Lat: " + root.numberText(statistics.centerLatitude, 8); horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "Lon: " + root.numberText(statistics.centerLongitude, 8); horizontalAlignment: Text.AlignRight }
                    StatLabel { width: parent.width; text: "Scale: " + root.numberText(root.maxDistanceValue(), 2) + " m"; horizontalAlignment: Text.AlignRight }
                }
            }
        }
    }
}
