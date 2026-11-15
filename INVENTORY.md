# Electronics Inventory

_Last updated: 2026-11-15_

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

## Displays

| # | Item | Qty | Details | Notes |
|---|------|-----|---------|-------|
| 7 | **Waveshare 7inch DPI LCD** kit: 7" IPS panel, **1024×600**, no touch + `RGB LCD HAT for Raspberry Pi` + `RGB 50P TO 40/50P` adapter + white FFC cable | 1 | Parallel **RGB (DPI)** interface. The panel has a 50-pin golden FPC cable; the HAT plugs onto a Raspberry Pi's 40-pin GPIO header | Added 2026-11-11. Box ticked "1024*600". Photos: `photos/waveshare-7in-dpi-lcd-hat-adapter.jpg`, `photos/waveshare-7in-dpi-lcd-box.jpg`. |

### Quick reference: Waveshare 7inch DPI LCD
- **Hookup (from the Waveshare wiki):** RGB LCD HAT → white FFC cable → adapter socket **JP1**; panel's golden FPC cable → adapter socket **JP2**. The HAT then sits on a Raspberry Pi's 40-pin header.
- **Made for a Raspberry Pi, not the ESP32 HW-394.** DPI uses about 28 GPIO pins for the pixel data and clocks, and a 1024×600 frame at 16-bit colour needs ~1.2 MB of RAM. The HW-394 has ~520 KB RAM, no PSRAM and no RGB-LCD hardware, so it can't drive this panel. Options: a Raspberry Pi (setup = overlays in `config.txt`, see the wiki), or an **ESP32-S3 with octal PSRAM** (e.g. DevKitC-1 N16R8) wired to the HAT's 40-pin header in RGB565 mode: ~20 signal wires + 5 V + GND, needs male-to-female jumpers, pixel clock ~25–30 MHz. Not tested yet; see the details below.
- **Verified from Waveshare's schematics and the panel datasheet (2026-11-11):**
  - Panel: **FY07024DI26A67-FT**, 50-pin RGB888. It needs DVDD 3.3 V, AVDD 9.6 V, VGH 18 V, VGL −6 V, and a backlight of 9.8 V / 160 mA. Its dot clock spec is 45–57 MHz (typ 51.2), but the Pi setup runs it at 37 MHz.
  - The **HAT is the power supply**: from the Pi header's 5 V it makes AVDD (MP1541), VGH/VGL (charge pumps) and the backlight (PT4103 boost, on/off switch). The wiki gives ~450 mA at 5 V. So the panel can't be wired straight to a microcontroller; the HAT has to be in between.
  - The HAT's 40-pin header carries **RGB666** (R2–R7, G2–G7, B2–B7) + PCLK, DE, HSYNC, VSYNC + 5 V + GND. MODE is pulled high, so the panel runs in DE mode.
  - The adapter's 40-pin side is a "LCD 4.3" pinout for other Waveshare panels; it is **not** compatible with the ESP32-S3-LCD-Driver-Board's 40-pin port, and that board can't make the panel's high voltages anyway.
