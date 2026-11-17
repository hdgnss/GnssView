import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

MenuItem {
    id: control

    Theme {
        id: theme
    }

    implicitWidth: Math.max(160, contentLayout.implicitWidth + 24)
    implicitHeight: 28
    leftPadding: 10
    rightPadding: 10

    contentItem: RowLayout {
        id: contentLayout
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: control.text
            font.family: theme.bodyFont
            font.pixelSize: 11
            color: control.enabled
                   ? (control.highlighted ? theme.textPrimary : theme.textSecondary)
                   : theme.textDisabled
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Text {
            visible: control.action && control.action.shortcut ? true : false
            text: control.action && control.action.shortcut ? (control.action.shortcut.nativeText || control.action.shortcut.toString()) : ""
            font.family: theme.monoFont
            font.pixelSize: 10
            color: theme.textDisabled
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        radius: theme.controlRadius
        color: control.highlighted ? theme.surface3 : "transparent"
    }
}
