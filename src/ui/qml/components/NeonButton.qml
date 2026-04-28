import QtQuick
import QtQuick.Controls

Button {
    id: control
    property color accent: theme.accent

    Theme {
        id: theme
    }

    implicitHeight: theme.controlHeight
    implicitWidth: 88

    font.family: theme.bodyFont
    font.pixelSize: theme.bodySize

    background: Rectangle {
        radius: theme.controlRadius
        border.width: 1
        border.color: control.enabled
            ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, control.down ? 0.9 : 0.5)
            : Qt.rgba(theme.panelBorderStrong.r, theme.panelBorderStrong.g, theme.panelBorderStrong.b, 0.7)
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: !control.enabled ? Qt.rgba(theme.surface2.r, theme.surface2.g, theme.surface2.b, 0.62)
                                    : (control.down ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.42)
                                                    : Qt.rgba(theme.surface4.r, theme.surface4.g, theme.surface4.b, control.hovered ? 0.84 : 0.72))
            }
            GradientStop {
                position: 1.0
                color: control.down ? Qt.rgba(theme.surface1.r, theme.surface1.g, theme.surface1.b, 0.95)
                                    : Qt.rgba(theme.surface0.r, theme.surface0.g, theme.surface0.b, 0.95)
            }
        }
    }

    contentItem: Text {
        text: control.text
        color: control.enabled ? theme.textPrimary : theme.textSecondary
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
