import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    default property alias content: contentHost.data
    property string title: ""
    property color accent: theme.accent
    property int contentMargins: theme.panelContentMargin
    property bool showHeader: false
    property int headerHeight: 32

    Theme {
        id: theme
    }

    radius: theme.radius
    border.width: 1
    border.color: theme.panelBorderStrong
    color: theme.panelBase
    gradient: Gradient {
        GradientStop { position: 0.0; color: theme.surface1 }
        GradientStop { position: 1.0; color: theme.panelBase }
    }
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
