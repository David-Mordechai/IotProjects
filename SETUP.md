# Setting up this project on a new computer

Everything here is for **Windows 10/11**. Versions in brackets are what was used and tested on 2026-11-10.

## 1. Copy the project

- Copy the whole `IotProjects` folder, or clone it with Git if you've pushed it somewhere.
- Recommended location: `C:\Development\IotProjects` (same path as before; see step 7 for why).
- What's **not** in the repo (on purpose, see `.gitignore`): `.pio/` (build output, downloaded libraries and compilers) and the machine-specific `.vscode` files. They are recreated automatically.

## 2. Install the tools

| Tool | Why | How |
|---|---|---|
| **Git** [2.46] | version control | https://git-scm.com/download/win |
| **VS Code** [1.145] | code editor | https://code.visualstudio.com |
| **PlatformIO IDE** extension [3.3.4] | build and upload buttons, serial monitor | VS Code → Extensions → search "PlatformIO IDE" → Install. Restart VS Code and wait until it finishes its first-time setup (bottom-right messages). |
| **uv** [0.7] | installs Python tools | In PowerShell: `powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 \| iex"` |
| **PlatformIO command line** [6.2.0] | lets Claude build and upload from the terminal | `uv tool install platformio`, then check with `pio --version` (open a new terminal first) |
| **Claude Code** [2.1] | the assistant | https://claude.com/claude-code |
| **FreeCAD** [1.1.4] *(only for 3D parts)* | enclosures and board models in `cad/` | https://www.freecad.org/downloads.php |

**USB driver:** the ESP32 boards use a **CH340** USB chip. Windows 11 normally installs its driver automatically when you plug a board in. If the board doesn't show up as `USB-SERIAL CH340 (COMx)` in Device Manager → Ports, install the driver from the chip maker, WCH: https://www.wch-ic.com/downloads/CH341SER_EXE.html

## 3. First build (downloads the ESP32 compiler, ~400 MB, once)

In VS Code: **File → Open Folder → `firmware\radio-hello`**. Open that folder itself, not the whole `IotProjects` folder, or VS Code shows a red error on `#include <Arduino.h>`.

Or in a terminal:

```
cd C:\Development\IotProjects\firmware\radio-hello
pio run
```

This downloads the exact versions pinned in `platformio.ini` (ESP32 platform 8.0.0, RadioLib 7.8.1) and builds all programs. If VS Code still shows red errors on includes after that, run `pio project init --ide vscode` in the same folder.

## 4. Upload and watch (radio project)

Plug in a board, then find its port:

```
pio device list
```

Upload (replace `COM3` with your port):

```
pio run -e sender   -t upload --upload-port COM3     # board 1
pio run -e receiver -t upload --upload-port COM5     # board 2
pio device monitor -p COM5                           # watch; Ctrl+C to stop
```

Watch both boards at once with timestamps (no restart):

```
uv run --with pyserial python tools/monitor_two.py COM3 COM5 60
```

Things we learned the hard way:
- **Port names change** when you use another cable or USB port. Check `pio device list` every time.
- **Only one program can use a port.** Close VS Code's monitor (Ctrl+C) before uploading.
- If a port stays silent or upload says *"No serial data received"*: **unplug and replug** the board.
- Some USB-C cables are **charge-only**: the red LED lights but the PC sees nothing. Use a data cable.

## 5. Wiring guides

Open the HTML files in `guides/` in any browser. They work offline because Three.js is stored in `guides/lib/`. Published copies:
- CC1101 radio: https://claude.ai/artifact/28B9YiApkpYcKitAiPAxFE
- HC-SR04 distance sensor: https://claude.ai/artifact/4x27JhT3auZ6ZPpT8aSGnc

## 6. FreeCAD + Claude (optional, only for 3D parts)

1. Install FreeCAD 1.1.4.
2. Install the FreeCAD MCP add-on (neka-nat, version 0.1.28): copy its `FreeCADMCP` folder to `%APPDATA%\FreeCAD\v1-1\Mod\`. Source: https://github.com/neka-nat/freecad-mcp
3. Register it with Claude Code. Pin the version to match the add-on:
   ```
   claude mcp add freecad -- uvx freecad-mcp==0.1.28
   ```
4. Each time: open FreeCAD → **MCP Addon** workbench → **Start RPC Server** (port 9875).

## 7. Claude's memory (optional)

Claude Code keeps notes about how you like to work outside the project, in
`C:\Users\<you>\.claude\projects\C--Development-IotProjects\memory\`.
To keep them, copy that folder to the same place on the new computer. The folder name comes from the project path, so it only matches if the project is again at `C:\Development\IotProjects`. The project's own rules and hardware notes are in `CLAUDE.md` and `INVENTORY.md`, which are in the repo.

## 8. Quick check that everything works

1. `pio --version` prints a version.
2. `pio run` in `firmware\radio-hello` ends with `SUCCESS`.
3. Plug in board 1: `pio device list` shows a CH340 port.
4. `uv run --with pyserial python tools/serial_read.py COM3` prints `=== Hello radio: SENDER ===` and `sent: hello #...`.
