#ifdef FLASH_LOG
#include <Arduino.h>
#include <LittleFS.h>
#include "flashlog.h"

// Lines are collected in RAM and written to flash in chunks: a flash write takes
// milliseconds, so doing it per line would delay the answers to the sender.
// At most the last FLUSH_MS of lines are lost if the power is cut.
static const char* PATH = "/log.txt";
static const size_t FLUSH_BYTES = 2048;
static const uint32_t FLUSH_MS = 3000;
static const size_t KEEP_FREE = 32 * 1024;   // stop recording when flash is this full

static String pending;
static uint32_t lastFlush = 0;
static bool ok = false, full = false;

static void flushNow() {
  lastFlush = millis();
  if (!ok || pending.isEmpty()) return;
  if (LittleFS.totalBytes() - LittleFS.usedBytes() < KEEP_FREE) {
    if (!full) Serial.println("flash log: FULL, recording stopped (send x to erase)");
    full = true;
    pending = "";
    return;
  }
  File f = LittleFS.open(PATH, FILE_APPEND);
  if (f) {
    f.print(pending);
    f.close();
  }
  pending = "";
}

void flashBegin() {
  ok = LittleFS.begin(true);   // true = format on first use (empty partition)
  if (!ok) {
    Serial.println("flash log: LittleFS mount FAILED, nothing will be recorded");
    return;
  }
  size_t size = 0;
  if (File f = LittleFS.open(PATH, FILE_READ)) { size = f.size(); f.close(); }
  Serial.printf("flash log: recording to %s (%u bytes so far, %u KB free). USB: d = dump, x = erase\n",
                PATH, (unsigned)size,
                (unsigned)((LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024));
  pending = "=== boot ===\n";
  flushNow();
}

void flashAdd(const char* line) {
  if (!ok || full) return;
  // Time since boot (m:ss.mmm). The board has no clock; the sender's message numbers
  // ("hello #n") and distances tie the lines to the phone's walk log.
  uint32_t ms = millis();
  char stamp[20];
  snprintf(stamp, sizeof(stamp), "%lu:%02lu.%03lu ", (unsigned long)(ms / 60000),
           (unsigned long)(ms / 1000 % 60), (unsigned long)(ms % 1000));
  pending += stamp;
  pending += line;
  pending += '\n';
}

void flashLoop() {
  if (pending.length() >= FLUSH_BYTES || (millis() - lastFlush >= FLUSH_MS && !pending.isEmpty()))
    flushNow();

  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'd') {
      flushNow();
      Serial.println("=== DUMP START ===");
      if (File f = LittleFS.open(PATH, FILE_READ)) {
        uint8_t buf[256];
        while (size_t n = f.read(buf, sizeof(buf))) Serial.write(buf, n);
        f.close();
      }
      Serial.println("=== DUMP END ===");
    } else if (c == 'x') {
      pending = "";
      LittleFS.remove(PATH);
      full = false;
      Serial.println("flash log: erased");
    }
  }
}
#endif
