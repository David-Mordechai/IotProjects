# Plan: GPS + LoRa tracker (battery-only, Heltec V4)

Started 2026-11-13. Inspired by the Electronic Clinic video "Nothing Can Block This $3 GPS Chip's Signal"
(https://www.youtube.com/watch?v=_tP54NJHzAI): XIAO ESP32-C3 + Ai-Thinker GP-02 GPS + OLED + BLE phone app,
with LoRa announced as the next step. Our version replaces Bluetooth with LoRa and puts the tracker in a
3D-printed outdoor box.

Status: **planning**. No parts bought yet. Shopping list: `plans/tracker-shopping-list.md`.

**Change 2026-11-14 (cost):** the first plan (XIAO ESP32-S3 + Wio-SX1262 kit, Seeed L76K, Adafruit bq24074
solar charger, 6 V 2 W panel, two IP67 USB-C wall ports) came to ~$309 with tools. It is replaced by a
**Heltec WiFi LoRa 32 V4** (charger and GPS port on board), **battery only** (solar later, see the end), and
**no USB-C wall ports**. The old plan, its research and the solar energy balance are in git (commit `cbb3cf0`).
`guides/tracker-3d.html` and `cad/tracker_box_draft.*` still show the old design and must be redone.

## Decisions (and why)

| Topic | Decision | Why |
|---|---|---|
| Board | **Heltec WiFi LoRa 32 V4** (ESP32-S3R2 + SX1262, 863-928 MHz "high-power" version). Tracker: "Pre-soldered Headers with GPS" variant; home receiver: "Standard with Display" | One board has the MCU, radio, OLED, Li-ion charger (CN3165, 540 mA from USB), a 1.25 mm battery socket, a GNSS socket and a solar socket. Replaces the XIAO kit + L76K + bq24074 and the two solder joints on the XIAO BAT pads. Same board at both ends = one firmware. Cheaper in total (~$68 for both on Amazon vs ~$75-90 in three shipments). Headers pre-soldered on the tracker so I2C sensors can be added without soldering (+$2). |
| GPS | **Heltec L76K GNSS module** (in the "with GPS" variant), 8-pin cable to the board's GNSS socket | Same L76K chip as the old plan (hot fix 1-5 s if kept in standby). GNSS power is switched by the board (GPIO34). Unknown: whether Heltec's module has a backup cell. |
| Power | **Battery only**: one protected 18650 Li-ion, 2200-3000 mAh. Recharge indoors | The board's charger already exists, so solar would only add the panel (~$5-25). Battery-only also removes the worst heat risk: the CN3165 has no battery-temperature sensor fitted (see "Solar later"), and charging a Li-ion in a hot box is what kills it. |
| Recharging / programming | **No USB-C wall ports.** Recharge: swap the cell through the side door and charge it on the **home receiver board** (same 1.25 mm battery socket, charges from USB at 540 mA, ~4-5 h), or open the lid. Program: Wi-Fi OTA, or open the lid | IP67 panel USB-C parts cost ~$15 (unrated) to $40-60 each (distributor). |
| LoRa band | **916.0 MHz, max +14 dBm (25 mW e.r.p.), 125 kHz, SF9-SF10** | Israel, Wireless Telegraph (Conformity Approvals) Regulations 2021, First Schedule Part B row 45: 915-917 MHz, 25 mW e.r.p., 1% duty cycle, <= 200 kHz. 868 MHz has no row. 917-920 MHz is LoRaWAN-only. Avoid 916.3 MHz (2 W RFID, row 46). |
| TX power on a +28 dBm board | **Front-end PA bypassed**, SX1262 set to +14 dBm minus the antenna's gain over a dipole | The V4 has a PA/LNA front-end (V4.2: GC1109, V4.3: KCT8103L). Meshtastic's `variant.h`: GC1109 adds ~+11 dB, and has a "Transmit bypass: CSD=1, CTX=1, CPS=0 (~1dB loss, no PA)" mode. Bypass avoids guessing the PA gain. e.r.p. counts the antenna: a 5 dBi antenna (~2.9 dBd) means ~+11 dBm from the chip. The KCT8103L (V4.3) bypass pin table is **still to verify** from its datasheet. |
| Duty cycle | one ~20-byte packet every few minutes | SF9 airtime ~185-226 ms, so 1% allows one packet every ~19-23 s. Every 5 min uses ~0.07%. |
| Expected range | ~1-1.5 km at street level (estimate, not measured) | +14 dBm SF9 link budget ~143 dB. The receiver's LNA (front-end) may help. |
| Battery level | Board's own divider: ADC on GPIO1, switched on by ADC_Ctrl GPIO37 (HIGH) | Divider 390k/100k behind a P-MOSFET (V4.3 schematic), so it doesn't drain in sleep. Meshtastic multiplier `4.9 * 1.045`. |
| Heat | light-coloured box, vent membrane, battery charged indoors | A closed box in Israeli summer sun can reach 65-70 C (vendor claims, unmeasured). Li-ion discharge limit is ~60 C. Log the inside temperature (ESP32-S3 internal sensor or an I2C sensor) before trusting it in summer. |

## Battery life (estimate, not measured)

Consumption, one fix + one packet every 5 min (288 cycles a day):

| Item | Hot fix ~5 s | Slow fix ~30 s (worst case) |
|---|---|---|
| GPS L76K tracking 41 mA | 205 mA.s | 1230 mA.s |
| ESP32-S3 awake ~40 mA (estimate, OLED off) | 240 mA.s | 1240 mA.s |
| LoRa TX +14 dBm ~45 mA x 0.2 s | 9 mA.s | 9 mA.s |
| Sleep ~0.38 mA (GPS standby ~360 uA dominates; board ~15 uA measured by users) | 118 mA.s | 108 mA.s |
| **Per day** | **~46 mAh** | **~210 mAh** |

2200 mAh x 80% usable = ~1760 mAh:

| Report interval | Per day | Battery lasts |
|---|---|---|
| 5 min, hot fixes (realistic: field data says ~30-70 mAh/day) | 30-70 mAh | **~25-60 days** |
| 5 min, slow fixes (bad case) | ~170-210 mAh | ~8-10 days |
| 15 min | about 1/3 of the above | ~2-5 months |

GPS fix time decides everything (4x range above): measure it on the bench first. Field evidence for the
numbers (old plan, still valid for the L76K): hot fix after 60 s standby 1.1-2.3 s outdoors
(https://github.com/badriram/l76k-at6558-agnss/blob/main/results/2026-10-17-heltec-l76k.md); L76K standby
360-600 uA (sources conflict).

## Firmware must-dos (from the schematic and forum reports)

1. **Deep sleep: hold VFEM_Ctrl (GPIO7) LOW** (with GPIO hold). The front-end LDO enable has a 5.1 MOhm
   pull-up to 3V3 (V4.3 schematic, R41), so it stays on in sleep otherwise: users measured **~9 mA** instead
   of 13-15 uA (Heltec forum, "Heltec WiFi LoRa 32 (V4) deep sleep current waveform",
   http://community.heltec.cn/t/heltec-wifi-lora-32-v4-deep-sleep-current-waveform/21355). Also `radio.sleep()`
   and no pin wake source if possible (wake-on-pin raised sleep current on the V3).
2. **Vext (GPIO36, active LOW) off** in sleep: it powers the OLED (and sensors, see Extensibility).
3. **TX power**: PA bypass + SX1262 at <= +14 dBm (see Decisions). Never leave the PA in its default state.
4. **GPS**: power on with GPIO34 LOW and leave it powered; between fixes put it in standby with GPIO40
   LOW (HIGH = force awake). Reset = GPIO42 low > 100 ms. Cutting GPS power would mean ~30 s cold starts.
5. **Battery**: GPIO37 HIGH only while reading GPIO1. Go to deep sleep with no wake-ups below ~3.4 V.
6. **Updates without opening**: tracker listens for a LoRa command from the receiver, then joins the home
   Wi-Fi and waits for OTA for a few minutes. (Or open the lid and use USB.)

## Heltec V4 pin plan

Sources: Meshtastic `variants/esp32s3/heltec_v4/variant.h`, heltec.org pin table, V4.3 schematic
(`resource.heltec.cn/download/WiFi_LoRa_32_V4/Schematic/HTIT-WB32LAF_V4.3.pdf`).

| Function | GPIO | Note |
|---|---|---|
| LoRa NSS / SCK / MOSI / MISO / RST / BUSY / DIO1 | 8 / 9 / 10 / 11 / 12 / 13 / 14 | Heltec says V4 is pin-compatible with V3; **confirm in variant.h** before coding |
| Front-end LDO enable (VFEM_Ctrl) | 7 | LOW in sleep |
| Front-end CSD / CTX (V4.3 KCT8103L) | 2 / 5 | CTX: HIGH = RX bypass, LOW = RX LNA |
| Front-end CPS (V4.2 GC1109) | 46 | |
| GPS power (VGNSS_Ctrl) | 34 | active LOW |
| GPS standby / reset / PPS | 40 / 42 / 41 | |
| GPS UART: GPS TX -> MCU, MCU -> GPS RX | 38 / 39 | 9600 baud |
| Vext (OLED, switched 3V3 on the header) | 36 | active LOW |
| Battery ADC / ADC_Ctrl | 1 / 37 | |
| LED / user button | 35 / 0 | |
| OLED I2C SDA / SCL / RST | 17 / 18 / 21 | V3 values; confirm |

Board 51.7 x 25.4 x 10.7 mm. LoRa antenna: u.FL (IPEX) at the board tip, opposite the GPS socket.

## Extensibility: room for more sensors (user requirement, 2026-11-13)

| Area | Design rule |
|---|---|
| Wiring | One **I2C bus** with a **STEMMA QT / Qwiic** (JST-SH 4-pin) socket; sensors daisy-chain. Bus pins TBD from the header (sharing the OLED's bus is possible; confirm pins). |
| Power | Sensors on **Vext** (GPIO36), the board's own switched 3V3, off in deep sleep. This replaces the separate load switch of the old plan. Check its current limit before power-hungry sensors. |
| Box | Empty **sensor bay** + a **spare PG7 gland** with a blanking plug (also usable for a solar cable later). Air sensors go **outside** the sealed box in a small vented shield. |
| Radio message | Self-describing **type + value** fields (e.g. Cayenne LPP). Keep messages short (airtime vs 1%). |
| Firmware | One small "sensor" interface (init / read / add to message); each sensor in its own file; detect sensors on the I2C bus at start-up. |

## Box (to redraw for the Heltec board)

No panel, no GPS tower and no USB-C ports any more, so the box can be much smaller than the old
156 x 144 x 52 mm draft. Keep from the old draft:
- **Battery door**: the 18650 slides out through a sealed door in a side wall, in a printed tube (45-deg
  roof, no supports), with a fabric pull-tab. The box must stand flat (no bottom hatch).
- GPS module patch facing up, under plain plastic (no metal or carbon filament above it); LoRa antenna on an
  SMA bulkhead outside the box, away from the GPS patch.
- Spare PG7 gland, M12 vent membrane, small drain at the lowest point.

## Steps

1. Buy parts (shopping list). Measure each with calipers.
2. Firmware on the bench: `gps-hello` (L76K on the V4, TinyGPSPlus, measure fix times), then LoRa hello
   between the two boards at 916 MHz, PA bypassed, then tracker (fix, send, deep sleep) + home receiver.
3. Bench power test with the multimeter's uA range: sleep current (expect ~0.4 mA with GPS standby; if it
   is ~9 mA, GPIO7 is not held low) and mAh per cycle. Fail fast: stop as soon as the first numbers are off.
4. Redraw the 3D guide and the box with the measured parts. Fit-test coupon, then one print by the friend.

## Box: rules so it prints right the first time

- Material: **ASA** if the friend's printer is enclosed, otherwise **white PETG**. Not PLA (softens ~60 C). No dark colours, no carbon-fibre or metal-filled filament (blocks radio).
- Walls 2.4-3 mm. Clearance: 0.3 mm per side for pockets, 0.4 mm for the lid. Holes printed undersized: add 0.2-0.3 mm. No overhangs above 45 deg, no supports. Every part under 170 x 170 mm (fits a Bambu A1 mini).
- Seal: groove for 2-3 mm silicone O-ring cord (~25% squeeze). Printed TPU gaskets seal poorly.
- Screws: self-tappers straight into plastic (pilot holes PETG: M2 1.7, M2.5 2.15, M3 2.6 mm, 4-5 mm deep). No heat-set inserts (needs a soldering iron).
- Openings: SMA bulkhead hole ~6.55 mm round (measure the part), PG7 gland 12.5 mm, M12 vent membrane, small drain.
- Mounting: slots, foam and a strap instead of tight pockets, so small size differences don't matter.
- **Fit-test coupon printed in the same job:** holes 6.0-7.0 mm in 0.1 mm steps, the screw pilot holes, a short piece of lid rim with the seal groove.

### Questions for the friend (ask once)

1. Printer model and bed size? Enclosed? Nozzle size (0.4)?
2. Can it print ASA? Otherwise PETG: which colours on hand (white/light grey)?
3. Will they print with >= 4 walls, >= 5 top/bottom layers, 30-40% gyroid infill, 0.2 mm layers?
4. OK to put a small fit-test coupon on the same plate?

## Solar later (researched 2026-11-14, not in this build)

The board has a 1.25 mm 2-pin solar socket, "supports 4.7~6V solar input" (heltec.org). Facts for later:
- Charger **CN3165** (V4.3 schematic; datasheet http://www.consonance-elec.com/103.html): input 4.4-6 V,
  **pin absolute maximum 6.5 V**. "Input current-limited mode": when the panel can't supply the set current
  it lowers the charge current so VIN stays above its minimum (datasheet p.7), so a weak panel doesn't
  collapse (the problem the bq24074 had in its default mode). Battery drain with no input < 3 uA.
- **Use a 5 V panel (open-circuit ~6 V), not a "6 V" panel** (open-circuit ~7 V+ when the battery is full).
- **No battery NTC fitted** (TEMP grounded = monitoring off, per the datasheet's disable method; check the
  real board). Solar charging in a hot box therefore has no temperature cut-off: solve heat first.
- The panel charges the battery; the board always runs from the battery (P-MOSFET path in the schematic).
- Old energy balance (git `cbb3cf0`): even a ~1 W panel gives several times the daily use in December
  (PVGIS, Tel Aviv).

## Open items

- Which revision Amazon ships (V4.2 GC1109 vs V4.3 KCT8103L front-end) and the KCT8103L bypass truth table.
- Whether Heltec's L76K module has a backup cell (fast hot starts after power-off).
- LoRa / OLED pin numbers: confirm against `variant.h`.
- Exact battery connector (1.25 mm 2-pin, "1.25X2P" on the schematic) and the cable in the box; **check
  polarity with a multimeter** before plugging any battery in.
- How the box is mounted (pole/wall bracket); it must leave the battery door reachable.
- Importing radio modules into Israel needs a conformity approval; rules for a private person not researched.

## Sources

- Heltec WiFi LoRa 32 V4: https://heltec.org/project/wifi-lora-32-v4/ ; schematic V4.3 https://resource.heltec.cn/download/WiFi_LoRa_32_V4/Schematic/HTIT-WB32LAF_V4.3.pdf
- CNX Software review: https://www.cnx-software.com/2025/11/27/heltec-wifi-lora-32-v4-an-esp32-s3-off-grid-lora-meshtastic-communicator-with-solar-input-gnss-and-28dbm-tx-power/
- Meshtastic variant: https://github.com/meshtastic/firmware/blob/master/variants/esp32s3/heltec_v4/variant.h
- Amazon listing (variants, price 2026-11-14): https://www.amazon.com/dp/B0FXX88NKP
- CN3165 datasheet: http://www.consonance-elec.com/103.html
- Israel regulations (Wikisource copy): https://he.wikisource.org/wiki/תקנות_הטלגרף_האלחוטי_(אישורי_התאמה)
- LoRa Alliance RP002-1.0.3 (Israel = AS923-4 for LoRaWAN): https://lora-alliance.org/wp-content/uploads/2021/05/RP002-1.0.3-FINAL-1.pdf
- SX1262 datasheet (sensitivity, airtime formula 6.1.4): https://datasheet.chipsmall.com/SX1262IMLTRT/249951.pdf
- Li-ion temperature limits: https://batteryuniversity.com/article/bu-410-charging-at-high-and-low-temperatures
- Enclosure: Prusa material guides (https://help.prusa3d.com/article/petg_2059, https://help.prusa3d.com/article/asa_1809), Hackaday water-resistant enclosures (https://hackaday.io/project/174011-water-resistant-3d-printed-enclosures), Tallysman embedded antenna guide (https://www.tallysman.com/app/uploads/2019/10/Embedded-Antennas-Reference-Guide-v4.pdf)
