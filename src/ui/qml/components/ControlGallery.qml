import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: galleryRoot
    width: 1280
    height: 860
    color: theme.bgStart

    property string currentTheme: typeof appSettings !== "undefined" && appSettings && appSettings.uiTheme ? appSettings.uiTheme : "dark"
    property bool currentCompact: true

    Theme {
        id: theme
        isCompact: galleryRoot.currentCompact
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ================= TOP BAR & CONTROL STRIP =================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: theme.surface1
            border.width: 1
            border.color: theme.panelBorder

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Label {
                    text: "GnssView Design System Gallery"
                    color: theme.textPrimary
                    font.family: theme.titleFont
                    font.pixelSize: 15
                    font.bold: true
                }

                Rectangle {
                    Layout.preferredHeight: 18
                    Layout.preferredWidth: 1
                    color: theme.divider
                }

                Label {
                    text: "Interactive Component & Token Testbench"
                    color: theme.textSecondary
                    font.family: theme.bodyFont
                    font.pixelSize: theme.bodySize
                }

                Item { Layout.fillWidth: true }

                // Theme Mode Switch
                RowLayout {
                    spacing: 4
                    Label {
                        text: "Theme:"
                        color: theme.textSecondary
                        font.family: theme.bodyFont
                        font.pixelSize: theme.bodySize
                    }

                    Rectangle {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 26
                        radius: theme.controlRadius
                        color: theme.surface2
                        border.width: 1
                        border.color: galleryRoot.currentTheme === "dark" ? theme.accentStrong : theme.border

                        Text {
                            anchors.centerIn: parent
                            text: "Dark"
                            color: galleryRoot.currentTheme === "dark" ? theme.textPrimary : theme.textSecondary
                            font.family: theme.bodyFont
                            font.pixelSize: 10
                            font.bold: galleryRoot.currentTheme === "dark"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                galleryRoot.currentTheme = "dark"
                                if (typeof appSettings !== "undefined" && appSettings) {
                                    appSettings.uiTheme = "dark"
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 26
                        radius: theme.controlRadius
                        color: theme.surface2
                        border.width: 1
                        border.color: galleryRoot.currentTheme === "light" ? theme.accentStrong : theme.border

                        Text {
                            anchors.centerIn: parent
                            text: "Light"
                            color: galleryRoot.currentTheme === "light" ? theme.textPrimary : theme.textSecondary
                            font.family: theme.bodyFont
                            font.pixelSize: 10
                            font.bold: galleryRoot.currentTheme === "light"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                galleryRoot.currentTheme = "light"
                                if (typeof appSettings !== "undefined" && appSettings) {
                                    appSettings.uiTheme = "light"
                                }
                            }
                        }
                    }
                }

                // Density Switch
                RowLayout {
                    spacing: 4
                    Label {
                        text: "Density:"
                        color: theme.textSecondary
                        font.family: theme.bodyFont
                        font.pixelSize: theme.bodySize
                    }

                    Rectangle {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 26
                        radius: theme.controlRadius
                        color: theme.surface2
                        border.width: 1
                        border.color: galleryRoot.currentCompact ? theme.accentStrong : theme.border

                        Text {
                            anchors.centerIn: parent
                            text: "Compact"
                            color: galleryRoot.currentCompact ? theme.textPrimary : theme.textSecondary
                            font.family: theme.bodyFont
                            font.pixelSize: 10
                            font.bold: galleryRoot.currentCompact
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: galleryRoot.currentCompact = true
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 26
                        radius: theme.controlRadius
                        color: theme.surface2
                        border.width: 1
                        border.color: !galleryRoot.currentCompact ? theme.accentStrong : theme.border

                        Text {
                            anchors.centerIn: parent
                            text: "Comfortable"
                            color: !galleryRoot.currentCompact ? theme.textPrimary : theme.textSecondary
                            font.family: theme.bodyFont
                            font.pixelSize: 10
                            font.bold: !galleryRoot.currentCompact
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: galleryRoot.currentCompact = false
                        }
                    }
                }
            }
        }

        // ================= GALLERY SCROLLABLE CONTENT =================
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: galleryRoot.width - 32
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 16

                Item { Layout.preferredHeight: 8 }

                // ---------------- 1. COLOR TOKENS & SURFACES ----------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    radius: theme.panelRadius
                    color: theme.panelBase
                    border.width: 1
                    border.color: theme.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: "1. Surface Hierarchy & Core Palette"
                            color: theme.textPrimary
                            font.family: theme.titleFont
                            font.pixelSize: 13
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                model: [
                                    { name: "surface0", c: theme.surface0, b: theme.borderSubtle },
                                    { name: "surface1", c: theme.surface1, b: theme.border },
                                    { name: "surface2", c: theme.surface2, b: theme.border },
                                    { name: "surface3", c: theme.surface3, b: theme.borderStrong },
                                    { name: "surface4", c: theme.surface4, b: theme.borderStrong },
                                    { name: "accent", c: theme.accent, b: theme.accent },
                                    { name: "accentStrong", c: theme.accentStrong, b: theme.accentStrong },
                                    { name: "success / ok", c: theme.ok, b: theme.ok },
                                    { name: "warning", c: theme.warning, b: theme.warning },
                                    { name: "danger / bad", c: theme.bad, b: theme.bad }
                                ]

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 52
                                    radius: theme.controlRadius
                                    color: modelData.c
                                    border.width: 1
                                    border.color: modelData.b

                                    Column {
                                        anchors.centerIn: parent
                                        spacing: 2
                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: modelData.name
                                            font.pixelSize: 9
                                            font.bold: true
                                            color: (modelData.name.indexOf("accent") >= 0 || modelData.name.indexOf("danger") >= 0
                                                    || modelData.name.indexOf("success") >= 0) ? "#ffffff" : theme.textPrimary
                                        }
                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: String(modelData.c).toUpperCase()
                                            font.pixelSize: 8
                                            color: (modelData.name.indexOf("accent") >= 0 || modelData.name.indexOf("danger") >= 0
                                                    || modelData.name.indexOf("success") >= 0) ? "#e0f0ff" : theme.textSecondary
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------------- 2. GNSS CONSTELLATION DOMAIN PALETTE ----------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    radius: theme.panelRadius
                    color: theme.panelBase
                    border.width: 1
                    border.color: theme.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: "2. GNSS Constellation Domain Palette (Used in Fix vs Tracked Visible)"
                            color: theme.textPrimary
                            font.family: theme.titleFont
                            font.pixelSize: 13
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                model: [
                                    { name: "GPS", used: theme.constellationGpsUsed, vis: theme.constellationGpsVisible },
                                    { name: "GLONASS", used: theme.constellationGloUsed, vis: theme.constellationGloVisible },
                                    { name: "Galileo", used: theme.constellationGalUsed, vis: theme.constellationGalVisible },
                                    { name: "BeiDou", used: theme.constellationBdsUsed, vis: theme.constellationBdsVisible },
                                    { name: "QZSS", used: theme.constellationQzsUsed, vis: theme.constellationQzsVisible },
                                    { name: "SBAS", used: theme.constellationSbsUsed, vis: theme.constellationSbsVisible },
                                    { name: "NavIC", used: theme.constellationNavicUsed, vis: theme.constellationNavicVisible }
                                ]

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 46
                                    radius: theme.controlRadius
                                    color: theme.surface1
                                    border.width: 1
                                    border.color: theme.panelBorder

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 4

                                        Text {
                                            Layout.alignment: Qt.AlignHCenter
                                            text: modelData.name
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: theme.textPrimary
                                        }

                                        RowLayout {
                                            Layout.alignment: Qt.AlignHCenter
                                            spacing: 6

                                            Rectangle {
                                                Layout.preferredWidth: 18
                                                Layout.preferredHeight: 14
                                                radius: 3
                                                color: modelData.used
                                                ToolTip.visible: usedMa.containsMouse
                                                ToolTip.text: modelData.name + " Used in Fix"
                                                MouseArea { id: usedMa; anchors.fill: parent; hoverEnabled: true }
                                            }

                                            Text {
                                                text: "Used"
                                                font.pixelSize: 8
                                                color: theme.textSecondary
                                            }

                                            Rectangle {
                                                Layout.preferredWidth: 18
                                                Layout.preferredHeight: 14
                                                radius: 3
                                                color: modelData.vis
                                                ToolTip.visible: visMa.containsMouse
                                                ToolTip.text: modelData.name + " Tracked"
                                                MouseArea { id: visMa; anchors.fill: parent; hoverEnabled: true }
                                            }

                                            Text {
                                                text: "Vis"
                                                font.pixelSize: 8
                                                color: theme.textSecondary
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------------- 3. BUTTON SYSTEM & INTERACTIVE STATES ----------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180
                    radius: theme.panelRadius
                    color: theme.panelBase
                    border.width: 1
                    border.color: theme.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        Label {
                            text: "3. Button Variants & Interaction States (Hover / Pressed / Focus Ring / Disabled)"
                            color: theme.textPrimary
                            font.family: theme.titleFont
                            font.pixelSize: 13
                            font.bold: true
                        }

                        GridLayout {
                            columns: 6
                            columnSpacing: 10
                            rowSpacing: 8

                            Label { text: "Variant"; color: theme.textSecondary; font.pixelSize: 10; font.bold: true }
                            Label { text: "Default"; color: theme.textSecondary; font.pixelSize: 10 }
                            Label { text: "Focused"; color: theme.textSecondary; font.pixelSize: 10 }
                            Label { text: "Success"; color: theme.textSecondary; font.pixelSize: 10 }
                            Label { text: "Danger"; color: theme.textSecondary; font.pixelSize: 10 }
                            Label { text: "Disabled"; color: theme.textSecondary; font.pixelSize: 10 }

                            // Primary Row
                            Label { text: "Primary"; color: theme.textPrimary; font.bold: true; font.pixelSize: 10 }
                            NeonButton { text: "Primary"; accent: theme.accentStrong }
                            Rectangle {
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: theme.controlHeight
                                radius: theme.controlRadius
                                color: theme.accentStrong
                                border.width: 2; border.color: theme.focusRingStrong
                                Text { anchors.centerIn: parent; text: "Focused"; color: "#ffffff"; font.pixelSize: 11; font.bold: true }
                            }
                            NeonButton { text: "Open"; accent: theme.ok }
                            NeonButton { text: "Disconnect"; accent: theme.bad }
                            NeonButton { text: "Disabled"; enabled: false }

                            // Secondary Row
                            Label { text: "Secondary"; color: theme.textPrimary; font.bold: true; font.pixelSize: 10 }
                            Rectangle {
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: theme.controlHeight
                                radius: theme.controlRadius
                                color: theme.surface2; border.width: 1; border.color: theme.border
                                Text { anchors.centerIn: parent; text: "Secondary"; color: theme.textPrimary; font.pixelSize: 11 }
                            }
                            Rectangle {
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: theme.controlHeight
                                radius: theme.controlRadius
                                color: theme.surface3; border.width: 2; border.color: theme.focusRingStrong
                                Text { anchors.centerIn: parent; text: "Focus Ring"; color: theme.textPrimary; font.pixelSize: 11 }
                            }
                            Rectangle {
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: theme.controlHeight
                                radius: theme.controlRadius
                                color: theme.surface2; border.width: 1; border.color: theme.ok
                                Text { anchors.centerIn: parent; text: "Apply"; color: theme.ok; font.pixelSize: 11; font.bold: true }
                            }
                            Rectangle {
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: theme.controlHeight
                                radius: theme.controlRadius
                                color: theme.surface2; border.width: 1; border.color: theme.bad
                                Text { anchors.centerIn: parent; text: "Delete"; color: theme.bad; font.pixelSize: 11; font.bold: true }
                            }
                            Rectangle {
                                Layout.preferredWidth: 92
                                Layout.preferredHeight: theme.controlHeight
                                radius: theme.controlRadius
                                color: theme.surface1; border.width: 1; border.color: theme.borderSubtle; opacity: 0.5
                                Text { anchors.centerIn: parent; text: "Disabled"; color: theme.textDisabled; font.pixelSize: 11 }
                            }
                        }
                    }
                }

                // ---------------- 4. FORM INPUTS & DATA ENTRY ----------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    radius: theme.panelRadius
                    color: theme.panelBase
                    border.width: 1
                    border.color: theme.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: "4. Form Controls & Input Fields"
                            color: theme.textPrimary
                            font.family: theme.titleFont
                            font.pixelSize: 13
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            // Normal TextField
                            ColumnLayout {
                                spacing: 4
                                Label { text: "Port / Hostname"; color: theme.textSecondary; font.pixelSize: 10 }
                                TextField {
                                    id: normalField
                                    implicitWidth: 160
                                    implicitHeight: theme.controlHeight
                                    text: "192.168.1.100"
                                    color: theme.textPrimary
                                    font.family: theme.monoFont
                                    font.pixelSize: theme.monoSize
                                    background: Rectangle {
                                        color: theme.inputBg; radius: theme.controlRadius
                                        border.width: 1; border.color: normalField.activeFocus ? theme.inputBorderFocus : theme.inputBorder
                                    }
                                }
                            }

                            // Error TextField
                            ColumnLayout {
                                spacing: 4
                                Label { text: "Baudrate (Validation Error)"; color: theme.bad; font.pixelSize: 10 }
                                TextField {
                                    implicitWidth: 160
                                    implicitHeight: theme.controlHeight
                                    text: "99999999"
                                    color: theme.bad
                                    font.family: theme.monoFont
                                    font.pixelSize: theme.monoSize
                                    background: Rectangle {
                                        color: theme.inputBg; radius: theme.controlRadius
                                        border.width: 1; border.color: theme.bad
                                    }
                                }
                            }

                            // ComboBox
                            ColumnLayout {
                                spacing: 4
                                Label { text: "Protocol / Interface"; color: theme.textSecondary; font.pixelSize: 10 }
                                ComboBox {
                                    id: comboControl
                                    implicitWidth: 170
                                    implicitHeight: theme.controlHeight
                                    model: ["UART / Serial", "TCP Client", "UDP Server", "Bluetooth Plugin"]
                                    background: Rectangle {
                                        color: theme.surface2; radius: theme.controlRadius
                                        border.width: 1; border.color: theme.border
                                    }
                                    contentItem: Text {
                                        leftPadding: 8; text: comboControl.displayText
                                        color: theme.textPrimary; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }

                            // Checkboxes & Switches
                            ColumnLayout {
                                spacing: 4
                                Label { text: "AppCheckBox & AppSwitch (Precision Vector)"; color: theme.textSecondary; font.pixelSize: 10 }
                                RowLayout {
                                    spacing: 12
                                    AppCheckBox {
                                        id: cb1
                                        text: "Auto-Reconnect"
                                        checked: true
                                    }
                                    AppCheckBox {
                                        id: cb2
                                        text: "Log Raw"
                                        checked: false
                                    }
                                    AppCheckBox {
                                        id: cb3
                                        text: "Disabled"
                                        checked: true
                                        enabled: false
                                    }
                                    AppSwitch {
                                        id: sw1
                                        checked: true
                                        text: "Active"
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------------- 5. TELEMETRY BADGES & METRIC CARDS ----------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    radius: theme.panelRadius
                    color: theme.panelBase
                    border.width: 1
                    border.color: theme.panelBorder

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: "5. GNSS Telemetry Metrics & Data Badges"
                            color: theme.textPrimary
                            font.family: theme.titleFont
                            font.pixelSize: 13
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            // Fix Status Card
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                radius: theme.cardRadius
                                color: theme.surface1
                                border.width: 1
                                border.color: theme.panelBorder

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 8

                                    Rectangle {
                                        Layout.preferredWidth: 8
                                        Layout.preferredHeight: 28
                                        radius: 4
                                        color: theme.ok
                                    }

                                    ColumnLayout {
                                        spacing: 1
                                        Text { text: "FIX STATUS"; color: theme.textSecondary; font.pixelSize: 8; font.bold: true }
                                        Text { text: "3D / DGPS FIXED"; color: theme.ok; font.pixelSize: 12; font.bold: true }
                                    }
                                }
                            }

                            // DOP Metric Card
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                radius: theme.cardRadius
                                color: theme.surface1
                                border.width: 1
                                border.color: theme.panelBorder

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 8

                                    Rectangle {
                                        Layout.preferredWidth: 8
                                        Layout.preferredHeight: 28
                                        radius: 4
                                        color: theme.semanticPrecision
                                    }

                                    ColumnLayout {
                                        spacing: 1
                                        Text { text: "DILUTION OF PRECISION"; color: theme.textSecondary; font.pixelSize: 8; font.bold: true }
                                        Text { text: "PDOP 1.15 | HDOP 0.78"; color: theme.textPrimary; font.family: theme.monoFont; font.pixelSize: 11; font.bold: true }
                                    }
                                }
                            }

                            // Raw Stream Badges
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                radius: theme.cardRadius
                                color: theme.surface1
                                border.width: 1
                                border.color: theme.panelBorder

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 8

                                    Rectangle {
                                        Layout.preferredWidth: 78
                                        Layout.preferredHeight: 24
                                        radius: theme.badgeRadius
                                        color: theme.badgeNmeaBg; border.width: 1; border.color: theme.badgeNmeaBorder
                                        Text { anchors.centerIn: parent; text: "NMEA 0183"; color: theme.badgeNmeaText; font.pixelSize: 9; font.bold: true }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 64
                                        Layout.preferredHeight: 24
                                        radius: theme.badgeRadius
                                        color: theme.badgeBinBg; border.width: 1; border.color: theme.badgeBinBorder
                                        Text { anchors.centerIn: parent; text: "BINARY"; color: theme.badgeBinText; font.pixelSize: 9; font.bold: true }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 76
                                        Layout.preferredHeight: 24
                                        radius: 4
                                        color: theme.rawRxRow
                                        Text { anchors.centerIn: parent; text: "RX: 14.8 kB/s"; color: theme.rawRxText; font.family: theme.monoFont; font.pixelSize: 9 }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 76
                                        Layout.preferredHeight: 24
                                        radius: 4
                                        color: theme.rawTxRow
                                        Text { anchors.centerIn: parent; text: "TX: 0.6 kB/s"; color: theme.rawTxText; font.family: theme.monoFont; font.pixelSize: 9 }
                                    }
                                }
                            }
                        }
                    }
                }

                Item { Layout.preferredHeight: 12 }
            }
        }
    }
}
