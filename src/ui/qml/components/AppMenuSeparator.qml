import QtQuick
import QtQuick.Controls

MenuSeparator {
    id: control

    Theme {
        id: theme
    }

    padding: 4
    topPadding: 4
    bottomPadding: 4

    contentItem: Rectangle {
        implicitHeight: 1
        implicitWidth: 140
        color: theme.divider
    }
}
