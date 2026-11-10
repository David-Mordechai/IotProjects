// Wiring check for the GDO0 wire. Build with: pio run -e diag -t upload
// The CC1101 is told to hold GDO0 HIGH, then LOW. Every ESP32 pin that follows
// it is connected to GDO0. Pins are pulled the opposite way while testing,
// so an unconnected (floating) pin can't fake a match.
#include <Arduino.h>
#include <RadioLib.h>

CC1101 radio = new Module(5, 4, RADIOLIB_NC, RADIOLIB_NC);

// Free GPIOs on the 30-pin board (SPI pins 5/18/19/23 excluded; 34-39 have no pulls)
const int CAND[] = {4, 2, 15, 16, 17, 21, 22, 13, 12, 14, 27, 26, 25, 33, 32};
const int IOCFG0 = 0x02;  // CC1101 register: what GDO0 outputs
const uint8_t GDO_LOW = 0x2F, GDO_HIGH = 0x6F;  // "hardwired 0", inverted = "hardwired 1"

void setAll(int mode) { for (int p : CAND) pinMode(p, mode); }

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== GDO0 wiring check ===");
  int st = radio.begin(434.0, 4.8, 5.2, 58.0, 0, 16);
  Serial.printf("radio.begin: %d (0 = OK)\n", st);
  if (st != 0) return;

  radio.SPIwriteRegister(IOCFG0, GDO_HIGH);
  setAll(INPUT_PULLDOWN);
  delay(5);
  bool high[sizeof(CAND) / sizeof(int)];
  for (int i = 0; i < (int)(sizeof(CAND) / sizeof(int)); i++) high[i] = digitalRead(CAND[i]);

  radio.SPIwriteRegister(IOCFG0, GDO_LOW);
  setAll(INPUT_PULLUP);
  delay(5);
  int found = 0;
  for (int i = 0; i < (int)(sizeof(CAND) / sizeof(int)); i++) {
    bool low = digitalRead(CAND[i]);
    if (high[i] && !low) { Serial.printf("GDO0 is connected to GPIO %d (D%d)\n", CAND[i], CAND[i]); found++; }
  }
  if (!found) Serial.println("GDO0 is not connected to any free ESP32 pin. Check the wire on CC1101 pin 3 and that it is pushed fully onto both pins.");
  Serial.printf("IOCFG0 readback: 0x%02X\n", radio.SPIreadRegister(IOCFG0));
}

void loop() {}
