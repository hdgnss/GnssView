import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    default property alias content: contentHost.data
    property string title: ""
    property color accent: theme.accent
    property int contentMargins: theme.panelContentMargin

    Theme {
        id: theme
    }

    radius: theme.radius
    border.width: 1
    border.color: theme.panelBorderStrong
    color: theme.panelBase
    clip: true
    antialiasing: true

    Item {
        id: contentHost
        anchors {
            fill: parent
            margins: root.contentMargins
        }
    }
}
