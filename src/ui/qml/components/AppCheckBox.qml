import QtQuick
import QtQuick.Controls

CheckBox {
    id: control

    property int indicatorSize: theme.isCompact ? 16 : 18
    property color accentColor: theme.accentStrong
    property bool wrapText: false

    Theme {
        id: theme
    }

    implicitHeight: Math.max(theme.controlHeight, indicatorSize + 4)
    implicitWidth: indicator.width + spacing + (contentItem ? contentItem.implicitWidth : 0) + leftPadding + rightPadding
    spacing: 8
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true

    font.family: theme.bodyFont
    font.pixelSize: theme.bodySize

    indicator: Rectangle {
        id: indicatorBox
        implicitWidth: control.indicatorSize
        implicitHeight: control.indicatorSize
        x: control.leftPadding
        y: Math.round((control.height - height) * 0.5)
        radius: 4
        antialiasing: true

        color: {
            if (!control.enabled) {
                return control.checked
                    ? Qt.rgba(theme.textDisabled.r, theme.textDisabled.g, theme.textDisabled.b, 0.3)
                    : Qt.rgba(theme.surface2.r, theme.surface2.g, theme.surface2.b, 0.4)
            }
            if (control.checked || control.checkState === Qt.PartiallyChecked) {
                return control.down
                    ? Qt.darker(control.accentColor, 1.15)
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

        // Precision vector checkmark
        Canvas {
            id: checkCanvas
            anchors.fill: parent
            anchors.margins: 1
            visible: control.checked || control.checkState === Qt.PartiallyChecked
            opacity: (control.checked || control.checkState === Qt.PartiallyChecked) ? 1.0 : 0.0

            Behavior on opacity {
                NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var w = width
                var h = height
                ctx.strokeStyle = control.enabled ? "#ffffff" : theme.textDisabled
                ctx.lineWidth = 1.85
                ctx.lineCap = "round"
                ctx.lineJoin = "round"

                if (control.checkState === Qt.PartiallyChecked) {
                    ctx.beginPath()
                    ctx.moveTo(w * 0.22, h * 0.5)
                    ctx.lineTo(w * 0.78, h * 0.5)
                    ctx.stroke()
                } else if (control.checked) {
                    ctx.beginPath()
                    ctx.moveTo(w * 0.20, h * 0.50)
                    ctx.lineTo(w * 0.42, h * 0.73)
                    ctx.lineTo(w * 0.80, h * 0.27)
                    ctx.stroke()
                }
            }

            Connections {
                target: control
                function onCheckedChanged() { checkCanvas.requestPaint() }
                function onCheckStateChanged() { checkCanvas.requestPaint() }
                function onEnabledChanged() { checkCanvas.requestPaint() }
            }
            Connections {
                target: theme
                function onLightModeChanged() { checkCanvas.requestPaint() }
            }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: control.enabled ? theme.textPrimary : theme.textDisabled
        verticalAlignment: Text.AlignVCenter
        elide: control.wrapText ? Text.ElideNone : Text.ElideRight
        wrapMode: control.wrapText ? Text.WordWrap : Text.NoWrap
    }
}
