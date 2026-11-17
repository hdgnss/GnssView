import QtQuick
import QtQuick.Controls

Switch {
    id: control

    property color accentColor: theme.accentStrong

    Theme {
        id: theme
    }

    implicitWidth: indicator.width + (contentItem && text.length > 0 ? spacing + contentItem.implicitWidth : 0) + leftPadding + rightPadding
    implicitHeight: Math.max(theme.controlHeight, 22)
    spacing: 8
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true

    font.family: theme.bodyFont
    font.pixelSize: theme.bodySize

    indicator: Rectangle {
        id: track
        implicitWidth: theme.isCompact ? 32 : 36
        implicitHeight: theme.isCompact ? 18 : 20
        x: control.leftPadding
        y: Math.round((control.height - height) * 0.5)
        radius: height * 0.5
        antialiasing: true

        color: {
            if (!control.enabled) {
                return control.checked
                    ? Qt.rgba(theme.textDisabled.r, theme.textDisabled.g, theme.textDisabled.b, 0.3)
                    : Qt.rgba(theme.surface3.r, theme.surface3.g, theme.surface3.b, 0.4)
            }
            if (control.checked) {
                return control.down ? Qt.darker(control.accentColor, 1.15)
                                    : (control.hovered ? Qt.lighter(control.accentColor, 1.08) : control.accentColor)
            }
            return control.down ? theme.surface4 : (control.hovered ? theme.surface3 : theme.surface2)
        }

        border.width: control.visualFocus ? 2 : 1
        border.color: {
            if (!control.enabled) return theme.borderSubtle
            if (control.visualFocus) return theme.focusRingStrong
            if (control.checked) return control.accentColor
            return control.hovered ? theme.borderHover : theme.borderStrong
        }

        // Focus halo ring
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 2
            visible: control.visualFocus
            color: "transparent"
            border.width: 2
            border.color: theme.focusRing
        }

        // Sliding knob
        Rectangle {
            id: thumb
            width: parent.height - 4
            height: parent.height - 4
            y: 2
            x: control.checked ? (parent.width - width - 2) : 2
            radius: width * 0.5
            color: control.enabled ? "#ffffff" : theme.textDisabled
            antialiasing: true

            Behavior on x {
                NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
            }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: control.enabled ? theme.textPrimary : theme.textDisabled
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        visible: control.text.length > 0
    }
}
