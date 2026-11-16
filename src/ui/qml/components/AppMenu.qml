import QtQuick
import QtQuick.Controls

Menu {
    id: control

    Theme {
        id: theme
    }

    implicitWidth: Math.max(160, contentWidth + 20)
    topPadding: 4
    bottomPadding: 4
    leftPadding: 4
    rightPadding: 4

    delegate: AppMenuItem {}

    background: Rectangle {
        implicitWidth: 160
        radius: theme.panelRadius
        color: theme.surface1
        border.width: 1
        border.color: theme.panelBorderStrong

        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            z: -1
            radius: parent.radius + 1
            color: Qt.rgba(0, 0, 0, 0.28)
        }
    }
}
