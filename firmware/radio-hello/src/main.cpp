// "Hello radio" - step 1 of the CC1101 plan.
//
// Wiring (both boards are identical, CC1101 pin numbers are printed on the module):
//   CC1101 pin 1 GND  -> ESP32 GND
//   CC1101 pin 2 VCC  -> ESP32 3V3   (NEVER VIN: 5 V destroys the CC1101)
//   CC1101 pin 3 GDO0 -> ESP32 D4    ("a packet arrived" signal)
//   CC1101 pin 4 CSN  -> ESP32 D5    (SPI chip select)
//   CC1101 pin 5 SCK  -> ESP32 D18   (SPI clock)
//   CC1101 pin 6 MOSI -> ESP32 D23   (SPI data ESP32 -> radio)
//   CC1101 pin 7 MISO -> ESP32 D19   (SPI data radio -> ESP32)
//   CC1101 pin 8 GDO2 -> ESP32 D22   ("message sent" signal, used by transmit())
//                        (not D2: that's the blue LED and a boot-mode pin)
// Screw the antenna on before powering up.

#include <Arduino.h>
#include <RadioLib.h>
#include "weblog.h"   // phone log page; does nothing unless built with -D WEB_LOG
#include "flashlog.h" // flash recorder; does nothing unless built with -D FLASH_LOG

// Radio settings. Israel: 433.05-434.79 MHz, low power. 434.0 MHz avoids the
// busy 433.92 MHz used by car keys and garage remotes.
// (Tested 433.42 MHz on 2026-11-10 to rule out clock harmonics at 434.000 MHz: the
// receiver limit stayed at about -87 dBm, so 434.0 stays.)
const float FREQ_MHZ   = 434.0;
const float BITRATE    = 4.8;   // kbit/s: slow = longer range, more reliable
const float FREQ_DEV   = 5.2;   // kHz
const float RX_BW      = 58.0;  // kHz receiver bandwidth
const int8_t POWER_DBM = 0;     // 0 dBm = 1 mW. Allowed: -30,-20,-15,-10,0,5,7,10
// Preamble: the 1010... bits before each message that let the receiver lock on.
// With RadioLib's default 16 bits the receiver had too little preamble to lock on and
// silently rejected most messages. 64 bits gives plenty of margin; the quality threshold
// itself is set to 16 in apply433MHzFixes().
const uint8_t PREAMBLE_BITS = 64;
// The radio drops any packet claiming to be longer than this (noise often does),
// so its 64-byte receive buffer can't overflow. Our messages are ~12 bytes.
const uint8_t MAX_PACKET_LEN = 60;

const int PIN_CSN  = 5;
const int PIN_GDO0 = 4;
const int PIN_GDO2 = 22;
const int PIN_LED  = 2;          // blue onboard LED

// Module(chip select, GDO0 = receive signal, reset = none, GDO2 = transmit signal)
CC1101 radio = new Module(PIN_CSN, PIN_GDO0, RADIOLIB_NC, PIN_GDO2);

void blink() {
  digitalWrite(PIN_LED, HIGH);
  delay(40);
  digitalWrite(PIN_LED, LOW);
}

void stopWithError(const char* what, int code) {
  Serial.printf("ERROR: %s failed, code %d\n", what, code);
  if (code == RADIOLIB_ERR_CHIP_NOT_FOUND) {
    Serial.println("The ESP32 cannot talk to the CC1101. Unplug USB and check:");
    Serial.println("  - VCC on 3V3 and GND on GND");
    Serial.println("  - CSN->D5, SCK->D18, MOSI->D23, MISO->D19 (MOSI/MISO not swapped)");
    Serial.println("  - every jumper pushed fully onto its pin");
  }
  while (true) {               // fast blinking = error
    blink();
    delay(150);
  }
}

// Both roles listen for packets now (the sender listens for the answer).
volatile bool packetArrived = false;
void IRAM_ATTR onPacket() { packetArrived = true; }  // runs when GDO0 fires

// Print a line to USB, and (in the *_web envs) to the phone page,
// and (in receiver_rec) to the flash recorder.
void logLine(const char* fmt, ...) {
  char buf[200];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
  webAdd(buf);
  flashAdd(buf);
}

