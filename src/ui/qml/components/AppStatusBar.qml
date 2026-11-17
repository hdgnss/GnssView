import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: statusBar
    implicitHeight: 26
    color: theme.surface1
    border.width: 1
    border.color: theme.panelBorder

    Theme {
        id: theme
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 10

        // 1. Connection Status Dot & Transport Name
        RowLayout {
            spacing: 6
            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: (transportViewModel && transportViewModel.lockedTransport && transportViewModel.lockedTransport.length > 0)
                       ? theme.ok
                       : theme.textDisabled
            }

            Text {
                text: (transportViewModel && transportViewModel.lockedTransport && transportViewModel.lockedTransport.length > 0)
                      ? (transportViewModel.lockedTransport + " Connected")
                      : "Disconnected"
                color: (transportViewModel && transportViewModel.lockedTransport && transportViewModel.lockedTransport.length > 0)
                       ? theme.textPrimary
                       : theme.textSecondary
                font.family: theme.bodyFont
                font.pixelSize: 10
                font.bold: true
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 12
            color: theme.divider
        }

        // 2. GNSS Fix Status
        RowLayout {
            spacing: 5
            Rectangle {
                Layout.preferredWidth: 6
                Layout.preferredHeight: 6
                radius: 3
                color: (appController && appController.quality > 0)
                       ? theme.ok
                       : ((appController && appController.fixText && appController.fixText.length > 0) ? theme.warning : theme.bad)
            }

            Text {
                text: (appController && appController.fixText && appController.fixText.length > 0)
                      ? appController.fixText
                      : "No Fix"
                color: (appController && appController.quality > 0)
                       ? theme.ok
                       : theme.textSecondary
                font.family: theme.bodyFont
                font.pixelSize: 10
                font.bold: true
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 12
            color: theme.divider
        }

        // 3. Satellite Tracking / Used Count
        Text {
            text: "Sats: " + (appController ? appController.satelliteUsage : "0/0")
            color: (appController && appController.satellitesUsed >= 4) ? theme.textPrimary : theme.textSecondary
            font.family: theme.monoFont
            font.pixelSize: 10
        }

        // 4. DOP (Collapses under 1360px window width)
        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 12
            color: theme.divider
            visible: parent.width >= 1360
        }

        Text {
            visible: parent.width >= 1360
            text: "PDOP: " + (appController && appController.pdop > 0 ? appController.pdop.toFixed(2) : "--")
            color: theme.dopColor(appController ? appController.pdop : NaN)
            font.family: theme.monoFont
            font.pixelSize: 10
        }

        // 5. UTC Clock (Collapses under 1460px window width)
        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 12
            color: theme.divider
            visible: parent.width >= 1460 && utcText.text.length > 0
        }

        Text {
            id: utcText
            visible: parent.width >= 1460 && text.length > 0
            text: (appController && appController.utcClock && appController.utcClock.length > 0)
                  ? ("UTC: " + appController.utcClock)
                  : ""
            color: theme.textSecondary
            font.family: theme.monoFont
            font.pixelSize: 10
        }

        Item { Layout.fillWidth: true }

        // 6. Application Version & Quick Update Check Button
        Rectangle {
            Layout.preferredHeight: 18
            Layout.preferredWidth: versionRow.implicitWidth + 10
            radius: 4
            color: versionMa.containsMouse ? theme.surface3 : "transparent"

            RowLayout {
                id: versionRow
                anchors.centerIn: parent
                spacing: 4

                Text {
                    text: "GnssView v" + (updateChecker ? updateChecker.currentVersion : "0.1.7")
                    color: versionMa.containsMouse ? theme.accent : theme.textSecondary
                    font.family: theme.bodyFont
                    font.pixelSize: 10
                }
            }

            ToolTip.visible: versionMa.containsMouse
            ToolTip.text: "Click to check for updates"
            ToolTip.delay: 400

            MouseArea {
                id: versionMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (updateChecker) updateChecker.checkForUpdates(false)
            }
        }
    }
}
