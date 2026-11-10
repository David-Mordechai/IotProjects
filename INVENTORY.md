# Electronics Inventory

_Last updated: 2026-11-10_

## Microcontrollers

| # | Item | Qty | Details | Notes |
|---|------|-----|---------|-------|
| 1 | ESP32 Dev Board **HW-394** (ESP-32D module, NodeMCU-style) | 4 | USB-C, 30-pin, Wi-Fi + Bluetooth (Classic + BLE), dual-core 240 MHz, 4 MB flash (N4), **CH340C** USB chip (confirmed), AMS1117 3.3 V regulator | No breakout board included. Has `EN` (reset) and `BOOT` buttons. Has a `PWR` LED and a blue LED on D2. |

### Quick reference: ESP32 Dev Board (30-pin, USB-C)
- **Logic level:** 3.3 V. **Do not feed 5 V into GPIO pins.**
- **Power:** USB-C, or 5 V on the `VIN` pin.
- **Programming:** Arduino IDE (add the "esp32 by Espressif" board package), PlatformIO or MicroPython.
- **Board to select in Arduino IDE:** `ESP32 Dev Module`
- **USB driver:** the board uses a **CH340C** chip (confirmed from a close-up photo), so it needs the WCH CH340 driver. Windows 11 usually installs it automatically. If no COM port appears, install it from wch-ic.com (CH341SER).
- **Upload tip:** if an upload gets stuck at "Connecting....", hold `BOOT` until the upload starts.
- **Onboard LED:** usually on GPIO 2, which is handy for a first "blink" test.
- **Input-only pins:** GPIO 34, 35, 36 (VP) and 39 (VN). They can't be used as outputs.
- **Misprinted label:** the pin printed **`D+`** (row with 3V3, 5th pin from the 3V3 end, between `D2` and `D16`) is **D4 / GPIO 4**. The "4" is badly printed. Similarly, `VN` at the end of the other row next to GND is **VIN**. See `photos/esp32-hw394-pin-labels.jpg`.

## Sensors

| # | Item | Qty | Details | Notes |
|---|------|-----|---------|-------|
| 2 | HC-SR04 Ultrasonic Distance Sensor (newer multi-mode version) | 4 | Measures about 2 cm to 4 m. Pins: `Vcc`, `Trig/RX/SCL/IO`, `Echo/TX/SDA`, `Gnd` | `T` = transmitter, `R` = receiver. Pin labels show the newer version, which usually runs on 3 to 5.5 V and supports GPIO (default), UART and I2C modes. |

| 3 | RCWL-0516 Microwave Radar Motion Sensor | 1 | Detects moving people/objects about 5 to 7 m away, **even through plastic, wood or thin walls**. Pins: `3V3`, `GND`, `OUT`, `VIN`, `CDS` | Bought 2; gave 1 to a friend. |


## Communication (radio)

| # | Item | Qty | Details | Notes |
|---|------|-----|---------|-------|
| 5 | CC1101 433 MHz Radio Transceiver module (V2.0) + SMA antenna | 2 | TI CC1101 chip, can both send and receive. 26 MHz crystal, SPI interface, **3.3 V only**. 8-pin header (2×4) | Two modules can talk to each other wirelessly, with a range of tens to hundreds of meters. |

## Output (sound and light)

| # | Item | Qty | Details | Notes |
|---|------|-----|---------|-------|
| 4 | WS2812 RGB LED + 5 V Buzzer board (FPV drone beeper) | 2 | Small board with a loud **active buzzer** and **addressable RGB LEDs (WS2812)**, made for drones. Runs on 5 V. Pins: `INPUT`, `GND`, `+5V`, `BB-`, `OUTPUT` | Bought 4; gave 2 to a friend. Pin labels confirmed from a photo. |

