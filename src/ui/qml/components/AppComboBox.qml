import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ComboBox {
    id: control

    Theme {
        id: theme
    }

    implicitHeight: theme.controlHeight
    implicitWidth: Math.max(140, contentItem ? contentItem.implicitWidth + 36 : 140)
    font.family: theme.bodyFont
    font.pixelSize: theme.bodySize
    hoverEnabled: true

    displayText: {
        if (currentIndex >= 0 && model && typeof model.length !== "undefined" && model.length > currentIndex) {
            var item = model[currentIndex]
            if (item) {
                if (control.textRole && item[control.textRole] !== undefined) {
                    return String(item[control.textRole])
                }
                if (item.label !== undefined) {
                    return String(item.label)
                }
            }
        }
        return currentText.length > 0 ? currentText : (editable ? editText : "")
    }

    background: Rectangle {
        id: bgRect
        radius: theme.controlRadius
        antialiasing: true
        color: !control.enabled
            ? Qt.rgba(theme.inputBg.r, theme.inputBg.g, theme.inputBg.b, 0.45)
            : (control.down ? theme.surface3 : (control.hovered ? theme.surface2 : theme.inputBg))

        border.width: control.activeFocus ? 1.5 : 1
        border.color: {
            if (!control.enabled) return theme.borderSubtle
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
            border.color: theme.focusRing
        }
    }

    contentItem: Text {
        leftPadding: 10
        rightPadding: 28
        text: control.displayText
        font: control.font
        color: control.enabled ? theme.textPrimary : theme.textDisabled
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Canvas {
        id: arrowCanvas
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        width: 10
        height: 6
        rotation: control.popup.visible ? 180 : 0

        Behavior on rotation {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = control.enabled ? (control.hovered ? theme.accent : theme.textSecondary) : theme.textDisabled
            ctx.lineWidth = 1.6
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            ctx.beginPath()
            ctx.moveTo(1, 1)
            ctx.lineTo(5, 5)
            ctx.lineTo(9, 1)
            ctx.stroke()
        }

        Connections {
            target: control
            function onHoveredChanged() { arrowCanvas.requestPaint() }
            function onEnabledChanged() { arrowCanvas.requestPaint() }
        }
    }

    popup: Popup {
        y: control.height + 2
        width: control.width
        implicitHeight: Math.min(220, contentItem.implicitHeight + 8)
        padding: 4
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: theme.controlRadius
            color: theme.surface1
            border.width: 1
            border.color: theme.panelBorderStrong

            // Subtle drop shadow line
            Rectangle {
                anchors.fill: parent
                anchors.margins: -1
                z: -1
                radius: parent.radius + 1
                color: Qt.rgba(0, 0, 0, 0.25)
            }
        }

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
    }

    delegate: ItemDelegate {
        id: itemDelegate
        width: control.width - 8
        height: Math.max(26, theme.controlHeight - 2)
        highlighted: control.highlightedIndex === index
        hoverEnabled: true

        background: Rectangle {
            radius: theme.controlRadius - 2
            color: {
                if (itemDelegate.highlighted || control.currentIndex === index) {
                    return theme.surface4
                }
                return itemDelegate.hovered ? theme.surface3 : "transparent"
            }
        }

        contentItem: RowLayout {
            spacing: 6
            Text {
                Layout.fillWidth: true
                leftPadding: 6
                text: {
                    if (typeof modelData === "object" && modelData !== null) {
                        if (control.textRole && modelData[control.textRole] !== undefined) {
                            return String(modelData[control.textRole])
                        }
                        if (modelData.label !== undefined) {
                            return String(modelData.label)
                        }
                    }
                    return String(modelData)
                }
                color: (itemDelegate.highlighted || control.currentIndex === index) ? theme.accent : theme.textPrimary
                font: control.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                visible: control.currentIndex === index
                rightPadding: 6
                text: "✓"
                color: theme.accent
                font.bold: true
                font.pixelSize: 11
            }
        }
    }
}
