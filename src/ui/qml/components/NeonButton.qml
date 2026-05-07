import QtQuick
import QtQuick.Controls

Button {
    id: control
    // Kept for compatibility with existing call sites. Button visuals are
    // intentionally unified through theme.tabSelectedAccent.
    property color accent: theme.accent
    readonly property color visualAccent: theme.tabSelectedAccent

    Theme {
        id: theme
    }

    implicitHeight: theme.controlHeight
    implicitWidth: 92
    leftPadding: 12
    rightPadding: 12
    hoverEnabled: true

    font.family: theme.bodyFont
    font.pixelSize: theme.bodySize
    font.weight: Font.DemiBold

    background: Rectangle {
        radius: theme.controlRadius
        antialiasing: true
        border.width: 1
        border.color: control.enabled
            ? Qt.rgba(control.visualAccent.r, control.visualAccent.g, control.visualAccent.b, control.down ? 0.92 : (control.hovered ? 0.64 : 0.42))
            : Qt.rgba(theme.panelBorderStrong.r, theme.panelBorderStrong.g, theme.panelBorderStrong.b, 0.7)
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: !control.enabled ? Qt.rgba(theme.surface2.r, theme.surface2.g, theme.surface2.b, 0.62)
                                    : (control.down ? Qt.rgba(control.visualAccent.r, control.visualAccent.g, control.visualAccent.b, 0.34)
                                                    : Qt.rgba(theme.surface4.r, theme.surface4.g, theme.surface4.b, control.hovered ? 0.72 : 0.48))
            }
            GradientStop {
                position: 1.0
                color: control.down ? Qt.rgba(theme.surface1.r, theme.surface1.g, theme.surface1.b, 0.95)
                                    : Qt.rgba(theme.surface0.r, theme.surface0.g, theme.surface0.b, 0.95)
            }
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: 1
            }
            height: 1
            color: Qt.rgba(1, 1, 1, control.enabled ? 0.12 : 0.04)
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: 1
            }
            height: control.down ? 3 : (control.hovered ? 2 : 1)
            radius: theme.tinyRadius
            color: control.enabled ? Qt.rgba(control.visualAccent.r, control.visualAccent.g, control.visualAccent.b, control.hovered || control.down ? 0.90 : 0.52)
                                   : Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.20)

            Behavior on height {
                NumberAnimation { duration: 90; easing.type: Easing.OutCubic }
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
