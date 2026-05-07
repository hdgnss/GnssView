# Plugin Development

GnssView plugins are Qt module libraries loaded at runtime. A plugin includes
the public ABI headers from `GnssView/include/hdgnss`, implements one plugin
interface, and is placed in a directory that GnssView scans.

## Supported Plugin Types

| Type | Header | Qt IID macro | Purpose |
| --- | --- | --- | --- |
| Protocol | `hdgnss/IProtocolPlugin.h` | `HDGNSS_PROTOCOL_PLUGIN_IID` | Identify, decode, encode, packetize files, and provide command templates. |
| TEC data | `hdgnss/ITecDataPlugin.h` | `HDGNSS_TEC_DATA_PLUGIN_IID` | Provide TEC grid data for the world map overlay. |
| Transport | `hdgnss/ITransportPlugin.h` | `HDGNSS_TRANSPORT_PLUGIN_IID` | Add a runtime transport backend. |
| Automation | `hdgnss/IAutomationPlugin.h` | `HDGNSS_AUTOMATION_PLUGIN_IID` | Add an automation panel that observes traffic and can request sends. |

Optional interfaces:

- `hdgnss/IConfigurablePlugin.h`: stable settings ID, display name, settings fields, and `applySettings()`.
- `hdgnss/IPluginMetadata.h`: plugin metadata such as `pluginVersion()`, shown in Settings when provided.
- `hdgnss/IPluginSettingsUi.h`: custom QML settings source.

## Compatibility Rules

- Build with a Qt version, compiler, and architecture compatible with GnssView.
- Include headers from `GnssView/include`; do not link against the GnssView executable.
- Implement `QObject`, `Q_PLUGIN_METADATA`, and `Q_INTERFACES(...)`.
- Keep plugin IDs stable because GnssView uses them for settings persistence.
- Put the built library in a directory selected by environment variables or Settings.

## Create A Protocol Plugin

Use this minimal layout:

```text
work/
  GnssView/
  MyGnssPlugin/
    CMakeLists.txt
    MyProtocolPlugin.h
    MyProtocolPlugin.cpp
```

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)

if(CMAKE_HOST_WIN32)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()

project(MyGnssPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

set(GNSSVIEW_ROOT "" CACHE PATH "Path to the GnssView source tree")
if(GNSSVIEW_ROOT STREQUAL "")
    get_filename_component(GNSSVIEW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../GnssView" ABSOLUTE)
endif()

if(NOT EXISTS "${GNSSVIEW_ROOT}/include/hdgnss/IProtocolPlugin.h")
    message(FATAL_ERROR "GNSSVIEW_ROOT must point to a GnssView source tree")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core)

add_library(MyProtocolPlugin MODULE
    MyProtocolPlugin.cpp
    MyProtocolPlugin.h
)

target_include_directories(MyProtocolPlugin PRIVATE
    "${GNSSVIEW_ROOT}/include"
)

target_link_libraries(MyProtocolPlugin PRIVATE Qt6::Core)

set_target_properties(MyProtocolPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    PREFIX ""
)
```

`MyProtocolPlugin.h`:

```cpp
#pragma once

#include <QObject>

#include "hdgnss/IPluginMetadata.h"
#include "hdgnss/IProtocolPlugin.h"

class MyProtocolPlugin final : public QObject,
                               public hdgnss::IProtocolPlugin,
                               public hdgnss::IPluginMetadata {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID HDGNSS_PROTOCOL_PLUGIN_IID)
    Q_INTERFACES(hdgnss::IProtocolPlugin hdgnss::IPluginMetadata)

public:
    QString protocolName() const override;
    QString pluginVersion() const override;
    QList<hdgnss::ProtocolPluginKind> pluginKinds() const override;
    int probe(const QByteArray &sample) const override;
    QList<hdgnss::ProtocolMessage> feed(const QByteArray &bytes) override;
    QByteArray encode(const QVariantMap &command) const override;
    QList<hdgnss::CommandTemplate> commandTemplates() const override;
    bool supportsFullDecode() const override;
};
```

`MyProtocolPlugin.cpp`:

```cpp
#include "MyProtocolPlugin.h"

#include <QString>

QString MyProtocolPlugin::protocolName() const {
    return QStringLiteral("MyProtocol");
}

QString MyProtocolPlugin::pluginVersion() const {
    return QStringLiteral("1.0.0");
}

QList<hdgnss::ProtocolPluginKind> MyProtocolPlugin::pluginKinds() const {
    return {hdgnss::ProtocolPluginKind::Binary};
}

int MyProtocolPlugin::probe(const QByteArray &sample) const {
    return sample.startsWith(QByteArrayLiteral("MY,")) ? 80 : 0;
}

