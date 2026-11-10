# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A learning workspace for a **beginner in electronics** building ESP32 projects. Firmware lives in `firmware/` (PlatformIO). Build with `pio run` inside a project folder; there are no automated tests. The user explains things in plain terms and learns step by step. Explain *why* (voltages, pin choices) and not only *what*.

## Files

- `INVENTORY.md` is the **source of truth** for the hardware the user owns: exact models, quantities, verified pinouts, wiring quick references and a "Needed / not yet owned" shopping list. Update it whenever hardware is added, given away or bought, or a pinout is confirmed.
- `guides/` holds interactive 3D wiring guides: single-file HTML pages built with Three.js r128.
  - Three.js is stored **locally** in `guides/lib/` (`three.min.js`, `OrbitControls.js`) so the guides work offline. New guides should load `lib/...` relative paths, not a CDN.
  - `guides/esp32-hcsr04-3d.html` is also published as a claude.ai artifact: https://claude.ai/artifact/4x27JhT3auZ6ZPpT8aSGnc. That published copy still loads Three.js from a CDN. If it is republished from the local file, include the `lib/` files via the Artifact `files` parameter.
  - `guides/esp32-cc1101-3d.html` (CC1101 radio wiring, 7 wires from the ESP32's right row) is published at https://claude.ai/artifact/28B9YiApkpYcKitAiPAxFE, with the `lib/` files included via `files`.
  - Geometry is in millimetres and is built from the real pin order and 2.54 mm pitch in `INVENTORY.md`. Each guide lists what is accurate vs. simplified.

- `firmware/` holds PlatformIO projects, one folder each. `firmware/radio-hello` is CC1101 step 1, with `sender` and `receiver` environments (`pio run -e sender -t upload`). Settings: 434.0 MHz, 0 dBm (Israel: 433.05–434.79 MHz, low power). Library: RadioLib 7.x. **Gotcha:** RadioLib's `CC1101::transmit()` waits on GDO2 (the Module's 4th pin, see `CC1101.cpp` lines 60/71/281 and the `CC1101_Transmit_Blocking` example), not GDO0, and returns -5 (TX timeout) when GDO2 is unwired. So GDO2 is wired to D22 and passed as `new Module(5, 4, RADIOLIB_NC, 22)`. Receiving uses GDO0 (D4). **Gotcha 2:** RadioLib sets the receiver's preamble quality threshold (PQT) to `preambleLength - 4` bits (max 28, see `setPreambleLength` in `CC1101.cpp`). With the default 16-bit preamble that's 12 of 16, which made the receiver silently drop ~70–85% of packets. Turning PQT off let noise through and overflowed the 64-byte FIFO. Fix (verified 56/56 packets on 2026-11-10): 64-bit preamble on both sides + `variablePacketLengthMode(60)`. The `receiver_debug` env prints radio state (MARCSTATE, RXBYTES, frequency offset). The `diag` env finds which ESP32 pin GDO0 is wired to. COM ports change when cables or USB ports change (board 1 / sender was COM3, board 2 / receiver COM5): check `pio device list` and read the banner before uploading. A port left open by a VS Code `pio device monitor` blocks uploads; if a port then stays silent ("No serial data received"), unplug and replug the board. Open each project folder itself in VS Code (not the repo root), or IntelliSense can't find `Arduino.h`; `pio project init --ide vscode` regenerates `.vscode/`.
- `SETUP.md`: what to install and how to get running on a new computer. Keep it updated when tools or versions change.
- `tools/`: `serial_read.py` (restart a board and print its output) and `monitor_two.py` (watch two boards at once, no restart). Run with `uv run --with pyserial python tools/<script> ...`.
- `photos/` holds the user's photos of their hardware and its package labels, with descriptive file names (e.g. `cc1101-module-antenna.jpg`). Check them before asking the user for a photo.
- `cad/` holds FreeCAD models. `cad/esp32_hw394.FCStd` (+ `.step`) is a reference model of the ESP32 board for designing enclosures: X across, Y along (antenna +Y, USB -Y), Z=0 at the PCB underside. It uses the same dimensions as the 3D guide; component sizes are approximate.

## Hardware constraints that drive every design

- **ESP32 HW-394** (ESP-32D, 30-pin, USB-C, CH340C USB chip). GPIO is **3.3 V only**. In Arduino/PlatformIO, use the board `ESP32 Dev Module` / `esp32dev`. GPIO 34/35/36/39 are input-only. D2 is the onboard LED.
- **Current pin plan** (keep it consistent across projects so they can be combined):
  - HC-SR04: Trig D25, Echo D26, powered from 3V3.
  - RCWL-0516: OUT D4, powered from VIN. Its `3V3` pin is an output, so leave it unconnected.
  - CC1101 (SPI, 3.3 V only): CSN D5, SCK D18, MOSI D23, MISO D19, GDO0 D4, GDO2 D22 (not D2: the onboard LED and a boot strapping pin). GDO0 on D4 conflicts with the RCWL-0516, so move the RCWL to D21 when both are used.
  - The HW-394 silkscreen prints D4 as `D+` and VIN as `VN` (next to GND).
  - WS2812 LED/buzzer board: data INPUT on D13, power +5V from VIN. `BB-` sits at 5 V and must **never** go directly to a GPIO. Drive it through an NPN transistor (not owned yet).
- The user has only **female-to-female** jumpers, with no breadboard, multimeter or soldering iron yet. Wiring plans must work with direct pin-to-pin F-F connections.

## Tooling

- **PlatformIO is installed** (2026-11-10): the CLI via `uv tool install platformio` at `%USERPROFILE%\.local\bin\pio.exe`, the VS Code extension, and the espressif32 platform. **Planned, not yet installed:** the Wokwi simulator (it supports the ESP32, HC-SR04 and WS2812, but not the CC1101 or RCWL-0516). Claude should drive `pio` / `arduino-cli` directly from the shell rather than through an MCP.
- **FreeCAD 1.1.4 + FreeCAD MCP** (neka-nat add-on 0.1.28, in `%APPDATA%\FreeCAD\v1-1\Mod\FreeCADMCP`) is used for 3D-printable parts such as enclosures. It is registered in Claude Code's local config as `uvx freecad-mcp==0.1.28`, pinned to match the add-on version. The FreeCAD tools only work while FreeCAD is open with the RPC server running: in the **MCP Addon** workbench, click **Start RPC Server** (port 9875). Check `Test-NetConnection 127.0.0.1 -Port 9875` if the tools fail. CadQuery is the fallback if the MCP breaks.
- The user's 3D printer model is not known yet. Ask before designing anything that depends on print size or tolerances.
