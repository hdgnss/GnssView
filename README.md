# GnssView

GnssView is a Qt 6 / QML / C++20 desktop GNSS client. It provides common
transport access, raw data logging, built-in NMEA parsing, runtime plugin
loading, editable command buttons, and release/update infrastructure.

## Current Scope

- Modern dark QML interface with a fixed 4x3 workspace layout.
- Built-in UART, TCP Client, and UDP Server transports.
- Runtime loading for Protocol, TEC data, Transport, and Automation plugins.
- Raw RX/TX recording:
  - `session.raw.bin`
  - `session.log`
- Built-in NMEA support:
  - `GGA / RMC / GSV / GSA / VTG / GLL / ZDA`
- Editable command buttons:
  - Groups
  - JSON import/export
  - Plugin-provided command templates

## Build GnssView

GnssView requires Qt 6.5+ with these modules:

- `Core`
- `Gui`
- `Qml`
- `Quick`
- `QuickControls2`
- `Network`
- `SerialPort`
- `Widgets`

Configure and build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQt6_DIR=/path/to/Qt/6.x.x/<platform>/lib/cmake/Qt6
cmake --build build --target GnssView
```

If Qt is already discoverable through `CMAKE_PREFIX_PATH`, `Qt6_DIR` is optional.

macOS DMG:

```bash
cmake --build build --target GnssViewDmg
```

Windows installer:

```powershell
cmake -S . -B build -DQt6_DIR="C:\Qt\6.8.0\msvc2022_64\lib\cmake\Qt6"
cmake --build build --config Release --target GnssView
.\scripts\build_windows_installer.ps1 -Configuration Release -BuildDir "$PWD\build" -QtRoot "C:\Qt\6.8.0\msvc2022_64"
```

GitHub Actions:

- `.github/workflows/macos.yml` builds `GnssViewDmg`, uploads the DMG as an artifact, and uploads it as a release asset for `v*` tags.
- `.github/workflows/windows.yml` builds `GnssView`, runs `windeployqt` and NSIS through `scripts/build_windows_installer.ps1`, uploads the installer as an artifact, and uploads it as a release asset for `v*` tags.

macOS tag releases require Developer ID signing and notarization secrets:

- `MACOS_CERTIFICATE`: base64-encoded Developer ID Application `.p12`
- `MACOS_CERTIFICATE_PASSWORD`
- `MACOS_SIGNING_IDENTITY`: Developer ID Application identity name or SHA-1 fingerprint
- `APPLE_ID`
- `APPLE_TEAM_ID`
- `APPLE_APP_SPECIFIC_PASSWORD`
- optional `MACOS_KEYCHAIN_PASSWORD`

To set up macOS release signing:

1. Create a `Developer ID Application` certificate in the Apple Developer
   portal. Export it from Keychain Access as a `.p12` file together with its
   private key.
2. Encode the `.p12` file for GitHub Actions:

   ```bash
   base64 -i DeveloperIDApplication.p12 | tr -d '\n' | pbcopy
   ```

3. Add the encoded value to the repository Actions secret
   `MACOS_CERTIFICATE`.
4. Add the `.p12` export password as `MACOS_CERTIFICATE_PASSWORD`.
5. Add the exact identity name, for example
   `Developer ID Application: Example Inc. (TEAMID)`, or its SHA-1 fingerprint
   as `MACOS_SIGNING_IDENTITY`. On a Mac that has the certificate installed,
   find it with:

   ```bash
   security find-identity -v -p codesigning
   ```

6. Create an Apple ID app-specific password and add these notarization secrets:
   `APPLE_ID`, `APPLE_TEAM_ID`, and `APPLE_APP_SPECIFIC_PASSWORD`.
7. Push a version tag such as `v0.1.0`. The macOS workflow signs the packaged
   app with Developer ID, signs the DMG, submits it to Apple notarization,
   staples the notarization ticket, then uploads the DMG as both an artifact and
   a release asset.

Useful checks for a downloaded macOS release:

```bash
codesign --verify --deep --strict --verbose=4 /Volumes/GnssView/GnssView.app
codesign -dv --verbose=4 /Volumes/GnssView/GnssView.app
xcrun stapler validate GnssView-0.1.0-macOS-arm64.dmg
spctl -a -vv -t open --context context:primary-signature GnssView-0.1.0-macOS-arm64.dmg
```

Notes:

- This repository currently does not provide `CMakePresets.json`; use explicit CMake commands.
- `GnssViewDmg` uses CPack DragNDrop and writes `build/GnssView-<version>-macOS-<arch>.dmg`.
- Release packages include GnssView, QML, assets, and Qt runtime files. They do not package external plugin binaries by default.
- If a restricted runner reports `neon` / `Incompatible processor` while launching Qt tools, validate from the host terminal. The native build may still be valid.

## Develop A Plugin

GnssView loads Qt plugin libraries at runtime. To create a plugin, write a small
Qt module library that includes the public headers under `GnssView/include/hdgnss`
and implements one of the supported interfaces.

Minimal source layout:

```text
work/
  GnssView/
  MyGnssPlugin/
    CMakeLists.txt
    MyProtocolPlugin.h
    MyProtocolPlugin.cpp
```

Build:

```bash
cd work
cmake -S MyGnssPlugin -B MyGnssPlugin/build -G Ninja \
  -DGNSSVIEW_ROOT=$PWD/GnssView \
  -DQt6_DIR=/path/to/Qt/6.x.x/<platform>/lib/cmake/Qt6
cmake --build MyGnssPlugin/build
```

Load the built plugin by setting `HDGNSS_PLUGIN_DIR` to the output directory, or
set the same path in Settings -> `Plugin load directory`.

Full step-by-step code and CMake examples are in
[docs/plugin-development.md](docs/plugin-development.md).

## Run

macOS:

```bash
open build/GnssView.app
```

Direct launch with a plugin directory:

```bash
HDGNSS_PLUGIN_DIR=$HOME/.hdgnss/plugins build/GnssView.app/Contents/MacOS/GnssView
```

Plugin search order:

1. `HDGNSS_PLUGIN_DIR`
2. `HDGNSS_PLUGIN_PATH`
3. Settings -> `Plugin load directory`
4. `<applicationDir>/plugins`

`HDGNSS_PLUGIN_PATH` uses the platform path separator (`:` on macOS/Linux, `;` on Windows).

## Project Layout

- `app/`
- `assets/`
- `docs/`
- `src/core`
- `src/models`
- `src/protocols`
- `src/storage`
- `src/transports`
- `src/ui`
- `src/utils`

## Transport Status

Built-in transports:

- UART
- TCP Client
- UDP Server

Additional transports can be provided by runtime plugins.

## Logging

Logging is disabled by default. GnssView does not create or open `logs/` until a
log root directory is set in Settings. After setting `Log root directory`, enable
`Record raw data` or `Record decode log`; the next connection or replay session
creates a session subdirectory under the configured root.

Session files:

- `session.raw.bin`
- `session.log`

`session.raw.bin` stores raw RX/TX bytes in transport order.

`session.log` uses text rows:

```text
2026-04-15T06:15:48.061Z,RX,HEX:  AA BB CC
2026-04-15T06:08:41.084Z,TX,HEX:  AA BB CC
2026-04-15T06:08:41.084Z,RX,ASC:  $GPGGA,...
```

- `HEX`: binary payload rendered as hex.
- `ASC`: printable ASCII payload, split by `CR/LF`.

## License Notes

This codebase is a new implementation. Reference projects were used only for
implementation ideas; source code was not copied. See
[docs/reference-notes.md](docs/reference-notes.md).
