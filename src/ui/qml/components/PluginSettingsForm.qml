import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: form
    property string pluginId: ""
    property var fields: []
    property string category: ""

    Theme { id: theme }

    width: parent ? parent.width : implicitWidth
    spacing: 12

    Repeater {
        model: form.fields || []

        delegate: Column {
            id: fieldDelegate
            required property var modelData
            readonly property var fieldData: modelData
            readonly property string fieldType: String(fieldData ? (fieldData.type || "text") : "text").toLowerCase()
            readonly property bool isReadOnly: Boolean(fieldData && (fieldData.readOnly || fieldData.readonly))
            width: form.width
            spacing: 6

            Label {
                text: fieldDelegate.fieldData ? (fieldDelegate.fieldData.label || fieldDelegate.fieldData.key || "") : ""
                color: theme.textPrimary
                font.family: theme.bodyFont
                font.pixelSize: theme.labelSize
                font.bold: true
                wrapMode: Text.WordWrap
            }

            // 1. Dropdown Combo Field
            AppComboBox {
                id: comboEditor
                visible: fieldDelegate.fieldType === "select" || fieldDelegate.fieldType === "enum" || fieldDelegate.fieldType === "combo"
                width: parent.width
                enabled: !fieldDelegate.isReadOnly
                textRole: "label"
                valueRole: "value"
                model: {
                    var source = fieldDelegate.fieldData ? (fieldDelegate.fieldData.options || fieldDelegate.fieldData.choices || fieldDelegate.fieldData.values) : null
                    if (!source || source.length === undefined) {
                        return []
                    }
                    var result = []
                    for (var i = 0; i < source.length; ++i) {
                        var item = source[i]
                        if (typeof item === "object" && item !== null) {
                            result.push({
                                label: item.label !== undefined ? String(item.label) : String(item.value),
                                value: item.value !== undefined ? item.value : item.label
                            })
                        } else {
                            result.push({ label: String(item), value: item })
                        }
                    }
                    return result
                }
                Component.onCompleted: {
                    var curVal = fieldDelegate.fieldData ? fieldDelegate.fieldData.value : undefined
                    for (var i = 0; i < model.length; ++i) {
                        if (String(model[i].value) === String(curVal)) {
                            currentIndex = i
                            return
                        }
                    }
                    if (model.length > 0) {
                        currentIndex = 0
                    }
                }
                onActivated: {
                    if (fieldDelegate.isReadOnly) {
                        return
                    }
                    if (appSettings && fieldDelegate.fieldData && currentIndex >= 0 && currentIndex < model.length) {
                        appSettings.setPluginSettingValue(form.pluginId, fieldDelegate.fieldData.key, model[currentIndex].value)
                    }
                }
            }

            // 2. Boolean Checkbox Field
            AppCheckBox {
                id: boolEditor
                visible: fieldDelegate.fieldType === "bool" || fieldDelegate.fieldType === "boolean" || fieldDelegate.fieldType === "checkbox"
                width: parent.width
                enabled: !fieldDelegate.isReadOnly
                wrapText: true
                text: fieldDelegate.fieldData ? (fieldDelegate.fieldData.checkboxLabel || "Enabled") : "Enabled"
                checked: {
                    var curVal = fieldDelegate.fieldData ? fieldDelegate.fieldData.value : undefined
                    return curVal === true || String(curVal).toLowerCase() === "true" || String(curVal) === "1"
                }
                onToggled: {
                    if (fieldDelegate.isReadOnly) {
                        return
                    }
                    if (appSettings && fieldDelegate.fieldData) {
                        appSettings.setPluginSettingValue(form.pluginId, fieldDelegate.fieldData.key, checked)
                    }
                }
            }

            // 3. Text / Password / Path Field
            AppTextField {
                id: textEditor
                visible: fieldDelegate.fieldType !== "select" && fieldDelegate.fieldType !== "enum" && fieldDelegate.fieldType !== "combo"
                         && fieldDelegate.fieldType !== "bool" && fieldDelegate.fieldType !== "boolean" && fieldDelegate.fieldType !== "checkbox"
                width: parent.width
                text: fieldDelegate.fieldData && fieldDelegate.fieldData.value !== undefined && fieldDelegate.fieldData.value !== null
                      ? String(fieldDelegate.fieldData.value)
                      : ""
                placeholderText: fieldDelegate.fieldData ? (fieldDelegate.fieldData.placeholder || "") : ""
                readOnly: fieldDelegate.isReadOnly
                echoMode: fieldDelegate.fieldType === "password" ? TextInput.Password : TextInput.Normal
                onEditingFinished: {
                    if (fieldDelegate.isReadOnly) {
                        return
                    }
                    if (appSettings && fieldDelegate.fieldData) {
                        appSettings.setPluginSettingValue(form.pluginId, fieldDelegate.fieldData.key, text)
                    }
                }
            }

            Label {
                visible: !!(fieldDelegate.fieldData && fieldDelegate.fieldData.description && fieldDelegate.fieldData.description.length > 0)
                width: parent.width
                text: fieldDelegate.fieldData ? (fieldDelegate.fieldData.description || "") : ""
                color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
                font.family: theme.bodyFont
                font.pixelSize: theme.bodySize
                wrapMode: Text.WordWrap
            }
        }
    }

    Label {
        visible: form.category !== "automation" && (!form.fields || form.fields.length === 0)
        width: parent.width
        text: "This plugin currently has no extra private settings."
        color: Qt.rgba(theme.textSecondary.r, theme.textSecondary.g, theme.textSecondary.b, 0.92)
        font.family: theme.bodyFont
        font.pixelSize: theme.bodySize
        wrapMode: Text.WordWrap
    }
}
