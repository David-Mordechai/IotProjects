// Step 1: HC-SR04 ultrasonic distance meter.
//
// How the sensor works:
//   1. We hold Trig HIGH for 10 microseconds: the sensor sends 8 ultrasonic "clicks" (40 kHz).
//   2. The sensor raises Echo HIGH and keeps it HIGH until the echo comes back.
//   3. Echo's HIGH time = time for sound to go to the object AND back.
//      Sound travels ~343 m/s = 0.0343 cm/us, so distance_cm = time_us * 0.0343 / 2 = time_us / 58.
//
// Powered from 3V3, so Echo is a 3.3 V signal: safe for the ESP32 (its pins take max 3.3 V).

#include <Arduino.h>

const int TRIG_PIN = 25;  // D25 -> sensor Trig
const int ECHO_PIN = 26;  // D26 <- sensor Echo
const int LED_PIN = 2;    // onboard blue LED: blinks on every good reading

// Give up waiting after 30 ms (~5 m round trip), beyond the sensor's ~4 m range.
const unsigned long ECHO_TIMEOUT_US = 30000;

// Returns the echo time in microseconds, or 0 if no echo came back in time.
unsigned long measureEchoUs() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  delay(200);
  Serial.println();
  Serial.println("distance-hello: HC-SR04 on Trig D25 / Echo D26, one reading every 0.5 s");
}

void loop() {
  unsigned long us = measureEchoUs();
  if (us == 0) {
    // No echo: nothing in range, sensor not powered, or a wire is loose.
    Serial.println("no echo (nothing within ~4 m, or check the wires)");
  } else {
    float cm = us / 58.0;
    Serial.printf("%6.1f cm   (echo %lu us)\n", cm, us);
    digitalWrite(LED_PIN, HIGH);
    delay(30);
    digitalWrite(LED_PIN, LOW);
  }
  // The datasheet asks for at least 60 ms between pings so old echoes die out.
  delay(500);
}
