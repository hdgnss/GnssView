import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: control
    property string title: ""
    property color accent: theme.accent
    default property alias content: sectionContent.data

    Theme { id: theme }

    width: parent ? parent.width : implicitWidth
    implicitHeight: sectionHeader.height + sectionContent.childrenRect.height + 28
    radius: theme.radius
    color: theme.panelBase
    border.width: 1
    border.color: theme.panelBorderStrong

    Item {
        id: sectionHeader
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: 34

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                color: control.accent
            }

            Label {
                Layout.fillWidth: true
                text: control.title
                color: theme.textPrimary
                font.family: theme.titleFont
                font.pixelSize: theme.bodySize + 1
                font.bold: true
            }
        }
    }

    Item {
        id: sectionContent
        anchors {
            left: parent.left
            right: parent.right
            top: sectionHeader.bottom
            leftMargin: 12
            rightMargin: 12
            topMargin: 8
        }
    }
}
