import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SettingsSection {
    id: section
    title: "Appearance"
    accent: theme.accentStrong

    Theme { id: theme }

    Column {
        width: parent.width
        spacing: 12

        // 1. Theme Mode (Dark / Light)
        Label {
            text: "Theme Style"
            color: theme.textPrimary
            font.family: theme.bodyFont
            font.pixelSize: theme.labelSize
            font.bold: true
        }

        RowLayout {
            width: parent.width
            spacing: 8

            Rectangle {
                id: darkChoice
                Layout.fillWidth: true
                Layout.preferredHeight: theme.controlHeight + 6
                radius: theme.controlRadius
                readonly property bool isSelected: appSettings ? appSettings.uiTheme !== "light" : true
                color: isSelected
                    ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.16)
                    : theme.tabButtonBg
                border.width: 1
                border.color: isSelected
                    ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.74)
                    : (darkMa.containsMouse ? theme.borderHover : theme.panelBorderStrong)

                Text {
                    anchors.centerIn: parent
                    text: "Dark"
                    color: darkChoice.isSelected ? theme.textPrimary : theme.textSecondary
                    font.family: theme.bodyFont
                    font.pixelSize: theme.bodySize
                    font.weight: darkChoice.isSelected ? Font.DemiBold : Font.Medium
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        margins: 1
                    }
                    height: darkChoice.isSelected ? 3 : (darkMa.containsMouse ? 2 : 0)
                    radius: theme.tinyRadius
                    color: Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, darkChoice.isSelected ? 0.94 : 0.58)
                }

                MouseArea {
                    id: darkMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (appSettings) appSettings.setUiTheme("dark")
                }
            }

            Rectangle {
                id: lightChoice
                Layout.fillWidth: true
                Layout.preferredHeight: theme.controlHeight + 6
                radius: theme.controlRadius
                readonly property bool isSelected: appSettings ? appSettings.uiTheme === "light" : false
                color: isSelected
                    ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.16)
                    : theme.tabButtonBg
                border.width: 1
                border.color: isSelected
                    ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.74)
                    : (lightMa.containsMouse ? theme.borderHover : theme.panelBorderStrong)

                Text {
                    anchors.centerIn: parent
                    text: "Light"
                    color: lightChoice.isSelected ? theme.textPrimary : theme.textSecondary
                    font.family: theme.bodyFont
                    font.pixelSize: theme.bodySize
                    font.weight: lightChoice.isSelected ? Font.DemiBold : Font.Medium
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        margins: 1
                    }
                    height: lightChoice.isSelected ? 3 : (lightMa.containsMouse ? 2 : 0)
                    radius: theme.tinyRadius
                    color: Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, lightChoice.isSelected ? 0.94 : 0.58)
                }

                MouseArea {
                    id: lightMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (appSettings) appSettings.setUiTheme("light")
                }
            }
        }

        Label {
            width: parent.width
            text: "Light keeps the same instrument layout and blue interaction language while using a cold daylight surface for field and lab viewing."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        // 2. Density Mode (Compact / Comfortable)
        Label {
            text: "Display Density"
            color: theme.textPrimary
            font.family: theme.bodyFont
            font.pixelSize: theme.labelSize
            font.bold: true
        }

        RowLayout {
            width: parent.width
            spacing: 8

            Rectangle {
                id: compactChoice
                Layout.fillWidth: true
                Layout.preferredHeight: theme.controlHeight + 6
                radius: theme.controlRadius
                readonly property bool isSelected: theme.isCompact
                color: isSelected
                    ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.16)
                    : theme.tabButtonBg
                border.width: 1
                border.color: isSelected
                    ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.74)
                    : (compactMa.containsMouse ? theme.borderHover : theme.panelBorderStrong)

                Text {
                    anchors.centerIn: parent
                    text: "Compact (Engineering)"
                    color: compactChoice.isSelected ? theme.textPrimary : theme.textSecondary
                    font.family: theme.bodyFont
                    font.pixelSize: theme.bodySize
                    font.weight: compactChoice.isSelected ? Font.DemiBold : Font.Medium
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        margins: 1
                    }
                    height: compactChoice.isSelected ? 3 : (compactMa.containsMouse ? 2 : 0)
                    radius: theme.tinyRadius
                    color: Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, compactChoice.isSelected ? 0.94 : 0.58)
                }

                MouseArea {
                    id: compactMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        theme.isCompact = true
                        if (appSettings && typeof appSettings.setUiDensity !== "undefined") {
                            appSettings.setUiDensity("compact")
                        }
                    }
                }
            }

            Rectangle {
                id: comfortableChoice
                Layout.fillWidth: true
                Layout.preferredHeight: theme.controlHeight + 6
                radius: theme.controlRadius
                readonly property bool isSelected: !theme.isCompact
                color: isSelected
                    ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.16)
                    : theme.tabButtonBg
                border.width: 1
                border.color: isSelected
                    ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.74)
                    : (comfortableMa.containsMouse ? theme.borderHover : theme.panelBorderStrong)

                Text {
                    anchors.centerIn: parent
                    text: "Comfortable (Standard)"
                    color: comfortableChoice.isSelected ? theme.textPrimary : theme.textSecondary
                    font.family: theme.bodyFont
                    font.pixelSize: theme.bodySize
                    font.weight: comfortableChoice.isSelected ? Font.DemiBold : Font.Medium
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        margins: 1
                    }
                    height: comfortableChoice.isSelected ? 3 : (comfortableMa.containsMouse ? 2 : 0)
                    radius: theme.tinyRadius
                    color: Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, comfortableChoice.isSelected ? 0.94 : 0.58)
                }

                MouseArea {
                    id: comfortableMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        theme.isCompact = false
                        if (appSettings && typeof appSettings.setUiDensity !== "undefined") {
                            appSettings.setUiDensity("comfortable")
                        }
                    }
                }
            }
        }

        Label {
            width: parent.width
            text: "Compact density prioritizes high data density for multi-frequency GNSS signals and observations. Comfortable provides standard touch/desktop padding."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }
    }
}
