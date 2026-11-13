# Plan: solar GPS + LoRa tracker

Started 2026-11-13. Inspired by the Electronic Clinic video "Nothing Can Block This $3 GPS Chip's Signal"
(https://www.youtube.com/watch?v=_tP54NJHzAI): XIAO ESP32-C3 + Ai-Thinker GP-02 GPS + OLED + BLE phone app,
with LoRa announced as the next step. Our version replaces Bluetooth with LoRa, adds solar charging and a
3D-printed outdoor box.

Status: **planning**. No parts bought yet. Sizes below come from datasheets. The box is NOT final until
the real parts are measured (see "Box").

## Decisions (and why)

| Topic | Decision | Why |
|---|---|---|
| Board | **Seeed XIAO ESP32-S3 + Wio-SX1262 kit** (pre-soldered XIAO) | The XIAO ESP32-C3 the user first picked runs out of pins: LoRa (8 pins) + GPS (3 pins) use all 11, leaving no pin to read the battery, and Seeed doesn't list the C3 as supported by the Wio-SX1262. In the S3 kit the radio uses a separate B2B connector underneath, so D0-D10 stay free. Official, documented, used by Meshtastic. Chosen by the user 2026-11-13. |
| GPS | **Seeed L76K GNSS for XIAO** | Plugs onto the XIAO, runs on 3.3 V, has a backup cell (hot start 1-5 s), active patch antenna on a 10 cm cable. The video's GP-02-Kit board is officially a 5 V part (Ai-Thinker kit spec) and has no backup pin, so every power-off means a ~32 s cold start. Cost: 41 mA tracking vs 24 mA. |
| LoRa band | **916.0 MHz, max +14 dBm (25 mW e.r.p.), 125 kHz, SF9-SF10** | Israel, Wireless Telegraph (Conformity Approvals) Regulations 2021, First Schedule Part B row 45: 915-917 MHz, 25 mW e.r.p., 1% duty cycle, <= 200 kHz. 868 MHz has no row (not allowed). 917-920 MHz is LoRaWAN-only. Avoid 916.3 MHz (2 W RFID, row 46). 433 MHz is also allowed (10 mW, 10% duty) but the kit radio is 862-930 MHz. |
| Duty cycle | one ~20-byte packet every few minutes | SF9 airtime ~185-226 ms, so 1% allows one packet every ~19-23 s. Every 5 min uses ~0.07%. |
| Expected range | ~1-1.5 km at street level (estimate, not measured) | +14 dBm SF9 link budget ~143 dB, ~55 dB more than our CC1101 setup (lost packets below -88 dBm, ~52 m walk test). |
| Solar charger | **Adafruit bq24074 (product 4755)** | 5-10 V input (6 V panel OK, 10.5 V over-voltage cutoff), power-path: the panel runs the load first, 4-6.5 uA battery drain at night (no diode needed), TS input for a 10k NTC (stops charging outside 0-50 C). Its VIN-DPM (keeps the panel from collapsing) only works in USB500 mode (EN1/EN2 jumpers), see TI datasheet 9.3.4.1.1. Not chosen: TP4056 (no input regulation, stalls on solar), 5 V boost boards (1-2 mA idle), harvesters BQ25570/SPV1050 (<= 100 mA, can't supply LoRa TX). |
| Feeding the XIAO | charger LOAD -> XIAO **BAT+/BAT- pads** | Avoids the 5V pin (it is wired straight to USB VBUS and back-feeds the PC). Pads are under the board = **solder joints**. |
| Panel | **6 V 2 W, e.g. Voltaic P126 (112 x 136 mm)** | Energy ~125-150 mAh/day (GPS fix + LoRa packet every 5 min, deep sleep between). December flat-panel sun in Tel Aviv ~2.8 kWh/m2/day (PVGIS). 0.55 Wh / (2.8 h x 0.4 derating) ~ 0.5 W, x3 to recover after cloudy days ~ 1.5-2 W. |
| Battery | **one protected Li-ion/LiPo, 2000-3000 mAh** | ~5 days without sun at 150 mAh/day, using 80% and allowing for heat aging. LiFePO4 rejected: the chargers charge to 4.2 V and would overcharge it. |
| Battery level | resistor divider (2 x 1 MOhm + 100 nF) from the battery to an ADC1 pin | Seeed's 2 x 100k divider would drain ~18 uA, a large share of sleep current. Pin TBD (S3 research). |
| Heat | white box, panel shades the box, NTC on the charger, log the inside temperature before trusting it | LiPo charging limit 0-45 C; a closed box in Israeli summer sun can reach 65-70 C (unmeasured, vendor claims). The battery fails before the plastic. |

## Energy balance: day and night (recalculated 2026-11-13, estimate, not measured)

Consumption, one fix + one packet every 5 min, 24 h a day (288 cycles):

| Item | Hot fix ~5 s (backup cell works) | Slow fix ~30 s (worst case) |
|---|---|---|
| GPS L76K tracking 41 mA | 205 mA.s | 1230 mA.s |
| ESP32-S3 awake ~40 mA (estimate) | 240 mA.s | 1240 mA.s |
| LoRa TX +14 dBm 45 mA x 0.2 s | 9 mA.s | 9 mA.s |
| Sleep ~0.4 mA (GPS standby 360 uA dominates; XIAO 14 uA, divider 2 uA) | 118 mA.s | 108 mA.s |
| **Per day** | **~46 mAh** | **~210 mAh** |

Harvest: the bq24074 is a linear charger, so the battery gets the panel's **current** (P126: Imp 0.33 A), not its power. Full sun ~0.33 A x 3.8 V = ~1.25 W into the battery. Peak-sun hours (PVGIS, Tel Aviv, flat panel): December 2.8 h, June 8.1 h. December: 0.33 A x 2.8 h = ~920 mAh/day ideal, ~600-800 mAh/day after dust, heat and losses. That is **3x to 15x** the consumption above.

Night and cloudy days: the tracker runs from the battery automatically (bq24074 power path: the input feeds the load first, the battery supplements when the input is too weak or gone; no switch-over gap). A winter night (~14 h) uses ~27-120 mAh. 2200 mAh x 80% usable = ~1760 mAh = **8 to 38 days** with no sun.

Risks that could break this (to test, not assumed):
1. **GPS fix time** decides consumption (4x range above). Measure it.
2. **Weak sun in the default ILIM mode:** TI's VIN-DPM (keeps the panel voltage from collapsing) works only in USB100/USB500 mode. In ILIM mode a weak panel may collapse toward the battery voltage and the charger may stop/start. Planned fix: set EN1/EN2 to USB500 (input limit 500 mA, more than the panel's 0.33 A anyway). Unverified on hardware.
3. **Heat:** with the NTC fitted, charging stops above ~50 C. A box in Israeli summer sun may pass that at midday, so summer charging may happen mostly in the morning and late afternoon. Summer has a large surplus, so this should still balance; log the box temperature to confirm.
4. **Shade, dust, mounting angle:** the numbers assume an unshaded, roughly flat panel. Tilting it ~30 deg south raises December from 2.8 to 4.2 peak hours (PVGIS).

**Field evidence (research 2026-11-13; the user does not want buy-and-test, so the design is checked against other people's measurements):**
- XIAO ESP32-S3 deep sleep 13.4 uA measured (Otii, 3.8 V on BAT): https://www.qoitech.com/?p=19961
- S3 + Wio-SX1262: **2.2 mA** in sleep with the radio's NSS floating; ~10-20 uA after `radio.sleep()` / NSS pulled up: https://forum.seeedstudio.com/t/xiao-esp32s3-sx1262-deep-sleep-current-problem/292361 (the two reporters disagree on whether the pull-up is needed). S3 sleep current also rises (to ~300 uA) when BAT is fed <= 3.3 V.
- Wio-SX1262 TX at 14 dBm ~45 mA once the PA is set for 14 dBm (~90 mA before); an RF-switch pin left high adds 55 uA: https://forum.seeedstudio.com/t/xiao-nrf52840-wio-sx1262-kit-for-meshtastic-power-consumption/294956
- L76K hot fix after 60 s standby measured 1.1-2.3 s outdoors (3 runs, Oct 2026): https://github.com/badriram/l76k-at6558-agnss/blob/main/results/2026-10-17-heltec-l76k.md . Long-run averages for hot fixes are often ~10 s (StuartsProjects). Standby current 360-600 uA (sources conflict, none measured independently).
- Budget from measured parts: **~30-70 mAh/day realistic, ~170 mAh/day bad case**.
- PVGIS stand-alone simulation, central Israel, 2005-2020, panel derated to 1.3 W (linear charger), 2200 mAh: **battery never empty in 5,844 days**, even at 0.5 W effective; at 0.8 Wh/day load. https://re.jrc.ec.europa.eu/api/v5_2/SHScalc?lat=32.0&lon=34.8&peakpower=1.3&batterysize=8.1&cutoff=20&consumptionday=0.8&angle=0&aspect=0&outputformat=json
- Charger: in default ILIM mode there is **no VIN-DPM**; TI E2E confirms the input gets pulled to ~4.3 V (VDPPM) https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/1203115 ; a user with a ~300 mA 6 V panel + load saw a flickering CHG LED and no net charge https://community.particle.io/t/updated-best-practice-for-boron-using-external-lipo-solar-charging/70745 ; a dead cell would not recover with the load attached (TI E2E 632061). Adafruit's "backs off under 4.5 V" claim only holds in USB100/USB500 mode. Its THERM jumper is closed, so **no battery temperature protection out of the box**.
- Heat: black box + black panel on a hot roof killed a LiPo in 3 weeks; builders who shade the box behind the panel with an air gap report only a few degrees above ambient (Meshtastic discourse, "Solar powered router heat problems"). No logged enclosure temperatures from hot climates were found.
- Experienced Meshtastic solar builders moved from the bq24074 to the **bq25185** (VIN-DPM tracks battery + ~330 mV): https://meshtastic.discourse.group/t/meshtastic-router-based-on-the-heltec-ht-ct62-and-ti-bq25185/14308 . Not adopted yet (energy margin is ~10x); revisit if a ready board with header pins fits.

**Decisions from this evidence:** charger jumpers to **USB500** (EN1 high, EN2 low; 500 mA input limit is above the panel's 0.33 A, so nothing is lost); **470 uF capacitor** across the panel input; **10 kOhm NTC** fitted (cut THERM jumper), so charging pauses above ~50 C at midday in summer (that is the protection working); firmware **goes to deep sleep below ~3.4 V** and puts the radio to sleep (`radio.sleep()`, RF switch low) before sleeping; panel on posts with an air gap; light-coloured box; vent membrane. Panel and battery sizes stay.


## XIAO ESP32-S3 pin plan

| Pin | GPIO | Use | Source |
|---|---|---|---|
| D0 | 1 | GPS WAKE_UP (low = sleep), via diode on the L76K | L76K V1.1 pinout |
| D1 | 2 | Battery level, 1 MOhm/1 MOhm divider from the bq24074 LIPO pin + 100 nF | ADC1, not strapping |
| D2 | 3 | GPS RESET (L76K V1.1; was D10 on V1.0 = clash with LoRa MOSI) | Seeed 4-layer guide |
| D3 | 4 | **Spare**: planned sensor-power switch (3V3 load switch) | |
| D4 / D5 | 5 / 6 | I2C SDA / SCL to the STEMMA QT sensor socket | XIAO default I2C |
| D6 / D7 | 43 / 44 | GPS UART (XIAO TX -> GPS RXD, GPS TXD -> XIAO RX), 9600 baud | Meshtastic `GPS_TX_PIN 43`, `GPS_RX_PIN 44` |
| D8 / D9 / D10 | 7 / 8 / 9 | LoRa SCK / MISO / MOSI (shared with the B2B) | Meshtastic `seeed_xiao_s3/variant.h` |
| B2B only | 41, 42, 40, 39, 38 | LoRa CS, RESET, BUSY, DIO1, RXEN; DIO2 = RF switch; TCXO 1.8 V | same |

Not wired (no pins left): bq24074 CHG/PGOOD status. If needed later, add an I2C GPIO expander on the sensor bus.
Physical stack: Wio on top (B2B), XIAO, L76K underneath with **long-pin stacking sockets** (solder), so extra wires plug onto the pins below. Sleep current: XIAO ~14 uA + GPS standby ~360 uA (dominant).

## Draft box (cad/tracker_box_draft.FCStd)

Base 156 x 144 x 52 mm (raised from 42: F-F jumper plugs under the GPS board and the charger need ~21 mm), lid 5 mm, screw ears on the short sides (164 mm overall), GPS tower to 72 mm. Charger on 4 M2.5 standoffs, XIAO stack in a U-cradle, 50 x 30 mm sensor bay, SMA on the -X wall, spare PG7 port on +Y, M12 vent on +X.

**Battery door (user requirements 2026-11-13: replace the battery without opening the box; the box must stand flat).** A bottom hatch was tried and dropped: it stuck out 4 mm so the box rocked, and a flush recess would leave a 12 mm overhang that fails when printed bottom-down. Now: the 18650 lies in a **tube** along the -X wall side (inside X 3..85, Y 18..42, 24 x 24 mm on a 5 mm shelf, 45-deg roof so it prints without supports) and **slides out through a 24 x 24 mm opening in the -X wall**, like a torch. Door: separate 4 x 52 x 40 mm plate, 1 mm above the bottom edge, 2.5 mm silicone cord in a 2.8 x 1.9 mm groove, 4 x M3 self-tappers into printed wall pads (pilot 2.6, 12 mm). The battery's + end and JST plug face the door, so it unplugs right there; the extension wire runs along the tube to a 5 mm grommet hole at the far end, toward the charger. Use a fabric pull-tab under the cell. Checks: box's lowest point is the base (door starts at z = 1), battery slide-out path clear.

**Lid (from field reports on heat, 2026-11-13).** The panel sits on **4 posts, 10 mm above the lid** (bonded with outdoor silicone): the air gap stops the hot panel heating the box, and the panel shades it. Because the panel is now higher, the GPS patch moved up into a **GPS tower** (30 mm cavity, 2 mm walls, 1.6 mm roof, top at 72 mm) so its top is above the panel top (no sky blocked; it was 61 deg on that side before). The panel cable enters through a **PG7 gland in the lid** beside the tower (the old 34 x 34 junction-box opening is gone). Overall height: 72 mm (tower), panel top 69.7 mm.

**USB-C (user requirement 2026-11-13: power and charge by USB-C, and program without opening).** Two IP67 panel-mount USB-C sockets (female outside with cap, short male pigtail inside):
- #1 on the -Y wall -> the bq24074's own USB-C: power + charging. Its inputs are diode-OR'd (Schottky D2 on USB, D4 on the DC jack, read from Adafruit's EAGLE files), so panel and USB can be connected together; the higher voltage wins, no back-feed. 5.1 kOhm CC pull-downs: works with C-to-C chargers. Power only (no data). Charge ~0.89 A, ~3-3.5 h for 2200 mAh; use a >= 1.5 A charger.
- #2 on the -X wall -> the XIAO's USB-C: programming and serial monitor. Safe with the battery connected: in the XIAO S3 schematic (SGM40567 charger, 110 mA) USB feeds the 3.3 V buck through a Schottky and a P-MOSFET cuts BAT off, so nothing reaches the PC; LOAD (4.4 V) is above the XIAO charger's 4.2 V, so the two chargers don't fight. Wi-Fi OTA updates remain an option.
- Wall holes are a 20 mm **placeholder**: panel USB-C parts seen with 16, 20 and 24 mm holes. Size to the part bought. Panel in a fenced area on the lid, junction-box opening 34 x 34 mm. GPS window (thinned to 1.6 mm) beside the panel. Collision check: none. Clearances: GPS patch to LoRa antenna 120 mm; stack top to lid 12.8 mm; battery top to lid 17.6 mm.
**Risk to check:** the GPS window is only 5.8 mm from the panel edge, and the panel stands 4.4 mm above the patch, so it blocks low satellites on that side. Measure fix quality with and without the panel before the final print; alternative is a taller GPS bump in the lid.

## Extensibility: room for more sensors (user requirement, 2026-11-13)

| Area | Design rule |
|---|---|
| Wiring | One **I2C bus** with a **STEMMA QT / Qwiic** (JST-SH 4-pin) socket. Sensors daisy-chain, each has its own address, no extra XIAO pins, no soldering per sensor. |
| Pins | Assign GPS, battery ADC and charger status first; list every remaining XIAO pin as "free for sensors" (keep at least one ADC1 pin free for analog sensors). |
| Power | Sensors on a **switched 3V3 line** (GPIO-controlled load switch/P-MOSFET), off during deep sleep. Energy budget keeps a reserve for sensors. Check the XIAO 3V3 regulator limit before adding power-hungry sensors. |
| Box | Empty **sensor bay** inside + a **spare sealed opening** (PG7 gland with a blanking plug). Air sensors (temperature, humidity, pressure) go **outside** the sealed box in a small vented radiation shield, or they measure the sun-heated box. |
| Radio message | Self-describing **type + value** fields (e.g. Cayenne LPP), so new readings don't break the home receiver. Keep messages short: every byte adds airtime against the 1% limit. |
| Firmware | One small "sensor" interface (init / read / add to message); each sensor is a separate file. Detect sensors on the I2C bus at start-up. |

## Parts list (to buy)

| Part | Qty | Approx. price | Notes |
|---|---|---|---|
| Seeed XIAO ESP32-S3 & Wio-SX1262 kit | 2 | TBD | One tracker + one home receiver. Check the antenna in the kit is 915 MHz. |
| Seeed L76K GNSS module for XIAO | 1 | $10.90 | Buy stock made after 2024-12-22 (fixes a RST pin clash with the Wio-SX1262). |
| Adafruit bq24074 solar charger (4755) | 1 | $14.95 | |
| Solar panel 6 V 2 W, Voltaic P126 (Adafruit 5366) | 1 | ~$21 | 112 x 136 x 2.7 mm, 79 g, junction box 30 x 30 x 5 mm on the back, 260 mm cable with a **3.5 x 1.1 mm** plug. |
| Adapter 3.5 x 1.1 mm -> 5.5 x 2.1 mm DC | 1 | | The bq24074's DC jack is **5.5 x 2.1 mm** (EAGLE part "2.0/2.1mm DC Jack"); Adafruit sells this adapter for it. |
| Protected 18650 Li-ion 2200 mAh with JST-PH (Adafruit 1781) | 1 | | 69 x 18 mm, 46 g. Chosen over a LiPo pouch: the steel can tolerates heat better, pouches swell. **Check JST polarity** against the charger before plugging in (other brands can be reversed): BATT + is the pin nearer the header edge. |
| 10k NTC thermistor (103AT) | 1 | | For the charger's TS input. |
| u.FL to SMA-female bulkhead pigtail + 915 MHz SMA antenna | 1 | | Antenna outside the box. |
| Resistors 2 x 1 MOhm, capacitor 100 nF | | | Battery divider. |
| IP67 panel-mount USB-C female (outside, with cap) -> USB-C male pigtail | 2 | | #1 to the charger (power), #2 to the XIAO (**must carry data**). Check the mounting hole size and the inside body length before the box is final. |
| M2.5/M3 self-tapping screws for plastic, 2.5 mm silicone O-ring cord, outdoor silicone | | | Box: 4 lid + 4 door screws (M3), 4 charger screws (M2.5); silicone bonds the panel to its posts. |
| Small rubber grommet ~5 mm, thin foam pad, fabric pull-tab | | | Battery wire pass-through; foam under the 18650; tab to pull the cell out of the tube. |
| 470 uF electrolytic capacitor (>= 16 V) | 1 | | Across the panel input of the charger (field advice against input collapse). |
| PG7 cable gland (3-6.5 mm) | 2 | | One for the panel cable through the lid, one spare sensor port. |
| Short JST-PH 2-pin extension (~15 cm) | 1 | | Battery JST at the door -> along the tube -> charger BATT. |
| **Digital calipers** | 1 | ~$10 | Needed to measure every part before the box is printed. |
| Soldering iron (decision deferred) | | | 1-2 joints: battery to XIAO BAT pads, NTC. Plan works either way. |

## Steps

1. 3D wiring guide (`guides/`), built from datasheet sizes. Shows how everything connects.
2. Draft box in FreeCAD (`cad/`), with all parts as models inside, checked for collisions.
3. Buy parts. Measure each one with calipers; update the models.
4. Firmware: `gps-hello` on the bench (L76K + TinyGPSPlus), then LoRa hello between two kits at 916 MHz +14 dBm, then the tracker (fix, send, deep sleep) and the home receiver.
5. Bench power test: measure real mAh per cycle, then confirm panel and battery sizes.
6. Final box check, then one print by the friend.

## Box: rules so it prints right the first time

- Material: **ASA** if the friend's printer is enclosed, otherwise **white PETG**. Not PLA (softens ~60 C). No dark colours, no carbon-fibre or metal-filled filament (blocks radio).
- Walls 2.4-3 mm. Clearance: 0.3 mm per side for pockets, 0.4 mm for the lid. Holes printed undersized: add 0.2-0.3 mm. No overhangs above 45 deg, no supports: base printed open side up, lid outer face down. Every part under 170 x 170 mm (fits a Bambu A1 mini).
- Seal: groove for 2-3 mm silicone O-ring cord (~25% squeeze). Printed TPU gaskets seal poorly.
- Screws: self-tappers straight into plastic (pilot holes PETG: M2 1.7, M2.5 2.15, M3 2.6 mm, 4-5 mm deep). No heat-set inserts (needs a soldering iron).
- Openings: SMA bulkhead hole ~6.55 mm round (measure the part), PG7 gland 12.5 mm if a cable is needed, M12 vent membrane, small drain at the lowest point.
- GPS: patch faces up under a "GPS window" in the lid that is **not under the solar panel** (the panel's metal backing blocks it). Keep the LoRa antenna away from the GPS patch.
- Mounting: slots, foam and a strap instead of tight pockets, so small size differences don't matter.
- **Fit-test coupon printed in the same job:** holes 6.0-7.0 mm in 0.1 mm steps, the screw pilot holes, a short piece of lid rim with the seal groove.

### Questions for the friend (ask once)

1. Printer model and bed size? Enclosed? Nozzle size (0.4)?
2. Can it print ASA? Otherwise PETG: which colours on hand (white/light grey)?
3. Will they print with >= 4 walls, >= 5 top/bottom layers, 30-40% gyroid infill, 0.2 mm layers?
4. OK to put a small fit-test coupon on the same plate?

## Open items

- Stack heights (B2B gap ~3.5 mm, socket height ~8.5 mm) and the kit antenna size are estimates: measure when the parts arrive.
- bq24074 EN1/EN2 default: Adafruit's guide and its EAGLE file disagree (file shows open jumpers = 100 mA input limit). Check the real board.
- The Wio-SX1262 in the S3 kit is a different board from the standalone "for XIAO" version (different pins). Use the kit's pin map.
- XIAO USB plugged in while the bq24074 runs on battery only: the XIAO charger may push up to ~110 mA back through the bq's battery FET into the 18650 (same chemistry, 4.2 V limit, probably harmless; TI doesn't specify reverse current). Confirm on the bench.
- How the box is mounted (pole/wall bracket) is not designed yet; it must leave the side battery door reachable.
- Importing radio modules into Israel needs a conformity approval; rules for a private person not researched.

## Sources

- Israel regulations (Wikisource copy): https://he.wikisource.org/wiki/תקנות_הטלגרף_האלחוטי_(אישורי_התאמה)
- LoRa Alliance RP002-1.0.3 (Israel = AS923-4 for LoRaWAN): https://lora-alliance.org/wp-content/uploads/2021/05/RP002-1.0.3-FINAL-1.pdf
- XIAO ESP32-C3 wiki and schematic v1.3: https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/, https://files.seeedstudio.com/wiki/XIAO_WiFi/Resources/XIAO_ESP32C3_v1.3_SCH_260116.pdf
- XIAO ESP32-S3 & Wio-SX1262 kit: https://wiki.seeedstudio.com/xiao_esp32s3_&_wio_SX1262_kit_for_meshtastic/
- 4-layer sandwich (L76K RST conflict): https://wiki.seeedstudio.com/4_layer_sandwich_for_meshtastic/
- L76K GNSS for XIAO: https://wiki.seeedstudio.com/get_start_l76k_gnss/
- GP-02 / GP-02-Kit specs: https://docs.ai-thinker.com/en/gps_1/index.html
- SX1262 datasheet (sensitivity, airtime formula 6.1.4): https://datasheet.chipsmall.com/SX1262IMLTRT/249951.pdf
- Adafruit bq24074: https://www.adafruit.com/product/4755, TI datasheet https://www.ti.com/lit/ds/symlink/bq24074.pdf
- Voltaic P126: https://www.adafruit.com/product/5366
- PVGIS (Tel Aviv): https://re.jrc.ec.europa.eu/api/v5_2/MRcalc?lat=32.08&lon=34.78&horirrad=1&optrad=1
- Li-ion temperature limits: https://batteryuniversity.com/article/bu-410-charging-at-high-and-low-temperatures
- Enclosure: Prusa material guides (https://help.prusa3d.com/article/petg_2059, https://help.prusa3d.com/article/asa_1809), Hackaday water-resistant enclosures (https://hackaday.io/project/174011-water-resistant-3d-printed-enclosures), Tallysman embedded antenna guide (https://www.tallysman.com/app/uploads/2019/10/Embedded-Antennas-Reference-Guide-v4.pdf)
