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

    Rectangle {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 1
        }
        height: Math.min(44, parent.height)
        radius: Math.max(0, root.radius - 1)
        color: Qt.rgba(1, 1, 1, 0.035)
    }

    Rectangle {
        id: panelHeader
        visible: root.showHeader
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 1
        }
        height: root.headerHeight
        color: "transparent"
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(theme.panelHeader.r, theme.panelHeader.g, theme.panelHeader.b, 0.96) }
            GradientStop { position: 1.0; color: Qt.rgba(theme.panelHeaderBottom.r, theme.panelHeaderBottom.g, theme.panelHeaderBottom.b, 0.90) }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 3
                Layout.preferredHeight: 16
                radius: theme.tinyRadius
                color: root.accent
            }

            Label {
                Layout.fillWidth: true
                text: root.title
                color: theme.textPrimary
                font.family: theme.titleFont
                font.pixelSize: theme.titleSize
                font.bold: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 1
                color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.52)
            }
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: 1
            color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.24)
        }
    }

    Item {
        id: contentHost
        anchors {
            fill: parent
            leftMargin: root.contentMargins
            rightMargin: root.contentMargins
            bottomMargin: root.contentMargins
            topMargin: root.contentMargins + (root.showHeader ? root.headerHeight : 0)
        }
    }
}