// Settings RadioLib 7.8.1 doesn't apply for our band/setup (found 2026-11-10 by comparing
// with TI SmartRF Studio values and the ELECHOUSE/SmartRC CC1101 library).
void apply433MHzFixes() {
  // 1) Frequency synthesizer for 430.5-464 MHz. RadioLib never writes these, so the chip
  //    kept power-on defaults meant for other bands. TI datasheet SWRS061I: FSCAL3[7:6],
  //    FSCAL0 and TEST0 must come from SmartRF Studio, and "the recommended settings for
  //    TEST0.VCO_SEL_CAL_EN change with frequency". Measured effect: the same power setting
  //    arrived ~9 dB stronger (sender was effectively transmitting ~8x weaker before).
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_FSCAL3, 0xE9);
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_FSCAL2, 0x2A);  // bit 5: high VCO core
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_FSCAL1, 0x00);
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_FSCAL0, 0x1F);
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_TEST0, 0x09);   // 0x09 for 430.5-464 MHz
  // TI design note DN022: RX filter <= 325 kHz -> TEST2 0x81, TEST1 0x35, FIFOTHR bit 6 set.
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_TEST2, 0x81);
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_TEST1, 0x35);
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_FIFOTHR, radio.SPIreadRegister(RADIOLIB_CC1101_REG_FIFOTHR) | 0x40);
  // 3) Frequency-offset compensation. RadioLib leaves FOC_BS_CS_GATE set (0x76), and per TI
  //    "the offset compensator will freeze until carrier sense asserts" (bit sync too).
  //    Carrier sense only asserts above a fixed level, so weak packets were demodulated
  //    with frequency correction and bit timing frozen. TI's 433 MHz 1.2 kBaud preset,
  //    SpaceTeddy CC1101 and SmartRC CC1101 all use 0x16 (gate off, 3K/K/2, limit BW/4).
  radio.SPIwriteRegister(RADIOLIB_CC1101_REG_FOCCFG, 0x16);
  radio.SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);
  radio.SPIsendCommand(RADIOLIB_CC1101_CMD_CAL);             // calibrate with the new settings
  delay(2);

  // 2) Preamble quality threshold. RadioLib sets it to (preamble - 4) capped at 28, the
  //    strictest value. The chip scores +1 per correct 1/0 change and -8 per wrong bit
  //    (datasheet 17.2), so weak messages were ignored before the sync word. 16 is still
  //    well above noise; also allow 1 wrong bit in the 16-bit sync word (mode 15/16).
  //    Measured: edge-of-range losses 11/60 -> 0/60.
  int state = radio.setPreambleLength(PREAMBLE_BITS, 16);
  if (state != RADIOLIB_ERR_NONE) stopWithError("setPreambleLength", state);
  state = radio.setSyncWord(0x12, 0xAD, 1);   // RadioLib's default sync word
  if (state != RADIOLIB_ERR_NONE) stopWithError("setSyncWord", state);
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  delay(1500);                 // time to open the Serial Monitor
#if defined(ROLE_NOISE)
  Serial.println("\n=== Hello radio: NOISE METER (listens only, never transmits) ===");
#elif defined(ROLE_SENDER)
  Serial.println("\n=== Hello radio: SENDER (asks, waits for the answer) ===");
#else
  Serial.println("\n=== Hello radio: RECEIVER (listens, answers) ===");
#endif
  flashBegin();

  int state = radio.begin(FREQ_MHZ, BITRATE, FREQ_DEV, RX_BW, POWER_DBM, PREAMBLE_BITS);
  if (state != RADIOLIB_ERR_NONE) stopWithError("radio.begin", state);
  state = radio.variablePacketLengthMode(MAX_PACKET_LEN);
  if (state != RADIOLIB_ERR_NONE) stopWithError("variablePacketLengthMode", state);
  Serial.printf("CC1101 found. %.2f MHz, %d dBm\n", FREQ_MHZ, POWER_DBM);
  radio.setPacketReceivedAction(onPacket);

  apply433MHzFixes();

  // Print every configuration register (0x00-0x2E) once, so the real chip settings can
  // be compared with TI SmartRF Studio / other libraries' known-good values.
  Serial.print("CC1101 registers 00-2E:");
  for (uint8_t reg = 0x00; reg <= 0x2E; reg++) {
    if (reg % 16 == 0) Serial.printf("\n  %02X:", reg);
    Serial.printf(" %02X", radio.SPIreadRegister(reg));
  }
  Serial.println();

#if defined(ROLE_RECEIVER) || defined(ROLE_NOISE)
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) stopWithError("startReceive", state);
  logLine("Listening on %.2f MHz...", FREQ_MHZ);
#endif
  webBegin();
}

#ifdef ROLE_NOISE
// Noise meter: with nobody transmitting, the radio's signal-strength reading (RSSI) is
// the background noise at 434 MHz. The receiver can only hear messages clearly
// stronger than this, so a high noise level = short range.
// Samples the RSSI register every 2 ms and prints lowest / average / highest each second.
// RSSI register -> dBm: signed value / 2 - 74 (TI datasheet, offset for 433 MHz).
float readRssiNow() {
  uint8_t raw = radio.SPIreadRegister(RADIOLIB_CC1101_REG_RSSI);
  return (raw >= 128 ? (int)raw - 256 : (int)raw) / 2.0f - 74.0f;
}

void loop() {
  webLoop();
  float lo = 0, hi = -200, sum = 0;
  int n = 0;
  uint32_t start = millis();
  while (millis() - start < 1000) {
    // Noise can look like the start of a message; the radio then stops listening.
    // Check it's still in receive mode (MARCSTATE 0x0D) and restart if not.
    if ((radio.SPIreadRegister(RADIOLIB_CC1101_REG_MARCSTATE) & 0x1F) != 0x0D) {
      radio.startReceive();
      delay(2);               // RSSI needs a moment in receive mode to be valid
      continue;
    }
    float r = readRssiNow();
    if (n == 0 || r < lo) lo = r;
    if (r > hi) hi = r;
    sum += r;
    n++;
    delay(2);
    webLoop();
  }
  if (n == 0) { logLine("noise: no valid samples"); return; }
  float avg = sum / n;
  logLine("noise: avg %6.1f dBm   lowest %6.1f   highest %6.1f   (%d samples)", avg, lo, hi, n);
  webStats(0, 0, avg);
  blink();
}
#endif

