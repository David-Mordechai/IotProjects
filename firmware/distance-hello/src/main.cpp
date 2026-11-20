// Step 1: HC-SR04 ultrasonic distance meter.
//
// How the sensor works:
//   1. We hold Trig HIGH for 10 microseconds: the sensor sends 8 ultrasonic "clicks" (40 kHz).
//   2. The sensor raises Echo HIGH and keeps it HIGH until the echo comes back.
//   3. Echo's HIGH time = time for sound to go to the object AND back.
//      Sound travels ~343 m/s = 0.0343 cm/us, so distance_cm = time_us * 0.0343 / 2 = time_us / 58.
//
// Powered from 3V3, so Echo is a 3.3 V signal: safe for the ESP32 (its pins take max 3.3 V).
//
// Env "nats" (build flag NATS_PUBLISH): also joins Wi-Fi and publishes each reading
// to the NATS server, so WireClaw can use it as a sensor:
//   in the WireClaw chat: Register a nats_value sensor called distance with subject sensors.distance, unit cm
// "No echo" is sent as 400 (= nothing within the sensor's ~4 m range), so rules like
// "below 20 cm" don't fire by mistake.

#include <Arduino.h>

const int TRIG_PIN = 25;  // D25 -> sensor Trig
const int ECHO_PIN = 26;  // D26 <- sensor Echo
const int LED_PIN = 2;    // onboard blue LED: blinks on every good reading

// Give up waiting after 30 ms (~5 m round trip), beyond the sensor's ~4 m range.
const unsigned long ECHO_TIMEOUT_US = 30000;

#ifdef NATS_PUBLISH
#include <WiFi.h>
#include "secrets.h"   // WIFI_SSID, WIFI_PASSWORD (git-ignored; template: secrets.example.h)

// NATS speaks plain text lines over TCP, so a few lines replace a library:
//   server -> "INFO {...}"           client -> "CONNECT {...}"
//   client -> "PUB <subject> <n>\r\n<n bytes>\r\n"
//   server -> "PING"                 client -> "PONG"  (or the server drops us)
static NetworkClient nats;
static unsigned long lastConnectTry = 0;

static void natsConnect() {
  if (nats.connected() || millis() - lastConnectTry < 3000) return;
  lastConnectTry = millis();
  Serial.printf("NATS: connecting to %s:%d ... ", NATS_HOST, NATS_PORT);
  if (!nats.connect(NATS_HOST, NATS_PORT)) { Serial.println("failed"); return; }
  nats.setNoDelay(true);
  nats.print("CONNECT {\"verbose\":false,\"pedantic\":false,\"name\":\"distance-hello\"}\r\n");
  Serial.println("connected");
}

// Answer the server's PINGs; ignore everything else it sends (INFO, PONG, +OK).
static void natsService() {
  static char line[128];
  static size_t n = 0;
  while (nats.available()) {
    char c = nats.read();
    if (c == '\n') {
      line[n] = '\0';
      if (strncmp(line, "PING", 4) == 0) nats.print("PONG\r\n");
      else if (strncmp(line, "-ERR", 4) == 0) Serial.printf("NATS error: %s\n", line);
      n = 0;
    } else if (c != '\r' && n < sizeof(line) - 1) {
      line[n++] = c;
    }
  }
}

static void natsPublish(float cm) {
  if (!nats.connected()) return;
  char payload[16];
  int len = snprintf(payload, sizeof(payload), "%.1f", cm);
  nats.printf("PUB %s %d\r\n%s\r\n", NATS_SUBJECT, len, payload);
}
#endif

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
#ifdef NATS_PUBLISH
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // keep the radio awake (see firmware/wireclaw/PORT-NOTES.md)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("WiFi: connecting to %s", WIFI_SSID);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(500); Serial.print("."); }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf(" OK, IP %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println(" FAILED (check secrets.h; the name is case-sensitive). Will keep trying.");
  Serial.printf("Publishing to NATS subject '%s'\n", NATS_SUBJECT);
#endif
}

void loop() {
  unsigned long us = measureEchoUs();
  float cm;
  if (us == 0) {
    // No echo: nothing in range, sensor not powered, or a wire is loose.
    Serial.println("no echo (nothing within ~4 m, or check the wires)");
    cm = 400.0;
  } else {
    cm = us / 58.0;
    Serial.printf("%6.1f cm   (echo %lu us)\n", cm, us);
    digitalWrite(LED_PIN, HIGH);
    delay(30);
    digitalWrite(LED_PIN, LOW);
  }
#ifdef NATS_PUBLISH
  if (WiFi.status() == WL_CONNECTED) {
    natsConnect();
    natsService();
    natsPublish(cm);
  }
#endif
  // The datasheet asks for at least 60 ms between pings so old echoes die out.
  delay(500);
}
