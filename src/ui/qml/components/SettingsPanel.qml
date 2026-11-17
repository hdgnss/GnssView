import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    objectName: "settingsPanel"
    parent: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    readonly property real hostWidth: parent ? parent.width : 960
    readonly property real hostHeight: parent ? parent.height : 760
    width: Math.max(360, Math.min(920, hostWidth - 40))
    height: Math.max(320, Math.min(760, hostHeight - 40))
    x: Math.max(20, Math.round((hostWidth - width) * 0.5))
    y: Math.max(20, Math.round((hostHeight - height) * 0.5))
    padding: 0

    readonly property color overlayColor: theme.overlayScrim

    Theme { id: theme }

    background: Rectangle {
        radius: theme.radius
        color: theme.panelBase
        border.width: 1
        border.color: theme.panelBorderStrong

        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            z: -1
            radius: parent.radius + 1
            color: Qt.rgba(0, 0, 0, 0.35)
        }
    }

    Overlay.modal: Rectangle {
        color: root.overlayColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: theme.radius
            color: theme.panelBase
            border.width: 1
            border.color: theme.panelBorderStrong

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: "Application Settings"
                        color: theme.textPrimary
                        font.family: theme.titleFont
                        font.pixelSize: theme.titleSize
                        font.bold: true
                    }

                    Label {
                        text: "Configure logs, plugin discovery, private plugin parameters, and app window behavior."
                        color: theme.textSecondary
                        font.family: theme.bodyFont
                        font.pixelSize: theme.bodySize
                        elide: Text.ElideRight
                    }
                }

                AppButton {
                    variant: "danger"
                    text: "Close"
                    onClicked: root.close()
                }
            }
        }

        Flickable {
            id: settingsFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: settingsContent.childrenRect.height
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Column {
                id: settingsContent
                width: settingsFlick.width
                spacing: 12

                AppearanceSection {
                    width: parent.width
                }

                StorageSection {
                    width: parent.width
                }

                PreferencesSection {
                    width: parent.width
                }

                PluginConfigSection {
                    width: parent.width
                }
            }
        }
    }
}
