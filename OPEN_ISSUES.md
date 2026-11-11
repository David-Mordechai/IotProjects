# Open issues

_Last updated: 2026-11-11. Radio project: `firmware/radio-hello`._

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
