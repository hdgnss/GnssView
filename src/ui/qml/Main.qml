import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import "components"

ApplicationWindow {
    id: window
    visible: true
    flags: isWindows
           ? (Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint)
           : Qt.Window
    width: 1920
    height: 1080
    minimumWidth: 1360
    minimumHeight: 820
    color: isWindows ? "#07101d" : "black"
    title: "GnssView"
    readonly property bool isMac: Qt.platform.os === "osx"
    readonly property bool isWindows: Qt.platform.os === "windows"
    readonly property int chromeInsetTop: 0
    readonly property int themedHeaderHeight: isWindows ? 42 : 0
    readonly property int resizeBorder: isWindows && visibility !== Window.Maximized ? 6 : 0
    property bool geometryReady: false
    property bool restoringWindowState: false

    function openSettings() {
        settingsPanel.open()
    }

    function toggleMaximized() {
        visibility = visibility === Window.Maximized ? Window.Windowed : Window.Maximized
    }

    Theme {
        id: theme
    }

    Settings {
        id: windowSettings
        category: "MainWindow"
        property int widthValue: 1920
        property int heightValue: 1080
        property int xValue: 80
        property int yValue: 60
        property bool maximized: false
    }

    Action {
        id: openSettingsAction
        text: "Settings\u2026"
        shortcut: StandardKey.Preferences
        onTriggered: window.openSettings()
    }

    Action {
        id: checkUpdatesAction
        text: "Check for Updates..."
        onTriggered: if (updateChecker) updateChecker.checkForUpdates(false)
    }

    menuBar: window.isMac ? macMenuBar : null

    MenuBar {
        id: macMenuBar

        Menu {
            title: "GnssView"

            MenuItem {
                action: openSettingsAction
            }

            MenuItem {
                action: checkUpdatesAction
            }
        }
    }

    header: window.isWindows ? windowsHeader : null

    Rectangle {
        id: windowsHeader
        visible: window.isWindows
        implicitHeight: themedHeaderHeight
        color: "transparent"

        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: visibility === Window.Maximized ? 8 : 10
            anchors.rightMargin: visibility === Window.Maximized ? 8 : 10
            anchors.topMargin: visibility === Window.Maximized ? 8 : 8
            anchors.bottomMargin: 0
            height: themedHeaderHeight - (visibility === Window.Maximized ? 8 : 8)
            radius: visibility === Window.Maximized ? 10 : theme.radius
            color: Qt.rgba(theme.surface1.r, theme.surface1.g, theme.surface1.b, 0.84)
            border.width: 1
            border.color: Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, 0.18)

            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: parent.radius - 1
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(theme.panelOverlay.r, theme.panelOverlay.g, theme.panelOverlay.b, 0.62) }
                    GradientStop { position: 1.0; color: Qt.rgba(theme.surface0.r, theme.surface0.g, theme.surface0.b, 0.72) }
                }
            }

            MouseArea {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    rightMargin: titleActionRow.width + 18
                }
                acceptedButtons: Qt.LeftButton
                propagateComposedEvents: true
                onPressed: mouse => {
                    if (window.visibility !== Window.Maximized)
                        window.startSystemMove()
                }
                onDoubleClicked: window.toggleMaximized()
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                Label {
                    text: "GnssView"
                    color: theme.textPrimary
                    font.family: theme.titleFont
                    font.pixelSize: 15
                    font.bold: true
                }

                Item {
                    Layout.fillWidth: true
                }

                RowLayout {
                    id: titleActionRow
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 24
                        radius: theme.controlRadius
                        color: settingsMouseArea.pressed ? Qt.rgba(theme.surface4.r, theme.surface4.g, theme.surface4.b, 0.85)
                                                         : Qt.rgba(theme.surface0.r, theme.surface0.g, theme.surface0.b, settingsMouseArea.containsMouse ? 0.82 : 0.62)
                        border.width: 1
                        border.color: Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, settingsMouseArea.containsMouse ? 0.7 : 0.42)
                        ToolTip.visible: settingsMouseArea.containsMouse
                        ToolTip.delay: 450
                        ToolTip.text: "Settings"

                        Canvas {
                            anchors.centerIn: parent
                            width: 15
                            height: 15
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = theme.textPrimary
                                ctx.lineWidth = 1.35
                                ctx.lineCap = "round"
                                ctx.beginPath()
                                for (var i = 0; i < 8; ++i) {
                                    var angle = Math.PI * 2 * i / 8
                                    ctx.moveTo(7.5 + Math.cos(angle) * 5.4, 7.5 + Math.sin(angle) * 5.4)
                                    ctx.lineTo(7.5 + Math.cos(angle) * 6.8, 7.5 + Math.sin(angle) * 6.8)
                                }
                                ctx.stroke()
                                ctx.beginPath()
                                ctx.arc(7.5, 7.5, 4.6, 0, Math.PI * 2)
                                ctx.stroke()
                                ctx.beginPath()
                                ctx.arc(7.5, 7.5, 1.8, 0, Math.PI * 2)
                                ctx.stroke()
                            }
                        }

                        MouseArea {
                            id: settingsMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: window.openSettings()
                        }
                    }

                    Item {
                        Layout.preferredWidth: 20
                    }

                    Rectangle {
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 24
                        radius: theme.controlRadius
                        color: minMouseArea.pressed ? Qt.rgba(theme.surface4.r, theme.surface4.g, theme.surface4.b, 0.85)
                                                    : Qt.rgba(theme.surface0.r, theme.surface0.g, theme.surface0.b, minMouseArea.containsMouse ? 0.82 : 0.62)
                        border.width: 1
                        border.color: Qt.rgba(theme.lineStrong.r, theme.lineStrong.g, theme.lineStrong.b, 0.35)

                        Text {
                            anchors.centerIn: parent
                            text: "\u2212"
                            color: theme.textPrimary
                            font.family: theme.bodyFont
                            font.pixelSize: 16
                        }

                        MouseArea {
                            id: minMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: window.showMinimized()
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 24
                        radius: theme.controlRadius
                        color: maxMouseArea.pressed ? Qt.rgba(theme.surface4.r, theme.surface4.g, theme.surface4.b, 0.85)
                                                    : Qt.rgba(theme.surface0.r, theme.surface0.g, theme.surface0.b, maxMouseArea.containsMouse ? 0.82 : 0.62)
                        border.width: 1
                        border.color: Qt.rgba(theme.lineStrong.r, theme.lineStrong.g, theme.lineStrong.b, 0.35)

                        Canvas {
                            anchors.centerIn: parent
                            width: 11
                            height: 11
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = theme.textPrimary
                                ctx.lineWidth = 1.2
                                if (window.visibility === Window.Maximized) {
                                    ctx.strokeRect(1, 3, 8, 8)
                                    ctx.strokeRect(3, 1, 8, 8)
                                } else {
                                    ctx.strokeRect(1, 1, 10, 10)
                                }
                            }
                        }

                        MouseArea {
                            id: maxMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: window.toggleMaximized()
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 24
                        radius: theme.controlRadius
                        color: closeMouseArea.pressed ? Qt.rgba(theme.bad.r, theme.bad.g, theme.bad.b, 0.95)
                                                      : Qt.rgba(theme.bad.r, theme.bad.g, theme.bad.b, closeMouseArea.containsMouse ? 0.82 : 0.38)
                        border.width: 1
                        border.color: Qt.rgba(theme.bad.r, theme.bad.g, theme.bad.b, 0.45)

                        Text {
                            anchors.centerIn: parent
                            text: "\u00d7"
                            color: theme.textPrimary
                            font.family: theme.bodyFont
                            font.pixelSize: 14
                        }

                        MouseArea {
                            id: closeMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: window.close()
                        }
                    }
                }
            }
        }
    }

    function restoreWindowState() {
        restoringWindowState = true
        if (appSettings && !appSettings.rememberWindowGeometry) {
            geometryReady = true
            restoringWindowState = false
            return
        }
        width = Math.max(minimumWidth, windowSettings.widthValue || 1920)
        height = Math.max(minimumHeight, windowSettings.heightValue || 1080)
        x = Math.max(0, windowSettings.xValue || 80)
        y = Math.max(0, windowSettings.yValue || 60)
        Qt.callLater(function() {
            if (windowSettings.maximized) {
                visibility = Window.Maximized
            }
            geometryReady = true
            restoringWindowState = false
        })
    }

    function persistWindowState() {
        if (appSettings && !appSettings.rememberWindowGeometry) {
            return
        }
        if (!geometryReady || visibility === Window.Maximized || visibility === Window.FullScreen) {
            return
        }
        windowSettings.widthValue = width
        windowSettings.heightValue = height
        windowSettings.xValue = x
        windowSettings.yValue = y
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: theme.bgStart }
            GradientStop { position: 1.0; color: theme.bgEnd }
        }
    }

    Canvas {
        anchors.fill: parent
        opacity: 0.16
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = theme.backgroundGrid
            ctx.lineWidth = 1
            var step = 48
            for (var x = 0; x <= width; x += step) {
                ctx.beginPath()
                ctx.moveTo(x, 0)
                ctx.lineTo(x, height)
                ctx.stroke()
            }
            for (var y = 0; y <= height; y += step) {
                ctx.beginPath()
                ctx.moveTo(0, y)
                ctx.lineTo(width, y)
                ctx.stroke()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: visibility === Window.Maximized ? 8 : 12
        anchors.rightMargin: visibility === Window.Maximized ? 8 : 12
        anchors.bottomMargin: visibility === Window.Maximized ? 8 : 12
        anchors.topMargin: window.isMac ? (8 + chromeInsetTop) : 0
        radius: visibility === Window.Maximized ? 8 : theme.shellRadius
        color: theme.shellGlass
        border.width: 1
        border.color: theme.shellOutline
    }

    Item {
        id: grid
        anchors.fill: parent
        anchors.leftMargin: visibility === Window.Maximized ? 14 : 18
        anchors.rightMargin: visibility === Window.Maximized ? 14 : 18
        anchors.bottomMargin: visibility === Window.Maximized ? 14 : 18
        anchors.topMargin: window.isMac ? (12 + chromeInsetTop) : 2
        readonly property int gap: 10
        readonly property real unitWidth: (width - gap * 3) / 4
        readonly property real unitHeight: (height - gap * 2) / 3

        function cellX(column) {
            return column * (unitWidth + gap)
        }

        function cellY(row) {
            return row * (unitHeight + gap)
        }

        function spanWidth(span) {
            return unitWidth * span + gap * (span - 1)
        }

        function spanHeight(span) {
            return unitHeight * span + gap * (span - 1)
        }

        TransportConfigPanel {
            x: grid.cellX(0)
            y: grid.cellY(0)
            width: grid.unitWidth
            height: grid.unitHeight
        }

        DataPanel {
            title: "Data & Navigation"
            x: grid.cellX(1)
            y: grid.cellY(0)
            width: grid.spanWidth(2)
            height: grid.unitHeight
        }

        SkyViewPanel {
            x: grid.cellX(3)
            y: grid.cellY(0)
            width: grid.unitWidth
            height: grid.unitHeight
        }

        CommandButtonsPanel {
            groupHint: "Command Center"
            x: grid.cellX(0)
            y: grid.cellY(1)
            width: grid.unitWidth
            height: grid.spanHeight(2)
        }

        AutoTestPanel {
            x: grid.cellX(1)
            y: grid.cellY(1)
            width: grid.unitWidth
            height: grid.unitHeight
        }

        WorldLocationPanel {
            x: grid.cellX(2)
            y: grid.cellY(1)
            width: grid.unitWidth
            height: grid.unitHeight
            showIonex: true
            ionexOverlaySource: tecMapOverlayModel ? tecMapOverlayModel.overlaySource : ""
        }

        DeviationMapPanel {
            x: grid.cellX(3)
            y: grid.cellY(1)
            width: grid.unitWidth
            height: grid.unitHeight
        }

        SignalBarsPanel {
            title: "Signal Spectrum"
            defaultBand: "L1"
            x: grid.cellX(1)
            y: grid.cellY(2)
            width: grid.spanWidth(3)
            height: grid.unitHeight
        }
    }

    SettingsPanel {
        id: settingsPanel
    }

    Connections {
        target: updateChecker

        function onUpdateAvailable(version, releaseUrl, releaseNotes) {
            updateDialog.version = version
            updateDialog.releaseUrl = releaseUrl
            updateDialog.releaseNotes = releaseNotes || ""
            updateDialog.open()
        }
    }

    Popup {
        id: updateDialog
        parent: Overlay.overlay
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        property string version: ""
        property string releaseUrl: ""
        property string releaseNotes: ""
        width: Math.max(360, Math.min(560, window.width - 60))
        height: Math.max(230, Math.min(window.height - 60, updateDialog.releaseNotes.length > 0 ? 430 : 260))
        x: Math.max(30, (window.width - width) * 0.5)
        y: Math.max(30, (window.height - height) * 0.5)
        padding: 0

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
                Layout.preferredHeight: 52
                radius: theme.radius
                color: theme.panelBase
                border.width: 1
                border.color: theme.panelBorderStrong

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 28
                        radius: 4
                        color: theme.accentStrong
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: "Update Available"
                            color: theme.textPrimary
                            font.family: theme.titleFont
                            font.pixelSize: theme.titleSize
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: updateChecker ? ("Current " + updateChecker.currentVersion) : ""
                            color: theme.textSecondary
                            font.family: theme.bodyFont
                            font.pixelSize: theme.bodySize
                            elide: Text.ElideRight
                        }
                    }

                    NeonButton {
                        text: "Later"
                        accent: theme.bad
                        onClicked: updateDialog.close()
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: "GnssView " + updateDialog.version + " is available."
                color: theme.textPrimary
                font.family: theme.titleFont
                font.pixelSize: theme.bodySize + 2
                font.bold: true
                wrapMode: Text.WordWrap
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: updateDialog.releaseNotes.length > 0
                clip: true

                TextArea {
                    text: updateDialog.releaseNotes
                    readOnly: true
                    wrapMode: Text.WordWrap
                    color: theme.textSecondary
                    font.family: theme.bodyFont
                    font.pixelSize: theme.bodySize
                    background: Rectangle {
                        color: theme.inputBg
                        radius: theme.controlRadius
                        border.width: 1
                        border.color: theme.inputBorder
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Item {
                    Layout.fillWidth: true
                }

                NeonButton {
                    text: "Release"
                    onClicked: {
                        if (updateChecker) updateChecker.openReleasePage()
                        updateDialog.close()
                    }
                }

                NeonButton {
                    text: "Download"
                    accent: theme.ok
                    onClicked: {
                        if (updateChecker) updateChecker.openDownloadPage()
                        updateDialog.close()
                    }
                }
            }
        }
    }

    MouseArea {
        visible: window.isWindows && window.resizeBorder > 0
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: window.resizeBorder
        cursorShape: Qt.SizeHorCursor
        onPressed: mouse => window.startSystemResize(Qt.LeftEdge)
    }

    MouseArea {
        visible: window.isWindows && window.resizeBorder > 0
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: window.resizeBorder
        cursorShape: Qt.SizeHorCursor
        onPressed: mouse => window.startSystemResize(Qt.RightEdge)
    }

    MouseArea {
        visible: window.isWindows && window.resizeBorder > 0
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: window.resizeBorder
        cursorShape: Qt.SizeVerCursor
        onPressed: mouse => window.startSystemResize(Qt.TopEdge)
    }

    MouseArea {
        visible: window.isWindows && window.resizeBorder > 0
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: window.resizeBorder
        cursorShape: Qt.SizeVerCursor
        onPressed: mouse => window.startSystemResize(Qt.BottomEdge)
    }

    MouseArea {
        visible: window.isWindows && window.resizeBorder > 0
        anchors.left: parent.left
        anchors.top: parent.top
        width: window.resizeBorder
        height: window.resizeBorder
        cursorShape: Qt.SizeFDiagCursor
        onPressed: mouse => window.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
    }

    MouseArea {
        visible: window.isWindows && window.resizeBorder > 0
        anchors.right: parent.right
        anchors.top: parent.top
        width: window.resizeBorder
        height: window.resizeBorder
        cursorShape: Qt.SizeBDiagCursor
        onPressed: mouse => window.startSystemResize(Qt.RightEdge | Qt.TopEdge)
    }

    MouseArea {
        visible: window.isWindows && window.resizeBorder > 0
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: window.resizeBorder
        height: window.resizeBorder
        cursorShape: Qt.SizeBDiagCursor
        onPressed: mouse => window.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
    }

    MouseArea {
        visible: window.isWindows && window.resizeBorder > 0
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: window.resizeBorder
        height: window.resizeBorder
        cursorShape: Qt.SizeFDiagCursor
        onPressed: mouse => window.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }

    onWidthChanged: persistWindowState()
    onHeightChanged: persistWindowState()
    onXChanged: persistWindowState()
    onYChanged: persistWindowState()
    onVisibilityChanged: {
        if (!geometryReady || restoringWindowState) {
            return
        }
        if (appSettings && !appSettings.rememberWindowGeometry) {
            return
        }
        windowSettings.maximized = window.visibility === Window.Maximized
        if (window.visibility !== Window.Maximized && window.visibility !== Window.FullScreen) {
            persistWindowState()
        }
    }
    onClosing: close => {
        if (appSettings && !appSettings.rememberWindowGeometry) {
            return
        }
        windowSettings.maximized = window.visibility === Window.Maximized
        if (window.visibility !== Window.Maximized && window.visibility !== Window.FullScreen) {
            persistWindowState()
        }
    }
    Component.onCompleted: Qt.callLater(restoreWindowState)
}
