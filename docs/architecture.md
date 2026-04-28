# GnssView Architecture

## Overview

Main data path:

`Transport -> RawRecorder -> StreamChunker -> ProtocolDispatcher -> AppController -> Qt Models -> QML UI`

## Modules

- `app/`
  - Qt Quick startup entry. It creates the core objects, exposes them to QML, and loads the main interface.
- `src/core`
  - `AppController` coordinates transports, logging, protocol dispatch, plugin loading, and UI state.
  - `StreamChunker` splits mixed byte streams into `NMEA / BIN / ASCII` chunks.
  - `ProtocolDispatcher` routes chunks to built-in parsing or runtime protocol plugins.
  - `ProtocolPluginLoader`, `TecPluginLoader`, `TransportPluginLoader`, and `AutomationPluginLoader` discover plugin libraries from runtime search paths.
  - `TransportViewModel` exposes built-in transports and runtime transport plugins to QML.
  - `UpdateChecker` checks GitHub releases and exposes update state to QML.
- `src/transports`
  - `ITransport` defines the byte-stream transport contract.
  - `SerialTransport`, `TcpClientTransport`, and `UdpServerTransport` implement built-in transports.
- `src/protocols`
  - Public protocol ABI is defined by `include/hdgnss/IProtocolPlugin.h`.
  - Built-in NMEA parsing is always available.
- `src/models`
  - `RawLogModel`, `SatelliteModel`, `SignalModel`, `CommandButtonModel`, and related models provide UI-facing state.
- `src/storage`
  - `RawRecorder` writes raw byte captures and optional text decode logs.
- `src/ui/qml`
  - Dark QML interface, panels, charts, and maps.

## Design Notes

- Raw data capture is independent of protocol decode success.
- Standard NMEA parsing is built in.
- Private or binary protocols can be added through runtime protocol plugins.
- GnssView depends on public plugin ABI headers, not on external plugin binaries.
- UI code consumes models and controller APIs instead of parsing protocol bytes directly.
- Command templates are provided through `CommandTemplate` and merged into the command button model.
- Plugin binaries are built separately and loaded at runtime from configured directories.
- Logs are disabled until the user sets a log root directory in Settings.
- Release packages do not include external plugin binaries by default.
