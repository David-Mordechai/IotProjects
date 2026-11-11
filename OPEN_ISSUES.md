# Open issues

_Last updated: 2026-11-11. Radio projects: `firmware/radio-hello` (CC1101) and `firmware/espnow-hello` (ESP-NOW control test)._

## ▶ Start here next session: ESP-NOW walk test (not done yet)

**Why:** a control experiment for issue 1. The same ask/answer walk over ESP-NOW (2.4 GHz, the ESP32's own Wi-Fi radio, no router) on the **same route** shows whether the route itself limits range. If ESP-NOW goes much further than the CC1101's 46 m, the problem is our 433 MHz setup. If it also stops around 30-50 m, the place or route limits both.

**State at the end of 2026-11-11:**
- Both boards run `firmware/espnow-hello` (not the CC1101 firmware). Receiver = the board that was on COM3, sender = COM5 (COM numbers swap: check the banner). **Label the boards** (e.g. tape on the sender) before unplugging.
- Both boards' flash logs hold desk-test lines only. Erase both before the walk: `uv run --with pyserial python tools/flash_dump.py COMx --erase`.
- The web page runs on the **sender** (`http://10.71.88.92/`, joins the hotspot `AndroidAP`). Its RSSI box shows the **receiver's answers** as the sender hears them (the weaker direction), not the phone link.

**Desk results (boards ~1 m apart, 2026-11-11):**

| Link | Strength |
|---|---|
| Phone hotspot -> sender board | -37 dBm |
| Phone hotspot -> receiver board | -45 dBm |
| Sender -> receiver (ESP-NOW) | -57 dBm |
| Receiver -> sender (ESP-NOW, shown on the page) | -63 to -67 dBm |

- Code and config checked against ESP-IDF 5.5.5 headers and the core's sdkconfig: RSSI is in dBm, TX power reads back 19.5 dBm (receiver) / 18 dBm (sender), both on channel 11, PHY max 20 dBm. Still trusted from docs, not read back: the 1 Mbit/s data rate.
- Both boards hear the phone normally, so their receivers work. Note: even the phone arrives ~15-20 dB below the free-space ideal at the desk, so -57/-67 between two PCB-antenna boards is plausible, not proof of a fault.
- **Open oddity:** the two board-to-board directions differ by 10 dB (-57 vs -67). The same path should be roughly equal both ways, so the receiver board (COM3) transmits weaker or its antenna is placed/pointed worse. Placement during the desk test wasn't controlled.
- About 1 lost answer per ~100 at the desk (no retries, channel shared with the hotspot and the page's polling). Normal.

**Walk procedure:**
1. Hotspot on, phone near the receiver. Power the **receiver first** (on start it scans for the hotspot to learn the channel and won't answer until it finds it), then the sender.
2. Receiver on a power bank at the same spot as today's CC1101 walk. Sender on a power bank, page open, Automate GPS on. Walk the same route.
3. Keep each ESP32's antenna end (the wavy copper line opposite the USB socket) clear of the CC1101 module and jumpers and pointing up. Note how each board was placed or held.
4. Back home: plug **both** boards into the PC while still on the power banks (no restart, nothing lost), then dump both: `tools/flash_dump.py COMx firmware/espnow-hello/logs/walk-<date>-rx.txt` / `-tx.txt`.

**Compare with the CC1101 walk** (`firmware/radio-hello/logs/walk-2026-11-11.txt`: reliable ~29 m, furthest 46 m, limit -87 dBm):
- Range: how far ESP-NOW reaches. Not a fair power comparison: ESP-NOW runs at ~90 mW vs the CC1101's 1 mW.
- Fair comparison: RSSI vs distance (path loss) along the route. 2.4 GHz should lose ~15 dB more than 433 MHz at the same distance (20*log10(2440/434)).
- Which direction fails first (sender log `NO ANSWER` vs receiver log gaps), to see if the 10 dB asymmetry holds outdoors.

**After that:** go back to issue 1's next tests (swap the CC1101 boards' roles, then the 17.3 cm wire antenna). That needs reflashing `radio-hello` (`receiver_rec` + `sender_web`); flash recording on the sender too was proposed so both directions are logged.

## 1. Receiver hears only down to about -87 dBm (main issue)
- **Symptom:** packets fail below about -87 dBm RSSI, and nothing decodes at about -92 dBm. TI's datasheet suggests about -108 dBm for these settings, so we are roughly 20 dB short.
- **Measured noise** at the PC: about -100 dBm (peaks about -92).
- **Already tested, no effect on the limit:**
  - preamble threshold / sync word 15/16 (helps a little at the edge, kept)
  - TEST2/TEST1 (DN022)
  - IF 152 kHz (made it worse)
  - FOCCFG 0x16 (kept)
  - 433.42 MHz instead of 434.0
- **Ruled out 2026-11-11: noise from the PC.** Walk test with the receiver (`receiver_rec`, flash log) on a power bank away from the PC, and the sender on a power bank. Weakest message received: -87.0 dBm. Losses start at about -83 to -87 dBm, the same limit as at the PC. Reliable to ~29 m (3 lost out of 130 up to 29 m), furthest 46 m. Log: `firmware/radio-hello/logs/walk-2026-11-11.txt`. Still not ruled out: the receiver's own ESP32 board, 2 cm from the radio.
- **Next tests** (research first, fail fast):
  1. ~~Receiver on a power bank, away from the PC~~ (done, no change, see above).
  2. Swap the two boards' roles, to compare modules and antennas. **Clue from the 2026-11-11 walk:** all 74 misses on the sender's page were hellos the receiver never got (its log counted exactly 74 lost); no answer was lost on the way back, even at 40-46 m. So one direction is clearly worse: either that receiver board hears worse or the sender board transmits weaker.
  3. 17.3 cm wire antenna (quarter wave at 433 MHz) instead of the stubby antenna.
  4. Retest IF 152 kHz (FSCTRL1 0x06). The earlier test ran before the synthesizer fix, so the result may differ now.
  5. Compare GFSK and AGCCTRL2 0x43 (TI 1.2 kBaud preset) against our 2-FSK / 0x03.

## 2. ~~Both boards still run the 433.42 MHz test firmware~~
- Fixed 2026-11-11: both are back on 434.0 MHz (`receiver_rec` and `sender_web`).

## 3. Wi-Fi joining may cost a few messages
- When `receiver_web` joined the hotspot, 3 messages were lost at a strong signal (-36 dBm). The cause is unknown.
- `sender_web` also joins Wi-Fi, so check that a Wi-Fi reconnect during a walk doesn't show up as radio loss.

## 4. Walk-test tooling
- The Automate flow has the sender's IP (`10.71.88.92`) hard-coded. It may change if the hotspot is restarted.
- GPS is poor indoors (jumps of 10-30 m). Home is set from the first good fix after the sender restarts.
- The sender counts "missed" answers but can't tell whether the question or the answer was lost. The receiver's PC log has the receiving side.

## 5. Not confirmed
- **Israeli legal power limit for 433.05-434.79 MHz:** the official regulator text wasn't found. We stay at 0 dBm (1 mW).

## Results so far (for comparison)

| | Before fixes | After synthesizer fix |
|---|---|---|
| Walk: reliable range | ~15 m | ~25 m |
| Walk: furthest message | 52 m | 56 m |
| Desk test, -30 dBm step | 0/60 | 28/30 |
