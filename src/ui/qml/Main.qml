import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import "components"

ApplicationWindow {
    id: window
    visible: true
    flags: useThemedChrome
           ? (Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint)
           : Qt.Window
    width: 1280
    height: 720
    minimumWidth: 1280
    minimumHeight: 720
    color: theme.bgEnd
    title: "GnssView"
    readonly property bool isMac: Qt.platform.os === "osx"
    readonly property bool isWindows: Qt.platform.os === "windows"
    readonly property bool isLinux: Qt.platform.os === "linux"
    readonly property bool useThemedChrome: isWindows || isLinux
    readonly property int chromeInsetTop: 0
    readonly property int themedHeaderHeight: useThemedChrome ? 38 : 0
    readonly property int resizeBorder: useThemedChrome && visibility !== Window.Maximized && visibility !== Window.FullScreen ? 10 : 0
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
        property int widthValue: 1280
        property int heightValue: 720
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
        text: "Check for Updates\u2026"
        onTriggered: if (updateChecker) updateChecker.checkForUpdates(false)
    }

    Action {
        id: toggleThemeAction
        text: theme.isDark ? "Light Theme" : "Dark Theme"
        shortcut: "Ctrl+T"
        onTriggered: {
            if (appSettings) {
                appSettings.setUiTheme(theme.isDark ? "light" : "dark")
            }
        }
    }

    Action {
        id: toggleDensityAction
        text: theme.isCompact ? "Comfortable Density" : "Compact Density"
        shortcut: "Ctrl+D"
        onTriggered: {
            theme.isCompact = !theme.isCompact
        }
    }

    Action {
        id: toggleFullscreenAction
        text: window.visibility === Window.FullScreen ? "Exit Fullscreen" : "Fullscreen"
        shortcut: StandardKey.FullScreen
        onTriggered: {
            if (window.visibility === Window.FullScreen) {
                window.visibility = windowSettings.maximized ? Window.Maximized : Window.Windowed
            } else {
                window.visibility = Window.FullScreen
            }
        }
    }

    Action {
        id: aboutAction
        text: "About GnssView\u2026"
        onTriggered: aboutDialog.open()
    }

    Action {
        id: quitAction
        text: "Quit"
        shortcut: StandardKey.Quit
        onTriggered: window.close()
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

            MenuSeparator {}

            MenuItem {
                action: quitAction
            }
        }

        Menu {
            title: "View"

            MenuItem {
                action: toggleThemeAction
            }

            MenuItem {
                action: toggleDensityAction
            }

            MenuSeparator {}

            MenuItem {
                action: toggleFullscreenAction
            }
        }

        Menu {
            title: "Help"

            MenuItem {
                action: checkUpdatesAction
            }

            MenuItem {
                action: aboutAction
            }
        }
    }

    header: window.useThemedChrome ? themedHeader : null
    footer: AppStatusBar {
        id: appStatusBar
    }

    Rectangle {
        id: themedHeader
        visible: window.useThemedChrome
        implicitHeight: themedHeaderHeight
        color: "transparent"

        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: visibility === Window.Maximized ? 8 : 10
            anchors.rightMargin: visibility === Window.Maximized ? 8 : 10
            anchors.topMargin: visibility === Window.Maximized ? 2 : 3
            anchors.bottomMargin: visibility === Window.Maximized ? 2 : 3
            height: themedHeaderHeight - (visibility === Window.Maximized ? 4 : 6)
            radius: visibility === Window.Maximized ? 8 : theme.radius
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

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                Label {
                    id: appTitleLabel
                    text: "GnssView"
                    color: theme.textPrimary
                    font.family: theme.titleFont
                    font.pixelSize: 15
                    font.bold: true
                }

                AppMenuBar {
                    id: winMenuBar
                    visible: !window.isMac
                    Layout.alignment: Qt.AlignVCenter

                    AppMenu {
                        title: "File"
                        AppMenuItem { action: openSettingsAction }
                        AppMenuSeparator {}
                        AppMenuItem { action: quitAction }
                    }

                    AppMenu {
                        title: "View"
                        AppMenuItem { action: toggleThemeAction }
                        AppMenuItem { action: toggleDensityAction }
                        AppMenuSeparator {}
                        AppMenuItem { action: toggleFullscreenAction }
                    }

                    AppMenu {
                        title: "Help"
                        AppMenuItem { action: checkUpdatesAction }
                        AppMenuItem { action: aboutAction }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onPressed: mouse => {
                            if (window.visibility !== Window.Maximized)
                                window.startSystemMove()
                        }
                        onDoubleClicked: window.toggleMaximized()
                    }
                }

                RowLayout {
                    id: titleActionRow
                    spacing: 8

                    Rectangle {
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 24
                        radius: theme.controlRadius
                        color: themeToggleMa.pressed ? Qt.rgba(theme.surface4.r, theme.surface4.g, theme.surface4.b, 0.85)
                                                     : Qt.rgba(theme.surface0.r, theme.surface0.g, theme.surface0.b, themeToggleMa.containsMouse ? 0.82 : 0.62)
                        border.width: 1
                        border.color: Qt.rgba(theme.accentStrong.r, theme.accentStrong.g, theme.accentStrong.b, themeToggleMa.containsMouse ? 0.7 : 0.42)
                        ToolTip.visible: themeToggleMa.containsMouse
                        ToolTip.delay: 450
                        ToolTip.text: theme.isDark ? "Switch to Light Theme (Ctrl+T)" : "Switch to Dark Theme (Ctrl+T)"

                        Canvas {
                            id: themeCanvas
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.fillStyle = theme.textPrimary
                                ctx.strokeStyle = theme.textPrimary
                                ctx.lineWidth = 1.3
                                ctx.lineCap = "round"
                                if (theme.isDark) {
                                    ctx.beginPath()
                                    ctx.arc(7, 7, 3.2, 0, Math.PI * 2)
                                    ctx.stroke()
                                    for (var i = 0; i < 8; ++i) {
                                        var a = i * Math.PI / 4
                                        ctx.beginPath()
                                        ctx.moveTo(7 + Math.cos(a) * 4.6, 7 + Math.sin(a) * 4.6)
                                        ctx.lineTo(7 + Math.cos(a) * 6.3, 7 + Math.sin(a) * 6.3)
                                        ctx.stroke()
                                    }
                                } else {
                                    ctx.beginPath()
                                    ctx.arc(6.5, 7, 4.5, -0.4 * Math.PI, 0.8 * Math.PI, false)
                                    ctx.arc(8, 5.5, 3.5, 0.8 * Math.PI, -0.4 * Math.PI, true)
                                    ctx.closePath()
                                    ctx.stroke()
                                }
                            }
                            Connections {
                                target: theme
                                function onIsDarkChanged() {
                                    themeCanvas.requestPaint()
                                }
                            }
                        }

                        MouseArea {
                            id: themeToggleMa
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: toggleThemeAction.trigger()
                        }
                    }

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
                            id: settingsCanvas
                            anchors.centerIn: parent
                            width: 15
                            height: 15
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = theme.textPrimary
                                ctx.lineWidth = 1.45
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

                            Connections {
                                target: theme
                                function onIsDarkChanged() {
                                    settingsCanvas.requestPaint()
                                }
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
                            id: maxCanvas
                            anchors.centerIn: parent
                            width: 11
                            height: 11
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = theme.textPrimary
                                ctx.lineWidth = 1.35
                                if (window.visibility === Window.Maximized) {
                                    ctx.strokeRect(1, 3, 7, 7)
                                    ctx.strokeRect(3, 1, 7, 7)
                                } else {
                                    ctx.strokeRect(1, 1, 9, 9)
                                }
                            }

                            Connections {
                                target: theme
                                function onIsDarkChanged() {
                                    maxCanvas.requestPaint()
                                }
                            }

                            Connections {
                                target: window
                                function onVisibilityChanged() {
                                    maxCanvas.requestPaint()
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
                                                      : Qt.rgba(theme.bad.r, theme.bad.g, theme.bad.b, closeMouseArea.containsMouse ? 0.82 : (theme.isDark ? 0.38 : 0.20))
                        border.width: 1
                        border.color: Qt.rgba(theme.bad.r, theme.bad.g, theme.bad.b, theme.isDark ? 0.45 : 0.40)

                        Text {
                            anchors.centerIn: parent
                            text: "\u00d7"
                            color: closeMouseArea.containsMouse ? "#ffffff" : (theme.isDark ? theme.textPrimary : theme.bad)
                            font.family: theme.bodyFont
                            font.pixelSize: 15
                            font.bold: true
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
        var targetW = Math.max(minimumWidth, windowSettings.widthValue || 1280)
        var targetH = Math.max(minimumHeight, windowSettings.heightValue || 720)
        var targetX = windowSettings.xValue !== undefined ? windowSettings.xValue : 80
        var targetY = windowSettings.yValue !== undefined ? windowSettings.yValue : 60

        // Multi-monitor support: preserve negative coordinates if window intersects any active screen
        var intersectsScreen = false
        var screens = (typeof Qt.application.screens !== "undefined") ? Qt.application.screens : []
        if (screens && screens.length > 0) {
            for (var i = 0; i < screens.length; ++i) {
                var s = screens[i]
                var sLeft = s.virtualX !== undefined ? s.virtualX : 0
                var sTop = s.virtualY !== undefined ? s.virtualY : 0
                var sRight = sLeft + s.width
                var sBottom = sTop + s.height

                if (targetX + targetW > sLeft + 100 && targetX < sRight - 100 &&
                    targetY + targetH > sTop + 100 && targetY < sBottom - 100) {
                    intersectsScreen = true
                    break
                }
            }
        } else {
            intersectsScreen = true
        }

        if (!intersectsScreen) {
            targetX = 80
            targetY = 60
        }

        width = targetW
        height = targetH
        x = targetX
        y = targetY
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
            GradientStop { position: 0.55; color: theme.bgMid }
            GradientStop { position: 1.0; color: theme.bgEnd }
        }
    }

    Canvas {
        id: backgroundGridCanvas
        anchors.fill: parent
        opacity: 0.24
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.lineWidth = 1
            var minorStep = 24
            var majorStep = 96

            for (var x = 0; x <= width; x += minorStep) {
                ctx.globalAlpha = x % majorStep === 0 ? 0.55 : 0.18
                ctx.strokeStyle = x % majorStep === 0 ? theme.chartGridMajor : theme.backgroundGrid
                ctx.beginPath()
                ctx.moveTo(x, 0)
                ctx.lineTo(x, height)
                ctx.stroke()
            }
            for (var y = 0; y <= height; y += minorStep) {
                ctx.globalAlpha = y % majorStep === 0 ? 0.55 : 0.18
                ctx.strokeStyle = y % majorStep === 0 ? theme.chartGridMajor : theme.backgroundGrid
                ctx.beginPath()
                ctx.moveTo(0, y)
                ctx.lineTo(width, y)
                ctx.stroke()
            }

            ctx.globalAlpha = 0.30
            ctx.strokeStyle = theme.accent
            ctx.setLineDash([2, 10])
            ctx.beginPath()
            ctx.moveTo(0, Math.round(height * 0.18))
            ctx.lineTo(width, Math.round(height * 0.18))
            ctx.moveTo(0, Math.round(height * 0.72))
            ctx.lineTo(width, Math.round(height * 0.72))
            ctx.stroke()
            ctx.setLineDash([])
            ctx.globalAlpha = 1.0
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        Connections {
            target: theme
            function onIsDarkChanged() {
                backgroundGridCanvas.requestPaint()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: visibility === Window.Maximized ? 8 : 12
        anchors.rightMargin: visibility === Window.Maximized ? 8 : 12
        anchors.bottomMargin: visibility === Window.Maximized ? 4 : 6
        anchors.topMargin: window.isMac ? (8 + chromeInsetTop) : 0
        radius: visibility === Window.Maximized ? 8 : theme.shellRadius
        color: theme.shellGlass
        border.width: 1
        border.color: theme.shellOutline
    }

    Item {
        id: grid
        anchors.fill: parent
        anchors.leftMargin: visibility === Window.Maximized ? (window.width < 1600 ? 10 : 14) : (window.width < 1600 ? 12 : 18)
        anchors.rightMargin: visibility === Window.Maximized ? (window.width < 1600 ? 10 : 14) : (window.width < 1600 ? 12 : 18)
        anchors.bottomMargin: visibility === Window.Maximized ? (window.height < 920 ? 6 : 10) : (window.height < 920 ? 8 : 12)
        anchors.topMargin: window.isMac ? (12 + chromeInsetTop) : 8
        readonly property int gap: window.height < 920 ? 8 : 12
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

    AboutDialog {
        id: aboutDialog
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
            color: theme.overlayScrim
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

    Item {
        id: windowResizeFrame
        parent: Overlay.overlay
        anchors.fill: parent
        visible: window.resizeBorder > 0
        z: 9999

        readonly property int borderSize: window.resizeBorder
        readonly property int cornerSize: 18

        // Edges (avoid overlapping corners)
        MouseArea {
            anchors {
                left: parent.left
                top: parent.top
                topMargin: windowResizeFrame.cornerSize
                bottom: parent.bottom
                bottomMargin: windowResizeFrame.cornerSize
            }
            width: windowResizeFrame.borderSize
            cursorShape: Qt.SizeHorCursor
            onPressed: mouse => window.startSystemResize(Qt.LeftEdge)
        }

        MouseArea {
            anchors {
                right: parent.right
                top: parent.top
                topMargin: windowResizeFrame.cornerSize
                bottom: parent.bottom
                bottomMargin: windowResizeFrame.cornerSize
            }
            width: windowResizeFrame.borderSize
            cursorShape: Qt.SizeHorCursor
            onPressed: mouse => window.startSystemResize(Qt.RightEdge)
        }

        MouseArea {
            anchors {
                top: parent.top
                left: parent.left
                leftMargin: windowResizeFrame.cornerSize
                right: parent.right
                rightMargin: windowResizeFrame.cornerSize
            }
            height: windowResizeFrame.borderSize
            cursorShape: Qt.SizeVerCursor
            onPressed: mouse => window.startSystemResize(Qt.TopEdge)
        }

        MouseArea {
            anchors {
                bottom: parent.bottom
                left: parent.left
                leftMargin: windowResizeFrame.cornerSize
                right: parent.right
                rightMargin: windowResizeFrame.cornerSize
            }
            height: windowResizeFrame.borderSize
            cursorShape: Qt.SizeVerCursor
            onPressed: mouse => window.startSystemResize(Qt.BottomEdge)
        }

        // 4 Corners
        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            width: windowResizeFrame.cornerSize
            height: windowResizeFrame.cornerSize
            cursorShape: Qt.SizeFDiagCursor
            onPressed: mouse => window.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
        }

        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            width: windowResizeFrame.cornerSize
            height: windowResizeFrame.cornerSize
            cursorShape: Qt.SizeBDiagCursor
            onPressed: mouse => window.startSystemResize(Qt.RightEdge | Qt.TopEdge)
        }

        MouseArea {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: windowResizeFrame.cornerSize
            height: windowResizeFrame.cornerSize
            cursorShape: Qt.SizeBDiagCursor
            onPressed: mouse => window.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
        }

        MouseArea {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: windowResizeFrame.cornerSize
            height: windowResizeFrame.cornerSize
            cursorShape: Qt.SizeFDiagCursor
            onPressed: mouse => window.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
        }
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
