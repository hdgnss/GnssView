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
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(theme.surface2.r, theme.surface2.g, theme.surface2.b, 0.72) }
            GradientStop { position: 1.0; color: control.fillColor }
        }
        border.width: 1
        border.color: control.borderColor

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: 1
            }
            height: 1
            color: Qt.rgba(1, 1, 1, 0.08)
        }
    }
}
