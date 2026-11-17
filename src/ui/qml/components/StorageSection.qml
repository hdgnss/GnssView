import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SettingsSection {
    id: section
    title: "Logging & Storage"
    accent: theme.accentStrong

    Theme { id: theme }

    Column {
        width: parent.width
        spacing: 12

        // Logging Checkboxes
        RowLayout {
            width: parent.width
            spacing: 16

            AppCheckBox {
                text: "Record raw data"
                enabled: appSettings ? appSettings.logDirectory.length > 0 : false
                checked: appSettings ? appSettings.recordRawData : false
                onToggled: if (appSettings) appSettings.recordRawData = checked
            }

            AppCheckBox {
                text: "Record decode log"
                enabled: appSettings ? appSettings.logDirectory.length > 0 : false
                checked: appSettings ? appSettings.recordDecodeLog : false
                onToggled: if (appSettings) appSettings.recordDecodeLog = checked
            }
        }

        // Log Root Directory
        Label {
            text: "Log root directory"
            color: theme.textPrimary
            font.family: theme.bodyFont
            font.pixelSize: theme.labelSize
            font.bold: true
        }

        RowLayout {
            width: parent.width
            spacing: 8

            AppTextField {
                Layout.fillWidth: true
                text: appSettings ? appSettings.logDirectory : ""
                onEditingFinished: if (appSettings) appSettings.logDirectory = text
            }

            AppButton {
                variant: "secondary"
                text: "Default"
                onClicked: if (appSettings) appSettings.logDirectory = appSettings.defaultLogDirectory
            }
        }

        Label {
            width: parent.width
            text: appSettings && appSettings.logDirectory.length > 0
                  ? "Raw data writes the binary stream capture. Decode log writes the parsed text log into the next recording session."
                  : "Set a log root directory before enabling raw or decode logging."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        // Plugin Load Directory
        Label {
            text: "Plugin load directory"
            color: theme.textPrimary
            font.family: theme.bodyFont
            font.pixelSize: theme.labelSize
            font.bold: true
        }

        RowLayout {
            width: parent.width
            spacing: 8

            AppTextField {
                Layout.fillWidth: true
                text: appSettings ? appSettings.pluginDirectory : ""
                onEditingFinished: if (appSettings) appSettings.pluginDirectory = text
            }

            AppButton {
                variant: "secondary"
                text: "Default"
                onClicked: if (appSettings) appSettings.pluginDirectory = appSettings.defaultPluginDirectory
            }
        }

        Label {
            width: parent.width
            text: "Base path used to scan for shared libraries implementing protocol, automation, TEC, and transport interfaces."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }
    }
}
