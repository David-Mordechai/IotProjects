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

// Radio settings. Israel: 433.05-434.79 MHz, low power. 434.0 MHz avoids the
// busy 433.92 MHz used by car keys and garage remotes.
const float FREQ_MHZ   = 434.0;
const float BITRATE    = 4.8;   // kbit/s: slow = longer range, more reliable
const float FREQ_DEV   = 5.2;   // kHz
const float RX_BW      = 58.0;  // kHz receiver bandwidth
const int8_t POWER_DBM = 0;     // 0 dBm = 1 mW. Allowed: -30,-20,-15,-10,0,5,7,10
// Preamble: the 1010... bits before each message that let the receiver lock on.
// RadioLib sets the receiver's preamble quality threshold to (length - 4) bits, max 28.
// With 16 bits the threshold is 12, leaving only 4 bits to settle: most messages were
// silently rejected. 64 bits gives a 28-bit threshold and 36 bits of margin.
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

#ifdef ROLE_RECEIVER
volatile bool packetArrived = false;
void IRAM_ATTR onPacket() { packetArrived = true; }  // runs when GDO0 fires
#endif

void setup() {
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  delay(1500);                 // time to open the Serial Monitor
#ifdef ROLE_SENDER
  Serial.println("\n=== Hello radio: SENDER ===");
#else
  Serial.println("\n=== Hello radio: RECEIVER ===");
#endif

  int state = radio.begin(FREQ_MHZ, BITRATE, FREQ_DEV, RX_BW, POWER_DBM, PREAMBLE_BITS);
  if (state != RADIOLIB_ERR_NONE) stopWithError("radio.begin", state);
  state = radio.variablePacketLengthMode(MAX_PACKET_LEN);
  if (state != RADIOLIB_ERR_NONE) stopWithError("variablePacketLengthMode", state);
  Serial.printf("CC1101 found. %.2f MHz, %d dBm\n", FREQ_MHZ, POWER_DBM);

#ifdef ROLE_RECEIVER
  radio.setPacketReceivedAction(onPacket);
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) stopWithError("startReceive", state);
  Serial.println("Listening...");
#endif
}

#ifdef ROLE_SENDER
uint32_t counter = 0;

void loop() {
  String msg = "hello #" + String(counter++);
  int state = radio.transmit(msg);   // waits for GDO2 to report "sent"
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("sent: " + msg);
    blink();
  } else {
    Serial.printf("send failed, code %d\n", state);
    if (state == RADIOLIB_ERR_TX_TIMEOUT)
      Serial.println("  (-5 = no 'sent' signal: check the wire from CC1101 pin 8 to D22)");
  }
  delay(1000);
}
#endif

#ifdef ROLE_RECEIVER
void loop() {
#ifdef RX_DEBUG
  // Diagnostic only: every 2 s print the radio's state, to find out why reception stops.
  static uint32_t lastDbg = 0;
  if (millis() - lastDbg > 2000) {
    lastDbg = millis();
    uint8_t marc = radio.SPIreadRegister(RADIOLIB_CC1101_REG_MARCSTATE) & 0x1F;  // 0x0D = RX, 0x01 = IDLE, 0x11 = RX overflow
    uint8_t rxb  = radio.SPIreadRegister(RADIOLIB_CC1101_REG_RXBYTES);          // bit 7 = overflow, bits 0-6 = bytes waiting
    uint8_t cfg0 = radio.SPIreadRegister(RADIOLIB_CC1101_REG_IOCFG0);
    Serial.printf("[dbg] MARCSTATE 0x%02X  RXBYTES %u%s  IOCFG0 0x%02X  GDO0 pin %d  flag %d\n",
                  marc, rxb & 0x7F, (rxb & 0x80) ? " OVERFLOW" : "", cfg0, digitalRead(PIN_GDO0), (int)packetArrived);
  }
#endif
  if (!packetArrived) return;
  packetArrived = false;
#ifdef RX_DEBUG
  // Frequency offset the radio measured on this packet: signed, 26 MHz / 2^14 = 1.587 kHz per step
  int8_t fe = (int8_t)radio.SPIreadRegister(RADIOLIB_CC1101_REG_FREQEST);
  Serial.printf("[dbg] packet frequency offset: %+.1f kHz\n", fe * 1.587f);
#endif

  String msg;
  int state = radio.readData(msg);
  if (state == RADIOLIB_ERR_NONE) {
    // RSSI: signal strength in dBm. -30 = very strong, -100 = barely heard.
    Serial.printf("got: %-12s  RSSI %6.1f dBm  LQI %u\n",
                  msg.c_str(), radio.getRSSI(), radio.getLQI());
    blink();
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("got a damaged packet (CRC error)");
  } else {
    Serial.printf("receive failed, code %d\n", state);
  }
  radio.startReceive();        // listen for the next one
}
#endif