- **`VCOM` knob on the HAT:** a trimmer that tunes the panel's common voltage (contrast/flicker). Leave it alone unless the picture flickers.
- **Docs:** https://www.waveshare.com/wiki/7inch_DPI_LCD

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
- **From the photo** (`photos/cc1101-module-antenna.jpg`): the board is marked `433M` and `V2.0`, and the header is numbered `1 2` at the top and `7 8` at the bottom. Hold the board with the antenna connector on the right and the header on the left: odd pins (1, 3, 5, 7) are the **left column** and even pins (2, 4, 6, 8) are the **right column**. Pin 1 is in the top-left corner. The pin **names** are not printed on the board, but on 2026-11-10 they were **confirmed by test** on the first board: the ESP32 found the chip over SPI (pins 1, 2, 4, 5, 6, 7 correct), and a GDO0 check program (since removed) found pin 3 on D4.
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

  \* D4 is also suggested for the RCWL-0516; if you use both, move the RCWL-0516 to D33 (an RTC pin, so motion can wake the ESP32 from deep sleep; D21 can't). GDO2 is required by RadioLib's `transmit()`. Don't use D2 for it: D2 is the onboard LED and a boot pin.
- **Libraries:** `SmartRC-CC1101-Driver-Lib` (ELECHOUSE, many examples) or `RadioLib` (more advanced).
- **Header pitch:** 2.54 mm, so standard female-to-female jumper wires fit.
- **Legal:** 433 MHz is a license-free (ISM) band in most countries, but only at low power and with local rules. Stick to the library's default power settings.

## Wiring and tools

| # | Item | Qty | Details | Notes |
|---|------|-----|---------|-------|
| 6 | Dupont jumper wires, **female-to-female**, about 10 cm, rainbow ribbon | 2 packs (probably 40 wires each) | Sockets on both ends. They fit the male pins on the ESP32, HC-SR04, RCWL-0516 and CC1101 | Peel wires off the ribbon as needed. Use red for power and black or brown for GND. |
| 8 | Dupont jumper wires, **male-to-female**, loose (mixed colours) | 10 | A pin on one end, a socket on the other. The socket goes on a module pin, the pin goes into a breadboard hole | Added 2026-11-15. Photo: `photos/jumpers-male-female.jpg`. |
| 9 | **Mini breadboard, 170 holes**, blue | 1 | 17 rows × 2 halves of 5 holes, with a gap down the middle. **No + / − power rails.** Has dovetail tabs to clip boards together, and probably a sticky back | Added 2026-11-15. Photo: `photos/mini-breadboard-170.jpg`. Too small for the ESP32 (its pin rows are 25.4 mm apart). Good for small parts: transistor + resistor, a sensor, or a voltage divider. |

### Resistors

> ⚠️ **Reminder: measure #10 (1 kΩ) and #12 (10 kΩ) with the multimeter before using them.** Their values are only read from the colour bands (no clear label). Set the meter to Ω, touch one probe to each leg, and don't hold both legs with your fingers (your body adds a parallel resistance). Expect about 990–1010 Ω and 9.9–10.1 kΩ. Once measured, change "Likely" to "Measured" in the table below and remove this note.

All are blue **metal-film, 1% tolerance** resistors with 5 colour bands (3 digits + multiplier + tolerance). Added 2026-11-15. Values come from the colour bands in the photos; check each bag with the multimeter when it arrives (on order).

| # | Value | Qty | Bands | How sure | Photo |
|---|---|---|---|---|---|
| 10 | **1 kΩ** | 6 | brown-black-black-brown-brown | Likely (no label). Read backwards it would be 110 Ω, which is far less common | `photos/resistors-1k-unlabeled.jpg` |
| 11 | **1.2 kΩ** | 8 | brown-red-black-brown + green (0.5%) | Sure: bag labelled "1.2k" and the bands match | `photos/resistors-1k2.jpg` |
| 12 | **10 kΩ** | 10 | brown-black-black-red-brown | Likely. The bag's label is crossed out, but the clearer photo shows the wider gap next to the brown band at the red end, so that brown band is the tolerance band (read backwards it would be 120 Ω) | `photos/resistors-10k.jpg`, `photos/resistors-10k-bag.jpg` |
| 13 | **100 kΩ** | 6 | brown-black-black-orange-brown | Sure: bag labelled "100k" and the bands match | `photos/resistors-100k.jpg` |

- **How to read 5 bands:** hold the resistor so the tolerance band (brown = 1%, green = 0.5%, usually a bit apart from the others) is on the right. The first 3 bands are digits, the 4th is how many zeros to add (black 0, brown 1, red 2, orange 3). Example: brown(1) black(0) black(0) orange(000) = 100 000 Ω = 100 kΩ.
- **Uses in this project:** 1 kΩ between a GPIO and the NPN transistor's base (home-station buzzer, `guides/home-breadboard-3d.html`). 1 kΩ + 2 kΩ (or 1.2 kΩ + 2.4 kΩ) makes the 5 V → 3.3 V divider for an HC-SR04 Echo powered at 5 V. 10 kΩ / 100 kΩ are typical pull-up/pull-down values.

**Tools owned:** none yet (no multimeter, no soldering iron).

## On order (Amazon, ordered 2026-11-14, not yet received)

Checked in the Amazon order history on 2026-11-15. Two orders:

| Item | Order | Arriving | For |
|---|---|---|---|
| REXQualis Electronics Component Fun Kit (830 breadboard, PN2222 NPN (E-B-C), 1 kΩ ×10, 20 M-F + 65 M-M jumpers, buzzer, LEDs…) | 1 | Nov 29 | Breadboard, transistor + resistor for the home-station buzzer |
| BOJACK breadboards: 2× 830 + 2× 400, 126 jumper wires | 1 | Nov 26 – Dec 4 | One breadboard per box (node + home) |
| ELEGOO 120 Dupont wires (40 each M-F, M-M, F-F, 20 cm) | 1 | Nov 29 | ESP32 pin → breadboard (M-F) |
| PINECIL USB-C soldering iron (no power supply included; the user has a USB-C PD charger) | 1 | Nov 29 | Soldering the LED/buzzer board's pads |
| KAIWEETS HT118A multimeter (auto-ranging, 6000 counts, 600 µA / 6000 µA / mA / 10 A ranges) | 2 | Nov 29 | Wire/polarity checks, continuity, sleep-current measurement. **First job: measure resistors #10 and #12** |
| MAIYUM 63/37 rosin-core solder, 0.8 mm, 100 g (contains lead: wash hands) | 2 | Nov 29 | Soldering |
| Glarks 70× 2.54 mm male pin headers (10 mm and 17 mm pins) | 2 | Nov 29 | Pins for the LED/buzzer board's pads |
| innhom 650 pcs heat-shrink tubing (2:1) | 2 | Nov 29 | Insulating soldered joints |
| KAIWEETS 6" flush cutters with stripping holes | 2 | Nov 26 | Cutting/stripping wire, trimming legs |

Order 1: ILS 262.57. Order 2: ILS 324.93, of which ILS 63.81 import charges + ILS 9.08 exchange fee: goods $79.35 > $75 VAT-free limit).

Move these into the tables above when they arrive. Build guide: `guides/station-build-3d.html`.

## Needed / not yet owned

| Priority | Part | Why |
|----------|------|-----|
| Later | **USB logic analyzer** 24 MHz 8 ch (e.g. HiLetgo, ~$13) | See the CC1101's SPI signals in PulseView (`OPEN_ISSUES.md` issue 1). |

Multimeter, full-size breadboards, more jumpers, the NPN transistor and soldering tools: on order (see above). Resistors (1 kΩ etc.), 10 M-F jumpers and a 170-hole mini breadboard are already owned (added 2026-11-15).

---
_Source: package label "ESP-WROOM-32 ESP32-S Development Board Type C ... Color: Type C 30PIN", manufacturer Zhejiang Tianya Tongxing Technology Co., item no. 1049926807163._
