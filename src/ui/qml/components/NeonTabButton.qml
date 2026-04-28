import QtQuick
import QtQuick.Controls

TabButton {
    id: control
    property color accent: theme.accent
    property int minButtonWidth: 72

    Theme { id: theme }

    implicitHeight: theme.controlHeight
    implicitWidth: minButtonWidth
    hoverEnabled: true
    padding: 0

    contentItem: Text {
        text: control.text
        color: !control.enabled ? Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.45)
                                : (control.checked ? theme.textPrimary : (control.hovered ? theme.hoverText : theme.textSecondary))
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        font.weight: control.checked ? Font.DemiBold : Font.Medium
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Math.max(2, theme.controlRadius - 1)
        clip: true
        antialiasing: true
        color: control.checked
               ? Qt.rgba(theme.tabSelectedAccent.r, theme.tabSelectedAccent.g, theme.tabSelectedAccent.b, 0.22)
               : (!control.enabled ? Qt.rgba(theme.tabButtonBg.r, theme.tabButtonBg.g, theme.tabButtonBg.b, 0.52)
                                   : (control.hovered ? theme.tabButtonHover : theme.tabButtonBg))
        border.width: control.checked || control.hovered ? 1 : 0
        border.color: control.checked ? theme.tabSelectedAccent : theme.panelBorderStrong
    }
}
