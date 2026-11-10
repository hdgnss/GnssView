import QtQuick
import QtTest
import "../../src/ui/qml/components" as Components
TestCase {
    name: "GenericTransportUi"
    visible: true; width: 900; height: 700
    when: windowShown
    QtObject { id: appSettings; property string uiTheme: "dark"; function setPluginSettingValues(id, values) {} }
    QtObject {
        id: transportViewModel
        property var availableTransports: [{transportName: "UART", category: "builtin"}, {transportName: "TCP", category: "builtin"}, {transportName: "UDP", category: "builtin"}, {transportName: "arbitrary-transport", displayName: "Arbitrary", category: "plugin", pluginId: "arbitrary-plugin", customUi: ({compactTransportPanel: true, qmlSource: 'import QtQuick; Item { objectName: "custom-root"; implicitHeight: 50; property bool canOpen: false; property bool transportLocked: false }'})}]
        property string lockedTransport: ""
        property string activeTransport: ""
        property var uartPorts: []
        signal serialPortsChanged()
        signal statusesChanged()
        property int statusRequests: 0
        signal transportDescriptorChanged(string transportName)
        function transportOpen(name) { return lockedTransport === name }
        function transportDescriptor(name) { return availableTransports[3] }
        function transportStoredSettings(name) { return {} }
        function transportStatus(name) { ++statusRequests; return "Plugin-owned status" }
        function refreshUartPorts() {}
    }
    Components.TransportConfigPanel { id: panel; visible: true; width: 900; height: 700 }
    function collect(item, predicate) {
        var result = []
        if (predicate(item)) result.push(item)
        if (item.children) for (var i = 0; i < item.children.length; ++i) result = result.concat(collect(item.children[i], predicate))
        return result
    }
    function pluginOpen() { var list = collect(panel, function(i) { return i.text === "Open" }); return list[list.length - 1] }
    function test_genericCapabilities() {
        panel.updateCurrentTransport("arbitrary-transport"); panel.preserveTransportSelection("arbitrary-transport", function() {})
        wait(50)
        var content = findChild(panel, "custom-root")
        verify(content !== null)
        compare(pluginOpen().enabled, false)
        content.canOpen = true
        compare(pluginOpen().enabled, true)
        var statuses = collect(panel, function(i) { return i.text === "Plugin-owned status" })
        compare(statuses.length, 0)
        compare(transportViewModel.statusRequests, 0)
        transportViewModel.lockedTransport = "arbitrary-transport"
        compare(content.transportLocked, true)
        compare(pluginOpen().enabled, false)
        transportViewModel.lockedTransport = ""
        compare(content.transportLocked, false)
        var descriptor = transportViewModel.availableTransports[3]
        descriptor.customUi.compactTransportPanel = false
        transportViewModel.transportDescriptorChanged("arbitrary-transport")
        wait(30)
        content = findChild(panel, "custom-root")
        content.canOpen = false
        compare(pluginOpen().enabled, false)
        compare(transportViewModel.statusRequests, 0)
        descriptor.customUi.qmlSource = 'import QtQuick; Item { implicitHeight: 50 }'
        transportViewModel.transportDescriptorChanged("arbitrary-transport")
        wait(30)
        compare(pluginOpen().enabled, true)
        descriptor.customUi = {compactTransportPanel: true}
        transportViewModel.transportDescriptorChanged("arbitrary-transport")
        wait(30)
        compare(pluginOpen().enabled, true)
    }
}



