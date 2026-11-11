// "ESP-NOW hello": the radio-hello range test, but over the ESP32's built-in Wi-Fi radio.
//
// ESP-NOW sends short messages (up to 250 bytes) straight from one ESP32 to another,
// with no router. It uses the Wi-Fi radio, so both boards must sit on the SAME Wi-Fi
// channel (1-13). The sender is joined to the phone hotspot (for the live page and GPS),
// so it is on the hotspot's channel; the receiver scans for the hotspot's name to find
// that channel, without joining it.
//
// Fair comparison with the CC1101 test: both sides BROADCAST, so there are no automatic
// retries (Wi-Fi unicast would quietly resend up to ~7 times). One try per message,
// like the CC1101. Rate: the default 1 Mbit/s, the most sensitive normal Wi-Fi rate.
// No extra wiring: the CC1101 can stay connected, it is simply not used here.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "weblog.h"
#include "flashlog.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy src/secrets.example.h to src/secrets.h and type in your phone hotspot's name and password"
#endif

const int PIN_LED = 2;
static const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void blink() {
  digitalWrite(PIN_LED, HIGH);
  delay(40);
  digitalWrite(PIN_LED, LOW);
}

// Print a line to USB, the phone page (sender) and the flash log.
void logLine(const char* fmt, ...) {
  char buf[220];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
  webAdd(buf);
  flashAdd(buf);
}

// ---- Receiving: the Wi-Fi task calls onReceive; it only copies the message into a
// queue, and loop() handles it (doing real work inside the callback is not allowed). ----
struct Packet {
  int len;
  int rssi;
  char text[251];
};
static QueueHandle_t rxQueue;

void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  Packet p;
  p.len = len > 250 ? 250 : len;
  memcpy(p.text, data, p.len);
  p.text[p.len] = 0;
  p.rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;   // signal strength, dBm
  xQueueSend(rxQueue, &p, 0);                          // queue full: drop it
}

void startEspNow() {
  // TX power: set explicitly so the logs say what was used. 19.5 dBm (~90 mW) is the
  // ESP32 maximum and within the 2.4 GHz limit (100 mW). The CC1101 runs at 0 dBm (1 mW),
  // so compare RSSI vs distance (path loss), not just how far each one reached.
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  if (esp_now_init() != ESP_OK) {
    logLine("ERROR: esp_now_init failed");
    while (true) { blink(); delay(150); }
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST, 6);
  peer.channel = 0;            // 0 = whatever channel the radio is on right now
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  esp_now_register_recv_cb(onReceive);
}

uint8_t currentChannel() {
  uint8_t ch;
  wifi_second_chan_t second;
  esp_wifi_get_channel(&ch, &second);
  return ch;
}

void sendText(const String& s) {
  esp_now_send(BROADCAST, (const uint8_t*)s.c_str(), s.length());
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  delay(1500);
  rxQueue = xQueueCreate(8, sizeof(Packet));
#ifdef ROLE_SENDER
  Serial.println("\n=== ESP-NOW hello: SENDER (asks, waits for the answer) ===");
#else
  Serial.println("\n=== ESP-NOW hello: RECEIVER (listens, answers) ===");
#endif
  flashBegin();
#ifdef ROLE_SENDER
  webBegin();                  // joins the hotspot: the radio moves to the hotspot's channel
  // Wi-Fi power saving switches the radio off between the hotspot's beacons, and
  // ESP-NOW messages arriving then are missed (Espressif ESP-NOW docs). Keep it on.
  WiFi.setSleep(false);
#else
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
#endif
  startEspNow();
  // Read the TX power back from the Wi-Fi driver (unit 0.25 dBm), to confirm it was applied.
  int8_t q = 0;
  esp_wifi_get_max_tx_power(&q);
  logLine("ESP-NOW ready, broadcast, 1 Mbit/s, TX power read back %.2f dBm, channel %u",
          q / 4.0f, currentChannel());
}

