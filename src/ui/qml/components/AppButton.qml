import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: control

    // Variants: "primary", "secondary" (default), "outline", "ghost", "danger", "success"
    property string variant: "secondary"
    property color accent: {
        if (variant === "danger") return theme.bad
        if (variant === "success") return theme.ok
        if (variant === "primary") return theme.accentStrong
        return theme.accent
    }
    readonly property color visualAccent: accent
    property bool loading: false
    property url iconSource: ""

    Theme {
        id: theme
    }

    implicitHeight: theme.controlHeight
    implicitWidth: Math.max(84, contentRow.implicitWidth + leftPadding + rightPadding)
    leftPadding: 12
    rightPadding: 12
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true

    font.family: theme.bodyFont
    font.pixelSize: theme.bodySize
    font.weight: variant === "primary" ? Font.DemiBold : Font.Medium

    background: Rectangle {
        id: bgRect
        radius: theme.controlRadius
        antialiasing: true

        // Background color & gradient
        color: {
            if (!control.enabled) {
                return Qt.rgba(theme.surface2.r, theme.surface2.g, theme.surface2.b, 0.45)
            }
            if (control.variant === "primary") {
                return control.down ? Qt.darker(control.visualAccent, 1.15)
                                    : (control.hovered ? Qt.lighter(control.visualAccent, 1.1) : control.visualAccent)
            }
            if (control.variant === "ghost") {
                return control.down ? theme.surface3 : (control.hovered ? theme.buttonGhostHover : "transparent")
            }
            if (control.variant === "outline") {
                return control.down ? Qt.rgba(control.visualAccent.r, control.visualAccent.g, control.visualAccent.b, 0.18)
                                    : (control.hovered ? Qt.rgba(control.visualAccent.r, control.visualAccent.g, control.visualAccent.b, 0.09) : "transparent")
            }
            // Secondary (default)
            return control.down ? theme.surface4 : (control.hovered ? theme.surface3 : theme.surface1)
        }

        border.width: control.variant === "ghost" ? 0 : (control.visualFocus ? 2 : 1)
        border.color: {
            if (!control.enabled) return theme.borderSubtle
            if (control.visualFocus) return theme.focusRingStrong
            if (control.variant === "primary" || control.variant === "outline") {
                return control.visualAccent
            }
            if (control.variant === "danger" || control.accent === theme.bad) {
                return control.hovered ? theme.bad : Qt.rgba(theme.bad.r, theme.bad.g, theme.bad.b, 0.6)
            }
            if (control.variant === "success" || control.accent === theme.ok) {
                return control.hovered ? theme.ok : Qt.rgba(theme.ok.r, theme.ok.g, theme.ok.b, 0.6)
            }
            return control.hovered ? theme.borderHover : theme.border
        }

        // Focus halo ring (for keyboard Tab navigation)
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 2
            visible: control.visualFocus
            color: "transparent"
            border.width: 2
            border.color: theme.focusRing
        }

        // Bottom accent highlight strip for secondary buttons
        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: 1
            }
            visible: control.variant !== "primary" && control.variant !== "ghost"
            height: control.down ? 3 : (control.hovered ? 2 : 1)
            radius: theme.tinyRadius
            color: control.enabled
                ? Qt.rgba(control.visualAccent.r, control.visualAccent.g, control.visualAccent.b, control.hovered || control.down ? 0.95 : 0.55)
                : Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.20)

            Behavior on height {
                NumberAnimation { duration: 90; easing.type: Easing.OutCubic }
            }
        }
    }

    contentItem: RowLayout {
        id: contentRow
        spacing: 6

        Item { Layout.fillWidth: true }

        // Loading spinner
        Canvas {
            id: spinner
            visible: control.loading
            Layout.preferredWidth: 12
            Layout.preferredHeight: 12
            property real angle: 0

            NumberAnimation on angle {
                from: 0
                to: 360
                duration: 900
                loops: Animation.Infinite
                running: control.loading
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = control.variant === "primary" ? "#ffffff" : control.visualAccent
                ctx.lineWidth = 1.8
                ctx.beginPath()
                ctx.arc(6, 6, 4.5, angle * Math.PI / 180, (angle + 270) * Math.PI / 180)
                ctx.stroke()
            }
        }

        Image {
            id: iconImg
            visible: String(control.iconSource).length > 0
            source: control.iconSource
            Layout.preferredWidth: 14
            Layout.preferredHeight: 14
            fillMode: Image.PreserveAspectFit
        }

        Text {
            text: control.text
            color: {
                if (!control.enabled) return theme.textDisabled
                if (control.variant === "primary") return "#ffffff"
                if (control.variant === "outline" || control.variant === "ghost") return control.visualAccent
                if (control.variant === "danger" || control.accent === theme.bad) return control.hovered ? theme.bad : theme.textPrimary
                if (control.variant === "success" || control.accent === theme.ok) return control.hovered ? theme.ok : theme.textPrimary
                return control.hovered ? theme.hoverText : theme.textPrimary
            }
            font: control.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Item { Layout.fillWidth: true }
    }
}
