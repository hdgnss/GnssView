import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: aboutDialog
    parent: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(460, parent ? parent.width - 40 : 460)
    height: Math.min(320, parent ? parent.height - 40 : 320)
    x: parent ? Math.round((parent.width - width) * 0.5) : 100
    y: parent ? Math.round((parent.height - height) * 0.5) : 100
    padding: 0

    readonly property color overlayColor: theme.overlayScrim

    Theme {
        id: theme
    }

    background: Rectangle {
        radius: theme.panelRadius
        color: theme.panelBase
        border.width: 1
        border.color: theme.panelBorderStrong

        // Subtle drop shadow line
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            z: -1
            radius: parent.radius + 1
            color: Qt.rgba(0, 0, 0, 0.35)
        }
    }

    Overlay.modal: Rectangle {
        color: aboutDialog.overlayColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        // Header with App Name & Close
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                radius: 8
                color: theme.surface2
                border.width: 1
                border.color: theme.border

                Text {
                    anchors.centerIn: parent
                    text: "🛰"
                    font.pixelSize: 20
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: "GnssView"
                    color: theme.textPrimary
                    font.family: theme.titleFont
                    font.pixelSize: 16
                    font.bold: true
                }

                Label {
                    text: "Version " + (updateChecker ? updateChecker.currentVersion : "0.1.6")
                    color: theme.textSecondary
                    font.family: theme.monoFont
                    font.pixelSize: 11
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: theme.divider
        }

        Label {
            Layout.fillWidth: true
            text: "High-density cross-platform GNSS observation, spectrum telemetry, and multi-constellation protocol workbench."
            color: theme.textPrimary
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: "• Qt Version: 6.11.2 (Qt Quick)"
                color: theme.textSecondary
                font.family: theme.bodyFont
                font.pixelSize: 10
            }

            Text {
                text: "• Platform: " + Qt.platform.os + " (" + (typeof Qt.platform.pluginName !== "undefined" ? Qt.platform.pluginName : "desktop") + ")"
                color: theme.textSecondary
                font.family: theme.bodyFont
                font.pixelSize: 10
            }

            Text {
                text: "• GitHub: github.com/hdgnss/GnssView"
                color: theme.accent
                font.family: theme.bodyFont
                font.pixelSize: 10
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            AppButton {
                variant: "secondary"
                text: "Check Updates"
                onClicked: {
                    if (updateChecker) updateChecker.checkForUpdates(false)
                    aboutDialog.close()
                }
            }

            AppButton {
                variant: "primary"
                text: "Close"
                onClicked: aboutDialog.close()
            }
        }
    }
}
