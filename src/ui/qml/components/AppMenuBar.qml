import QtQuick
import QtQuick.Controls

MenuBar {
    id: control

    Theme {
        id: theme
    }

    background: Rectangle {
        color: "transparent"
    }

    delegate: MenuBarItem {
        id: item

        Theme {
            id: itemTheme
        }

        contentItem: Text {
            text: item.text
            font.family: itemTheme.bodyFont
            font.pixelSize: 12
            font.bold: false
            color: item.highlighted ? itemTheme.textPrimary : (item.hovered ? itemTheme.textPrimary : itemTheme.textSecondary)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            implicitWidth: 44
            implicitHeight: 24
            radius: itemTheme.controlRadius
            color: item.highlighted ? itemTheme.surface3 : (item.hovered ? itemTheme.surface2 : "transparent")
            border.width: item.visualFocus ? 1 : 0
            border.color: itemTheme.borderFocus
        }
    }
}
