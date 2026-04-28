import QtQuick
import QtQuick.Controls

TabBar {
    id: control
    property color fillColor: theme.tabBarBg
    property color borderColor: theme.tabBarBorder
    property color glowColor: theme.tabBarGlow

    Theme { id: theme }

    implicitHeight: theme.controlHeight + 8
    spacing: 3
    padding: 4

    background: Rectangle {
        radius: theme.controlRadius
        clip: true
        antialiasing: true
        color: control.fillColor
        border.width: 1
        border.color: control.borderColor
    }
}
