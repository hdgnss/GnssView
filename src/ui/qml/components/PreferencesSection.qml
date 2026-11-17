import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SettingsSection {
    id: section
    title: "Preferences & System"
    accent: theme.ok

    Theme { id: theme }

    Column {
        width: parent.width
        spacing: 12

        // 1. Version & Update Checks
        Label {
            text: "Software Version & Updates"
            color: theme.textPrimary
            font.family: theme.bodyFont
            font.pixelSize: theme.labelSize
            font.bold: true
        }

        RowLayout {
            width: parent.width
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: updateChecker
                      ? ("Current " + updateChecker.currentVersion
                         + (updateChecker.latestVersion && updateChecker.latestVersion.length > 0
                            ? " | Latest " + updateChecker.latestVersion
                            : ""))
                      : "Current version unavailable"
                color: theme.textSecondary
                font.family: theme.monoFont
                font.pixelSize: theme.bodySize
                elide: Text.ElideRight
            }

            AppButton {
                variant: "secondary"
                text: updateChecker && updateChecker.checking ? "Checking" : "Check"
                enabled: updateChecker ? !updateChecker.checking : false
                loading: updateChecker ? updateChecker.checking : false
                onClicked: if (updateChecker) updateChecker.checkForUpdates(false)
            }

            AppButton {
                variant: "outline"
                text: "Release"
                visible: updateChecker && updateChecker.releaseUrl && updateChecker.releaseUrl.length > 0
                onClicked: if (updateChecker) updateChecker.openReleasePage()
            }

            AppButton {
                variant: "primary"
                text: "Download"
                visible: updateChecker && updateChecker.hasUpdate
                accent: theme.ok
                onClicked: if (updateChecker) updateChecker.openDownloadPage()
            }
        }

        AppCheckBox {
            text: "Check for updates on startup"
            checked: appSettings ? appSettings.checkForUpdatesOnStartup : true
            onToggled: if (appSettings) appSettings.checkForUpdatesOnStartup = checked
        }

        Label {
            width: parent.width
            text: updateChecker ? updateChecker.statusText : "Update checker is not available."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }

        Rectangle {
            width: parent.width
            height: 1
            color: theme.divider
        }

        // 2. Window and Session Persistence
        Label {
            text: "Session & Window Persistence"
            color: theme.textPrimary
            font.family: theme.bodyFont
            font.pixelSize: theme.labelSize
            font.bold: true
        }

        AppCheckBox {
            text: "Remember main window position and size"
            checked: appSettings ? appSettings.rememberWindowGeometry : true
            onToggled: if (appSettings) appSettings.rememberWindowGeometry = checked
        }

        AppCheckBox {
            text: "Clear RawData, SkyView, Signal Spectrum, statistics, and Deviation Map on a new transport connection"
            checked: appSettings ? appSettings.resetUiOnNewConnection : true
            onToggled: if (appSettings) appSettings.resetUiOnNewConnection = checked
        }

        Rectangle {
            width: parent.width
            height: 1
            color: theme.divider
        }

        // 3. Deviation Map Reference Origin
        Label {
            text: "Deviation Map Reference Center"
            color: theme.textPrimary
            font.family: theme.bodyFont
            font.pixelSize: theme.labelSize
            font.bold: true
        }

        AppCheckBox {
            text: "Use fixed center latitude/longitude"
            checked: appSettings ? appSettings.useFixedDeviationCenter : false
            onToggled: if (appSettings) appSettings.useFixedDeviationCenter = checked
        }

        GridLayout {
            width: parent.width
            columns: 2
            columnSpacing: 10
            rowSpacing: 8

            Label {
                text: "Latitude (°)"
                color: theme.textSecondary
                font.family: theme.bodyFont
                font.pixelSize: theme.bodySize
            }

            AppTextField {
                Layout.fillWidth: true
                text: appSettings ? String(appSettings.fixedDeviationLatitude) : "0"
                enabled: appSettings ? appSettings.useFixedDeviationCenter : false
                onEditingFinished: if (appSettings) appSettings.fixedDeviationLatitude = Number(text)
            }

            Label {
                text: "Longitude (°)"
                color: theme.textSecondary
                font.family: theme.bodyFont
                font.pixelSize: theme.bodySize
            }

            AppTextField {
                Layout.fillWidth: true
                text: appSettings ? String(appSettings.fixedDeviationLongitude) : "0"
                enabled: appSettings ? appSettings.useFixedDeviationCenter : false
                onEditingFinished: if (appSettings) appSettings.fixedDeviationLongitude = Number(text)
            }
        }

        Label {
            width: parent.width
            text: "If disabled, the map uses the arithmetic mean of all received valid position fixes as the reference origin."
            color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
            font.family: theme.bodyFont
            font.pixelSize: theme.bodySize
            wrapMode: Text.WordWrap
        }
    }
}