### Quick reference: HC-SR04 with ESP32
- **How it works:** the `T` speaker sends an ultrasonic "ping" and `R` hears the echo. Distance in cm = echo time (µs) / 58.
- **Wiring (default GPIO mode):** `Vcc` → `3V3`, `Gnd` → `GND`, `Trig` → for example `D25`, `Echo` → for example `D26`. (D5 and D18 are kept free for the CC1101's SPI bus.)
- **Power it from 3V3 first.** The newer version usually works at 3.3 V, and then the Echo signal is safe for the ESP32. If readings are unreliable, power it from `VIN` (5 V), but then **Echo outputs 5 V**. In that case add a voltage divider (1 kΩ + 2 kΩ) on Echo to protect the ESP32.
- **Arduino library:** none needed. `pulseIn()` works, or use the `NewPing` library.

### Quick reference: RCWL-0516 with ESP32
- **Wiring:** `VIN` → ESP32 `VIN` (5 V from USB), `GND` → `GND`, `OUT` → any GPIO, for example `D4`.
- ⚠️ **The `3V3` pin on the sensor is an OUTPUT, not a power input.** Leave it unconnected.
- **OUT** goes HIGH (3.3 V, safe for the ESP32) for about 2 s when motion is detected, otherwise LOW. Read it with `digitalRead()`; no library needed.
- **`CDS`:** optional light sensor (LDR) pad that disables detection in daylight. Leave it unconnected.
- **Tips:** keep metal away from the back of the board. It may also detect motion behind it and through walls, so expect some false triggers.

### Quick reference: WS2812 LED + Buzzer board
- **Pins (confirmed):**
  - `+5V` → ESP32 `VIN`
  - `GND` → `GND`
  - `INPUT` → LED data in from a GPIO, for example `D13`
  - `OUTPUT` → LED data out, used to chain to the next board's `INPUT`
  - `BB-` → buzzer negative. The buzzer's + side is already tied to `+5V` on the board.
- **LEDs:** use the **Adafruit NeoPixel** or **FastLED** library. The 3.3 V data signal usually works fine. Several boards can be daisy-chained: `OUTPUT` → next `INPUT`.
- **Buzzer:** it beeps when `BB-` is connected to GND, and you can test that with a jumper wire. ⚠️ **Do not connect `BB-` directly to an ESP32 pin.** When the buzzer is off, that pin sits at 5 V, which is too much for the ESP32 (max 3.3 V). Use an NPN transistor (2N2222, S8050 or BC547):
  - collector → `BB-`
  - emitter → `GND`
  - base → 1 kΩ resistor → a GPIO, for example `D12`
  - GPIO HIGH = beep

### Quick reference: CC1101 433 MHz module with ESP32
- ⚠️ **3.3 V only.** Never connect VCC to 5 V/VIN.
- ⚠️ **Always attach the antenna before transmitting.** Transmitting without one can damage the chip.
- **From the photo** (`photos/cc1101-module-antenna.jpg`): the board is marked `433M` and `V2.0`, and the header is numbered `1 2` at the top and `7 8` at the bottom. Hold the board with the antenna connector on the right and the header on the left: odd pins (1, 3, 5, 7) are the **left column** and even pins (2, 4, 6, 8) are the **right column**. Pin 1 is in the top-left corner. The pin **names** are not printed on the board, but on 2026-11-10 they were **confirmed by test** on the first board: the ESP32 found the chip over SPI (pins 1, 2, 4, 5, 6, 7 correct), and a GDO0 check program (`firmware/radio-hello`, env `diag`) found pin 3 on D4.
- **Antenna:** a short (~5 cm) rubber "stubby" SMA antenna. It's a coiled antenna, so it's shorter than a full 17 cm quarter-wave antenna and has less range.
- **Pinout** (standard for this blue V2.0 board). To verify, use a multimeter in continuity (beep) mode: pin 1 should beep against the outer metal of the antenna connector, which is ground.

  | Module pin | Name | ESP32 pin |
  |---|---|---|
  | 1 | GND | GND |
  | 2 | VCC | 3V3 |
  | 3 | GDO0 | D4 (data/interrupt)* |
  | 4 | CSN | D5 |
  | 5 | SCK | D18 |
  | 6 | MOSI | D23 |
  | 7 | MISO | D19 |
  | 8 | GDO2 | D22 (needed for sending)* |

  \* D4 is also suggested for the RCWL-0516; if you use both, move the RCWL-0516 to D21. GDO2 is required by RadioLib's `transmit()`. Don't use D2 for it: D2 is the onboard LED and a boot pin.
- **Libraries:** `SmartRC-CC1101-Driver-Lib` (ELECHOUSE, many examples) or `RadioLib` (more advanced).
- **Header pitch:** 2.54 mm, so standard female-to-female jumper wires fit.
- **Legal:** 433 MHz is a license-free (ISM) band in most countries, but only at low power and with local rules. Stick to the library's default power settings.

## Wiring and tools

| # | Item | Qty | Details | Notes |
|---|------|-----|---------|-------|
| 6 | Dupont jumper wires, **female-to-female**, about 10 cm, rainbow ribbon | 2 packs (probably 40 wires each) | Sockets on both ends. They fit the male pins on the ESP32, HC-SR04, RCWL-0516 and CC1101 | Peel wires off the ribbon as needed. Use red for power and black or brown for GND. |

**Tools owned:** none yet (no multimeter, no soldering iron).

## Needed / not yet owned

| Priority | Part | Why |
|----------|------|-----|
| ⭐ High | **Multimeter** (basic, about $10–15) | Check voltages and find pins (e.g. CC1101 pin 1), and debug "why doesn't it work?" moments. |
| ⭐ High | **Breadboard** (830 points) + **male-to-male** and **male-to-female** jumper wires | Build circuits without soldering. Female-female wires can't plug into a breadboard. |
| Medium | **NPN transistor** (2N2222 / S8050 / BC547) + **1 kΩ resistor**, or a beginner component kit | Drive the 5 V buzzer safely from the ESP32. |
| Medium | **Soldering iron** (a basic temperature-controlled one) + solder | The buzzer/LED board has solder pads, not pins, so wires must be soldered on. |

---
_Source: package label "ESP-WROOM-32 ESP32-S Development Board Type C ... Color: Type C 30PIN", manufacturer Zhejiang Tianya Tongxing Technology Co., item no. 1049926807163._
