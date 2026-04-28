import QtQuick

Rectangle {
    id: root

    property color accent: theme.panelBorderStrong
    property bool showSunlight: true
    property bool showIonex: false
    property bool showSubpoints: false
    property url ionexOverlaySource: ""
    property url subpointOverlaySource: ""
    property date nowUtc: new Date()
    property var selectedIonexSample: ({})
    readonly property var tecSources: tecMapOverlayModel ? tecMapOverlayModel.activeTecSources : []
    readonly property int activeTecSourceIndex: tecMapOverlayModel ? tecMapOverlayModel.activeSourceIndex : -1

    readonly property real latitude: appController ? appController.latitude : NaN
    readonly property real longitude: appController ? appController.longitude : NaN
    readonly property bool positionValid: appController
        ? appController.quality > 0 && isFinite(latitude) && isFinite(longitude)
        : false
    readonly property bool ionexReady: ionexOverlaySource.toString().length > 0
    readonly property bool subpointReady: subpointOverlaySource.toString().length > 0
    readonly property string ionexStatusText: tecMapOverlayModel ? tecMapOverlayModel.statusText : ""
    readonly property string ionexDatasetLabel: tecMapOverlayModel ? tecMapOverlayModel.datasetLabel : ""
    readonly property real worldMinY: -91.296
    readonly property real worldHeight: 182.592
    readonly property var robinsonX: [
        1.0, 0.9986, 0.9954, 0.99, 0.9822, 0.973, 0.96, 0.9427, 0.9216,
        0.8962, 0.8679, 0.835, 0.7986, 0.7597, 0.7186, 0.6732, 0.6213,
        0.5722, 0.5322
    ]
    readonly property var robinsonY: [
        0.0, 0.062, 0.124, 0.186, 0.248, 0.31, 0.372, 0.434, 0.4958,
        0.5571, 0.6176, 0.6769, 0.7346, 0.7903, 0.8435, 0.8936, 0.9394,
        0.9761, 1.0
    ]

    Theme {
        id: theme
    }

    radius: theme.radius
    border.width: 1
    border.color: theme.panelBorderStrong
    color: theme.panelBase
    clip: true
    antialiasing: true

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function lerp(a, b, t) {
        return a + (b - a) * t
    }

    function degreesToRadians(value) {
        return value * Math.PI / 180.0
    }

    function radiansToDegrees(value) {
        return value * 180.0 / Math.PI
    }

    function wrapDegrees(value) {
        var wrapped = value % 360
        if (wrapped < 0) {
            wrapped += 360
        }
        return wrapped
    }

    function wrapLongitude(value) {
        return ((value + 180) % 360 + 360) % 360 - 180
    }

    function formatSigned(value, positiveSuffix, negativeSuffix, digits) {
        if (!isFinite(value)) {
            return "--"
        }
        var suffix = value >= 0 ? positiveSuffix : negativeSuffix
        return Math.abs(value).toFixed(digits) + "°" + suffix
    }

    function selectedIonexText() {
        if (!selectedIonexSample || !selectedIonexSample.valid) {
            return ""
        }

        var lonText = formatSigned(Number(selectedIonexSample.longitude), "E", "W", 2)
        var latText = formatSigned(Number(selectedIonexSample.latitude), "N", "S", 2)
        var tecText = Number(selectedIonexSample.tec).toFixed(2)
        return "Selected " + latText + ", " + lonText
            + " | TEC " + tecText
            + " | QF " + Number(selectedIonexSample.qualityFlag || 0)
    }

    function tecSourceTabText(source, index) {
        return source.displayName || source.pluginId || ("Source " + (index + 1))
    }

    function robinsonProject(lonDeg, latDeg) {
        if (!isFinite(lonDeg) || !isFinite(latDeg)) {
            return Qt.point(NaN, NaN)
        }

        var absLat = clamp(Math.abs(latDeg), 0, 90)
        var bandIndex = Math.min(robinsonX.length - 2, Math.floor(absLat / 5))
        var bandStart = bandIndex * 5
        var ratio = (absLat - bandStart) / 5
        var xFactor = lerp(robinsonX[bandIndex], robinsonX[bandIndex + 1], ratio)
        var yFactor = lerp(robinsonY[bandIndex], robinsonY[bandIndex + 1], ratio)
        var x = lonDeg * xFactor
        var y = (latDeg >= 0 ? -1 : 1) * 90 * 1.0144 * yFactor
        return Qt.point(x, y)
    }

    function projectedCanvasPoint(widthValue, heightValue, lonDeg, latDeg) {
        if (widthValue <= 0 || heightValue <= 0) {
            return Qt.point(NaN, NaN)
        }

        var projected = robinsonProject(lonDeg, latDeg)
        if (!isFinite(projected.x) || !isFinite(projected.y)) {
            return Qt.point(NaN, NaN)
        }

        return Qt.point(
            (projected.x + 180) / 360 * widthValue,
            (projected.y - worldMinY) / worldHeight * heightValue
        )
    }

    function julianDay(dateValue) {
        var year = dateValue.getUTCFullYear()
        var month = dateValue.getUTCMonth() + 1
        var day = dateValue.getUTCDate()
            + (dateValue.getUTCHours()
               + dateValue.getUTCMinutes() / 60
               + dateValue.getUTCSeconds() / 3600
               + dateValue.getUTCMilliseconds() / 3600000) / 24

        if (month <= 2) {
            year -= 1
            month += 12
        }

        var a = Math.floor(year / 100)
        var b = 2 - a + Math.floor(a / 4)
        return Math.floor(365.25 * (year + 4716))
            + Math.floor(30.6001 * (month + 1))
            + day + b - 1524.5
    }

    function solarGeometry(dateValue) {
        var jd = julianDay(dateValue)
        var t = (jd - 2451545.0) / 36525.0
        var l0 = wrapDegrees(280.46646 + 36000.76983 * t + 0.0003032 * t * t)
        var m = wrapDegrees(357.52911 + 35999.05029 * t - 0.0001537 * t * t)
        var mRad = degreesToRadians(m)
        var c = (1.914602 - t * (0.004817 + 0.000014 * t)) * Math.sin(mRad)
            + (0.019993 - 0.000101 * t) * Math.sin(2 * mRad)
            + 0.000289 * Math.sin(3 * mRad)
        var trueLongitude = l0 + c
        var omega = 125.04 - 1934.136 * t
        var apparentLongitude = trueLongitude - 0.00569 - 0.00478 * Math.sin(degreesToRadians(omega))
        var epsilon0 = 23
            + (26 + (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60) / 60
        var epsilon = epsilon0 + 0.00256 * Math.cos(degreesToRadians(omega))
        var lambdaRad = degreesToRadians(apparentLongitude)
        var epsilonRad = degreesToRadians(epsilon)
        var rightAscension = radiansToDegrees(Math.atan2(
            Math.cos(epsilonRad) * Math.sin(lambdaRad),
            Math.cos(lambdaRad)
        ))
        var declination = radiansToDegrees(Math.asin(Math.sin(epsilonRad) * Math.sin(lambdaRad)))
        var gmst = wrapDegrees(
            280.46061837
            + 360.98564736629 * (jd - 2451545.0)
            + 0.000387933 * t * t
            - t * t * t / 38710000.0
        )

        return {
            declination: declination,
            subsolarLongitude: wrapLongitude(rightAscension - gmst)
        }
    }

    function daylightRanges(latDeg, subsolarLongitude, declination) {
        var latitudeRad = degreesToRadians(latDeg)
        var declinationRad = degreesToRadians(declination)
        var argument = -Math.tan(latitudeRad) * Math.tan(declinationRad)

        if (argument <= -1) {
            return [{ start: -180, end: 180 }]
        }
        if (argument >= 1) {
            return []
        }

        var span = radiansToDegrees(Math.acos(argument))
        var start = subsolarLongitude - span
        var end = subsolarLongitude + span

        if (start < -180) {
            return [
                { start: start + 360, end: 180 },
                { start: -180, end: end }
            ]
        }
        if (end > 180) {
            return [
                { start: start, end: 180 },
                { start: -180, end: end - 360 }
            ]
        }

        return [{ start: start, end: end }]
    }

    function drawProjectedBand(ctx, widthValue, heightValue, lat0, lat1, lonStart, lonEnd) {
        if (lonEnd <= lonStart) {
            return
        }

        var step = 3
        var first = true
        ctx.beginPath()

        for (var lon = lonStart; lon <= lonEnd + 0.001; lon += step) {
            var p0 = projectedCanvasPoint(widthValue, heightValue, lon > lonEnd ? lonEnd : lon, lat0)
            if (!isFinite(p0.x) || !isFinite(p0.y)) {
                continue
            }
            if (first) {
                ctx.moveTo(p0.x, p0.y)
                first = false
            } else {
                ctx.lineTo(p0.x, p0.y)
            }
        }

        var edgeTop = projectedCanvasPoint(widthValue, heightValue, lonEnd, lat0)
        ctx.lineTo(edgeTop.x, edgeTop.y)

        for (var lonBack = lonEnd; lonBack >= lonStart - 0.001; lonBack -= step) {
            var p1 = projectedCanvasPoint(widthValue, heightValue, lonBack < lonStart ? lonStart : lonBack, lat1)
            if (!isFinite(p1.x) || !isFinite(p1.y)) {
                continue
            }
            ctx.lineTo(p1.x, p1.y)
        }

        var edgeBottom = projectedCanvasPoint(widthValue, heightValue, lonStart, lat1)
        ctx.lineTo(edgeBottom.x, edgeBottom.y)
        ctx.closePath()
        ctx.fill()
    }

    Timer {
        interval: 1000
        repeat: true
        running: root.showSunlight && root.positionValid
        onTriggered: root.nowUtc = new Date()
    }

    Item {
        id: mapBounds
        anchors.fill: parent
        anchors.margins: theme.panelContentMargin

        readonly property real contentWidth: basemap.status === Image.Ready && basemap.paintedWidth > 0
            ? basemap.paintedWidth
            : width
        readonly property real contentHeight: basemap.status === Image.Ready && basemap.paintedHeight > 0
            ? basemap.paintedHeight
            : height
        readonly property real contentX: (width - contentWidth) * 0.5
        readonly property real contentY: (height - contentHeight) * 0.5
        readonly property point livePoint: root.projectedCanvasPoint(contentWidth, contentHeight, root.longitude, root.latitude)

        onContentWidthChanged: sunlightLayer.requestPaint()
        onContentHeightChanged: sunlightLayer.requestPaint()

        NeonTabBar {
            id: tecTabs
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: implicitHeight
            visible: root.showIonex && tecSources && tecSources.length > 0
            currentIndex: Math.max(0, activeTecSourceIndex)
            z: 12

            Repeater {
                model: tecSources

                delegate: NeonTabButton {
                    required property var modelData
                    required property int index

                    text: root.tecSourceTabText(modelData, index)
                    minButtonWidth: 96
                    enabled: !!tecMapOverlayModel
                    onClicked: tecMapOverlayModel.setActiveSourceIndex(index)
                }
            }
        }

        Image {
            id: basemap
            anchors.fill: parent
            source: "assets/worldUltra.svg"
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
        }

        Canvas {
            id: sunlightLayer
            x: mapBounds.contentX
            y: mapBounds.contentY
            width: mapBounds.contentWidth
            height: mapBounds.contentHeight
            visible: root.showSunlight && root.positionValid
            opacity: 1.0

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                if (!root.showSunlight || !root.positionValid || width <= 0 || height <= 0) {
                    return
                }

                var solar = root.solarGeometry(root.nowUtc)
                ctx.fillStyle = "rgba(116, 190, 255, 0.16)"
                for (var lat = -88; lat < 88; lat += 4) {
                    var midLat = lat + 2
                    var ranges = root.daylightRanges(midLat, solar.subsolarLongitude, solar.declination)
                    for (var i = 0; i < ranges.length; ++i) {
                        var range = ranges[i]
                        root.drawProjectedBand(ctx, width, height, lat, lat + 4, range.start, range.end)
                    }
                }

                var subsolarPoint = root.projectedCanvasPoint(
                    width,
                    height,
                    solar.subsolarLongitude,
                    solar.declination
                )
                if (isFinite(subsolarPoint.x) && isFinite(subsolarPoint.y)) {
                    var glow = ctx.createRadialGradient(
                        subsolarPoint.x,
                        subsolarPoint.y,
                        8,
                        subsolarPoint.x,
                        subsolarPoint.y,
                        72
                    )
                    glow.addColorStop(0.0, "rgba(144, 214, 255, 0.36)")
                    glow.addColorStop(0.45, "rgba(106, 186, 255, 0.16)")
                    glow.addColorStop(1.0, "rgba(106, 186, 255, 0.0)")
                    ctx.fillStyle = glow
                    ctx.beginPath()
                    ctx.arc(subsolarPoint.x, subsolarPoint.y, 72, 0, Math.PI * 2)
                    ctx.fill()
                }
            }
        }

        Image {
            x: mapBounds.contentX
            y: mapBounds.contentY
            width: mapBounds.contentWidth
            height: mapBounds.contentHeight
            source: root.ionexOverlaySource
            visible: root.showIonex && root.ionexReady
            fillMode: Image.PreserveAspectFit
            opacity: 0.42
            smooth: true
        }

        Image {
            x: mapBounds.contentX
            y: mapBounds.contentY
            width: mapBounds.contentWidth
            height: mapBounds.contentHeight
            source: root.subpointOverlaySource
            visible: root.showSubpoints && root.subpointReady
            fillMode: Image.PreserveAspectFit
            opacity: 0.5
            smooth: true
        }

        MouseArea {
            anchors.fill: parent
            enabled: root.showIonex && root.ionexReady && tecMapOverlayModel
            hoverEnabled: false
            acceptedButtons: Qt.LeftButton
            z: 11

            onClicked: function(mouse) {
                var localX = mouse.x - mapBounds.contentX
                var localY = mouse.y - mapBounds.contentY
                if (localX < 0 || localY < 0
                        || localX > mapBounds.contentWidth
                        || localY > mapBounds.contentHeight) {
                    return
                }

                var sample = tecMapOverlayModel.sampleAtCanvasPoint(
                    localX,
                    localY,
                    mapBounds.contentWidth,
                    mapBounds.contentHeight
                )
                if (sample && sample.valid) {
                    root.selectedIonexSample = sample
                }
            }
        }

        Rectangle {
            width: 26
            height: 26
            radius: 13
            visible: root.positionValid && isFinite(mapBounds.livePoint.x) && isFinite(mapBounds.livePoint.y)
            x: mapBounds.contentX + mapBounds.livePoint.x - width * 0.5
            y: mapBounds.contentY + mapBounds.livePoint.y - height * 0.5
            color: Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, 0.10)
            border.width: 1
            border.color: Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, 0.38)

            SequentialAnimation on scale {
                loops: Animation.Infinite
                running: root.positionValid
                NumberAnimation { from: 0.78; to: 1.18; duration: 1400; easing.type: Easing.OutQuad }
                NumberAnimation { from: 1.18; to: 0.78; duration: 1400; easing.type: Easing.InQuad }
            }
        }

        Rectangle {
            width: 10
            height: 10
            radius: 5
            visible: root.positionValid && isFinite(mapBounds.livePoint.x) && isFinite(mapBounds.livePoint.y)
            x: mapBounds.contentX + mapBounds.livePoint.x - width * 0.5
            y: mapBounds.contentY + mapBounds.livePoint.y - height * 0.5
            color: theme.accentStrong
            border.width: 1
            border.color: theme.textPrimary
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: 10
            }
            height: ionexInfoText.implicitHeight + 10
            radius: 8
            color: Qt.rgba(theme.surface0.r, theme.surface0.g, theme.surface0.b, 0.72)
            border.width: 1
            border.color: Qt.rgba(theme.panelBorderStrong.r, theme.panelBorderStrong.g, theme.panelBorderStrong.b, 0.28)
            visible: root.showIonex
            z: 12

            Text {
                id: ionexInfoText
                anchors.fill: parent
                anchors.margins: 5
                color: theme.textSecondary
                font.family: uiMonoFontFamily
                font.pixelSize: 11
                wrapMode: Text.Wrap
                text: {
                    var lines = []
                    if (ionexReady && ionexDatasetLabel.length > 0) {
                        lines.push(ionexDatasetLabel)
                    } else if (ionexStatusText.length > 0) {
                        lines.push(ionexStatusText)
                    }

                    var selectedText = root.selectedIonexText()
                    if (selectedText.length > 0) {
                        lines.push(selectedText)
                    } else if (root.showIonex && root.ionexReady) {
                        lines.push("Click map to inspect raw TEC cell")
                    }
                    return lines.join("\n")
                }
            }
        }
    }

    Connections {
        target: appController

        function onLocationChanged() {
            sunlightLayer.requestPaint()
        }
    }

    onIonexOverlaySourceChanged: selectedIonexSample = ({})
    onNowUtcChanged: sunlightLayer.requestPaint()
    onShowSunlightChanged: sunlightLayer.requestPaint()
}
