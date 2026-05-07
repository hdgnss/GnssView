# Reference Notes

## Summary

Reference implementations were used only for product and architecture ideas. Code
was not copied. The main reasons are license boundaries, different technology
stacks, and GnssView's Qt Quick architecture.

## Legacy GnssView-Style Client

- Useful ideas:
  - Separation of communication management from parsing.
  - A dedicated NMEA parser layer.
  - Protocol registration and command-template organization.
- Used only as reference:
  - Window layout and high-level interaction hierarchy.
- Not copied:
  - Widget-driven command dialogs and legacy UI structure.
- License note:
  - Top-level license metadata and source file headers are not fully consistent, so this codebase treats it as reference material only.

## gpsd

- Useful ideas:
  - Streaming NMEA recognition.
  - Accumulated receiver state.
  - Defensive handling of malformed input.
  - General fix, DOP, and sky-view data modeling.
- Used only as reference:
  - Device-driver layering and long-term compatibility strategy.
- Not copied:
  - Daemon/client architecture and large C driver implementations.
- License note:
  - The project is BSD-licensed, but GnssView still uses it only as reference material.

## PyGPSClient

- Useful ideas:
  - Input stream -> parser -> UI/status data flow.
  - Product-level organization of views, command presets, logs, and mixed-protocol input.
- Used only as reference:
  - Interaction concepts and user-facing workflow.
- Not copied:
  - Python threading, queues, third-party parser integration, and Tkinter widgets.
- License note:
  - The project is BSD-licensed, and some visual assets have separate provenance. GnssView does not reuse those assets.

## RTKLIB

- Useful ideas:
  - Serial/TCP/UDP stream abstraction.
  - Receiver frame organization.
  - GNSS ID and signal-frequency mapping concepts.
  - Command-template naming and grouping ideas.
- Used only as reference:
  - Low-level GNSS data modeling.
- Not copied:
  - Historical C implementation and legacy Qt Widgets application code.
- License note:
  - The project has permissive licensing in current metadata, but older distribution traces exist. GnssView treats it as reference material only.
