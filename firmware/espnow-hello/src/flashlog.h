// Flash recorder (both envs here): keeps every log line in the ESP32's flash
// (LittleFS file /log.txt), so a board running on a power bank, away from the PC, still
// leaves a full log behind. The file survives power loss and new boots (each boot adds
// a "=== boot" marker). In every other env these do nothing.
//
// Over USB (115200 baud) the board accepts one-letter commands:
//   d = dump the file between "=== DUMP START ===" and "=== DUMP END ==="
//   x = erase the file
// tools/flash_dump.py does this for you.
#pragma once

#ifdef FLASH_LOG
void flashBegin();                 // mount LittleFS, write the boot marker
void flashAdd(const char* line);   // queue one line (written in the background)
void flashLoop();                  // call often when idle: writes the queue, answers d/x
#else
inline void flashBegin() {}
inline void flashAdd(const char*) {}
inline void flashLoop() {}
#endif
