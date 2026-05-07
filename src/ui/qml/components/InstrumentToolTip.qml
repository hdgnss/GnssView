import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property bool active: false
    property string titleText: ""
    property string detailText: ""
    property color accent: theme.accentStrong

    Theme { id: theme }

    visible: opacity > 0.01
    opacity: active ? 1.0 : 0.0
    z: 20

    radius: theme.cardRadius
    color: theme.tooltipBg
    border.width: 1
    border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.7)

    implicitWidth: Math.max(140, tooltipColumn.implicitWidth + 18)
    implicitHeight: tooltipColumn.implicitHeight + 16

    Behavior on opacity {
        NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: 3
        radius: theme.tinyRadius
        color: root.accent
    }

    ColumnLayout {
        id: tooltipColumn
        anchors.fill: parent
        anchors.margins: 9
        anchors.leftMargin: 12
        spacing: 2

        Text {
            text: root.titleText
            color: theme.tooltipText
            font.pixelSize: theme.bodySize
            font.bold: true
            font.family: theme.bodyFont
        }

        Text {
            text: root.detailText
            color: theme.tooltipDetail
            font.pixelSize: theme.labelSize
            font.family: theme.bodyFont
        }
    }
}
