import QtQuick
import QtQuick.Controls

TextField {
    id: control

    property bool hasError: false
    property string errorMessage: ""
    property bool monospace: false

    Theme {
        id: theme
    }

    implicitHeight: theme.controlHeight
    font.family: monospace ? theme.monoFont : theme.bodyFont
    font.pixelSize: monospace ? theme.monoSize : theme.bodySize
    color: (!enabled || readOnly) ? theme.textSecondary : theme.textPrimary
    placeholderTextColor: theme.textPlaceholder
    selectByMouse: true
    leftPadding: 10
    rightPadding: hasError ? 28 : 10
    topPadding: 0
    bottomPadding: 0

    background: Rectangle {
        id: bgRect
        radius: theme.controlRadius
        antialiasing: true
        color: (!control.enabled || control.readOnly)
            ? Qt.rgba(theme.inputBg.r, theme.inputBg.g, theme.inputBg.b, 0.45)
            : theme.inputBg

        border.width: control.activeFocus ? 1.5 : 1
        border.color: {
            if (control.hasError) return theme.bad
            if (!control.enabled || control.readOnly) return theme.panelBorder
            if (control.activeFocus) return theme.inputBorderFocus
            return control.hovered ? theme.borderHover : theme.inputBorder
        }

        // Focus halo ring
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 2
            visible: control.activeFocus
            color: "transparent"
            border.width: 2
            border.color: control.hasError
                ? Qt.rgba(theme.bad.r, theme.bad.g, theme.bad.b, 0.35)
                : theme.focusRing
        }

        // Error icon indicator
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            visible: control.hasError
            text: "!"
            color: theme.bad
            font.bold: true
            font.pixelSize: 12

            ToolTip.visible: errorMa.containsMouse && control.errorMessage.length > 0
            ToolTip.text: control.errorMessage

            MouseArea {
                id: errorMa
                anchors.fill: parent
                hoverEnabled: true
            }
        }
    }
}