#ifdef ROLE_SENDER
// Every second: send "hello #n" (+ distance and position if the phone sent a GPS fix),
// then listen up to ACK_TIMEOUT_MS for the receiver's "ack #n".
// The blue LED blinks ONLY when the answer arrives: dark LED = out of range.
const uint32_t PERIOD_MS = 1000;
const uint32_t ACK_TIMEOUT_MS = 300;   // our packets take ~40-80 ms each way
uint32_t counter = 0;
long answered = 0, missed = 0;

void loop() {
  uint32_t start = millis();
  uint32_t n = counter++;

  String msg = "hello #" + String(n);
#ifdef POWER_STEPS
  // Desk sensitivity test: step down the power every 30 messages and tag each
  // message with it ("p-20"), so the receiver's log shows at which signal strength
  // messages start to fail. Allowed CC1101 levels: -30,-20,-15,-10,0,5,7,10 dBm.
  static const int8_t LEVELS[] = {0, -10, -15, -20, -30};
  int8_t level = LEVELS[(n / 30) % 5];
  radio.setOutputPower(level);
  msg += " p" + String(level);
#endif
  double lat, lon;
  int acc, dist;
  if (webLocation(lat, lon, acc, dist))
    msg += " " + String(dist) + "m " + String(lat, 6) + "," + String(lon, 6) + " acc" + String(acc);

  int state = radio.transmit(msg);     // waits for GDO2 to report "sent"
  if (state != RADIOLIB_ERR_NONE) {
    logLine("send failed, code %d", state);
    if (state == RADIOLIB_ERR_TX_TIMEOUT)
      logLine("  (-5 = no 'sent' signal: check the wire from CC1101 pin 8 to D22)");
  } else {
    // Listen for the answer. Clear the flag first: GDO0 may have toggled while sending.
    packetArrived = false;
    radio.startReceive();
    while (!packetArrived && millis() - start < ACK_TIMEOUT_MS + 150) webLoop();

    String ack;
    bool ok = false;
    float rssi = 0;
    if (packetArrived) {
      packetArrived = false;
      if (radio.readData(ack) == RADIOLIB_ERR_NONE && ack == "ack #" + String(n)) {
        ok = true;
        rssi = radio.getRSSI();
      }
    }
    radio.standby();
    if (ok) {
      answered++;
      blink();
      logLine("sent %-40s answered  RSSI %6.1f dBm  | answered %ld, missed %ld", msg.c_str(), rssi, answered, missed);
    } else {
      missed++;
      logLine("sent %-40s NO ANSWER               | answered %ld, missed %ld", msg.c_str(), answered, missed);
    }
    webStats(answered, missed, ok ? rssi : 0);
  }
  while (millis() - start < PERIOD_MS) webLoop();   // keep the phone page alive meanwhile
}
#endif

#ifdef ROLE_RECEIVER
void loop() {
  webLoop();                   // answer the phone page (receiver_web only)
  if (!packetArrived) {
    flashLoop();               // write queued log lines only while idle (receiver_rec only)
    return;
  }
  packetArrived = false;

  // Lost-message counter for the range test: the sender numbers every message
  // ("hello #108"), so a jump in the number means the ones in between were lost.
  static long lastNum = -1, received = 0, lost = 0;

  String msg;
  int state = radio.readData(msg);
  if (state == RADIOLIB_ERR_NONE) {
    float rssi = radio.getRSSI();      // read before we transmit the answer
    uint8_t lqi = radio.getLQI();
    long n = -1;
    int hash = msg.indexOf('#');
    if (hash >= 0) {
      n = msg.substring(hash + 1).toInt();
      if (n <= lastNum) { received = 0; lost = 0; }            // sender restarted: count again
      else if (lastNum >= 0) lost += n - lastNum - 1;           // gap = lost messages
      lastNum = n;
      // Answer "ack #n". Short pause so the sender has switched to listening.
      delay(5);
      radio.transmit("ack #" + String(n));
    }
    received++;
    // RSSI: signal strength in dBm. -30 = very strong, -100 = barely heard.
    logLine("got: %-44s RSSI %6.1f dBm  LQI %3u  | received %ld, lost %ld (%ld%%)",
            msg.c_str(), rssi, lqi, received, lost, (100 * lost) / (received + lost));
    webStats(received, lost, rssi);
    blink();
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    logLine("got a damaged packet (CRC error)");
  } else {
    logLine("receive failed, code %d", state);
  }
  packetArrived = false;       // ignore GDO0 activity from our own answer
  radio.startReceive();        // listen for the next one
}
#endif