QList<hdgnss::ProtocolMessage> MyProtocolPlugin::feed(const QByteArray &bytes) {
    if (bytes.isEmpty()) {
        return {};
    }

    hdgnss::ProtocolMessage message;
    message.protocol = protocolName();
    message.messageName = QStringLiteral("RAW");
    message.rawFrame = bytes;
    message.logDecodeText = QString::fromLatin1(bytes.toHex(' ')).toUpper();
    message.fields.insert(QStringLiteral("byteCount"), bytes.size());
    return {message};
}

QByteArray MyProtocolPlugin::encode(const QVariantMap &command) const {
    return command.value(QStringLiteral("payload")).toString().toLatin1();
}

QList<hdgnss::CommandTemplate> MyProtocolPlugin::commandTemplates() const {
    hdgnss::CommandTemplate ping;
    ping.name = QStringLiteral("Ping");
    ping.group = QStringLiteral("MyProtocol");
    ping.protocol = protocolName();
    ping.description = QStringLiteral("Example command");
    ping.payload = QStringLiteral("MY,PING\n");
    ping.contentType = QStringLiteral("ASCII");
    return {ping};
}

bool MyProtocolPlugin::supportsFullDecode() const {
    return false;
}
```

Build it:

```bash
cd work
cmake -S MyGnssPlugin -B MyGnssPlugin/build -G Ninja \
  -DGNSSVIEW_ROOT=$PWD/GnssView \
  -DQt6_DIR=/path/to/Qt/6.x.x/<platform>/lib/cmake/Qt6
cmake --build MyGnssPlugin/build
```

Expected output:

- macOS/Linux: `MyGnssPlugin/build/plugins/MyProtocolPlugin.so`
- Windows: `MyGnssPlugin\build\plugins\MyProtocolPlugin.dll`

Launch on macOS/Linux:

```bash
HDGNSS_PLUGIN_DIR=$PWD/MyGnssPlugin/build/plugins \
  $PWD/GnssView/build/GnssView.app/Contents/MacOS/GnssView
```

Launch on Windows PowerShell:

```powershell
$env:HDGNSS_PLUGIN_DIR = "$PWD\MyGnssPlugin\build\plugins"
.\GnssView\build\Release\GnssView.exe
```

You can also set the same output directory in Settings -> `Plugin load directory`.

## Runtime Discovery

GnssView searches these locations:

1. `HDGNSS_PLUGIN_DIR`
2. `HDGNSS_PLUGIN_PATH`
3. Settings -> `Plugin load directory`
4. `<applicationDir>/plugins`

`HDGNSS_PLUGIN_DIR` selects one directory. `HDGNSS_PLUGIN_PATH` accepts multiple
directories separated by `:` on macOS/Linux and `;` on Windows.

## Protocol Plugin Notes

- `probe(sample)` returns a confidence score. Return `0` when the bytes do not match.
- `feed(bytes)` must be stream-safe. Cache partial frames inside the plugin when needed.
- `encode(command)` converts a UI command map into outbound bytes.
- `commandTemplates()` can return an empty list.
- Binary protocols should implement `parseBinaryFrame(buffer)` when frame boundaries are known.
- Override `packetizeFile(bytes, errorMessage)` when file sends must preserve protocol frames.

## TEC Data Plugin Notes

- Implement `sourceId()`, `sourceName()`, `downloadIntervalSeconds()`, and `requestForObservationTime()`.
- The plugin `QObject` should emit `loadingChanged(bool)`, `dataReady(hdgnss::TecGridData)`, and `errorOccurred(QString)`.
- Use `hdgnss/TecTypes.h` for TEC data structures.

## Transport Plugin Notes

- Implement `transportId()`, `transportDisplayName()`, and `transportInstance()`.
- `transportInstance()` returns an `hdgnss::ITransport` instance.
- Implement `IConfigurablePlugin` when settings must be persisted.
- Implement `IPluginSettingsUi` only when the generated settings fields are not enough.

## Automation Plugin Notes

- Implement `initialize()`, `onRawBytesReceived()`, `controllerObject()`, and optionally `panelUi()`.
- To send data, the controller object should emit `sendRequested(QByteArray, QString)`.
- `panelUi()` may return QML source. The QML root should declare `property var controller` and `property var style`.

## Validation Checklist

- CMake configure succeeds with the intended `GNSSVIEW_ROOT`.
- The plugin binary is written to `build/plugins`.
- `HDGNSS_PLUGIN_DIR` or Settings points to that output directory.
- GnssView shows the plugin in Settings without load errors.
- A clean rebuild of GnssView and the plugin still loads successfully.