#ifdef ROLE_SENDER
const uint32_t PERIOD_MS = 1000;
const uint32_t ACK_TIMEOUT_MS = 300;
uint32_t counter = 0;
long answered = 0, missed = 0;

void loop() {
  uint32_t start = millis();
  uint32_t n = counter++;

  String msg = "hello #" + String(n);
  double lat, lon;
  int acc, dist;
  if (webLocation(lat, lon, acc, dist))
    msg += " " + String(dist) + "m " + String(lat, 6) + "," + String(lon, 6) + " acc" + String(acc);

  xQueueReset(rxQueue);        // forget late answers to older questions
  sendText(msg);

  String want = "ack #" + String(n);
  bool ok = false;
  int rssi = 0;
  Packet p;
  while (!ok && millis() - start < ACK_TIMEOUT_MS) {
    if (xQueueReceive(rxQueue, &p, pdMS_TO_TICKS(5)) == pdTRUE && want == p.text) {
      ok = true;
      rssi = p.rssi;
    }
    webLoop();
  }
  if (ok) {
    answered++;
    blink();
    // "phone": how strongly this board hears the hotspot (reference for the ESP-NOW RSSI)
    logLine("sent %-40s answered  RSSI %4d dBm  phone %4d dBm  ch %u | answered %ld, missed %ld",
            msg.c_str(), rssi, (int)WiFi.RSSI(), currentChannel(), answered, missed);
  } else {
    missed++;
    logLine("sent %-40s NO ANSWER                        ch %u | answered %ld, missed %ld",
            msg.c_str(), currentChannel(), answered, missed);
  }
  webStats(answered, missed, ok ? rssi : 0);
  flashLoop();                 // write the log to flash now, between messages
  while (millis() - start < PERIOD_MS) webLoop();
}
#endif

#ifdef ROLE_RECEIVER
// Find the hotspot's channel by scanning for its name (never joins it).
// Returns true if found. Scanning takes ~2 s and hops channels, so only do it
// when nothing is arriving anyway.
bool followHotspot() {
  int n = WiFi.scanNetworks(false, true);
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == WIFI_SSID) {
      uint8_t ch = WiFi.channel(i);
      int hotspotRssi = WiFi.RSSI(i);   // the phone = known-good reference transmitter
      WiFi.scanDelete();
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      logLine("hotspot \"%s\" found on channel %u (heard at %d dBm), listening there",
              WIFI_SSID, ch, hotspotRssi);
      return true;
    }
  }
  WiFi.scanDelete();
  logLine("hotspot \"%s\" not seen, staying on channel %u", WIFI_SSID, currentChannel());
  return false;
}

void loop() {
  static bool first = true;
  static uint32_t lastHeard = 0;
  static long lastNum = -1, received = 0, lost = 0;

  if (first) {
    first = false;
    while (!followHotspot()) delay(3000);   // at startup the hotspot must be found
    lastHeard = millis();
  }
  // Nothing for a minute: maybe the hotspot moved to another channel. Look again.
  // (Out on the walk the phone may be too far to be seen: then the channel stays.)
  if (millis() - lastHeard > 60000) {
    followHotspot();
    lastHeard = millis();
  }

  Packet p;
  if (xQueueReceive(rxQueue, &p, pdMS_TO_TICKS(10)) != pdTRUE) {
    flashLoop();               // idle: write queued log lines
    return;
  }
  if (strncmp(p.text, "hello #", 7) != 0) return;   // not ours
  lastHeard = millis();

  long n = atol(p.text + 7);
  sendText("ack #" + String(n));                    // answer first, log after
  if (n <= lastNum) { received = 0; lost = 0; }     // sender restarted: count again
  else if (lastNum >= 0) lost += n - lastNum - 1;   // gap = lost messages
  lastNum = n;
  received++;
  logLine("got: %-44s RSSI %4d dBm  ch %u | received %ld, lost %ld (%ld%%)",
          p.text, p.rssi, currentChannel(), received, lost, (100 * lost) / (received + lost));
  blink();
}
#endif
